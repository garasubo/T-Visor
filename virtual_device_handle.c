#include "basic.h"
#include "rbtree.h"
#include "debug.h"
#include "virtual_device_handle.h"
#include "cp_access.h"
#include "vcpu.h"

static tree_t *tree; // irq_num -> handler function

typedef struct {
    UW start;
    UW end;
} addr_range_t;

typedef struct {
    virtual_deveice_handler_func_t func;
    void *param;
} handler_func_t;

addr_range_t rng[32];
handler_func_t hfunc[32];
UW rng_num = 0;
UW hfunc_num = 0;

static int addr_comp(void *a, void *b){
    addr_range_t *ra = (addr_range_t *)a;
    addr_range_t *rb = (addr_range_t *)b;

    if((ra->start<=rb->start&&rb->end<ra->end)||(rb->start<=ra->start&&ra->end<rb->end)){
        return 0;
    }


    if(rb->start >= ra->end){
        return 1;
    }
    else if(ra->start >= rb->end){
        return -1;
    }

    tv_abort("#iligal nodes!\n");
    return 0;
}

void virtual_device_handle_init(void){
    tree = rbtree_alloc(addr_comp);
}

void virtual_device_handle_register(UW start, UW end, virtual_deveice_handler_func_t func, void *param){
    if(rng_num>=sizeof(rng)/sizeof(rng[0])||hfunc_num>=sizeof(hfunc)/sizeof(hfunc[0])){
        tv_abort("limit over\n");
    }
    addr_range_t *key = &(rng[rng_num++]);
    key->start = start;
    key->end = end;
    handler_func_t *f = &(hfunc[hfunc_num++]);
    f->func = func;
    f->param = param;
    rbtree_insert(tree, key, f);
}

void virtual_device_handle(UW addr, UW iss){
    addr_range_t range;
    range.start = addr;
    range.end = addr;
    node_t *node = rbtree_search(tree, &range);
    if(node==NULL) {
        tv_abort("!!! cannot handle !!!\n");
        return;
    }
    handler_func_t *f = (handler_func_t *)node->val;
    (f->func)(f->param, addr, iss);
}
