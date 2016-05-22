#include "memory_define.h"
#include "vcpu.h"
#include "vm.h"
#include "debug.h"
#include "hyp_call.h"
#include "cortexa7.h"
#include "basic.h"
#include "default_schedule.h"
#include "my_scheduler.h"
#include "fp_scheduler.h"
#include "edf_scheduler.h"
#include "hcr.h"
#include "port.h"
#include "virtual_device_handle.h"
#include "setting_tkernel.h"
#include "vint_sender.h"

#include "timer.h"
#include "logger.h"

UW test_vcpu_id;

static void virtual_gic_handle(UW eoir, void *arg){
    vgic_t *vgic = (vgic_t *)arg;
    if(virtual_gic_send_hardware_int(vgic, eoir)){
        OUTW(GICC_EOIR,eoir);
    } else {
        tv_abort("# gic handle failed\n");
        OUTW(GICC_EOIR,eoir);
        OUTW(GICC_DIR,eoir);
    }
}


void user_init( void ){
    logger_init();
    scheduler_init(NULL, &edf_scheduler);
    static edf_sched_param_t p1 = { 0, TIMER_CLOCK_FREQ/10, TIMER_CLOCK_FREQ/30 };
    p1.first_time = ptimer_get_value();

    T_CVCPU vcpu_params[2][1] = {
        {{
            TK_START_ADDR,
            &p1,
            0
        }},
        {{
            TK_START_ADDR,
            &p1,
            0
        }},
    };

    T_CVM vm_params[] = {
        {
            TK_MEMORY_AREAS_1,
            TK_MEMORY_AREA_NUM_1,
            vcpu_params[0],
            1,
            HCR_DEFAULT
        },
        {
            TK_MEMORY_AREAS_2,
            TK_MEMORY_AREA_NUM_2,
            vcpu_params[1],
            1,
            HCR_DEFAULT
        }
    };

    T_VM *vm = vm_init(&(vm_params[0]));
    vm_ready(vm);
    virtual_gic_register_int(vm->vgic, 0x1b, 0x1b);
    virtual_gic_register_int(vm->vgic, 0xff, 0xff);

    vm = vm_init(&(vm_params[1]));
    vm_ready(vm);
    virtual_gic_register_int(vm->vgic, 0x1b, 0x1b);
    virtual_gic_register_int(vm->vgic, 0xff, 0xff);

    // enable virtual interrupt
    vint_sender_add_permission(0, 1, 0xff);
    vint_sender_add_permission(1, 0, 0xff);
}
