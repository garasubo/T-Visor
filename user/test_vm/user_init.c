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
#include "board.h"
#include "page_table.h"
#include "my_scheduler.h"

#include "timer.h"
#include "logger.h"


static memory_area_t mem_areas[] = {
    {0x00000000, RAM_START_ADDR, 0x00000000, MEM_AREA_VDEV | MEM_AREA_VRW | (1 << 10), 0},
    {RAM_START_ADDR, RAM_END_ADDR, RAM_START_ADDR, MEM_AREA_VNORMAL | MEM_AREA_VRW | (1 << 10), 0},
    {RAM_END_ADDR, 0xffffffff, RAM_END_ADDR, MEM_AREA_VDEV | MEM_AREA_VRW | (1 << 10), 0},
    {GICD_BASE_ADDR, GICD_BASE_ADDR+0x1000, GICD_BASE_ADDR, MEM_AREA_VDEV | MEM_AREA_VNO | (1 << 10), 0},
    {GICV_BASE_ADDR, GICV_BASE_ADDR+0x2000, GICC_BASE_ADDR, MEM_AREA_VDEV | MEM_AREA_VRW | (1 << 10), 0},
};

static UW mem_area_num = sizeof(mem_areas)/sizeof(mem_areas[0]);

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

static void virtual_irq_register(vgic_t *vgic, UHW pintvec, UHW vintvec){
    virtual_gic_register_int(vgic, pintvec, vintvec);
    irq_handler_register(pintvec, virtual_gic_handle, vgic);
    //gic_enable_int(pintvec, 0x0);
}


void user_init(void){

    scheduler_init(NULL, &my_scheduler);

    static fp_sched_param_t p1 = { 0 };

    T_CVCPU vcpu_params[] = {
        {
            RAM_START_ADDR,
            &p1,
            0
        },
        {
            RAM_START_ADDR,
            &p1,
            0
        },
    };

    T_CVM vm_params = {
        mem_areas,
        mem_area_num,
        vcpu_params,
        sizeof(vcpu_params)/sizeof(vcpu_params[0]),
        HCR_DEFAULT
    };

    T_VM *vm = vm_init(&vm_params);

    vm_ready(vm);


    virtual_gic_register_int(vm->vgic, 0x1b, 0x1b);
    for(int i=32;i<SPI_ID_MAX;i++){
        virtual_irq_register(vm->vgic, i,  i);
    }
    virtual_irq_register(vm->vgic, 27,  27);
    vint_sender_add_permission(vm->id, vm->id, 0xff);

}


