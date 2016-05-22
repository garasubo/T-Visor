#ifndef __INCLUDED_BASIC_H__
#define __INCLUDED_BASIC_H__

#define Asm __asm__ volatile

#include "type.h"

#define NULL ((void *)0x0)

typedef struct t_que{
    struct t_que *prev;
    struct t_que *next;
} QUEUE;

typedef struct {
    QUEUE head;
    QUEUE tail;
} QUEUE_LIST;

void queue_test(void);
void queue_remove(QUEUE *que);
void queue_insert(QUEUE *que, QUEUE *prev);
void queue_insert_prev(QUEUE *que, QUEUE *next);
void queue_list_init(QUEUE_LIST *ls);
bool queue_list_check(QUEUE_LIST *ls);
void queue_list_enque(QUEUE_LIST *ls, QUEUE *que);
bool queue_list_is_empty(QUEUE_LIST *ls);
bool queue_list_find(QUEUE_LIST *ls, QUEUE *que);
QUEUE* queue_list_top(QUEUE_LIST *ls);
QUEUE* queue_list_pop(QUEUE_LIST *ls);
void queue_list_print(QUEUE_LIST *ls);

void irq_handler_init(void);
void irq_handler_register(UW irq_no, void (*func)(UW, void *), void *arg);

#endif
