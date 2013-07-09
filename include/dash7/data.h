#ifndef DASH7_DATA_LAYER
#define DASH7_DATA_LAYER

#include <stdint.h>
#include <dash7/phy.h>

/**
 data_param_t keep the parameters that are
 used by the data layer and don't change in each
 procedure
 */
typedef struct data_param {
    uint8_t CSMAen : 1;
    uint8_t Ecca;
    //uint8_t Esm; //used as Ecca
    //uint8_t FPP; //aways used as MFPP
    //uint8_t MFPP; //defined phy.h as a constant value
    //uint16_t RTSM;
    //uint16_t RTSV;
    uint16_t Tbsd;
    uint16_t Tc;
    uint16_t Tca;
    uint16_t Tfsd;
    uint16_t Tg;
    uint16_t Tgd;
    uint16_t Tl;
    //uint16_t Tnse; //not needed
} data_param_t;

/**
 data_proc_param_t are the values that
 are specific to a procedure
 */
typedef struct data_proc_param {
    uint8_t channel;
    uint8_t pack_class;
    uint8_t next_scan;
    uint8_t tx_eirp;
    uint8_t fec;
    uint8_t my_subnet;
} data_proc_param_t;

/**
 these variable should be invisible so extern library
 can have access to their values
 */
extern data_param_t data_conf;
extern data_proc_param_t data_proc_conf;

//subnet will be represented by an uint8_t
/**
 return the specifier from the subnet
 */
static inline uint8_t data_subnet_specifier(uint8_t subnet)
{
    return ((subnet >> 4) & 0xF);
}

/**
 return the mask from the subnet
 */
static inline uint8_t data_subnet_mask(uint8_t subnet)
{
    return (subnet & 0xF);
}

/**
 Verify if the device subnet specifier match the frame subnet specifier
 */
static inline uint8_t data_subnet_specifier_verify(uint8_t dss, uint8_t fss)
{
    if(fss == 0xF) return 1;
    if(fss == dss) return 1;
    return 0;
}

/**
 Verify if the device mask match the frame's
 */
static inline uint8_t data_subnet_mask_verify(uint8_t dsm, uint8_t fsm)
{
    return ((fsm & dsm) == dsm);
}

/**
 Verify if two subnet matches
 */
static inline uint8_t data_subnet_verify(uint8_t ds, uint8_t fs)
{
    if(data_subnet_specifier_verify(data_subnet_specifier(ds),data_subnet_specifier(fs)))
        return data_subnet_mask_verify(data_subnet_mask(ds),data_subnet_mask(fs));
    return 0;
}

//uid will be represented by an uint64_t
/**
 return the manufacturer part from an ID
 */
static inline uint64_t data_uid_manufID(uint64_t uid)
{
    return ((uid >> 48) & 0xFF);
}

/**
 Return the extension from an ID
 */
static inline uint64_t data_uid_ext(uint64_t uid)
{
    return ((uid >> 40) & 0xF);
}

/**
 Return the serial number from an ID
 */
static inline uint64_t data_uid_serial(uint64_t uid)
{
    return (uid & 0xFFFFF);
}

/**
 Set a manufacturer in an ID
 Return the uid with the manufID set
 */
static inline uint64_t data_uid_set_manufID(uint64_t uid, uint64_t manufID)
{
    return (manufID << 48) & (uid & 0xFFFFFF);
}

/**
 Set an extension in an ID
 Return the uid with the extension set
 */
static inline uint64_t data_uid_set_ext(uint64_t uid, uint64_t ext){
    return (uid & 0xFF0FFFFF) & (ext << 40);
}

/**
 Set a serial number in an ID
 Return the uid with the serial set
 */
static inline uint64_t data_uid_set_serial(uint64_t uid, uint64_t serial)
{
    return (uid & 0xFFF00000) & serial;
}

/**
 start a procedure
 */
static inline void data_proc_start(void)
{
    phy_proc_start();
}

/**
 finish a procedure
 */
static inline void data_proc_finish(void)
{
    phy_proc_finish();
}

/**
 add a frame
 */
static inline int data_add_frame(uint8_t *frame)
{
    return phy_add_frame(frame);
}

/**
 get a received frame
 */
static inline int data_get_frame(uint8_t *frame, int max_size)
{
    return phy_get_frame(frame, max_size);
}

/*Sends a packet when the channel is already protected (no need to check for colisions)
  return values are the same from data_send_packet, but never returns DATA_SEND_TIMEOUT */
//static int data_send_packet_protected(void)
//{
//    return phy_send_packet(0);
//}
#define data_send_packet_protected() phy_send_packet(0,FALSE)

/**
 Configure a procedure from the data layer 
 also configure the physical layer
 */
void data_proc_config(data_proc_param_t *conf);

/**
 This function will is called before sending a packet
 so the guard times especified by the spec can be
 respected.
 If the using the data_send_packet, there is no need
 to use this function, it is only here to help in case of need
 */
#define SAFE_SLEEP_WINDOW 5
#define DATA_GUARD_FAIL -1
#define DATA_GUARD_SUCCESS 0
int data_guard_break(d7_ti time);

/**
 This function will wait a random amount of time.
 
 If the using the data_send_packet, there is no need
 to use this function, it is only here to help in case of need
 */
#define DATA_CSMA_SUCCESS 0
#define DATA_CSMA_FAIL -1
int data_csma(d7_ti time);

/**
 Sends a packet respecting the data layer conversation protocol
 */
#define DATA_SEND_SUCCESS 0
#define DATA_CHANNEL_BUSY -1
#define DATA_SEND_TIMEOUT -2
int data_send_packet(void);

/**
 Listen for a packet (already filter by subnet)
 */
#define DATA_LISTEN_TIMEOUT -1
#define DATA_LISTEN_ERROR -2
#define DATA_LISTEN_EMPTY -3
int data_listen(void);

/**
 Always used after a data_send_packet
 is called only when the an answer for the sended
 packet is expected.
 This function will leave the procedure for the unused time and
 will return when needed.
 */
#define DATA_DIALOGUE_TIMEOUT -4
int data_dialogue(systime_t sended, d7_ti);

/**
 Called before answering a packet.
 This is needed since a time window is expected between receiving and
 answerting a packet, this function will wait for that time window
 and when it return, data_add_frame and data_send_packet should be
 called.
 */
#define DATA_ANSWER_TIMEOUT -1
#define DATA_ANSWER_READY 0
int data_answer_time(systime_t received, d7_ti Tc);

#endif
