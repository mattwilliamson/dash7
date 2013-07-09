#include "dash7/bg_frame.h"

#define FG_FRAME_SIZE 5


int advp_scan_bg(bg_param_t *advp_conf)
{
    uint8_t frame[FG_FRAME_SIZE];
    int error;

    error = data_listen();
    if(error < 0)
        return error;

    error = data_get_frame(frame,FG_FRAME_SIZE);
    if(error < 0)
        return error -5;

    bg_get_advp(frame,advp_conf);
    if(advp_conf->protocol != 0xF0) return BG_BAD_PROTOCOL;
    else return 0;
}

int advp_scan(void)
{
    int error;
    systime_t time1;
    systime_t time2;
    systime_t sleep;
    bg_param_t conf;

    //prepare a background scan procedure
    data_proc_conf.pack_class = BACKGROUND_CLASS;
    data_proc_config(&data_proc_conf);
    //scan the packet
    error = advp_scan_bg(&conf);
    if(error)
        return error;

    time1 = chTimeNow();

    //prepare a foreground scan procedure
    data_proc_conf.pack_class = FOREGROUND_CLASS;
    data_proc_conf.channel = conf.channel;
    data_proc_config(&data_proc_conf);

    time2 = chTimeNow();

    sleep = d7_ti_to_systime(conf.time) - (time2 - time1) - SAFE_SLEEP_WINDOW;
    //sleep = d7_ti_to_systime(conf.time) - (time2 - time1);
    //return -sleep;
    //if(sleep > (data_conf.Tfsd / 2))
    //sleep until the time to receive the packet is reached
    chThdSleepMilliseconds(sleep);

    //scan the foreground packet
    error = data_listen();

    if(error < 0) return error - 10;
    else return error;
}

#define SAFE_ADVP_WINDOW 100
#define MIN_ADVP_TIME 500
#define PROT_SEND_TIME 310
int advp_send(uint8_t *frame, d7_ti time, uint8_t bg_channel, uint8_t fg_channel)
{
    systime_t now;
    systime_t end;
    uint8_t bg_frame[FG_FRAME_SIZE];
    bg_param_t conf;
    int error;
    int step = 0;

    //min advp time is needed because the first send takes some time
    if(time < MIN_ADVP_TIME) time = MIN_ADVP_TIME;

    //configure the background send procedure
    bg_set_subnet(bg_frame, data_proc_conf.my_subnet);
    conf.channel = fg_channel;

    data_proc_conf.pack_class = BACKGROUND_CLASS;
    data_proc_conf.channel = bg_channel;
    data_proc_config(&data_proc_conf);

    now = chTimeNow();
    end = now + d7_ti_to_systime(time);

    //keep sending packets with the remaining time
    while(now < (end - (PROT_SEND_TIME)))
    {
        conf.time = systime_to_d7_ti(end - now - (PROT_SEND_TIME - SAFE_ADVP_WINDOW));
        bg_set_advp(bg_frame,&conf);
        data_add_frame(bg_frame);
        if(!step){
            error = data_send_packet();
            step = 1;
        } else
            error = data_send_packet_protected();
        if(error){
        }

        now = chTimeNow();
    }

    //prepare the foreground send procedure
    data_proc_conf.pack_class = FOREGROUND_CLASS;
    data_proc_conf.channel = fg_channel;
    data_proc_config(&data_proc_conf);
    data_add_frame(frame);
    now = chTimeNow();
    if(now > end) 
        return -(now - end);
    chThdSleepMilliseconds(end-now);

    return data_send_packet_protected();
}

int rsvp(systime_t end)
{
    systime_t now;
    bg_param_t conf;
    int error;
    uint8_t old_class;
    uint8_t bg_frame[FG_FRAME_SIZE];

    conf.channel = data_proc_conf.channel;
    old_class = data_proc_conf.pack_class;
    data_proc_conf.pack_class = BACKGROUND_CLASS;
    data_proc_config(&data_proc_conf);

    now = chTimeNow();
    while(now < end)
    {
        conf.time = systime_to_d7_ti(end - now);
        bg_set_rsvp(bg_frame,&conf);
        data_add_frame(bg_frame);
        error = data_send_packet_protected();
        if(error){
        }

        now = chTimeNow();
    }

    data_proc_conf.pack_class = old_class;
    data_proc_config(&data_proc_conf);

    return 0;
}
