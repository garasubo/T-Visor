#ifndef __INCLUDED_EDF_SCHEDULER_H__
#define __INCLUDED_EDF_SCHEDULER_H__

#include "vcpu.h"


typedef struct {
    UD first_time;
    UW period;
    UW exection_time;
} edf_sched_param_t;

extern schedule_operation_t edf_scheduler;

#endif
