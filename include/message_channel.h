#ifndef __INCLUDE_MESSAGE_CHANNEL_H__
#define __INCLUDE_MESSAGE_CHANNEL_H__

#include "type.h"

typedef enum {
    MESSAGE_CHANNEL_EMPTY,
    MESSAGE_CHANNEL_READY,
    MESSAGE_CHANNEL_BUSY
} message_channel_state_t;

typedef struct {
    UB id;
    UB sender_id;
    UB receiver_id;
    message_channel_state_t state;
    UW addr;
    UW addr_end;
    UW virt_addr;
    bool sender_wait;
    bool receiver_wait;
    UW wait_arg;
} message_channel_t;

void message_channel_init(void);

W message_channel_find(UB sender_id, UB receiver_id);

message_channel_t *message_channel_alloc(UB sender_id, UB receiver_id, UW size);

void message_channel_hyp_interface(void);

void message_channel_write_start(message_channel_t *channel, UW virt_addr);
void message_channel_write_end(message_channel_t *channel);

void message_channel_read_start(message_channel_t *channel, UW virt_addr);
void message_channel_read_end(message_channel_t *channel);


#endif
