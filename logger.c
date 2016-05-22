#include "type.h"
#include "port.h"
#include "debug.h"

static UW start;
static UW end;
static UW max_len;
extern volatile void *_log_area_start;
extern volatile void *_log_area_end;
static UB* log_area;

void logger_init(void){
    start = 0; end = 0;
    max_len = (UW)(&_log_area_end - &_log_area_start);
    log_area = (UB *)&_log_area_start;
    log_area[end] = '\0';
}

void logger_putc(UB c){
    log_area[end++] = c;
    end %= max_len;
    if(start==end) {
        start++;
        start %= max_len;
    }
    log_area[end] = '\0';
}

void logger_output(void){
    if(start==end) {
        tv_puts("no log\n");
    }
    else if(start<=end) {
        tv_puts(log_area);
    }
    else {
        while(start<max_len){
            if(log_area[start]=='\n') put_serial('\r');
            put_serial(log_area[start++]);
        }
        tv_puts(log_area);
    }
    start = end = 0;
}


