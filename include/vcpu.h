#ifndef __INCLUDED_VCPU_H__
#define __INCLUDED_VCPU_H__

struct t_vcpu;
typedef struct t_vcpu T_VCPU;
#define ID_SVC 0x0
#define ID_ABT 0x1
#define ID_UND 0x2
#define ID_IRQ 0x3
#define ID_FIQ 0x4
#define ID_USR 0x5

typedef enum {
    VCPU_STATE_INIT,
    VCPU_STATE_START,
    VCPU_STATE_WAIT,
    VCPU_STATE_SLEEP
} vcpu_state_t;

#include "basic.h"
#include "type.h"

typedef struct{
    UW pc;
    void *sched_param;
    UD vtoff;
} T_CVCPU;


#include "virtual_gic.h"
#include "page_table.h"
#include "schedule.h"
#include "settings.h"

#ifndef NUM_OF_T_VCPU
#define NUM_OF_T_VCPU 16
#endif


typedef struct {
    UW fpexc;
    UW fpscr;
    UD vfpr[32];
} vcpu_fpu_t;

typedef struct {
    vcpu_fpu_t fpu_state;
    UW reg[14];
} vcpu_stack_t;

struct t_vcpu{
    UW id;
    UB vid;
    vcpu_state_t state;
    void *page_table;
    T_VM *vm;
    UW pc;
    UW cpsr;
    UW reg[14];
    UW spsr[5];
    UW sp[6];
    UW lr[6];
    UW fiq_banked[5];

    void *sched_param;
    void *sched_state;

    UW actlr;
    UW sctlr;
    UW cpacr;

    UW csselr;

    UW dacr;
    UW dfsr;
    UW ifsr;
    UW adfsr;
    UW aifsr;
    UW dfar;
    UW ifar;
    UW prrr;
    UW nmrr;
    UW cidr;
    UW tpidrprw;
    UW tpidruro;
    UW tpidrurw;
    UW ttbcr;
    UD ttbr0;
    UD ttbr1;

    UW par_lo;
    UW par_hi;

    UW vbar;

    UW mpid;

    UW mair0;
    UW mair1;

    QUEUE *timer_event;

    // timer states
    UW vtctl;
    UD vtcmp;
    UD vtoff;

    vcpu_fpu_t fpu_state;
};

void vcpu_set_current_sp(vcpu_stack_t *sp);
vcpu_stack_t *vcpu_get_current_sp(void);

void vcpu_save_state(T_VCPU *vcpu);

UW vcpu_make_empty_cpu(void *shced_param);

void vcpu_reset(T_VCPU *vcpu);

void vcpu_off(T_VCPU *vcpu);

void vcpu_ready(T_VCPU *vcpu);

void vcpu_start(T_VCPU *vcpu);

void vcpu_stop(void);

void vcpu_sleep(T_VCPU *vcpu);

void vcpu_make_wait(T_VCPU *vcpu);

void vcpu_wakeup(T_VCPU *vcpu);

void vcpu_preemption(T_VCPU *vcpu);


T_VCPU *vcpu_find_by_id(UW id);

T_VCPU *vcpu_setup(T_CVCPU *cvcpu, UW num, T_VM *vm);

T_VCPU *vcpu_get_executing( void );
T_VCPU *vcpu_get_last( void );

void vcpu_debug_print(T_VCPU *vcpu);

#endif
