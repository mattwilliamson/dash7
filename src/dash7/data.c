#include "dash7/data.h"
#include "ch.h"
#include "random.h"
#include "sx1231.h"
#include "dio.h"

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
    phy_conf.tx_eirp = conf->tx_eirp;
    phy_config(&phy_conf);
}

int data_guard_break(d7_ti time){
    systime_t start;
    systime_t end;
    systime_t max_diff;
    uint8_t rssi;

    if(time <= 0) return DATA_GUARD_FAIL;

    max_diff = d7_ti_to_systime(time);
    
    start = chTimeNow();
    end = start;

    sx_mode(STDBY);
    empty_fifo();
    // While we still have time to wait for the guard time
    while (max_diff > (end + d7_ti_to_systime(data_conf.Tg) - start)) {
        //The rssi value is <= 0, but represented as positive, for that reason we use the >
        //if ((rssi = scan_rssi()) > data_conf.Ecca){
        if ((rssi = wait_cca_timeout(0))){
            chThdSleepMilliseconds(d7_ti_to_systime(data_conf.Tg));
            //if (scan_rssi() > data_conf.Ecca) This will be done by the phy_send_packet
            return DATA_GUARD_SUCCESS;
        } else {
            chThdSleepMilliseconds(d7_ti_to_systime(data_conf.Tg));
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

    chThdSleepMilliseconds(random()%d7_ti_to_systime(time));
    return DATA_CSMA_SUCCESS;

}

int data_send_packet(void)
{
    systime_t start;
    systime_t end;
    systime_t time_diff;
    end = start = chTimeNow();
    int error;
    while (d7_ti_to_systime(data_conf.Tca) > (end - start)) {
        time_diff = d7_ti_to_systime(data_conf.Tca - data_conf.Tg) - (end - start);
        if(time_diff > 5000000) time_diff = 0;
        //does the colision avoidance procedure
        if(data_conf.CSMAen){
            if(time_diff <= 0) time_diff = 0;
            error = data_csma(systime_to_d7_ti(time_diff));
            end = chTimeNow();
            if (error == DATA_CSMA_FAIL){
                end = chTimeNow();
                continue; 
            }
            time_diff = d7_ti_to_systime(data_conf.Tca - (end - start));
            error = data_guard_break(systime_to_d7_ti(time_diff));
            if (error == DATA_GUARD_SUCCESS){
                error = phy_send_packet(0,TRUE);
                return error;
            }
        } else {
            //sends the packet
            error = phy_send_packet(0,FALSE);
            return error;
        }
        end = chTimeNow();
    }
    return DATA_SEND_TIMEOUT;
}

int data_listen(void)
{
    int error = 1;

    //Background are only listened when the channel is already full
    if (data_proc_conf.pack_class == BACKGROUND_CLASS){
        sx_mode(RX);
        rssi_on_dio4();
        error = wait_cca_timeout(0);
        sx_mode(STDBY);
        if((error)){
            return DATA_LISTEN_EMPTY;
        }
    }

    if(data_proc_conf.pack_class == BACKGROUND_CLASS){
        error = phy_receive_packet(data_conf.Tbsd);
    } else
        error = phy_receive_packet(data_conf.Tfsd);
        
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

int data_dialogue(systime_t sended, d7_ti Tc)
{
    systime_t start;
    systime_t end;
    data_proc_param_t aux_conf;
    aux_conf = data_proc_conf;

    start = chTimeNow();
    if ((start - sended) > (d7_ti_to_systime(Tc) - SAFE_SLEEP_WINDOW)) 
        return DATA_DIALOGUE_TIMEOUT;

    phy_proc_finish();
    chThdSleepMilliseconds(sended + d7_ti_to_systime(Tc) - start - SAFE_SLEEP_WINDOW);
    phy_proc_start();

    data_proc_config(&aux_conf);
    end = chTimeNow();
    if ((end - sended) > d7_ti_to_systime(Tc + data_conf.Tl))
        return DATA_DIALOGUE_TIMEOUT;
    /* Start listening procedure for Tl */
    return data_listen();
}

/*receives the time when the package was received. When called this function
 will wait until the proper time to send the answer. If the time has already
 passed it will return an error */
int data_answer_time(systime_t received, d7_ti Tc)
{
    systime_t now = chTimeNow();
    if (now > (received + d7_ti_to_systime(Tc + data_conf.Tl)))
        return DATA_ANSWER_TIMEOUT;
    if (now > (received + d7_ti_to_systime(Tc)))
        return DATA_ANSWER_READY;
    chThdSleepMilliseconds((received + d7_ti_to_systime(Tc)) - now );
    return DATA_ANSWER_READY;
}
