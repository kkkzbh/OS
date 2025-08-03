

#ifndef _KINTERRUPT_H
#define _KINTERRUPT_H

#include <cdefs.h>

__BEGIN_DECLS

#include <stdint.h>

typedef void* intr_handler;

void idt_init();

typedef enum intr_status : bool
{
    INTR_OFF = false, // 表示关中断
    INTR_ON  = true   // 表示开中断
} intr_status;

intr_status intr_get_status();

intr_status intr_set_status(intr_status status);

intr_status intr_enable();

intr_status intr_disable();

void register_handler(u8 vector_no,intr_handler func);

__END_DECLS

#endif