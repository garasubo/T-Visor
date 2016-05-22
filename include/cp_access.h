#ifndef __INCLUDED_CP_ACCESS_H__

#include "basic.h"

#define SCTLR_READ(x) Asm("mrc p15, 0, %0, c1, c0, 0":"=r"(x))
#define SCTLR_WRITE(x) Asm("mcr p15, 0, %0, c1, c0, 0"::"r"(x))

#define VBAR_READ(x) Asm("mrc p15, 0, %0, c12, c0, 0":"=r"(x))
#define VBAR_WRITE(x) Asm("mcr p15, 0, %0, c12, c0, 0"::"r"(x))

#define VMPIDR_READ(x) Asm("mrc p15, 4, %0, c0, c0, 5":"=r"(x))
#define VMPIDR_WRITE(x) Asm("mcr p15, 4, %0, c0, c0, 5"::"r"(x))

#define DFAR_READ(x) Asm("mrc p15, 0, %0, c6, c0, 0":"=r"(x))
#define DFAR_WRITE(x) Asm("mcr p15, 0, %0, c6, c0, 0"::"r"(x))

#define DFSR_READ(x) Asm("mrc p15, 0, %0, c5, c0, 0":"=r"(x))
#define DFSR_WRITE(x) Asm("mcr p15, 0, %0, c5, c0, 0"::"r"(x))

#define MAIR0_READ(x) Asm("mrc p15, 0, %0, c10, c2, 0":"=r"(x))
#define MAIR1_READ(x) Asm("mrc p15, 0, %0, c10, c2, 1":"=r"(x))
#define MAIR0_WRITE(x) Asm("mcr p15, 0, %0, c10, c2, 0"::"r"(x))
#define MAIR1_WRITE(x) Asm("mcr p15, 0, %0, c10, c2, 1"::"r"(x))

#define HCR_READ(x) Asm("mrc p15, 4, %0, c1, c1, 0":"=r"(x))
#define HCR_WRITE(x) Asm("mcr p15, 4, %0, c1, c1, 0"::"r"(x))

#define CPACR_READ(x) Asm("mrc p15, 0, %0, c1, c0, 0":"=r"(x))
#define CPACR_WRITE(x) Asm("mcr p15, 0, %0, c1, c0, 0"::"r"(x))

#define DACR_READ(x) Asm("mrc p15, 0, %0, c3, c0, 0":"=r"(x))
#define DACR_WRITE(x) Asm("mcr p15, 0, %0, c3, c0, 0"::"r"(x))

#define DFSR_READ(x) Asm("mrc p15, 0, %0, c5, c0, 0":"=r"(x))
#define DFSR_WRITE(x) Asm("mcr p15, 0, %0, c5, c0, 0"::"r"(x))

#define IFSR_READ(x) Asm("mrc p15, 0, %0, c5, c0, 1":"=r"(x))
#define IFSR_WRITE(x) Asm("mcr p15, 0, %0, c5, c0, 1"::"r"(x))

#define ADFSR_READ(x) Asm("mrc p15, 0, %0, c5, c1, 0":"=r"(x))
#define ADFSR_WRITE(x) Asm("mcr p15, 0, %0, c5, c1, 0"::"r"(x))

#define AIFSR_READ(x) Asm("mrc p15, 0, %0, c5, c1, 1":"=r"(x))
#define AIFSR_WRITE(x) Asm("mcr p15, 0, %0, c5, c1, 1"::"r"(x))

#define DFAR_READ(x)  Asm("mrc p15, 0, %0, c6, c0, 0":"=r"(x))
#define DFAR_WRITE(x) Asm("mcr p15, 0, %0, c6, c0, 0"::"r"(x))

#define IFAR_READ(x) Asm("mrc p15, 0, %0, c6, c0, 2":"=r"(x))
#define IFAR_WRITE(x) Asm("mcr p15, 0, %0, c6, c0, 2"::"r"(x))

#define PRRR_READ(x) Asm("mrc p15, 0, %0, c10, c2, 0":"=r"(x))
#define PRRR_WRITE(x) Asm("mcr p15, 0, %0, c10, c2, 0"::"r"(x))

#define NMRR_READ(x) Asm("mrc p15, 0, %0, c10, c2, 1":"=r"(x))
#define NMRR_WRITE(x) Asm("mcr p15, 0, %0, c10, c2, 1"::"r"(x))

#define CIDR_READ(x) Asm("mrc p15, 0, %0, c13, c0, 1":"=r"(x))
#define CIDR_WRITE(x) Asm("mcr p15, 0, %0, c13, c0, 1"::"r"(x))

#define TPIDRPRW_READ(x) Asm("mrc p15, 0, %0, c13, c0, 4":"=r"(x))
#define TPIDRPRW_WRITE(x) Asm("mcr p15, 0, %0, c13, c0, 4"::"r"(x))

#define TPIDRURO_READ(x) Asm("mrc p15, 0, %0, c13, c0, 3":"=r"(x))
#define TPIDRURO_WRITE(x) Asm("mcr p15, 0, %0, c13, c0, 3"::"r"(x))

#define TPIDRURW_READ(x) Asm("mrc p15, 0, %0, c13, c0, 2":"=r"(x))
#define TPIDRURW_WRITE(x) Asm("mcr p15, 0, %0, c13, c0, 2"::"r"(x))

#define VTCR_READ(x)  Asm("mrc p15, 4, %0, c2, c1, 2":"=r"(x))
#define VTCR_WRITE(x) Asm("mcr p15, 4, %0, c2, c1, 2"::"r"(x))

#define CSSELR_READ(x)  Asm("mrc p15, 2, %0, c0, c0, 0":"=r"(x))
#define CSSELR_WRITE(x) Asm("mcr p15, 2, %0, c0, c0, 0"::"r"(x))

#define PAR_READ(lo, hi)  Asm("mrrc p15, 0, %0, %1, c7":"=r"(lo),"=r"(hi))
#define PAR_WRITE(lo, hi) Asm("mcrr p15, 0, %0, %1, c7"::"r"(lo),"r"(hi))

#define L2CTLR_READ(x) Asm("mrc p15, 1, %0, c9, c0, 2":"=r"(x))

#endif
