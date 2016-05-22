#include "basic.h"
#include "debug.h"
#include "vcpu.h"

void queue_remove(QUEUE *que){
    que->prev->next = que->next;
    que->next->prev = que->prev;
    que->prev = NULL;
    que->next = NULL;
}

void queue_insert(QUEUE *que, QUEUE *prev){
    if(que==NULL||prev==NULL){
        tv_abort("Null !!\n");
        return;
    }
    que->next = prev->next;
    que->next->prev = que;
    que->prev = prev;
    prev->next = que;
}

void queue_insert_prev(QUEUE *que, QUEUE *next){
    if(que==NULL||next==NULL){
        tv_abort("Null !\n");
        return;
    }
    que->prev = next->prev;
    que->prev->next = que;
    que->next = next;
    next->prev = que;
}

void queue_list_init(QUEUE_LIST *ls){
    ls->head.prev = NULL;
    ls->head.next = &(ls->tail);
    ls->tail.prev = &(ls->head);
    ls->tail.next = NULL;
}

void queue_list_enque(QUEUE_LIST *ls, QUEUE *que){

    queue_insert(que, ls->tail.prev);

}

bool queue_list_is_empty(QUEUE_LIST *ls){
    return ls->head.next->next==NULL;
}

QUEUE* queue_list_top(QUEUE_LIST *ls){
    if(queue_list_is_empty(ls)) {
        tv_abort("# pop failed\n");
        return NULL;
    }
    QUEUE *que = ls->head.next;
    return que;
}

QUEUE* queue_list_pop(QUEUE_LIST *ls){
    if(queue_list_is_empty(ls)) {
        tv_abort("# pop failed\n");
        return NULL;
    }
    QUEUE *que = ls->head.next;

    queue_remove(que);
    return que;
}

bool queue_list_check(QUEUE_LIST *ls){
    QUEUE* tmp = &(ls->head);
    QUEUE* tail = &(ls->tail);
    if(tmp->prev!=NULL) return false;
    if(tail->next!=NULL) return false;
    while(tmp->next!=NULL){
        if(tmp->next->prev!=tmp) return false;
        tmp = tmp->next;
        if(tmp->prev->next!=tmp) return false;
    }
    return tail==tmp;
}

void queue_list_print(QUEUE_LIST *ls){
    QUEUE *tmp=ls->head.next;
    while(tmp!=&(ls->tail)){
        if(tmp==tmp->next){
            tv_abort("error break\n");
            break;
        }
        tmp = tmp->next;
    }
}

bool queue_list_find(QUEUE_LIST *ls, QUEUE *que){
    QUEUE *tmp=ls->head.next;
    while(tmp!=&(ls->tail)){
        if(tmp==que) return true;
        tmp = tmp->next;
    }
    return false;
}

void queue_test(void){
    QUEUE que[4];
    QUEUE_LIST ls;
    queue_list_init(&ls);
    if(!queue_list_check(&ls)) tv_error_message("init error\n");

    queue_list_enque(&ls, &que[0]);
    if(!queue_list_check(&ls)) tv_error_message("insert error\n");
    if(!queue_list_find(&ls,&que[0])) tv_error_message("find error\n");
    queue_list_enque(&ls, &que[1]);
    if(!queue_list_check(&ls)) tv_error_message("insert error 2\n");
    if(!queue_list_find(&ls,&que[1])) tv_error_message("find error1\n");
    if(queue_list_pop(&ls)!=&que[0]) tv_error_message("pop elem error\n");
    if(!queue_list_check(&ls)) tv_error_message("pop error\n");
    if(queue_list_find(&ls,&que[0])) tv_error_message("find error0\n");
    if(queue_list_pop(&ls)!=&que[1]) tv_error_message("pop elem error2\n");
    if(!queue_list_check(&ls)) tv_error_message("pop error 2\n");
    if(!queue_list_is_empty(&ls)) tv_error_message("empty check error 2\n");

    queue_list_enque(&ls, &que[1]);
    if(!queue_list_check(&ls)) tv_error_message("insert error 3\n");
    queue_remove(&que[1]);
    if(!queue_list_check(&ls)) tv_error_message("remove error\n");
    if(!queue_list_is_empty(&ls)) tv_error_message("empty check error 3\n");

    queue_list_enque(&ls, &que[1]);
    if(!queue_list_check(&ls)) tv_error_message("insert error 4\n");
    queue_remove(&que[1]);
    if(!queue_list_check(&ls)) tv_error_message("remove error 2\n");
    if(!queue_list_is_empty(&ls)) tv_error_message("empty check error 4\n");

    queue_list_enque(&ls, &que[3]);
    if(!queue_list_check(&ls)) tv_error_message("insert error 5\n");
    queue_remove(&que[3]);
    if(!queue_list_check(&ls)) tv_error_message("remove error 3\n");
    if(!queue_list_is_empty(&ls)) tv_error_message("empty check error\n");
    tv_error_message("test done\n");
    tv_getc();
}
