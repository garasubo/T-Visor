#ifndef __INCLUDED_CPU_MODE_H__
#define __INCLUDED_CPU_MODE_H__

#define PSM_USR (0x10)
#define PSM_FIQ (0x11)
#define PSM_IRQ (0x12)
#define PSM_SVC (0x13)
#define PSM_MON (0x16)
#define PSM_ABT (0x17)
#define PSM_HYP (0x1a)
#define PSM_UND (0x1b)
#define PSM_SYS (0x1f)

#define CPSR_N (0x80000000)
#define CPSR_Z (0x40000000)
#define CPSR_C (0x20000000)
#define CPSR_V (0x10000000)
#define CPSR_E (0x00000200)
#define CPSR_A (0x00000100)
#define CPSR_I (0x00000080)
#define CPSR_F (0x00000040)

#define CPSR_DEFAULT (PSM_SVC)

#endif
