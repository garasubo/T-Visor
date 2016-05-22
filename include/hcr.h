#ifndef __INCLUDED_HCR_H__
#define __INCLUDED_HCR_H__

#define HCR_VM 0x00000001   //stage 2 MMU enable
#define HCR_FMO (1 << 3)
#define HCR_IMO (1 << 4)
#define HCR_AMO (1 << 5)
#define HCR_VA  (1 << 8)    // virtual abort
#define HCR_VI  (1 << 7)    // virtual irq
#define HCR_VF  (1 << 6)    // virtual fiq
#define HCR_TW1 (1 << 13)
#define HCR_TWE (1 << 14)
#define HCR_DC  (1 << 12)
#define HCR_TGE (1 << 27)
#define HCR_PTW (1 << 2)
#define HCR_TSC (1 << 19)
#define HCR_TIDCP (1 << 20)
#define HCR_TAC (1 << 21)   // ACTLR access
#define HCR_SWIO (1 << 1)   // set/way invalidation override
#define HCR_FB  (1 << 9)    // Force Broadcast

#define HCR_DEFAULT (HCR_VM|HCR_IMO|HCR_FMO|HCR_TSC|HCR_TW1|HCR_TWE|HCR_TAC|HCR_TIDCP|HCR_FB|HCR_SWIO)
#define HCR_SLEEP_MODE (HCR_IMO|HCR_FMO|HCR_TSC)

#endif
