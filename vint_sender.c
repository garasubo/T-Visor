#include "basic.h"
#include "type.h"
#include "rbtree.h"
#include "vm.h"
#include "vint_sender.h"
#include "debug.h"

static int comp_func(void *a, void *b){
    W na = (W)a;
    W nb = (W)b;
    return nb-na;
}

static tree_t *tree; // send_id & irq_num -> permit_mask

void vint_sender_init(void){
    tree = rbtree_alloc(comp_func);
}

void vint_sender_interface(void){
    UW *sp = vcpu_get_current_sp()->reg;
    UB sid = vcpu_get_executing()->id;
    UB rid = sp[1];
    UHW irq_id = sp[2];

    UW key = ((UW)rid) << 16 | irq_id;

    node_t *node = rbtree_search(tree, (void *)key);
    if(node){
        UW mask = (UW)node->val;
        if(mask&(1 << sid)){
            T_VM *vm = vm_find_by_id(rid);
            if(virtual_gic_send_software_int(vm->vgic, irq_id)){
                // do nothing
            } else {
            }
        }
    } else {
        tv_abort("this call is not registered");
    }
}

void vint_sender_add_permission(UB sender_id, UB receiver_id, UHW irq_id){
    UW key = ((UW)receiver_id) << 16 | irq_id;

    node_t *node = rbtree_search(tree, (void *)key);
    if(node){
        UW mask = (UW)node->val | (1 << sender_id);
        rbtree_insert(tree, (void *)key, (void *)mask);
    }
    else {
        rbtree_insert(tree, (void *)key, (void *)(1 << sender_id));
    }
}
