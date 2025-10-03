#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "pico/stdlib.h"

static inline uint32_t timer_read(void) {
    return to_ms_since_boot(get_absolute_time());
}

static inline uint32_t timer_elapsed(uint32_t last) {
    return timer_read() - last;
}

static inline void wait_ms(uint32_t ms) {
    sleep_ms(ms);
}

#endif // TIMER_H