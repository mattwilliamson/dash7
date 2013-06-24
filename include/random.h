/**
 * @file  random.h
 * @brief Function generating random numbers.
 */

#ifndef __RANDOM_H__
#define __RANDOM_H__

#include <stdint.h>

#define rand_mult 16807
#define rand_mod 2147483647

static uint32_t random_value;

/**
 * @brief Initialise the random number generator.
 *
 * @param seed  The initialiseing value of the generator.
 */
static void rand_init(uint32_t seed)
{
    random_value = seed;
}

/**
 * @brief Get a random value.
 *
 * @return A pseudo random value.
 */
static uint32_t random(void)
{
    return random_value = (random_value * rand_mult) % rand_mod;
}

#endif // __RANDOM_H__
