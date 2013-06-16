/**
 * @file  led.h
 * @brief Some function for the LED.
 * */

#ifndef __LED_H__
#define __LED_H__

#ifndef HAL_H
#define HAL_H
#include "hal.h"

// LEDs control
#define LED_PWMD &PWMD2

#define LED_CHANNEL 2

// Initial value of the brigthness of the LCD (between 0 and 256)
#define LCD_BRIGHTNESS_INIT 255
#endif // HAL_H

/**
 * @brief Initialize PWM for LEDs.
 */
void led_init(void);

/**
 * Start a thread, doing a animation with the led.
 */
void led_thread(void);

/**
 * @brief Toggle the LED.
 */
void led_toggle(void);

/**
 * @brief power the led with pwd intensity.
 */
void led_power(unsigned int pwd);

#endif // __LED_H__
