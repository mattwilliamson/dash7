#ifndef BF_FRAME_PROT
#define BF_FRAME_PROT

#include "dash7/data.h"

/*
 returns the subnet from a bg frame
 */
static inline uint8_t bg_get_subnet(uint8_t *frame)
{
    return frame[0];
}

/*
 sets the subnet in a bg frame
 */
static inline void bg_set_subnet(uint8_t *frame, uint8_t subnet)
{
    frame[0] = subnet;
}

/*
 Parameters that are needed to create a bg frame
 */
typedef struct bg_param {
    uint8_t protocol;
    uint8_t channel;
    uint16_t time;
} bg_param_t;

/*
 Uses the parameters to initialize a bg frame for the advertising protocol
 */
static inline void bg_set_advp(uint8_t *frame, bg_param_t *conf)
{
    frame[1] = 0xF0;
    frame[2] = conf->channel;
    frame[3] = ((conf->time >> 8) & 0xFF);
    frame[4] = (conf->time & 0xFF);
}

/*
 Uses the parameters to initialize a bg frame for the reservation protocol
 */
static inline void bg_set_rsvp(uint8_t *frame, bg_param_t *conf)
{
    frame[1] = 0xF1;
    frame[2] = conf->channel;
    frame[3] = ((conf->time >> 8) & 0xFF);
    frame[4] = (conf->time & 0xFF);
}

/*
 Get the parameters informed in an advp frame
 */
static inline void bg_get_advp(uint8_t *frame, bg_param_t *conf)
{
    conf->protocol = frame[1];
    conf->channel = frame[2];
    conf->time = frame[4] + (frame[3] << 8);
}

/*
 Scans for an advp frame, should not be called by the user
 since advp_scan does that automatically
 The parameters from the advp frame are placed in advp_conf
 */
#define BG_BAD_PROTOCOL 1
int advp_scan_bg(bg_param_t *advp_conf);

/*
 Scans an advp frame, and uses it's parameters to then scan the expected foreground frame.
 If error > 10, error -10 is the value returned by the foreground scan
 if error < 10, error is the value returned by the background scan
 */
int advp_scan(void);

/*
 Inform the advp channel, the targeted channel and the advertising time and
 this function will send the respectives advp frames and the fg frame
 */
int advp_send(uint8_t *frame, d7_ti time, uint8_t bg_channel, uint8_t fg_channel);

/*
 Flood the channel until the specified time is reached
 */
int rsvp(systime_t end);

#endif
