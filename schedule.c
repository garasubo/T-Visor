#include "vcpu.h"
#include "schedule.h"

static schedule_info_t scheduler;
static bool flag = false;

void scheduler_init(void *info, schedule_operation_t *operations){
    scheduler.info = info;
    scheduler.operations = operations;
    scheduler.operations->init();
}

void scheduler_set_flag(void){
    flag = true;
}

void scheduler_clear_flag(void){
    flag = false;
}

bool scheduler_get_flag(void){
    return flag;
}

void *scheduler_get_info(void) {
    return scheduler.info;
}

schedule_operation_t *scheduler_get_operations(void){
    return scheduler.operations;
}
