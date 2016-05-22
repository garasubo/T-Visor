#include "basic.h"
#include "type.h"
#include "debug.h"
#include "page_table.h"
#include "rbtree.h"
#include "vint_sender.h"
#include "gic.h"
#include "virtual_gic.h"
#include "cp_access.h"
#include "virtual_device_handle.h"
#include "timer_event.h"
#include "memory_manage.h"
#include "cortexa7.h"
#include "vcpu.h"
#include "logger.h"

extern void user_init(void);

void system_init(void){
    serial_init();
    tv_display_boot_process(1);
    page_table_init();
    tv_display_boot_process(2);
    rbtree_init();
    tv_display_boot_process(3);
    irq_handler_init();
    tv_display_boot_process(4);
    logger_init();
    tv_display_boot_process(5);
    virtual_device_handle_init();
    tv_display_boot_process(6);
    gic_virtual_init();
    tv_display_boot_process(7);
    gic_enable_int(IRQ_ID_HT,0);
    tv_display_boot_process(8);
    virtual_gic_register_handle();
    tv_display_boot_process(9);
    timer_event_init();
    tv_display_boot_process(0xa);
    vint_sender_init();
    tv_display_boot_process(0xb);
    user_init();
    tv_display_boot_process(0xc);
    vcpu_preemption(scheduler_get_operations()->schedule());
    tv_display_boot_process(0xd);
    flush_cache();
    tv_display_boot_process(0xe);
    FlushTLB();
    tv_display_boot_process(0xf);
}
