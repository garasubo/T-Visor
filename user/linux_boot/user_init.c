#include "memory_define.h"
#include "vm.h"
#include "vcpu.h"
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
#include "vint_sender.h"
#include "setting_linux_boot.h"
#include "timer_event.h"

#include "timer.h"
#include "logger.h"


extern int TestVm(void);

UW test_vcpu_id;

static void virtual_gic_handle(UW eoir, void *arg){
    vgic_t *vgic = (vgic_t *)arg;
    if(virtual_gic_send_hardware_int(vgic, eoir)){
        OUTW(GICC_EOIR,eoir);
        /* scheduler_set_flag(); */
    } else {
        OUTW(GICC_EOIR,eoir);
        OUTW(GICC_DIR,eoir);
    }
}

static void virtual_irq_register(vgic_t *vgic, UHW pintvec, UHW vintvec){
    virtual_gic_register_int(vgic, pintvec, vintvec);
    irq_handler_register(pintvec, virtual_gic_handle, vgic);
}


static void linux_boot(void){

    tv_error_message("start linux boot\n");
    scheduler_init(NULL, &my_scheduler);

    T_CVCPU vcpu_params[] = {
        {
            LINUX_START_ADDR,
            NULL,
            0
        },
        {
            LINUX_START_ADDR,
            NULL,
            0
        },
    };

    T_CVM vm_params = {
        LINUX_MEMORY_AREAS,
        LINUX_MEMORY_AREA_NUM,
        vcpu_params,
        sizeof(vcpu_params)/sizeof(vcpu_params[0]),
        HCR_DEFAULT
    };

    T_VM *vm = vm_init(&vm_params);

    vm_ready(vm);

    vm->vcpus[0].reg[0] = 0;
    vm->vcpus[0].reg[1] = 0;
    vm->vcpus[0].reg[2] = LINUX_DBT_ADDR;

    virtual_gic_register_int(vm->vgic, 0x1b, 0x1b);
    for(int i=32;i<SPI_ID_MAX;i++){
        virtual_irq_register(vm->vgic, i,  i);
    }

}


void user_init( void ){
    linux_boot();
}
