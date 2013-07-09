#ifndef DASH7_CONFIGURATIONS
#define DASH7_CONFIGURATIONS

#include <stdint.h>
#include "hal.h"
#include "ch.h"

/***************************
PHYSICAL LAYER
***************************/
/**
 * N_FRAMES
 * defines the maximum number of frames that can
 * exist in a packet.
 *
 * This value can go from 1 to 255, but the program
 * uses a buffer to store all the frames, so in
 * give user controll of the buffer size this
 * variable was created
 */
#define N_FRAMES 5

#define FRAME_SIZE 257

/********************
TIME UNITS
********************/
typedef uint16_t d7_ti;
typedef uint16_t d7_sti;

static inline systime_t d7_ti_to_systime(d7_ti ti)
{
    return ((ti*977)/1000);
}

static inline d7_ti systime_to_d7_ti(systime_t milli)
{
    return (int)((milli*1000)/977);
}

#endif //DASH7_CONFIGURATIONS
