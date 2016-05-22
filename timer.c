#include "basic.h"
#include "type.h"
#include "timer.h"
#include "debug.h"


#define TIMER_CLOCK_FREQ	12500000

void ptimer_init( void )
{
	UW ip;
	ip = TIMER_CLOCK_FREQ;
	Asm("mcr p15, 0, %0, c14, c0, 0":: "r"(ip));
}

UD ptimer_get_value( void ){
	UW t1,t2;
	UD ret;
    Asm("ISB");
	Asm("mrrc p15, 0, %0, %1, c14":"=r"(t1),"=r"(t2));
	ret = t2;
	ret <<= 32;
	ret += t1;
	return ret;
}

UD vtimer_get_value( void ){
	UW t1,t2;
	UD ret;
    Asm("ISB");
	Asm("mrrc p15, 1, %0, %1, c14":"=r"(t1),"=r"(t2));
	ret = t2;
	ret <<= 32;
	ret += t1;
	return ret;
}

void ptimer_set_value( UD next_time ){
	UW t1,t2;
	t1 = next_time;
	t2 = next_time >> 32;

	Asm("mcrr p15, 2, %0, %1, c14"::"r"(t1),"r"(t2));
	UW ip = 0x5;
	Asm("mcr p15, 0, %0, c14, c2, 1":: "r"(ip));
}

void ptimer_set( UW status, UD next_time ){
	UW t1,t2;
	t1 = next_time & 0xffffffffLL;
	t2 = next_time >> 32;

	Asm("mcrr p15, 2, %0, %1, c14"::"r"(t1),"r"(t2));
	Asm("mcr p15, 0, %0, c14, c2, 1":: "r"(status));
}

void vtimer_set( UW status, UD next_time ){
	UW t1,t2;
	t1 = next_time & 0xffffffffLL;
	t2 = next_time >> 32;

	Asm("mcrr p15, 3, %0, %1, c14"::"r"(t1),"r"(t2));
	Asm("mcr p15, 0, %0, c14, c3, 1":: "r"(status));
}

void ptimer_clear( void ){
	UW t1 = 0xffffffff, t2 = 0xffffffff;
	Asm("mcrr p15, 2, %0, %1, c14"::"r"(t1),"r"(t2));

	UW ip = 0x2;
	Asm("mcr p15, 0, %0, c14, c2, 1":: "r"(ip));
}

UW ptimer_get_status( void ){
    UW r0;
    Asm("mrc p15, 0, %0, c14, c2, 1":"=r"(r0));
    return r0;
}

UW vtimer_get_status( void ){
    UW r0;
    Asm("mrc p15, 0, %0, c14, c3, 1":"=r"(r0));
    return r0;
}

UD ptimer_get_cmp_value( void ){
    UW r0,r1;
    Asm("ISB");
    Asm("mrrc p15, 2, %0, %1, c14":"=r"(r0),"=r"(r1));
    UD ret = r1;
    ret <<= 32;
    ret += r0;
    return ret;
}

UD vtimer_get_cmp_value( void ){
    UW r0,r1;
    Asm("ISB");
    Asm("mrrc p15, 3, %0, %1, c14":"=r"(r0),"=r"(r1));
    UD ret = r1;
    ret <<= 32;
    ret += r0;
    return ret;
}

UD vtimer_get_offset( void ){
    UW r0, r1;
    Asm("mrrc p15, 4, %0, %1, c14":"=r"(r0), "=r"(r1));
    UD ret = r1;
    ret <<= 32;
    ret += r0;
    return ret;
}

void vtimer_set_offset( UD offset ){
    UW r0 = offset&0xffffffff, r1 = offset >> 32;
    Asm("mcrr p15, 4, %0, %1, c14"::"r"(r0), "r"(r1));
}

UD htimer_get_cmp_value( void ) {

    UW r0,r1;
    Asm("ISB");
    Asm("mrrc p15, 6, %0, %1, c14":"=r"(r0),"=r"(r1));
    UD ret = r1;
    ret <<= 32;
    ret += r0;
    return ret;
}

void htimer_set_value( UD next_time ){
	UW t1,t2;
	t1 = next_time;
	t2 = next_time >> 32;

	Asm("mcrr p15, 6, %0, %1, c14"::"r"(t1),"r"(t2));
	UW ip = 0x5;
	Asm("mcr p15, 4, %0, c14, c2, 1":: "r"(ip));
}

void htimer_clear( void ){
	UW t1 = 0xffffffff, t2 = 0xffffffff;
	Asm("mcrr p15, 6, %0, %1, c14":"=r"(t1),"=r"(t2));

	UW ip = 0x2;
	Asm("mcr p15, 4, %0, c14, c2, 1":: "r"(ip));
}

bool htimer_is_enable( void ){
    UW ip;
    Asm("mrc p15, 4, %0, c14, c2, 1":"=r"(ip));
    return ip!=0x2;
}
