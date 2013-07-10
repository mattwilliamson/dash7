#include "dash7/fg_frame.h"
#include "sx1231.h"

void fg_get_FH(uint8_t *frame, fg_FH_t *fh)
{
    fh->tx_eirp = frame[1];
    fh->subnet = frame[2];
    fh->frame_ctl = frame[3];
}

int fg_addr_ctl_pos(uint8_t *frame)
{
    fg_FH_t fh;
    int pos = 4; //lenght + FH

    fg_get_FH(frame,&fh);
    if (!(fh.frame_ctl && Fctl_EN_ADDR))
        return FIELD_NOT_PRESENT;
    if (fh.frame_ctl && Fctl_DLLS){
        /*TODO: add to pos the new position*/
    }
    return pos;
}

int fg_addr_ctl_get(uint8_t *frame, fg_addr_ctl_t *addr_ctl)
{
    int pos = fg_addr_ctl_pos(frame);
    if(pos < 0) 
        return pos;
    addr_ctl->dialog_id = frame[pos];
    addr_ctl->flags = frame[pos + 1];
    return 0;
}

int fg_addr_ctl_set(uint8_t *frame, fg_addr_ctl_t *addr_ctl)
{
    int pos = fg_addr_ctl_pos(frame);
    if(pos < 0)
        return FIELD_NOT_PRESENT;

    frame[pos] = addr_ctl->dialog_id;
    frame[pos+1] = addr_ctl->flags;
    return 0;
}

int fg_source_pos(uint8_t *frame)
{
    int pos = fg_addr_ctl_pos(frame);
    if (pos < 0)
        return pos;
    else return pos + FG_ADDR_CTL_SIZE;
}

int fg_target_pos(uint8_t *frame)
{
    fg_addr_ctl_t addr_ctl;
    int pos = fg_addr_ctl_get(frame,&addr_ctl);
    if (pos < 0)
        return pos;
    if (!(addr_ctl.flags && ADDR_NLS))
        return FIELD_NOT_PRESENT;
    if (addr_ctl.flags && ADDR_VID)
        return fg_source_pos(frame) + 2;
    else
        return fg_source_pos(frame) + 8; 
}

int fg_payload_pos(uint8_t *frame)
{
    int pos = 4;
    fg_addr_ctl_t addr_ctl;
    fg_FH_t fh;

    fg_get_FH(frame,&fh);
    if (fh.frame_ctl && Fctl_DLLS) {
        /*TODO: add to pos*/
    }
    if (fh.frame_ctl && Fctl_EN_ADDR) {
        pos += 3;
        fg_addr_ctl_get(frame,&addr_ctl);
        if (addr_ctl.flags && ADDR_VID) {
            if (addr_ctl.flags && ADDR_NLS)
                pos += 16;
            else
                pos += 8;
        } else {
            if (addr_ctl.flags && ADDR_NLS)
                pos += 4;
            else
                pos += 2;
        }           
    }

    return pos;
}

#define MAX_GAIN 13
void fg_FH_init(fg_FH_t *fh, uint8_t frame_ctl)
{
    if(data_proc_conf.tx_eirp == GAIN_1) fh->tx_eirp = MAX_GAIN;
    else if(data_proc_conf.tx_eirp == GAIN_2) fh->tx_eirp = MAX_GAIN - 6;
    else if(data_proc_conf.tx_eirp == GAIN_3) fh->tx_eirp = MAX_GAIN - 12;
    else if(data_proc_conf.tx_eirp == GAIN_4) fh->tx_eirp = MAX_GAIN - 24;
    else if(data_proc_conf.tx_eirp == GAIN_5) fh->tx_eirp = MAX_GAIN - 36;
    else if(data_proc_conf.tx_eirp == GAIN_6) fh->tx_eirp = MAX_GAIN - 48;
    fh->subnet = data_proc_conf.my_subnet;
    fh->frame_ctl = frame_ctl; 
}

void fg_frame_start(uint8_t *frame, fg_FH_t *fh)
{
    frame[1] = fh->tx_eirp;
    frame[2] = fh->subnet;
    frame[3] = fh->frame_ctl;
}

void fg_set_addr(uint8_t *frame, uint8_t *addr, uint8_t type)
{
    int pos;
    int size;
    
    if(type == SOURCE_ID)
        pos = fg_source_pos(frame);
    else
        pos = fg_target_pos(frame);
    if (pos > 0) {
        fg_addr_ctl_t addr_ctl;
        fg_addr_ctl_get(frame,&addr_ctl);
        if (addr_ctl.flags & ADDR_VID)
            size = 2;
        else
            size = 8;
        for(int i = 0; i < size; i++)
            frame[pos+i] = addr[i];
    }
}

void fg_set_payload(uint8_t *frame, uint8_t *payload, uint8_t size)
{
    int pos = fg_payload_pos(frame);
    
    for (int i = 0; i < size; i++)
        frame[pos + i] = payload[i];

    /* TODO: this size doesn't count with the DSSL footer, if we give support to it this must
     * be changed */
    //Set the frame length
    frame[0] = pos+size;
}

int fg_get_payload(uint8_t *frame, uint8_t *payload, uint8_t size)
{
    /* TODO: give support to DSSL footer */
    int pos = fg_payload_pos(frame);
    int payload_size = frame[0] - pos;

    if(size < payload_size) return -1;

    for (int i = 0; i < payload_size; i++)
        payload[i] = frame[pos + i];

    return payload_size;
}

void M2DP_frame_init(uint8_t *frame, uint8_t DLLS)
{
    fg_FH_t fh;
    uint8_t frame_ctl;

    if(DLLS)
        frame_ctl = Fctl_DLLS;
    else
        frame_ctl = 0; 
    fg_FH_init(&fh, frame_ctl);

    if(DLLS) {
        /* TODO Treat DLLS case */
    }

    fg_frame_start(frame,&fh);
}

void M2DP_frame_ID_set(uint8_t *frame, uint8_t id)
{
    int pos = fg_payload_pos(frame);
    frame[pos] = id;
}

//returns the ID
uint8_t M2DP_frame_ID_get(uint8_t *frame)
{
    int pos = fg_payload_pos(frame);
    return frame[pos];
}

int M2DP_payload_pos(uint8_t *frame)
{
    return (fg_payload_pos(frame) + 1);
}


