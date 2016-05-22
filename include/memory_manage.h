#ifndef __INCLUDED_MEMORY_MANAGE_H__
#define __INCLUDED_MEMORY_MANAGE_H__

#include "type.h"

void EnableMMU(void);
void DisableMMU(void);
void EnableHMMU(void);
void SetVTTBR(UD vttbr_value);
void SetTTBCR(UW vttbr_value);
void SetTTBR(UD ttbr0, UD ttbr1);
UW GetTTBCR(void);
UD GetTTBR0(void);
UD GetTTBR1(void);
void EnableVMMU(void);
void EnableCache(void);
void DisableCache(void);
void FlushTLB(void);
void flush_cache(void);
void flush_cache_by_va(void *addr, UW len);


#endif
