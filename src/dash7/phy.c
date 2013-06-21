/**
 * @file    phys.c
 * @brief   Implements the phyical layer of dash7 on the SX1231
 * */
#include "dash7/phy.h"
/**********************************************
conf.h declares (or not) MULTI_FRAME_PACKET
this is used to define if we'll support packets
with multiple frames
**********************************************/

#include "sx1231.h"
#include "dio.h"
#include "global.h"
#include "crc16.h"

#define ROUND(x) ((uint32_t) ((x) + 0.5))

// Convert a frequency in MHz to its FRF/FDEV equivalent
#define FREQ(d) ROUND(((d) * 1e6) / (32e6 / (1 << 19)))

//N_FRAMES defined in dash7/conf.h
uint8_t phy_frame_buf[N_FRAMES][FRAME_SIZE];
uint8_t phy_frame_pos;
uint8_t phy_frame_filled;
uint8_t phy_rec_rssi;
static Mutex phy_proc_mtx;


static const uint32_t center_frequencies[] = {
    FREQ(433.164), FREQ(433.272), FREQ(433.380), FREQ(433.488),
    FREQ(433.596), FREQ(433.704), FREQ(433.812), FREQ(433.920),
    FREQ(434.028), FREQ(434.136), FREQ(433.244), FREQ(434.352),
    FREQ(434.460), FREQ(434.568), FREQ(434.676)
};

static const uint32_t bandwidth[] = {
    FREQ(0.432 / 2), FREQ(0.216 / 2), FREQ(0.432 / 2), FREQ(0.648 / 2)
};

static inline size_t min(size_t a,size_t b)
{
    return a < b ? a : b;
}

static inline bool_t fg_frame_cont(uint8_t pos)
{
    return (phy_frame_buf[pos][3] & (1<<4));
}

static int phy_receive_foreground(d7_ti ti)
{
    int fi; //frame iterator
    size_t size;
    size_t bytes_read;
    uint8_t byte_timeout = 0;
    uint8_t bad_size = 0;
    sx_mode(RX);
    systime_t timeout = d7_ti_to_systime(ti); 
    

    if (!wait_message(timeout))
        return PHY_REC_TIMEOUT;

    phy_rec_rssi = sx_read8(REG_RSSI_VALUE);

    for (fi = 0; fi < N_FRAMES; fi++) {
        if (!wait_message(NEXT_BYTE_TIMEOUT)){
            byte_timeout = 1;
            goto _end;
        }

        size = phy_frame_buf[fi][0] = sx_read8(REG_FIFO);
        bytes_read = 1;
        if(size < 6){
            bad_size = 1;
            goto _end;
        }
        while (bytes_read < size) {
            if(!wait_message(NEXT_BYTE_TIMEOUT)){
                byte_timeout = 1;
                goto _end;
            }

            phy_frame_buf[fi][bytes_read++] = sx_read8(REG_FIFO);
        }
        //read the crc
        for (size = 2; size > 0; size--){
            if(!wait_message(NEXT_BYTE_TIMEOUT)){
                byte_timeout = 1;
                goto _end;
            }
            phy_frame_buf[fi][bytes_read++] = sx_read8(REG_FIFO);
        }
        if (!fg_frame_cont(fi))
            break;
    }

_end:
    phy_frame_filled = fi+1;
    sx_mode(STDBY);
    empty_fifo();
    if (byte_timeout)
        return PHY_BYTE_TIMEOUT;
    if (bad_size)
        return PHY_BAD_SIZE;

    return phy_frame_filled;
}

static int phy_receive_background(d7_ti ti)
{
    systime_t timeout = d7_ti_to_systime(ti); 
    uint8_t byte_timeout = 0;
    sx_mode(RX);

    if (!wait_message(timeout))
        return PHY_REC_TIMEOUT;

    phy_rec_rssi = sx_read8(REG_RSSI_VALUE);

    for (int i = 0; i < 7; i++) {
        if(!wait_message(NEXT_BYTE_TIMEOUT)){
            byte_timeout = 1;
            goto _end;
        }

        phy_frame_buf[0][i] = sx_read8(REG_FIFO);
    }

_end:
    phy_frame_filled = 1;
    sx_mode(STDBY);
    empty_fifo();
    if (byte_timeout)
        return PHY_BYTE_TIMEOUT;

    return 1;

}

// Convert a symbol rate in kHz to its BITRATE equivalent
#define SYMRATE(s) ROUND(32e6 / ((s) * 1e3))

static const uint16_t sync_word[2][2] = {{0xE6D0, 0xF498},{0x0B67, 0x192F}}; 

static phy_param_t phy_conf;

/**
 * @brief Configure the physical layer
 *
 * @param conf structure with the configuration
 */
void phy_config(phy_param_t *conf)
{
    if(conf)
        phy_conf = *conf;
    else {
        phy_conf.channel = CH_NORMAL_0;
        phy_conf.preamble = 0;
        phy_conf.fec = WITHOUT_FEC;
        phy_conf.pack_class = FOREGROUND_CLASS;
        phy_conf.rssi_thresh = 100;
    }
    // BASE and LEGACY channels are treated different
    if(!(phy_conf.channel >> 8)) {
        sx_write24(REG_FRF_MSB, center_frequencies[7]);
        sx_write16(REG_BITRATE_MSB, SYMRATE(55.555));
    } else {
        // Select the base frequency
        sx_write24(REG_FRF_MSB, center_frequencies[phy_conf.channel & 0xf]);

        // Select the symbol rate (FSK 1.8 or FSK 0.5)
        uint8_t bandwidth_index = (phy_conf.channel >> 8) & 0x7;
        sx_write16(REG_BITRATE_MSB, SYMRATE(bandwidth_index <= 2 ? 55.555 : 200));
    }

    //Set sync word according to package class and fec
    sx_write16(REG_SYNC_VALUE_1,sync_word[phy_conf.pack_class][phy_conf.fec]);

    //Set number of preamble bytes
    if((phy_conf.channel >> 8) < 2)
        if(phy_conf.preamble < 4 || phy_conf.preamble > 8)
            sx_write16(REG_PREAMBLE_MSB,phy_conf.preamble);
        else
            sx_write16(REG_PREAMBLE_MSB,4);
    else
        if(phy_conf.preamble < 6 || phy_conf.preamble > 16)
            sx_write16(REG_PREAMBLE_MSB,phy_conf.preamble);
        else
            sx_write16(REG_PREAMBLE_MSB,6);

    //We set the RSSI threshold
    set_rssi_thresh((phy_conf.rssi_thresh));
}

/**
 * @brief Initialize the physical layer
 */
void phy_init(void)
{
    //Set the kind of modulation(Mode packet, modulation FSK, Gaussian filter, BT = 1.0).
    sx_write8(REG_DATA_MODUL, DATA_MODE(PACKET_MODE) | MODULATION_TYPE(FSK) |
              MODULATION_SHAPING(FILTER_1));
    
    //Select the frequency deviation: 0.50MHz.
    sx_write16(REG_FDEV_MSB, FREQ(0.050));

    //Select 50 ohms impedance for the out circuit. And use the maximum gain.
    sx_write8(REG_LNA, LNA_ZIN(0) | LNA_GAIN_SELECT(GAIN_0));

    //Select the channel filter and DC cancellation parameters - 83kHz & 206Hz (0.25%).
    // TODO: check if the default value (4%) is better or worse
    sx_write8(REG_RX_BW, RX_BW_MANT(MANT_24) | RX_BW_EXP(2) | DCC_FREQ(6));

    //Synchronisation word is set to two bytes.
    sx_write8(REG_SYNC_CONFIG, SYNC_ON(ON) | SYNC_SIZE(1));  // 1+1 = 2

    //We set the TX to start when the FIFO not empty. We set the FIFO threshold to VALUE_FIFO_THRESH (defined in sx1231.h).
    sx_write8(REG_FIFO_THRESH,
              TX_START_CONDITION(TX_CONDITION_FIFO_NOT_EMPTY) |
              FIFO_THRESHOLD(VALUE_FIFO_THRESH));

    //Packet format will be "unlimited length", so the format bit is set to FIXED_LENGTH and the payload reg is set to 0.
    //Packet format is fixed size, CRC done by hand, do not filter on address.
    sx_write8(REG_PACKET_CONFIG_1, PACKET_FORMAT(FIXED_LENGTH) |
              DC_FREE(DC_WHITENING) | CRC_ON(OFF) | CRC_AUTO_CLEAR_OFF(ON) |
              ADDRESS_FILTERING(ADDR_FILT_NONE));

    //We set the payload length to 0.
    sx_write8(REG_PAYLOAD_LENGTH, 0);

    //Initialize the procedure mutex
    chMtxInit(&phy_proc_mtx);
}


/**
 * @brief Start a physical layer procedure (needed to create mutual exclusion in procedures)
 */
void phy_proc_start(void)
{
    chMtxLock(&phy_proc_mtx);
    phy_frame_pos = 0;
    phy_frame_filled = 0;
}

/**
 * @brief Finish a physical layer procedure
 */
void phy_proc_finish(void)
{
    chMtxUnlock();
}

#define FEC_MAX_SIZE 0
/*
 * @brief add a frame to the send buffer
 *
 * @param frame pointer to the frame
 * @param size  size of the frame
 *
 * @return number of avaible frames in the buffer after adding the frame (if the buffer)
 * is full, returns PHY_BUFFER_FULL, and if the size becomes a problem due to FEC returns
 * PHY_SIZE_PROBLEM
 *
 * OBS: when a frame is given it is supposed that size is the size of the payload without 
 * the size (it'll be included by the physical layer)
 */
int phy_add_frame(uint8_t *frame)
{
    uint8_t size;

    if(phy_conf.pack_class == BACKGROUND_CLASS)
        size = 5;
    else
        size = frame[0];

    if(phy_frame_pos == N_FRAMES)
        return PHY_BUFFER_FULL;
    if(phy_conf.fec && (size > FEC_MAX_SIZE))
        return PHY_SIZE_PROBLEM;

    if(phy_conf.fec){
        /* TODO: Treat the fec case*/
    } else {
        for(int i = 0; i < size; i++)
            phy_frame_buf[phy_frame_pos][i] = frame[i];
    }

    uint16_t crc = 0;
    for(int i = 0; i < size; i++)
        crc = crc16_calc(crc,phy_frame_buf[phy_frame_pos][i]);
    phy_frame_buf[phy_frame_pos][size] = (crc >> 8);
    phy_frame_buf[phy_frame_pos][size + 1] = (crc & 0xFF);

    phy_frame_pos++;
    return (N_FRAMES - phy_frame_pos);
}

/**
 * @brief send all frames in buffer in one packet
 * @param ti timeout for the CCA in Dash7 time units
 * @return PHY_CHANNEL_BUSY or PHY_SEND_SUCCESS
 */
int phy_send_packet(d7_ti ti)
{
    // Wait for RSSI level
    sx_mode(RX);
    rssi_on_dio4();
    int channel_clear = wait_cca_timeout(d7_ti_to_systime(ti));
    rx_ready_on_dio4();
    if (!channel_clear) {
        sx_mode(STDBY);
        return PHY_CHANNEL_BUSY;
    }

    sx_mode(STDBY);

    packet_sent_on_dio0();

    sx_mode(TX);

    for(unsigned int fi = 0; fi < phy_frame_pos; fi++) {
        unsigned int size;

        if(phy_conf.fec){
            /* TODO: Calculate the true size of the frame */
        } else {
            if(phy_conf.pack_class == BACKGROUND_CLASS)
                size = 7; //background frame size + crc
            else
                size = phy_frame_buf[fi][0] + 2;
        }

        for(unsigned int fj = 0; fj < size;){
            wait_not_fifo_level();
            unsigned int sent = min(size - fj, FIFO_SIZE - VALUE_FIFO_THRESH -1);
            sx_write(REG_FIFO, &phy_frame_buf[fi][fj],sent);
            fj += sent;
        }
    }
    wait_packet_sent();
    assert(!check_fifo_not_empty());
    chThdSleepMilliseconds(10);
    sx_mode(STDBY);
    return PHY_SEND_SUCCESS;
}

int phy_receive_packet(d7_ti ti)
{
    if(phy_conf.pack_class == BACKGROUND_CLASS)
        return phy_receive_background(ti);
    else
        return phy_receive_foreground(ti);
}

int phy_get_frame(uint8_t *frame, int max_size)
{
    if(phy_frame_pos >= phy_frame_filled) return PHY_NO_MORE_FRAMES;
    uint16_t crc_calc = 0;
    uint16_t crc_given = 0;
    int frame_end = 0;

    if(phy_conf.pack_class == BACKGROUND_CLASS)
        frame_end = 5;
    else
        frame_end = phy_frame_buf[phy_frame_pos][0];

    if(frame_end > max_size)
        return PHY_INSUFFICIENT_SPACE;
    //Get the received crc
    crc_given = phy_frame_buf[phy_frame_pos][frame_end];
    crc_given = (crc_given << 8) & 0xFF00;
    crc_given += phy_frame_buf[phy_frame_pos][frame_end + 1];
    //Calculate the crc
    for (int i = 0; i < frame_end; i++) {
        crc_calc = crc16_calc(crc_calc,phy_frame_buf[phy_frame_pos][i]);
    }

    if (crc_calc != crc_given) {
        phy_frame_pos++;
        return PHY_CRC_ERROR;
    }

    for (int i = 0; i < frame_end; i++)
        frame[i] = phy_frame_buf[phy_frame_pos][i];

    phy_frame_pos++;
    return frame_end;
}

uint8_t phy_packet_rssi(void)
{
    return phy_rec_rssi;
}
