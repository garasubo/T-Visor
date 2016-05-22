#ifndef __INCLUDED_TIMER_H__
#define __INCLUDED_TIMER_H__

#include "type.h"
#define TIMER_CLOCK_FREQ	12500000

void ptimer_init( void );

UD ptimer_get_value( void );

void ptimer_set_value( UD next_time );

void ptimer_set( UW status, UD next_time );

void ptimer_clear( void );

UW ptimer_get_status( void );

UD ptimer_get_cmp_value( void );

UD htimer_get_cmp_value( void );

void htimer_set_value( UD next_time );

void htimer_clear( void );

bool htimer_is_enable( void );

UD vtimer_get_offset( void );

void vtimer_set_offset( UD offset );

UD vtimer_get_value( void );

UW vtimer_get_status( void );

UD vtimer_get_cmp_value( void );

void vtimer_set( UW status, UD next_time );


#endif
