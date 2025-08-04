

#ifndef _TIME_H
#define _TIME_H

#include <cdefs.h>

__BEGIN_DECLS

auto constexpr IRQ0_FREQUENCY   =          100;
auto constexpr INPUT_FREQUENCY  =          1193180;
auto constexpr COUNTER0_VALUE   =          INPUT_FREQUENCY / IRQ0_FREQUENCY;
auto constexpr COUNTER0_PORT    =          0x40;
auto constexpr COUNTER0_NO      =          0;
auto constexpr COUNTER_MODE     =          2;
auto constexpr READ_WRITE_LATCH =          3;
auto constexpr PIT_CONTROL_PORT =          0x43;


void intr_time_handler();

__END_DECLS

#endif //TIME_H
