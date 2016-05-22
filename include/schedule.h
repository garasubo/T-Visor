#ifndef __INCLUDED_SCHEDULE_H__
#define __INCLUDED_SCHEDULE_H__

struct t_schedule_operation;
struct t_schedule_info;


typedef struct t_schedule_operation {
    void (*init)(void);
    T_VCPU *(*schedule)(void);
    void (*yield)(void);
    void (*block)(T_VCPU *);
    void (*unblock)(T_VCPU *);
    void *(*allocate)(T_VCPU *);
    void (*enque)(T_VCPU *);
} schedule_operation_t;

typedef struct t_schedule_info{
    void *info;
    schedule_operation_t *operations;
} schedule_info_t;

void scheduler_init(void *info, schedule_operation_t *operations);
void *scheduler_get_info(void);
schedule_operation_t *scheduler_get_operations(void);
void scheduler_set_flag(void);
void scheduler_clear_flag(void);
bool scheduler_get_flag(void);

#endif
