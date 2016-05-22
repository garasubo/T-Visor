#include "debug.h"
#include "port.h"
#include "basic.h"



static bool enable_output = true;

static void (*put_serial_func)(UB) = put_serial;

extern B get_serial(void);
extern void put_serial(UB);

void tv_set_serial_func(void (*func)(UB)){
    put_serial_func = func;
}


void tv_print_hex(UD val)
{
    tv_puts("0x");
    char str[20];
    int i=0;
    do {
        int x = val&0xf;
        if(x>=0xa) str[i++] = x-0xa+'a';
        else str[i++] = x+'0';
        val >>= 4;
    } while(val>0);
    int j;
    for(j=0;j<i/2;j++){
        char tmp = str[j];
        str[j] = str[i-j-1];
        str[i-j-1] = tmp;
    }
    str[i] = '\0';
    tv_puts(str);
}

void tv_puts(UB* str){
    while(*str!='\0'){
        if(*str=='\n') put_serial_func('\r');
        put_serial_func(*str);
        str++;
    }
}

void tv_error_message(UB* str){
    bool tmp = enable_output;
    enable_output = true;
    tv_puts(str);
    enable_output = tmp;
    // tv_getc();
}

UB tv_getc(){
    bool tmp = enable_output;
    enable_output =true;
    tv_puts("key input\n");
    enable_output = tmp;
    return get_serial();
}

void tv_message(UW val){
    tv_puts("debug: ");
    tv_print_hex(val);
    tv_puts("\n");
}


void tv_print_lr(void){
    tv_puts("lr value: ");
    UW r0;
    Asm("mov %0, lr":"=r"(r0));
    tv_print_hex(r0);
    tv_puts("\n");

}

void tv_disable_print(void){
    enable_output = false;
}

void tv_enable_print(void){
    enable_output = true;
}

void tv_print_string_hex(UB *str, UD val){
    tv_puts(str);
    tv_print_hex(val);
    tv_puts("\n");
}

void __attribute__((weak))tv_abort(UB *str){
    enable_output = true;
    tv_puts(str);
    tv_getc();
}

void __attribute__((weak))tv_display_boot_process(UW num){
    // tv_print_string_hex("# now booting...:",num);
    return;
}
