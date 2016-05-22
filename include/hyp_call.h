#ifndef __INCLUDED_HYP_CALL_H__
#define __INCLUDED_HYP_CALL_H__

#define HYP_EC_HVC 0x12

#include "type.h"

UW hyp_get_elr( void );
void hyp_set_elr(UW r0);
UW hyp_reg_read(UB reg_id, UW *sp);
void hyp_reg_write(UB reg_id, UW value, UW *sp);
void hyp_memory_passthrough(UW iss, UW *addr, UW *sp);

#endif
