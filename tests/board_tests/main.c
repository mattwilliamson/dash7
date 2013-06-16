#include "ch.h"
#include "hal.h"
#include "waded_usb.h"
#include "sx1231.h"
#include "dash7.h"
#include "dio.h"
#include "fram.h"
#include "led.h"

int main (void)
{
    halInit();
    chSysInit();
    fram_init();
    usb_init();
    sx_init();
    dash7_init();
    dio_init();
    led_init();

    chThdSleepSeconds(10);

    usb_puts("Hardware test beggin\n");
    /* USB test */
    usb_puts("If you are reading this, the USB is working =)\n");

    /* fram test */
    usb_puts("--fram test:\n");
    usb_puts("This test writes in the fram and than reads to check if it was done.\n");
    fram_write32(0,0);
    fram_write32(4,1);
    fram_write32(8,2);
    fram_write32(12,8);
    fram_write32(16,64);
    fram_write32(20,512);
    fram_write32(24,1024);
    if(fram_read32(0) != 0) usb_puts("failled to read from address 0.\n");
    if(fram_read32(4) != 1) usb_puts("failled to read from address 4.\n");
    if(fram_read32(8) != 2) usb_puts("failled to read from address 8.\n");
    if(fram_read32(12) != 8) usb_puts("failled to read from address 12.\n");
    if(fram_read32(16) != 64) usb_puts("failled to read from address 16.\n");
    if(fram_read32(20) != 512) usb_puts("failled to read from address 20.\n");
    if(fram_read32(24) != 1024) usb_puts("failled to read from address 24.\n");

    /* Led test */
    usb_puts("--Led test:\n");
    usb_puts("The led is supposed to be blinking, can you please check that for me?\n");
    for(int i = 0; i < 5; i++){
        led_toggle();
        chThdSleepSeconds(1);
    }
    usb_puts("if it didn't blink, then we have a problem.\n");

    return 0;
}
