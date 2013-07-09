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

#define STARTER_MODE 0
#define REPLY_MODE 1

#define TEST_MODE STARTER_MODE
//#define TEST_MODE REPLY_MODE

uint8_t buffer[20] = {10,0,0x06,4,5,6,7,8,9,1};

int main (void)
{
    int error;
    //phy_param_t conf;

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
    //data_conf.Esm = 180; //so far I've been using Ecca like Esm, see what needs to change 
    //data_conf.FPP = 5; 
    //data_conf.MFPP = 5; //This one is useless since it is done by a define
    data_conf.Tbsd = 50; 
    data_conf.Tc = 500; //This should be done by the payload, but this is for testing
    data_conf.Tca = 100;
    data_conf.Tfsd = 20;
    data_conf.Tg = 10; 
    data_conf.Tgd = 5; /* Not beeing used yet */ 
    data_conf.Tl = 100; 
    /* bad section end */

    data_proc_param_t proc_conf;
    proc_conf.channel = CH_NORMAL_0;
    proc_conf.pack_class = FOREGROUND_CLASS;
    proc_conf.my_subnet = 0x06;
    proc_conf.fec = 0;

    rand_init(30);
    phy_init();
    data_proc_config(&proc_conf);

    usb_puts("Configurations ready\n");

    systime_t time;

    while(random()){
        data_proc_start();
#if TEST_MODE == STARTER_MODE
        data_proc_config(&proc_conf);
        usb_puts("Sending message\n");
        error = data_add_frame(buffer); 
        usb_printf("data_add_frame result=%d\n",error);
        if(error < 0)
            usb_puts("data_add_frame\n");
        if(error == -1)
            usb_puts("PHY_CBUFFER_FULL\n");
        if(error == -2)
            usb_puts("PHY_SIZE_PROBLEM\n");
        error = data_send_packet();
        time = chTimeNow(); 
        //error = phy_send_packet(0); 
        usb_printf("data_send_packet result=%d\n",error);
        usb_puts("sended?\n");
        if(error == -2)
            usb_puts("DATA_SEND_TIMEOUT\n");
        if(error == -1)
            usb_puts("DATA_CHANNEL_BUSY\n");
        error = data_dialogue(time,data_conf.Tc);
        time = chTimeNow(); 
        usb_printf("data_dialogue result=%d\n",error);
        if(error >= 0){
            data_get_frame(buffer,20);
            for(int i = 0; i < buffer[0]; i++)
                usb_printf("%d ",buffer[i]);
            usb_puts("\n");
            buffer[6]++;
            chThdSleepMilliseconds(30);
            error = data_add_frame(buffer); 
            data_answer_time(time,data_conf.Tc);
            error = data_send_packet_protected();
            usb_printf("data_send_packet result=%d\n",error);
        }
        usb_puts("_____________________________________\n");
        chThdSleepMilliseconds(3);
#else
        usb_puts("RECEIVING Message message\n");
        error = phy_receive_packet(1000);
        time = chTimeNow();
        usb_printf("data_listen result=%d\n",error);
        /* SHOW PACKET */
        if(error >= 0){
            data_get_frame(buffer,20);
            for(int i = 0; i < buffer[0]; i++)
                usb_printf("%d ",buffer[i]);
            usb_puts("\n");
            buffer[7]++;
            data_answer_time(time,data_conf.Tc);
            error = data_add_frame(buffer); 
            error = data_send_packet_protected();
            usb_printf("data_send_packet_protected result=%d\n",error);
            time = chTimeNow(); 
            error = data_dialogue(time,data_conf.Tc);
            usb_printf("data_dialogue result=%d\n",error);
            if(error >= 0){
                data_get_frame(buffer,20);
                for(int i = 0; i < buffer[0]; i++)
                    usb_printf("%d ",buffer[i]);
                usb_puts("\n");
            }
        }
        
        usb_puts("_____________________________________\n");
#endif
        data_proc_finish(); 
    }

    usb_puts("--Test end\n");
    return 0;
}
