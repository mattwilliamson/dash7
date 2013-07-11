#ifndef FOREGROUND_FRAME_SUPPORT
#define FOREGROUND_FRAME_SUPPORT
#include "dash7/data.h"
#include "stdint.h"

typedef struct fg_FH {
    uint8_t tx_eirp;
    uint8_t subnet;
    uint8_t frame_ctl;
} fg_FH_t;

/*FRAME CONTROL values*/
#define Fctl_LISTEN (1<<7)
#define Fctl_DLLS (1<<6)
#define Fctl_EN_ADDR (1<<5)
#define Fctl_CONT (1<<4)
#define Fctl_CRC32 (1<<3)
#define Fctl_NM2 (1<<2)

#define Fctl_M2FR 0x0
#define Fctl_M2NACK 0x1
#define Fctl_M2ST 0x2

#define FG_ADDR_CTL_SIZE 2
typedef struct fg_addr_ctl {
    uint8_t dialog_id;
    uint8_t flags;
} fg_addr_ctl_t;

/* addr ctl flags value */
#define ADDR_VID (1<<5)
#define ADDR_NLS (1<<4)

#define ADDR_UNICAST (0x0 << 6)
#define ADDR_BROADCAST (0x1 << 6)
#define ADDR_ANYCAST (0x2 << 6)
#define ADDR_MULTICAST (0x3 << 6)
#define ADDR_APP_FLAGS(flags) (flags & 0xF)

/*
 reads a fg_FH_t from a frame
 */
void fg_get_FH(uint8_t *frame, fg_FH_t *fh);

// Value returned when the field doesn't exist in that frame
#define FIELD_NOT_PRESENT -1
/*
return the position of the addr_ctl
 */
int fg_addr_ctl_pos(uint8_t *frame);

/*
 Fills the structure with the addr_ctl from the frame
 */
int fg_addr_ctl_get(uint8_t *frame, fg_addr_ctl_t *addr_ctl);

/*
 Use the addr_ctl to set the values in the frame
 */
int fg_addr_ctl_set(uint8_t *frame, fg_addr_ctl_t *addr_ctl);

/*
 return the source_id position
 */
int fg_source_pos(uint8_t *frame);

/*
 return the target_id position
 */
int fg_target_pos(uint8_t *frame);

/*
 return the position where the payload starts
 */
int fg_payload_pos(uint8_t *frame);

/*
 Initialize the fg_FH_t structure with values from the data_layer
 and with values from the flags
 */
void fg_FH_init(fg_FH_t *fh, uint8_t flags);

/*
 Initialize a frame with values from fh
 */
void fg_frame_start(uint8_t *frame, fg_FH_t *fh);

#define SOURCE_ID 0
#define TARGET_ID 1
/*
 set the id value in a frame (target or source ID come from the type parameter)
 */
void fg_set_addr(uint8_t *frame, uint8_t *addr, uint8_t type);

/*
 return the maximum number of bytes the payload can have
 */
int fg_payload_max_size(uint8_t *frame);

/*
 set the payload and the frame length
 return 0 if ok, else return the maximum lenght the payload can have
 */
int fg_set_payload(uint8_t *frame, uint8_t *payload, uint8_t size);

#define SMALL_SIZE -1
/*
 Get the payload, return the size of the payload or SMALL_SIZE if size is smaller than the
 payload data
 */
int fg_get_payload(uint8_t *frame, uint8_t *payload, uint8_t size);

/*
 Start a M2DP frame
 */
void M2DP_frame_init(uint8_t *frame, uint8_t DLLS);

/*
 Set the M2DP frame ID
 */
void M2DP_frame_ID_set(uint8_t *frame, uint8_t id);

/*
 Get the M2DP frame ID from a frame
 */
uint8_t M2DP_frame_ID_get(uint8_t *frame);

/*
 Return the payload position from a M2DP frame
 */
int M2DP_payload_pos(uint8_t *frame);

#endif
