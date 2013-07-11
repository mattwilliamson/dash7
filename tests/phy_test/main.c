#include "ch.h"
#include "hal.h"
#include "waded_usb.h"
#include "sx1231.h"
#include "dio.h"
#include "fram.h"
#include "led.h"

#include "dash7/phy.h"

#define REC_FOREGROUND_MODE 0
#define REC_BACKGROUND_MODE 1
#define SEND_FOREGROUND_MODE 2
#define SEND_BACKGROUND_MODE 3

#define TEST_MODE SEND_FOREGROUND_MODE

uint8_t buffer[500] = {8,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49};

int main (void)
{
    int error;
    phy_param_t conf;

    halInit();
    chSysInit();
    fram_init();
    usb_init();
    sx_init();
    dio_init();
    led_init();

    chThdSleepSeconds(3);

    usb_puts("Hardware test beggin\n");
    /* USB test */
    usb_puts("If you are reading this, the USB is working =)\n");

    phy_init();
#if (TEST_MODE == REC_FOREGROUND_MODE)
    for(int i = 0; i < 50; i++) buffer[i] = 0;
    usb_puts("Receive foreground test\n");
    conf.channel = CH_NORMAL_0;
    conf.preamble = 0;
    conf.fec = WITHOUT_FEC;
    conf.pack_class = FOREGROUND_CLASS;
    conf.rssi_thresh = 100;
    conf.tx_eirp = GAIN_0;

    phy_config(&conf);
    while(1) {
        phy_proc_start();
        error = phy_receive_packet(3000);
        if(error > 0){
            usb_puts("packet received!");
            error = phy_get_frame(buffer,500);
            usb_puts("rssi value = ");
            usb_printf("%u\n",phy_packet_rssi());
            if(error == PHY_CRC_ERROR) {
                usb_puts("crc error\n");
            } else {
            for(int i = 0; i < buffer[0]; i++)
                usb_printf("%u ",buffer[i]);
            }
            usb_puts("\n"); 
        } else if (error == PHY_REC_TIMEOUT){
            usb_puts("rec timeout\n");
        } else if (error == PHY_BYTE_TIMEOUT){
            usb_puts("byte timeout\n");
        }

        phy_proc_finish();
    }
#elif (TEST_MODE == REC_BACKGROUND_MODE)
    for(int i = 0; i < 50; i++) buffer[i] = 0;
    usb_puts("Receive background test\n");
    conf.channel = CH_NORMAL_0;
    conf.preamble = 0;
    conf.fec = WITHOUT_FEC;
    conf.pack_class = BACKGROUND_CLASS;
    conf.rssi_thresh = 100;

    phy_config(&conf);
    while(1) {
        phy_proc_start();
        error = phy_receive_packet(3000);
        if(error > 0){
            usb_puts("packet received!");
            error = phy_get_frame(buffer,500);
            usb_puts("rssi value = ");
            usb_printf("%u\n",phy_packet_rssi());
            if(error == PHY_CRC_ERROR) {
                usb_printf("get_frame error: %u ",error);
                usb_puts("\n");
            } else {
            for(int i = 0; i < error; i++)
                usb_printf("%u ",buffer[i]);
            }
            usb_puts("\n"); 
        } else if (error == PHY_REC_TIMEOUT){
            usb_puts("rec timeout\n");
        } else if (error == PHY_BYTE_TIMEOUT){
            usb_puts("byte timeout\n");
        }

        phy_proc_finish();
    }
#elif (TEST_MODE == SEND_FOREGROUND_MODE)
    usb_puts("Send foreground test\n");
    conf.channel = CH_NORMAL_0;
    conf.preamble = 0;
    conf.fec = WITHOUT_FEC;
    conf.pack_class = FOREGROUND_CLASS;
    conf.rssi_thresh = 100;

    phy_config(&conf);
    while(1) {
        phy_proc_start();
        phy_add_frame(buffer);
        error = phy_send_packet(20,TRUE);
        if(error == PHY_SEND_SUCCESS){
            usb_puts("send succes\n");
        } else {
            usb_puts("channel busy\n");
        }
        phy_proc_finish();
    }

#elif (TEST_MODE == SEND_BACKGROUND_MODE)
    usb_puts("Send background test\n");
    conf.channel = CH_NORMAL_0;
    conf.preamble = 0;
    conf.fec = WITHOUT_FEC;
    conf.pack_class = BACKGROUND_CLASS;
    conf.rssi_thresh = 100;

    phy_config(&conf);
    while(1) {
        phy_proc_start();
        phy_add_frame(buffer);
        error = phy_send_packet(20,TRUE);
        if(error == PHY_SEND_SUCCESS){
            usb_puts("send succes\n");
        } else {
            usb_puts("channel busy\n");
        }
        phy_proc_finish();
    }
#endif

    usb_puts("--Test end\n");
    return 0;
}
