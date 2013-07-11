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
#include "pn9.h"

#define ROUND(x) ((uint32_t) ((x) + 0.5))

// Convert a frequency in MHz to its FRF/FDEV equivalent
#define FREQ(d) ROUND(((d) * 1e6) / (32e6 / (1 << 19)))

//N_FRAMES defined in dash7/conf.h
uint8_t phy_frame_buf[N_FRAMES][FRAME_SIZE];
uint8_t phy_frame_size[N_FRAMES];
uint8_t phy_frame_pos;
uint8_t phy_frame_filled;
uint8_t phy_rec_rssi;
#define PROC_START 0
#define PROC_LISTEN 1
#define PROC_SEND 2
uint8_t proc_event;
static Mutex phy_proc_mtx;


static const uint32_t center_frequencies[] = {
    FREQ(433.164), FREQ(433.272), FREQ(433.380), FREQ(433.488),
    FREQ(433.596), FREQ(433.704), FREQ(433.812), FREQ(433.920),
    FREQ(434.028), FREQ(434.136), FREQ(433.244), FREQ(434.352),
    FREQ(434.460), FREQ(434.568), FREQ(434.676)
};
/*
static const uint32_t bandwidth[] = {
    FREQ(0.432 / 2), FREQ(0.216 / 2), FREQ(0.432 / 2), FREQ(0.648 / 2)
};
*/
static inline size_t min(size_t a,size_t b)
{
    return a < b ? a : b;
}

/**
 Verifies if a bit in the data header that informs if there is a next frame
 in the packet
 */
static inline bool_t fg_frame_cont(uint8_t pos)
{
    return (phy_frame_buf[pos][3] & (1<<4));
}

/**
 Reset the state of the procedure if needed
 */
static inline void proc_event_handler(uint8_t wanted){
    if (wanted != proc_event) {
        phy_frame_pos = 0;
        phy_frame_filled = 0;
        proc_event = wanted;
    }
}

/**
 Handles the reception of a foreground packet
 */ 
static int phy_receive_foreground(d7_ti ti)
{
    int fi; //frame iterator
    size_t size;
    size_t bytes_read;
    uint8_t byte_timeout = 0;
    uint8_t bad_size = 0;
    uint8_t lsfr1 = 0xFF;
    uint8_t lsfr2 = 1;
    payload_ready_on_dio0();
    sx_mode(RX);
    systime_t timeout = d7_ti_to_systime(ti); 
    
    //wait for the arrival of a packet
    if (!wait_message(timeout)){
        sx_mode(STDBY);
        empty_fifo();
        return PHY_REC_TIMEOUT;
    }

    //calculate the rssi in the moment of the packet arrival
    phy_rec_rssi = sx_read8(REG_RSSI_VALUE);

    //start packet reception
    for (fi = 0; fi < N_FRAMES; fi++) {
        //verify timeout between bytes
        if (!wait_message(NEXT_BYTE_TIMEOUT)){
            byte_timeout = 1;
            goto _end;
        }

        //read the lenght byte
        phy_frame_buf[fi][0] = sx_read8(REG_FIFO);
        phy_frame_buf[fi][0] = pn9_calc(&lsfr1,&lsfr2,phy_frame_buf[fi][0]);
        size = phy_frame_buf[fi][0]; //length

        //check if the size is a good one
        bytes_read = 1;
        if(size < 8){
            bad_size = 1;
            goto _end;
        }

        //read the frame's bytes
        while (bytes_read < size) {
            if(!wait_message(NEXT_BYTE_TIMEOUT)){
                byte_timeout = 1;
                goto _end;
            }

            phy_frame_buf[fi][bytes_read] = sx_read8(REG_FIFO);
            phy_frame_buf[fi][bytes_read] = pn9_calc(&lsfr1,&lsfr2,phy_frame_buf[fi][bytes_read]);
            bytes_read++;
        }
        //check if there is an other frame in the packet, stops if not
        if (!fg_frame_cont(fi))
            break;
    }

_end:
    phy_frame_filled = fi+1;
    phy_frame_pos = 0;
    sx_mode(STDBY);
    empty_fifo();
    if (byte_timeout)
        return PHY_BYTE_TIMEOUT;
    if (bad_size)
        return PHY_BAD_SIZE;

    //returns the number of frames read
    return phy_frame_filled;
}

#define BG_FRAME_SIZE 5
#define BG_CRC_SIZE 7

/**
 Handles the reception of a foreground packet
 */ 
static int phy_receive_background(d7_ti ti)
{
    uint8_t lsfr1 = 0xFF;
    uint8_t lsfr2 = 1;
    systime_t timeout = d7_ti_to_systime(ti); 
    uint8_t byte_timeout = 0;
    payload_ready_on_dio0();
    sx_mode(RX);

    //wait for message arrival
    if (!wait_message(timeout))
        return PHY_REC_TIMEOUT;

    //calculate rssi
    phy_rec_rssi = sx_read8(REG_RSSI_VALUE);

    //read the size of a background frame + crc
    for (int i = 0; i < BG_CRC_SIZE; i++) {
        if(!wait_message(NEXT_BYTE_TIMEOUT)){
            byte_timeout = 1;
            goto _end;
        }

        phy_frame_buf[0][i] = sx_read8(REG_FIFO);
        phy_frame_buf[0][i] = pn9_calc(&lsfr1,&lsfr2,phy_frame_buf[0][i]);
    }

_end:
    phy_frame_filled = 1;
    sx_mode(STDBY);
    empty_fifo();
    if (byte_timeout)
        return PHY_BYTE_TIMEOUT;

    //only one frame per packet
    return 1;
}

// Convert a symbol rate in kHz to its BITRATE equivalent
#define SYMRATE(s) ROUND(32e6 / ((s) * 1e3))

//syncwords [PACK_CLASS][FEC]
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
    if(!(phy_conf.channel >> 4)) {
        sx_write24(REG_FRF_MSB, center_frequencies[7]);
        sx_write16(REG_BITRATE_MSB, SYMRATE(55.555));
    } else {
        // Select the base frequency
        sx_write24(REG_FRF_MSB, center_frequencies[phy_conf.channel & 0xf]);

        // Select the symbol rate (FSK 1.8 or FSK 0.5)
        uint8_t bandwidth_index = (phy_conf.channel >> 4) & 0x7;
        sx_write16(REG_BITRATE_MSB, SYMRATE(bandwidth_index < 2 ? 55.555 : 200));
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

    //Select 50 ohms impedance for the out circuit. And use the maximum gain.
    sx_write8(REG_LNA, LNA_ZIN(0) | LNA_GAIN_SELECT(phy_conf.tx_eirp));
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
              DC_FREE(DC_NONE) | CRC_ON(OFF) | CRC_AUTO_CLEAR_OFF(ON) |
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
    proc_event = PROC_START;
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

    //start a send operation
    proc_event_handler(PROC_SEND);

    //calculate the size of the packet
    if(phy_conf.pack_class == BACKGROUND_CLASS)
        size = 5;
    else
        size = frame[0];

    //Check if there is enough buffer
    if(phy_frame_pos == N_FRAMES)
        return PHY_BUFFER_FULL;
    if(phy_conf.fec && (size > FEC_MAX_SIZE))
        return PHY_SIZE_PROBLEM;

    if(phy_conf.fec){
        /* TODO: Treat the fec case*/
    } else {
        //copy frame to buffer
        for(int i = 0; i < size; i++)
            phy_frame_buf[phy_frame_pos][i] = frame[i];
    }
    if(phy_conf.pack_class != BACKGROUND_CLASS)
        phy_frame_buf[phy_frame_pos][0] += 2; //add the crc to the length 

    //calculate the crc of the frame
    uint16_t crc = 0xFFFF;
    for(int i = 0; i < size; i++)
        crc = crc16_calc(crc,phy_frame_buf[phy_frame_pos][i]);
    phy_frame_buf[phy_frame_pos][size] = (crc >> 8);
    phy_frame_buf[phy_frame_pos][size + 1] = (crc & 0xFF);

    phy_frame_pos++;
    //return number of avaiable frames
    return (N_FRAMES - phy_frame_pos);
}

/**
 Do the data whitening of a foreground packet
 */
static void foreground_pn9(void) 
{
    uint8_t lsfr1 = 0xFF;
    uint8_t lsfr2 = 0x1;

    for (int i = 0; i < phy_frame_pos; i++) {
        phy_frame_size[i] = phy_frame_buf[i][0]; //length
        for(int j = 0; j < phy_frame_size[i]; j++)
            phy_frame_buf[i][j] = pn9_calc(&lsfr1,&lsfr2,phy_frame_buf[i][j]);
    }
}

/**
 Do the data whitening of a background packet
 */
static void background_pn9(void) 
{
    uint8_t lsfr1 = 0xFF;
    uint8_t lsfr2 = 0x1;

    for (int i = 0; i < BG_CRC_SIZE; i++)
        phy_frame_buf[0][i] = pn9_calc(&lsfr1,&lsfr2,phy_frame_buf[0][i]);
}

/**
 * @brief send all frames in buffer in one packet
 * @param ti timeout for the CCA in Dash7 time units
 * @return PHY_CHANNEL_BUSY or PHY_SEND_SUCCESS
 */
int phy_send_packet(d7_ti ti, uint8_t ca)
{
    // Wait for RSSI level
    if(ca){
        sx_mode(RX);
        rssi_on_dio4();
        int channel_clear = wait_cca_timeout(d7_ti_to_systime(ti));
        sx_mode(STDBY);
        if (!channel_clear)
            return PHY_CHANNEL_BUSY;
    }

    packet_sent_on_dio0();

    //Do the data whitening
    if(phy_conf.pack_class == BACKGROUND_CLASS)
        background_pn9();
    else
        foreground_pn9();

    sx_mode(TX);

    for (unsigned int fi = 0; fi < phy_frame_pos; fi++) {
        unsigned int size;

        if (phy_conf.fec) {
            /* TODO: Calculate the true size of the frame */
        } else {
            if (phy_conf.pack_class == BACKGROUND_CLASS)
                size = BG_CRC_SIZE; //background frame size + crc
            else
                size = phy_frame_size[fi];
        }

        for (unsigned int fj = 0; fj < size;) {
            wait_not_fifo_level();
            unsigned int sent = min(size - fj, FIFO_SIZE - VALUE_FIFO_THRESH -1);
            sx_write(REG_FIFO, &phy_frame_buf[fi][fj],sent);
            fj += sent;
        }
    }
    wait_packet_sent();
    assert(!check_fifo_not_empty());
    sx_mode(STDBY);
    proc_event = PROC_START;
    return PHY_SEND_SUCCESS;
}

/**
 Receives a packet from the radio
 */
int phy_receive_packet(d7_ti ti)
{
    proc_event = PROC_START;
    proc_event_handler(PROC_LISTEN);
    if(phy_conf.pack_class == BACKGROUND_CLASS)
        return phy_receive_background(ti);
    else
        return phy_receive_foreground(ti);
}

/**
 Retreives a frame from the received packet
 */
int phy_get_frame(uint8_t *frame, int max_size)
{
    uint16_t crc_calc = 0xFFFF;
    uint16_t crc_given = 0;
    int frame_end = 0;

    //check if there are frames to read
    if(phy_frame_pos >= phy_frame_filled) return PHY_NO_MORE_FRAMES;

    //find frame crc position
    if(phy_conf.pack_class == BACKGROUND_CLASS)
        frame_end = BG_FRAME_SIZE;
    else
        frame_end = phy_frame_buf[phy_frame_pos][0] - 2;

    //check if there is enough space for the frame
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
    //compare crc
    if (crc_calc != crc_given) {
        phy_frame_pos++;
        return PHY_CRC_ERROR;
    }

    //copy frame to user space
    for (int i = 0; i < frame_end; i++)
        frame[i] = phy_frame_buf[phy_frame_pos][i];

    if(phy_conf.pack_class != BACKGROUND_CLASS)
        frame[0] -= 2;

    phy_frame_pos++;
    //return size of frame
    return frame_end;
}

/**
 Get the rssi from the received packet
 */
uint8_t phy_packet_rssi(void)
{
    return phy_rec_rssi;
}
