#ifndef __INCLUDED_TIMER_EVENT_H__
#define __INCLUDED_TIMER_EVENT_H__

#include "basic.h"

void timer_event_init(void);
QUEUE *timer_event_insert(UD time, void (*func)(void *), void *arg);
void timer_event_tick(void);
void timer_event_remove(QUEUE *que);

#endif
