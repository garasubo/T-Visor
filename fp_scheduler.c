#include "basic.h"
#include "fp_scheduler.h"
#include "timer.h"
#include "debug.h"
#include "logger.h"
#include "timer_event.h"

typedef struct t_queue {
    QUEUE que;
    T_VCPU *vcpu;
} queue_t;

static queue_t _que[16];
static QUEUE_LIST _ls[MAX_PRIORITY];
static QUEUE_LIST _fl; 


static void my_init(void) {
    int i;
    queue_list_init(&_fl);
    for(i=0;i<sizeof(_que)/sizeof(_que[0]);i++){
        queue_list_enque(&_fl, &(_que[i].que));
    }
    for(i=0;i<MAX_PRIORITY;i++){
        queue_list_init(&_ls[i]);
    }
}

static void *my_allocate(T_VCPU *vcpu){
    if(queue_list_is_empty(&_fl)){
        tv_error_message("# fp sched: canot allocate !");
        return NULL;
    }
    queue_t *que = (queue_t *)queue_list_pop(&_fl);
    que->vcpu = vcpu;
    return que;
}

static T_VCPU *my_schedule(void){
    int i;
    int max_pri = MAX_PRIORITY;
    T_VCPU *exe = vcpu_get_executing();
    if(exe!=NULL){
        max_pri = ((fp_sched_param_t *)exe->sched_param)->priority;
        if(max_pri >= MAX_PRIORITY) max_pri = MAX_PRIORITY-1;
    }
    for(i=0;i<=max_pri;i++) if(!queue_list_is_empty(&_ls[i])){
        queue_t *que = (queue_t *)queue_list_pop(&_ls[i]);
        return que->vcpu;
    }
    if(exe==NULL){
        //tv_error_message("no vcpu!\n");
    }
    return exe;
}

static void my_enque(T_VCPU *vcpu){
    queue_t *que = (queue_t *)vcpu->sched_state;
    int pri = ((fp_sched_param_t *)vcpu->sched_param)->priority;

    if(pri>=MAX_PRIORITY) pri = MAX_PRIORITY-1;

    queue_list_enque(&_ls[pri], (QUEUE *)que);

    return;
}

static void my_block(T_VCPU *vcpu){
    my_enque(vcpu);
    scheduler_set_flag();
}

static void my_unblock(T_VCPU *vcpu){
    if(vcpu->state==VCPU_STATE_SLEEP){
        my_enque(vcpu);
        scheduler_set_flag();
    }
}


static void my_yield(void){
    scheduler_set_flag();
}




schedule_operation_t fp_scheduler = {
    my_init,
    my_schedule,
    my_yield,
    my_block,
    my_unblock,
    my_allocate,
    my_enque,
};
