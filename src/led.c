/**
 * @file  led.c
 * @brief Some function for the LED.
 * */

#include "led.h"

/**
 * Configuration of PWM from Timer 2 for the LED.
 */
static const PWMConfig pwmcfg2 = {
    20000,       // 78.125Hz PWM clock frequency.
    256,         // Period.
    NULL,        // No callback.
    {
        {PWM_OUTPUT_DISABLED, NULL},    // Nothing.
        {PWM_OUTPUT_DISABLED, NULL},    // Nothing.
        {PWM_OUTPUT_ACTIVE_HIGH, NULL}, // Led.
        {PWM_OUTPUT_DISABLED, NULL},    // Nothing.
    },
    0
};

void led_init(void) {
    pwmStart(LED_PWMD, &pwmcfg2);
}

static char toggle = 0; /**< The last state of the LED, used to toggle it. */

void led_power(unsigned int pwd){
    if(pwd > 255) pwd = 255;
    pwmEnableChannel(LED_PWMD, LED_CHANNEL, pwd);
}

void led_toggle(void)
{
    if(toggle)
        pwmEnableChannel(LED_PWMD, LED_CHANNEL, 0);
    else
        pwmEnableChannel(LED_PWMD, LED_CHANNEL, 255);
    toggle = !toggle;
}
