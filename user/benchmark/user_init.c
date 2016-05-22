#include "memory_define.h"
#include "vm.h"
#include "vcpu.h"
#include "debug.h"
#include "hyp_call.h"
#include "cortexa7.h"
#include "basic.h"
#include "fp_scheduler.h"
#include "hcr.h"
#include "port.h"
#include "virtual_device_handle.h"
#include "vint_sender.h"
#include "setting_benchmark.h"
#include "timer_event.h"

#include "timer.h"
#include "logger.h"


extern int TestVm(void);

UW test_vcpu_id;

static void virtual_gic_handle(UW eoir, void *arg){
    vgic_t *vgic = (vgic_t *)arg;
    if(virtual_gic_send_hardware_int(vgic, eoir)){
        OUTW(GICC_EOIR,eoir);
    } else {
        tv_abort("# failed to send irq\n");
        tv_getc();
        OUTW(GICC_EOIR,eoir);
        OUTW(GICC_DIR,eoir);
    }
}

static void virtual_irq_register(vgic_t *vgic, UHW pintvec, UHW vintvec){
    virtual_gic_register_int(vgic, pintvec, vintvec);
    irq_handler_register(pintvec, virtual_gic_handle, vgic);
    //gic_enable_int(pintvec, 0x0);
}


void user_init( void ){

    scheduler_init(NULL, &fp_scheduler);

    static fp_sched_param_t p1 = { 0 };

    T_CVCPU vcpu_params[] = {
        {
            START_ADDR,
            &p1,
            0
        },
    };

    T_CVM vm_params_1 = {
        MemArea_1,
        MemAreaNum_1,
        vcpu_params,
        sizeof(vcpu_params)/sizeof(vcpu_params[0]),
        HCR_DEFAULT
    };


    T_VM *vm = vm_init(&vm_params_1);

    vm_ready(vm);


    virtual_gic_register_int(vm->vgic, 0x1b, 0x1b);
    for(int i=32;i<SPI_ID_MAX;i++){
        virtual_irq_register(vm->vgic, i,  i);
    }
}
