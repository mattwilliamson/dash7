/**
 * @file  sleep_until.h
 * @brief A function that sleep until a given date.
 */

#ifndef __SLEEP_UNTIL_H__
#define __SLEEP_UNTIL_H__

#include "ch.h"

void sleepUntil(systime_t *previous, systime_t period);

#endif // __SLEEP_UNTIL_H__
