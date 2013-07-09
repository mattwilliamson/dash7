#ifndef DASH7_PHYSICAL_LAYER
#define DASH7_PHYSICAL_LAYER

#include <stdint.h>
#include <dash7/conf.h>
#include "hal.h"
#include "ch.h"

/**
 * Channel possible values
 */

#define CH_BASE 0x00
#define CH_LEGACY 0x01
#define CH_NORMAL_0 0x10
#define CH_NORMAL_1 0x12
#define CH_NORMAL_2 0x14
#define CH_NORMAL_3 0x16
#define CH_NORMAL_4 0x18
#define CH_NORMAL_5 0x1A
#define CH_NORMAL_6 0x1C
#define CH_NORMAL_7 0x1E
#define CH_HI_RATE_0A 0x21
#define CH_HI_RATE_1A 0x25
#define CH_HI_RATE_2A 0x29
#define CH_HI_RATE_3A 0x2D
#define CH_HI_RATE_0B 0x23
#define CH_HI_RATE_1B 0x27
#define CH_HI_RATE_2B 0x2B
#define CH_BLINK_0 0x32
#define CH_BLINK_1 0x3C

/*
 * Packet class values
 */
#define BACKGROUND_CLASS 0b0
#define FOREGROUND_CLASS 0b1

/*
 *fec values
 */
#define WITHOUT_FEC 0
#define WITH_FEC 1

typedef struct phy_param{
    uint8_t channel;
    uint8_t preamble;
    uint8_t fec : 1;
    uint8_t pack_class : 1;
    uint8_t rssi_thresh;
    uint8_t tx_eirp; //gain value from sx1231.h
} phy_param_t;

// Buffer for the frames
extern uint8_t phy_frame_buf[N_FRAMES][FRAME_SIZE];

void phy_config(phy_param_t *conf);
void phy_init(void);
void phy_proc_start(void);
void phy_proc_finish(void);
#define PHY_SIZE_PROBLEM -2
#define PHY_BUFFER_FULL -1
int phy_add_frame(uint8_t *frame);
#define PHY_SEND_SUCCESS 0
#define PHY_CHANNEL_BUSY -1
int phy_send_packet(d7_ti ti, uint8_t ca);
#define NEXT_BYTE_TIMEOUT 20
#define PHY_REC_TIMEOUT -1
#define PHY_BYTE_TIMEOUT -2
#define PHY_BAD_SIZE -3
int phy_receive_packet(d7_ti ti);
#define PHY_NO_MORE_FRAMES -1
#define PHY_INSUFFICIENT_SPACE -2
#define PHY_CRC_ERROR -3
int phy_get_frame(uint8_t *frame, int max_size);
uint8_t phy_packet_rssi(void);

#endif //DASH7_PHYSICAL_LAYER
