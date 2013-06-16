/**
 * @file  global.h
 * @brief Define global define to change the compilation mode.
 */

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include "ch.h"

#ifdef NDEBUG
#define assert(x) (void) x;
#else
static inline void assert(int condition)
{
    if (!condition)
        chDbgPanic("assert failed");
}
#endif

#endif // __GLOBAL_H__
