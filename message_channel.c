#include "message_channel.h"
#include "debug.h"
#include "vcpu.h"
#include "page_table.h"
#include "memory_define.h"
#include "memory_manage.h"

extern volatile void *_channel_area_start;
extern volatile void *_channel_area_end;

static UW area_start;

static message_channel_t channels[16];
static UW channel_num = 0;

void message_channel_init(void){
    area_start = (UW)&_channel_area_start;
}

void message_channel_hyp_interface(void){
    UW *sp = vcpu_get_current_sp()->reg;
    message_channel_t *channel;
    switch(sp[0]){
        case 0:
            channel = message_channel_alloc(sp[1], sp[2], sp[3]);
            if(channel==NULL) sp[0] = -1;
            else sp[0] = channel->id;
            break;
        case 1:
            sp[0] = message_channel_find(sp[1],sp[2]);
            break;
        case 2:
            channel = &(channels[sp[1]]);
            if(channel->state==MESSAGE_CHANNEL_EMPTY){
                message_channel_write_start(channel,sp[2]);
            } else {
                channel->sender_wait = true;
                channel->wait_arg = sp[2];
                vcpu_make_wait(vcpu_find_by_id(channel->sender_id));
            }
            break;
        case 3:
            channel = &(channels[sp[1]]);
            message_channel_write_end(channel);
            if(channel->receiver_wait){
                channel->receiver_wait = false;
                message_channel_read_start(channel, channel->wait_arg);
                vcpu_wakeup(vcpu_find_by_id(channel->receiver_id));
            }
            break;
        case 4:
            channel = &(channels[sp[1]]);
            if(channel->state==MESSAGE_CHANNEL_READY){
                message_channel_read_start(channel,sp[2]);
            } else {
                channel->receiver_wait = true;
                channel->wait_arg = sp[2];
                vcpu_make_wait(vcpu_find_by_id(channel->receiver_id));
            }
            break;
        case 5:
            channel = &(channels[sp[1]]);
            message_channel_read_end(channel);
            if(channel->sender_wait){
                channel->sender_wait = false;
                message_channel_write_start(channel, channel->wait_arg);
                vcpu_wakeup(vcpu_find_by_id(channel->sender_id));
            }
            break;
        default:
            tv_abort("!!!  unknown operation !!!\n");
    }
}

W message_channel_find(UB sender_id, UB receiver_id){
    UW i;
    for(i=0;i<channel_num;i++){
        if(channels[i].sender_id==sender_id&&channels[i].receiver_id==receiver_id)
            return i;
    }
    return -1;
}

message_channel_t *message_channel_find_by_id(UB id){
    return &(channels[(UW)id]);
}

message_channel_t *message_channel_alloc(UB sender_id, UB receiver_id, UW size)
{
    if(area_start+size >= (UW)&_channel_area_end || channel_num >= sizeof(channels)/sizeof(channels[0])){
        tv_abort("! allocation failed\n");
        return NULL;
    }

    message_channel_t *ret = &(channels[channel_num]);
    ret->id = channel_num++;
    ret->receiver_id = receiver_id;
    ret->sender_id = sender_id;
    ret->addr = area_start;
    ret->state = MESSAGE_CHANNEL_EMPTY;
    area_start = (area_start+size+0xfff)&(0xfffff000);
    ret->addr_end = area_start;
    ret->receiver_wait = false;
    ret->sender_wait = false;
    return ret;
}

void message_channel_write_start(message_channel_t *channel, UW virt_addr){
    T_VCPU *sender = vcpu_find_by_id(channel->sender_id);
    memory_area_t area = { channel->addr, channel->addr_end, virt_addr, MEM_AREA_VNORMAL | MEM_AREA_VRW | (1 << 10), 0};
    vcpu_add_page(sender, &area);
    channel->state = MESSAGE_CHANNEL_BUSY;
    channel->virt_addr = virt_addr;
}

void message_channel_write_end(message_channel_t *channel){
    T_VCPU *sender = vcpu_find_by_id(channel->sender_id);
    memory_area_t area = { channel->addr, channel->addr_end, channel->virt_addr, MEM_AREA_VNO , 0};
    vcpu_add_page(sender, &area);
    flush_cache();
    FlushTLB();
    channel->state = MESSAGE_CHANNEL_READY;
}

void message_channel_read_start(message_channel_t *channel, UW virt_addr){
    T_VCPU *sender = vcpu_find_by_id(channel->receiver_id);
    memory_area_t area = { channel->addr, channel->addr_end, virt_addr, MEM_AREA_VNORMAL | MEM_AREA_VRO | (1 << 10), 0};
    vcpu_add_page(sender, &area);
    channel->state = MESSAGE_CHANNEL_BUSY;
    channel->virt_addr = virt_addr;
}

void message_channel_read_end(message_channel_t *channel){
    T_VCPU *sender = vcpu_find_by_id(channel->receiver_id);
    memory_area_t area = { channel->addr, channel->addr_end, channel->virt_addr, MEM_AREA_VNO , 0};
    vcpu_add_page(sender, &area);
    flush_cache();
    FlushTLB();
    channel->state = MESSAGE_CHANNEL_EMPTY;
}
