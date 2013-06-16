// This code belongs to Samuel Tardieu. It was originally taken from his blog
// and can be found at the followin address:
// http://www.rfc1149.net/blog/2013/04/03/sleeping-just-the-right-amount-of-time/
#include "sleep_until.h"

void sleepUntil(systime_t *previous, systime_t period)
{
    systime_t future = *previous + period;
    chSysLock();
    systime_t now = chTimeNow();
    int mustDelay = now < *previous ?
        (now < future && future < *previous) :
        (now < future || future < *previous);
    if (mustDelay)
        chThdSleepS(future - now);
    chSysUnlock();
    *previous = future;
}
