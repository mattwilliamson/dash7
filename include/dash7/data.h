#include <stdint.h>
#include <dash7/phy.h>

typedef struct data_param {
    uint8_t CSMAen : 1;
    uint8_t Ecca;
    uint8_t Esm;
    uint8_t FPP;
    uint8_t MFPP;
    uint16_t RTSM;
    uint16_t RTSV;
    uint16_t Tbsd;
    uint16_t Tc;
    uint16_t Tca;
    uint16_t Tfsd;
    uint16_t Tg;
    uint16_t Tgd;
    uint16_t Tl;
    uint16_t Tnse;
} data_param_t;

typedef struct data_proc_param {
    uint8_t channel;
    uint8_t pack_class;
    uint8_t scan_timeout;
    uint8_t next_scan;
    uint8_t fec;
    uint8_t my_subnet;
} data_proc_param_t;
/*
typedef data_param {
    uint8_t CSMAen : 1;
    uint8_t Ecca;
    uint16_t Tc;
    uint16_t Tca;
    uint16_t Tg;
    uint16_t Tl;
}
*/
//this variable should be invisible
extern data_param_t data_conf;

//subnet will be represented by an uint8_t
static inline uint8_t data_subnet_specifier(uint8_t subnet)
{
    return ((subnet >> 4) & 0xF);
}

static inline uint8_t data_subnet_mask(uint8_t subnet)
{
    return (subnet & 0xF);
}

static inline uint8_t data_subnet_specifier_verify(uint8_t dss, uint8_t fss)
{
    if(fss == 0xF) return 1;
    if(fss == dss) return 1;
    return 0;
}

static inline uint8_t data_subnet_mask_verify(uint8_t dsm, uint8_t fsm)
{
    return ((fsm & dsm) == dsm);
}

static inline uint8_t data_subnet_verify(uint8_t ds, uint8_t fs)
{
    if(data_subnet_specifier_verify(data_subnet_specifier(ds),data_subnet_specifier(fs)))
        return data_subnet_mask_verify(data_subnet_mask(ds),data_subnet_mask(fs));
    return 0;
}

//uid will be represented by an uint64_t
static inline uint64_t data_uid_manufID(uint64_t uid)
{
    return ((uid >> 48) & 0xFF);
}

static inline uint64_t data_uid_ext(uint64_t uid)
{
    return ((uid >> 40) & 0xF);
}

static inline uint64_t data_uid_serial(uint64_t uid)
{
    return (uid & 0xFFFFF);
}

static inline uint64_t data_uid_set_manufID(uint64_t uid, uint64_t manufID)
{
    return (manufID << 48) & (uid & 0xFFFFFF);
}

static inline uint64_t data_uid_set_ext(uint64_t uid, uint64_t ext){
    return (uid & 0xFF0FFFFF) & (ext << 40);
}

static inline uint64_t data_uid_set_serial(uint64_t uid, uint64_t serial)
{
    return (uid & 0xFFF00000) & serial;
}

static inline void data_proc_start(void)
{
    phy_proc_start();
}

static inline void data_proc_finish(void)
{
    phy_proc_finish();
}

static inline int data_add_frame(uint8_t *frame)
{
    return phy_add_frame(frame);
}

static inline int data_get_frame(uint8_t *frame, int max_size)
{
    return phy_get_frame(frame, max_size);
}

/*Sends a packet when the channel is already protected (no need to check for colisions)
  return values are the same from data_send_packet, but never returns DATA_SEND_TIMEOUT */
static int data_send_packet_protected(void)
{
    return phy_send_packet(0);
}

void data_proc_config(data_proc_param_t *conf);
#define SAFE_SLEEP_WINDOW 2
#define DATA_GUARD_FAIL -1
#define DATA_GUARD_SUCCESS 0
int data_guard_break(d7_ti time);
#define DATA_CSMA_SUCCESS 0
#define DATA_CSMA_FAIL -1
int data_csma(d7_ti time);
#define DATA_SEND_SUCCESS 0
#define DATA_CHANNEL_BUSY -1
#define DATA_SEND_TIMEOUT -2
int data_send_packet(void);
#define DATA_LISTEN_TIMEOUT -1
#define DATA_LISTEN_ERROR -2
#define DATA_LISTEN_EMPTY -3
int data_listen(void);
#define DATA_DIALOGUE_TIMEOUT -4
int data_dialogue(void);
#define DATA_ANSWER_TIMEOUT -1
#define DATA_ANSWER_READY 0
int data_answer_time(systime_t received);
