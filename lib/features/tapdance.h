#ifndef TAPDANCE_H
#define TAPDANCE_H

#include <stdint.h>
#include <stdbool.h>

typedef void (*td_fn_t)(uint8_t tap_count);

typedef struct {
    uint16_t kc_single;
    uint16_t kc_double;
    td_fn_t on_dance_finished;
    td_fn_t on_reset;
} tap_dance_action_t;

bool process_tap_dance(uint16_t keycode, bool pressed, uint8_t row, uint8_t col);
void tap_dance_task(void);

#endif // TAPDANCE_H