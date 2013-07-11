#include "ch.h"
#include "hal.h"
#include "waded_usb.h"
#include "sx1231.h"
#include "dio.h"
#include "fram.h"
#include "led.h"
#include "random.h"

#include "dash7/phy.h"
#include "dash7/data.h"
#include "dash7/bg_frame.h"

#define STARTER_MODE 0
#define REPLY_MODE 1

//#define TEST_MODE STARTER_MODE
#define TEST_MODE REPLY_MODE
//#define TEST_MODE 10

uint8_t buffer[20] = {10,0,0x06,4,5,6,7,8,9,1};

int main (void)
{
    int error;
    //phy_param_t conf;
    systime_t time1, time2;

    halInit();
    chSysInit();
    fram_init();
    usb_init();
    sx_init();
    dio_init();
    led_init();

    chThdSleepSeconds(3);

    usb_puts("Software test beggin\n");

    /*This first section must be changed for a porper function */
    data_conf.CSMAen = 1;
    data_conf.Ecca = 110; //standard is 110, but the channel is always busy in this case 
    data_conf.Tbsd = 100; 
    data_conf.Tca = 100;
    data_conf.Tfsd = 50;
    data_conf.Tg = 10; 
    data_conf.Tl = 100; 
    /* bad section end */

    data_proc_param_t proc_conf;
    //proc_conf.channel = 0x12;
    proc_conf.channel = CH_NORMAL_0;
    proc_conf.pack_class = BACKGROUND_CLASS;
//    proc_conf.pack_class = FOREGROUND_CLASS;
    proc_conf.my_subnet = 0x06;
    proc_conf.fec = 0;

    rand_init(30);

    usb_puts("Configurations ready\n");
    phy_init();

    //systime_t time;

    while(random()){
        data_proc_start();
        data_proc_config(&proc_conf);
#if TEST_MODE == STARTER_MODE
        data_proc_config(&proc_conf);
        usb_puts("Background Send\n");
        time1 = chTimeNow();
        error = advp_send(buffer,1000, 0x10, 0x10);
        time2 = chTimeNow();
        usb_printf(" time = %u \n",time2-time1);
        usb_printf(" error = %d \n",error);
        usb_puts("_____________________________________\n");
        chThdSleepMilliseconds(5000);
#elif TEST_MODE == REPLY_MODE
        usb_puts("RECEIVING Message message\n");
        time1 = chTimeNow();
        error = advp_scan();
        time2 = chTimeNow();
        usb_printf(" time = %u \n",time2-time1);
        if(error > 0){
            data_get_frame(buffer,20);
            usb_puts("Message received!\n");
            for(int i = 0; i < buffer[0]; i++)
                usb_printf("%d ",buffer[i]);
            usb_puts("\n");
        } else
            usb_printf(" error = %d \n",error);
        usb_puts("_____________________________________\n");
        chThdSleepMilliseconds(900);
#else
        time1 = 0;
        if (time1) {
            time2 = 0;
            if (time2) time1 = 0;
        }
        error = data_listen();
        if(error > 0){
            error = phy_get_frame(buffer,20);
            if (error < 0) 
                usb_printf(" get_error = %d \n",error);
            for(int i = 0; i < buffer[0]; i++)
                usb_printf("%d ",buffer[i]);
            usb_puts("\n");
            //proc_conf.pack_class = !proc_conf.pack_class;
            //data_proc_config(&proc_conf);
        } else {
            usb_printf("erro = %d \n",error);
        }
        /*
        sx_mode(RX);
        rssi_on_dio4();
        error = wait_cca_timeout(0);
        sx_mode(STDBY);
        usb_printf("rssi = %d \n",error);
        */
#endif
        data_proc_finish(); 
    }

    usb_puts("--Test end\n");
    return 0;
}
