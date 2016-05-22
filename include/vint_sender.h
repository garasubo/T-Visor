#ifndef __INCLUDED_VINT_SENDER_H__
#define __INCLUDED_VINT_SENDER_H__

#include "type.h"

void vint_sender_init(void);
void vint_sender_interface(void);
void vint_sender_add_permission(UB sender_id, UB receiver_id, UHW irq_id);

#endif
