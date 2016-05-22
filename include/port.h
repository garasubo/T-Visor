#ifndef __INCLUDED_PORT_H__
#define __INCLUDED_PORT_H__

#include "type.h"

#define INLINE static inline

#define INB(a) in_b((volatile void *)(a))
INLINE B in_b(volatile void *addr){
	return *(volatile B *)addr;
}

#define OUTB(a,v) out_b((volatile void *)(a),v)
INLINE void out_b(volatile void *addr, B val){
	*(volatile B *)addr = val;
}

#define INHW(a) in_hw((volatile void *)(a))
INLINE HW in_hw(volatile void *addr){
	return *(volatile HW *)addr;
}

#define OUTHW(a,v) out_hw((volatile void *)(a),v)
INLINE void out_hw(volatile void *addr, HW val){
	*(volatile HW *)addr = val;
}

#define INW(a) in_w((volatile void *)(a))
INLINE W in_w(volatile void *addr){
	return *(volatile W *)addr;
}

#define OUTW(a,v) out_w((volatile void *)(a),v)
INLINE void out_w(volatile void *addr, W val){
	*(volatile W *)addr = val;
}

#endif
