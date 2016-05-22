#include "type.h"
#include "basic.h"
#include "debug.h"

void tv_abort(UB *str){
    tv_enable_print();
    serial_init();
    tv_puts(str);
    tv_getc();
}
