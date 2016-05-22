#include "vm.h"
#include "debug.h"
#include "cp_access.h"
#include "hcr.h"
#include "virtual_gic.h"
#include "memory_manage.h"

static T_VM vm[NUM_OF_T_VM];
static UW num_vm = 0;

T_VM* vm_init(T_CVM *cvm){
    if(num_vm >= NUM_OF_T_VM){
        tv_error_message("! cannot create VM!\n");
        return NULL;
    }
    T_VM *ret = &(vm[num_vm]);
    ret->id = num_vm++;
    ret->page_table = page_table_setup(cvm->memory_area, cvm->memory_area_num);

    ret->vcpus = vcpu_setup(cvm->vcpu_param, cvm->vcpu_num, ret);
    ret->vcpu_num = cvm->vcpu_num;
    ret->vgic = vgic_alloc();
    virtual_gic_init(ret->vgic, ret);
    ret->hcr = cvm->hcr;
    L2CTLR_READ(ret->l2ctlr);
    ret->l2ctlr &= ~(0x3 << 24);
    ret->l2ctlr |= (cvm->vcpu_num-1) << 24;

    return ret;
}

void vm_ready(T_VM *vm){
    tv_enable_print();
    tv_print_string_hex("vcpu num:", vm->vcpu_num);
    vcpu_ready(&(vm->vcpus[0]));
}

void vm_start(T_VM *vm){
    HCR_WRITE(vm->hcr);
    virtual_gic_start(vm->vgic);
}

void vm_stop(T_VM *vm){
    virtual_gic_stop(vm->vgic);
}

T_VM* vm_find_by_id(UW id){
    return &(vm[id]);
}

void vm_add_page(T_VM *vm, memory_area_t *area){
    page_table_add_pages(vm->page_table, area, 1);
}
