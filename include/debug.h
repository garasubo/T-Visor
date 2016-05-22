#ifndef __INCLUDED_DEBUG_H__
#define __INCLUDED_DEBUG_H__

#include "type.h"

void tv_set_serial_func(void (*func)(UB));
void tv_puts(UB* str);
void tv_print_hex(UD val);
void serial_init(void);
UB tv_getc(void);
void put_serial(UB c);
void tv_message(UW val);
void tv_error_message(UB* str);
void tv_print_lr(void);
void tv_disable_print(void);
void tv_enable_print(void);
void tv_print_string_hex(UB *str, UD val);
void tv_abort(UB *str);
void tv_display_boot_process(UW num);
#endif
