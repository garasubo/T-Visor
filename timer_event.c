#include "type.h"
#include "basic.h"
#include "timer.h"
#include "debug.h"

typedef struct t_queue{
    QUEUE que;
    void (*func)(void *);
    void *arg;
    UD time;
} queue_t;

static queue_t _que[16];
static QUEUE_LIST _fl;
static QUEUE_LIST _el;

static queue_t *alloc(){
    queue_t *que = (queue_t *)queue_list_pop(&_fl);
    if(que==NULL) {
        tv_abort("# allocation failed\n");
    }
    return que;
}

void timer_event_init(void){
    int i;
    queue_list_init(&_fl);
    queue_list_init(&_el);
    for(i=0;i<sizeof(_que)/sizeof(_que[0]);i++){
        queue_list_enque(&_fl, &(_que[i].que));
    }
}

QUEUE *timer_event_insert(UD time, void (*func)(void *), void *arg){
    queue_t *que = alloc();
    que->func = func;
    que->time = time;
    que->arg = arg;

    QUEUE *tmp = &(_el.head);
    while(tmp->next!=&(_el.tail)){
        queue_t *next = (queue_t *)(tmp->next);
        if(next->time > que->time) break;
        tmp = tmp->next;
    }
    queue_insert_prev((QUEUE *)que, tmp->next);

    if(!queue_list_find(&_el, &(que->que))){
        tv_abort("! insert failed\n");
    }

    if(tmp==&(_el.head)) htimer_set_value(time);
    return &(que->que);
}

void timer_event_tick(void){
    if(queue_list_is_empty(&_el)) {
        if(!htimer_is_enable()) return;
        tv_abort("! que is empty!\n");
        return;
    }

    queue_t *que = (queue_t *)queue_list_pop(&_el);
    queue_list_enque(&_fl, &(que->que));
    que->func(que->arg);

    if(!queue_list_is_empty(&_el)){
        que = (queue_t *) _el.head.next;
        htimer_set_value(que->time);
    }
}

void timer_event_remove(QUEUE *que){
    if(que == NULL || !queue_list_find(&_el, que)){
        //tv_abort("not found error\n");
        return;
    }
    queue_remove(que);
    if(_el.head.next==que){
        if(queue_list_is_empty(&_el)) htimer_clear();
        else htimer_set_value(((queue_t *)_el.head.next)->time);
    }
    queue_list_enque(&_fl, que);
}
