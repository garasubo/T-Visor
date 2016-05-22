#ifndef __INCLUDED_VM_H__
#define __INCLUDED_VM_H__

#include "basic.h"
#include "type.h"

struct t_vm;
typedef struct t_vm T_VM;

#include "vcpu.h"
#include "virtual_gic.h"
#include "page_table.h"

#ifndef NUM_OF_T_VM
#define NUM_OF_T_VM 8
#endif

typedef struct {
    memory_area_t *memory_area;
    UW memory_area_num;
    T_CVCPU *vcpu_param;
    UW vcpu_num;
    UW hcr;
} T_CVM;

struct t_vm {
    UW id;
    UD *page_table;
    T_VCPU *vcpus;
    UW vcpu_num;
    vgic_t *vgic;
    UW hcr;
    UW l2ctlr;
};

T_VM* vm_init(T_CVM *cvm);
T_VM* vm_find_by_id(UW id);
void vm_start(T_VM *vm);
void vm_stop(T_VM *vm);
void vm_ready(T_VM *vm);
void vm_add_page(T_VM *vm, memory_area_t *area);

#endif
