#include "vcpu.h"
#include "schedule.h"
#include "timer.h"
#include "logger.h"
#include "debug.h"
#include "port.h"
#include "gic.h"
#include "my_scheduler.h"
#include "timer_event.h"
#include "vcpu.h"

typedef struct t_queue {
    QUEUE que;
    T_VCPU *vcpu;
} queue_t;

static queue_t _que[16];
static QUEUE_LIST _wque;
static QUEUE_LIST _fque;

static char error_mes[] = "!# stop\n";

static void scheduler_tick(void * arg){
    timer_event_insert(ptimer_get_value()+TIMER_CLOCK_FREQ/10, scheduler_tick, NULL);
    scheduler_set_flag();
}

void scheduler_init(void) {
    int i;
    queue_list_init(&_wque);
    queue_list_init(&_fque);
    for(i=0;i<16;i++){
        queue_list_enque(&_fque, &(_que[i].que));
    }
    timer_event_insert(ptimer_get_value()+TIMER_CLOCK_FREQ/10, scheduler_tick, NULL);
    timer_event_insert(ptimer_get_value()+TIMER_CLOCK_FREQ*100, (void (*)(void *))tv_abort, (void *)error_mes);
}

void *scheduler_allocate(T_VCPU *vcpu){
    if(queue_list_is_empty(&_fque)){
        tv_abort("# canot allocate !");
        return NULL;
    }
    queue_t *ret = (queue_t *)queue_list_pop(&_fque);
    ret->vcpu = vcpu;
    return ret;
}

void scheduler_enque(T_VCPU *vcpu){
    queue_list_enque(&_wque, (QUEUE *)vcpu->sched_state);
    if(vcpu_get_executing()==NULL) scheduler_set_flag();
}


T_VCPU *scheduler_schedule(void) {
    if(queue_list_is_empty(&_wque)){
        return vcpu_get_executing();
    }
    T_VCPU *ret = ((queue_t *)queue_list_pop(&_wque))->vcpu;
    return ret;
}

void scheduler_yield(void){
    scheduler_set_flag();
}

void scheduler_block(T_VCPU *vcpu){
    scheduler_enque(vcpu);
}

void scheduler_unblock(T_VCPU *vcpu){
    if(vcpu->state==VCPU_STATE_SLEEP){
        scheduler_enque(vcpu);
        if(vcpu_get_executing()==NULL) scheduler_set_flag();
    }
}

