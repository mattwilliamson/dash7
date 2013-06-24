#include "dash7/data.h"
#include "ch.h"
#include "random.h"
#include "sx1231.h"

data_param_t data_conf;
data_proc_param_t data_proc_conf;

void data_proc_config(data_proc_param_t *conf)
{
    rand_init(44); //TODO: REMOVE THIS FROM HERE!!
    data_proc_conf = *conf;
    phy_param_t phy_conf;
    phy_conf.channel = conf->channel;
    phy_conf.preamble = 0;
    phy_conf.fec = conf->fec;
    phy_conf.pack_class = conf->pack_class;
    phy_conf.rssi_thresh = data_conf.Ecca;
    phy_config(&phy_conf);
}

int data_guard_break(d7_ti time){
    systime_t start;
    systime_t end;
    systime_t max_diff;
    //data_proc_param_t aux_conf;

    if(time <= 0) return DATA_GUARD_FAIL;

    max_diff = d7_ti_to_systime(time);
    start = chTimeNow();
    end = start;
    //aux_conf = data_proc_conf;

    // While we still have time to wait for the guard time
    while (max_diff < (end + d7_ti_to_systime(data_conf.Tg) - start)) {
        //The rssi value is <= 0, but represented as positive, for that reason we use the >
        if (scan_rssi() > data_conf.Ecca){
            //phy_proc_finish();
            chThdSleepMilliseconds(d7_ti_to_systime(data_conf.Tg));
            //phy_proc_start();
            //data_proc_config(&aux_conf);
            //if (scan_rssi() > data_conf.Ecca) This will be done by the phy_send_packet
            return DATA_GUARD_SUCCESS;
        } else {
            //phy_proc_finish();
            chThdSleepMilliseconds(d7_ti_to_systime(data_conf.Tg));
            //phy_proc_start();
            //data_proc_config(&aux_conf);
        }
        end = chTimeNow();
    }
    return DATA_GUARD_FAIL;
}

//TODO: add more variety to the csma
/* Whaaaat? a csma that does nothing but wait?
since channels with csma are always guarded, the
other steps will folow in the guard break */
int data_csma(d7_ti time)
{
    if(time <= 0) 
        return DATA_CSMA_FAIL;
    //data_proc_param_t aux_conf;
    //aux_conf = data_proc_conf;

    //phy_proc_finish();
    chThdSleepMilliseconds(random()%d7_ti_to_systime(time));
    //phy_proc_start();
    //data_proc_config(&aux_conf);
    return DATA_CSMA_SUCCESS;

}

//TODO: right now, if time_diff is < Tg, we may have a problem.
int data_send_packet(void)
{
    systime_t start;
    systime_t end;
    systime_t time_diff;
    end = start = chTimeNow();
    int error;
    while (d7_ti_to_systime(data_conf.Tca) > (end - start)) {
        time_diff = d7_ti_to_systime(data_conf.Tca - data_conf.Tg) - (end - start);
        if(data_conf.Ecca){
            if(time_diff <= 0) time_diff = 0;
            error = data_csma(systime_to_d7_ti(time_diff));
            end = chTimeNow();
            if (error == DATA_CSMA_FAIL)
                continue; 
            time_diff = d7_ti_to_systime(data_conf.Tca - (end - start));
            error = data_guard_break(systime_to_d7_ti(time_diff));
            if (error == DATA_GUARD_SUCCESS)
                error = data_send_packet_protected();
                return error;
        } else {
            error = phy_send_packet(0);
            return error;
        }
    }
    return DATA_SEND_TIMEOUT;
}

int data_listen(void)
{
    int error;
    if ((data_proc_conf.pack_class == BACKGROUND_CLASS) && scan_rssi() > data_conf.Ecca)
        return DATA_LISTEN_EMPTY;
    error = phy_receive_packet(data_conf.Tbsd);
    if(error == PHY_REC_TIMEOUT) {
        return DATA_LISTEN_TIMEOUT;
    } else if(error > 0) {
        int index;
        if(data_proc_conf.pack_class == BACKGROUND_CLASS) 
            index = 0;
        else
            index = 2;
        //TODO: we verify the subnet just for the first frame, should we do it in the others?
        if (data_subnet_verify(data_proc_conf.my_subnet, phy_frame_buf[0][index]))
            return error; //success
    }
    //last case, return error
    return DATA_LISTEN_ERROR;

}

int data_dialogue(void)
{
    systime_t start;
    systime_t end;
    data_proc_param_t aux_conf;
    aux_conf = data_proc_conf;

    start = chTimeNow();
    phy_proc_finish();
    chThdSleepMilliseconds(d7_ti_to_systime(data_conf.Tc));
    phy_proc_start();
    data_proc_config(&aux_conf);
    end = chTimeNow();
    if ((end - start) > d7_ti_to_systime(data_conf.Tc + data_conf.Tl))
        return DATA_DIALOGUE_TIMEOUT;
    /* Start listening procedure for Tl */
    return data_listen();
}

/*receives the time when the package was received. When called this function
 will wait until the proper time to send the answer. If the time has already
 passed it will return an error */
int data_answer_time(systime_t received)
{
    systime_t now = chTimeNow();
    if (now > (received + d7_ti_to_systime(data_conf.Tc + data_conf.Tl)))
        return DATA_ANSWER_TIMEOUT;
    if (now > (received + d7_ti_to_systime(data_conf.Tc)))
        return DATA_ANSWER_READY;
    chThdSleepMilliseconds((received + d7_ti_to_systime(data_conf.Tc)) - now);
    return DATA_ANSWER_READY;
}
