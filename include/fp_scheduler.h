#ifndef __INCLUDED_FP_SCHEDULER_H__
#define __INCLUDED_FP_SCHEDULER_H__

#include "vcpu.h"

#define MAX_PRIORITY 8

typedef struct {
    UB priority;
} fp_sched_param_t;

extern schedule_operation_t fp_scheduler;

#endif
