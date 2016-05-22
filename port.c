#include "port.h"

extern inline B in_b(volatile void *addr);
extern inline void out_b(volatile void *addr, B val);
extern inline HW in_hw(volatile void *addr);
extern inline void out_hw(volatile void *addr, HW val);
extern inline W in_w(volatile void *addr);
extern inline void out_w(volatile void *addr, W val);
