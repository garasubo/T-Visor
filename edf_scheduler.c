#include "basic.h"
#include "edf_scheduler.h"
#include "timer.h"
#include "debug.h"
#include "logger.h"
#include "timer_event.h"

typedef struct t_queue {
    QUEUE que;
    T_VCPU *vcpu;
    UW remained_time;
    UD dead_line;
    UD start_time;
} queue_t;

static queue_t _que[16];
static QUEUE_LIST _ls;
static QUEUE_LIST _ws;
static QUEUE_LIST _fl;

static QUEUE *_end_event;
static QUEUE *_insert_event;

static void list_enque(QUEUE_LIST *ls, queue_t *que){
    QUEUE *tmp = &(ls->head);
    while(tmp->next!=&(ls->tail)){
        queue_t *next = (queue_t *)(tmp->next);
        if(next->dead_line > que->dead_line) break;
        tmp = tmp->next;
    }
    queue_insert_prev((QUEUE *)que, tmp->next);

    return;
}

static void my_init(void) {
    int i;
    queue_list_init(&_fl);
    for(i=0;i<sizeof(_que)/sizeof(_que[0]);i++){
        queue_list_enque(&_fl, &(_que[i].que));
    }
    queue_list_init(&_ls);
    queue_list_init(&_ws);
    // timer_event_insert(ptimer_get_value() + TIMER_CLOCK_FREQ*10, (void (*)(void *))my_tick, NULL);
}

static void *my_allocate(T_VCPU *vcpu){
    if(queue_list_is_empty(&_fl)){
        tv_error_message("# canot allocate !");
        return NULL;
    }
    queue_t *que = (queue_t *)queue_list_pop(&_fl);
    que->vcpu = vcpu;
    edf_sched_param_t *param = (edf_sched_param_t *)vcpu->sched_param;
    if(param->period!=0){
        que->dead_line = param->first_time + param->period;
    } else {
        que->dead_line = 0xffffffffffffffffLL;
    }
    que->remained_time = param->exection_time;

    return que;
}

static T_VCPU *my_schedule(void){
    T_VCPU *exe = vcpu_get_executing();
    UD now = ptimer_get_value();
    queue_t *que;
    while(!queue_list_is_empty(&_ws) && ((queue_t *)queue_list_top(&_ws))->dead_line >= now){
        que = (queue_t *)queue_list_pop(&_ws);
        edf_sched_param_t *param = (edf_sched_param_t *)(que->vcpu->sched_param);
        que->dead_line += param->period;
        que->remained_time = param->exection_time;
        list_enque(&_ls, que);
    }

    if(queue_list_is_empty(&_ws)){
        timer_event_remove(_insert_event);
        _insert_event = NULL;
    } else {
        timer_event_remove(_insert_event);
        UD time = ((queue_t *)queue_list_top(&_ws))->dead_line;
        _insert_event = timer_event_insert(time, (void (*)(void *))scheduler_set_flag, NULL);
    }

    if(queue_list_is_empty(&_ls)) return exe;
    que = (queue_t *)queue_list_top(&_ls);


    if(exe!=NULL){
        queue_t *sched_state  = (queue_t *)exe->sched_state;
        UW remained_time = sched_state->remained_time;
        UW start_time = sched_state->start_time;
        UD exec_dl = sched_state->dead_line;

        if(remained_time > now-start_time && exec_dl > que->dead_line){
            return exe;
        }
    }


    que->start_time = now;
    queue_list_pop(&_ls);
    timer_event_remove(_end_event);
    _end_event = timer_event_insert(now+((edf_sched_param_t *)que->vcpu->sched_param)->exection_time, (void (*)(void *))scheduler_set_flag, NULL);
    return que->vcpu;
}

static void my_enque(T_VCPU *vcpu){
    queue_t *que = (queue_t *)vcpu->sched_state;


    list_enque(&_ls, que);
    return;
}

static void my_block(T_VCPU *vcpu){
    if(vcpu->state==VCPU_STATE_START){
        UD now = ptimer_get_value();
        queue_t *info = (queue_t *)vcpu->sched_state;
        info->remained_time -= now - info->start_time;
        if(info->remained_time <= 0){
            list_enque(&_ws, info);
        } else {
            list_enque(&_ls, info);
        }
        timer_event_remove(_end_event);
        scheduler_set_flag();
    } else {
        tv_abort("block failed\n");
    }
}

static void my_unblock(T_VCPU *vcpu){
    if(vcpu->state==VCPU_STATE_SLEEP){
        queue_t *info = (queue_t *)vcpu->sched_state;
        if(info->remained_time <= 0){
            list_enque(&_ws, info);
        } else {
            list_enque(&_ls, info);
        }
        scheduler_set_flag();
    }
}

#if 0
static void my_free(T_VCPU *vcpu){
    if(vcpu->state==VCPU_STATE_WAIT) queue_remove((QUEUE *)vcpu->sched_info);
    else if(vcpu->state==VCPU_STATE_START) scheduler_set_flag();
    queue_list_enque(&_fl, (QUEUE *)vcpu->sched_info);
}
#endif

static void my_yield(void){
    timer_event_remove(_end_event);
    scheduler_set_flag();
}


schedule_operation_t edf_scheduler = {
    my_init,
    my_schedule,
    my_yield,
    my_block,
    my_unblock,
    my_allocate,
    my_enque,
};
