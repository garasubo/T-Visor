#ifndef __INCLUDED_VIRTUAL_DEVICE_HANDLE_H__
#define __INCLUDED_VIRTUAL_DEVICE_HANDLE_H__

#include "type.h"

typedef void (*virtual_deveice_handler_func_t)(void *, UW, UW);

void virtual_device_handle_init(void);
void virtual_device_handle_register(UW start, UW end, virtual_deveice_handler_func_t func, void *param);
void virtual_device_handle(UW addr, UW hsr);

#endif
