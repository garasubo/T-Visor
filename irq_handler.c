#include "basic.h"
#include "type.h"
#include "debug.h"
#include "timer.h"
#include "port.h"
#include "gic.h"
#include "vcpu.h"
#include "virtual_gic.h"
#include "cortexa7.h"
#include "memory_manage.h"
#include "logger.h"
#include "schedule.h"
#include "timer_event.h"
#include "hcr.h"
#include "hyp_call.h"
#include "cp_access.h"
#include "cpsr.h"
#include "rbtree.h"


extern UW test_vcpu_id;
extern char* sleep_vector;

int tmp_irq_no = -1;


static tree_t *tree; // irq_num -> handler function

typedef struct {
    void (*func)(UW ,void *);
    void *arg;
} handler_t;

handler_t hdlr[512];
UW hdlr_num = 0;

static int handler_comp(void *a, void *b){
    W na = (W)a;
    W nb = (W)b;
    return nb-na;
}

static void ht_handle(UW eoir, void *arg){
    htimer_clear();
    timer_event_tick();
    OUTW(GICC_EOIR,eoir);
    OUTW(GICC_DIR,eoir);
    return;
}

static void vmi_handle(UW eoir, void *arg){
    T_VCPU *vcpu = vcpu_get_executing();
    virtual_gic_maintenance(vcpu->vm->vgic, vcpu->vid);
    OUTW(GICC_EOIR,eoir);
    OUTW(GICC_DIR,eoir);
}

static void nstimer_handle(UW eoir, void *arg){
    if(ptimer_get_value() >= ptimer_get_cmp_value() ){
        virtual_gic_send_hardware_int(vcpu_get_last()->vm->vgic, eoir);
        OUTW(GICC_EOIR,eoir);
    } else {
        OUTW(GICC_EOIR,eoir);
        OUTW(GICC_DIR,eoir);
    }
}

static void vtimer_handle(UW eoir, void *arg){
    if(vtimer_get_value() >= vtimer_get_cmp_value() ){
        virtual_gic_send_hardware_int(vcpu_get_last()->vm->vgic, eoir);
        OUTW(GICC_EOIR,eoir);
    } else {
        OUTW(GICC_EOIR,eoir);
        OUTW(GICC_DIR,eoir);
    }
}

void irq_handler_init(void){
    tree = rbtree_alloc(handler_comp);
    irq_handler_register(IRQ_ID_HT, ht_handle, NULL);
    irq_handler_register(IRQ_ID_VMI, vmi_handle, NULL);
    irq_handler_register(IRQ_ID_NSPT, nstimer_handle, NULL);
    irq_handler_register(IRQ_ID_VT, vtimer_handle, NULL);
}

void irq_handler_register(UW irq_no, void (*func)(UW, void *), void *arg){
    handler_t *h = &(hdlr[hdlr_num++]);
    h->func = func;
    h->arg = arg;

    rbtree_insert(tree, (void *)irq_no, (void *)h);
}



void irq_handler(UW eoir){
    UW irq_no = eoir & 0x3ff;
    tmp_irq_no = irq_no;


    node_t *node;
    if((node = rbtree_search(tree, (void *)irq_no))!=NULL){
        handler_t *handler = (handler_t *)node->val;
        handler->func(eoir, handler->arg);
    } else {
        tv_abort("unknown irq!\n");
    }

    T_VCPU *prev = vcpu_get_executing();
    if(scheduler_get_flag()){
        vcpu_preemption(scheduler_get_operations()->schedule());
    }
    if(prev!=NULL){
        while(vcpu_get_executing()==NULL){
            Asm("mcr p15, 4, %0, c12, c0, 0\n"
                "ldr lr, =1f\n"
                "dsb\n"
                "cpsie aif\n"
                "wfi\n"
                "1: nop"
                    ::"r"(&sleep_vector)
                    :"lr");

        }
    }

}


