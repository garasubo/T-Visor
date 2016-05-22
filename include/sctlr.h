#ifndef __INCLUDED_SCTLR_H__
#define __INCLUDED_SCTLR_H__

// cache disabled, swap inst enable
#define SCTLR_DEFALT (0x00c50050)

#define SCTLR_TE (0x40000000)
#define SCTLR_AFE (0x20000000)
#define SCTLR_TEX (0x10000000)
#define SCTLR_NMDI (0x08000000)
#define SCTLR_EE (0x02000000)
#define SCTLR_VE (0x01000000)
#define SCTLR_FI (0x00200000)
#define SCTLR_UWXN (0x00100000)
#define SCTLR_WXN (0x00080000)
#define SCTLR_HA (0x00020000)
#define SCTLR_RR (0x00004000)
#define SCTLR_V (0x00002000)
#define SCTLR_I (0x00001000)
#define SCTLR_Z (0x00000800)
#define SCTLR_SW (0x00000400)
#define SCTLR_BEN (0x00000020)
#define SCTLR_C (0x00000004)
#define SCTLR_A (0x00000002)
#define SCTLR_M (0x00000001)


#endif
