#ifndef COMBOS_H
#define COMBOS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    const uint16_t *keys;
    uint8_t key_count;
    uint16_t keycode;
} combo_t;

void init_combos(void);
bool process_combo(uint16_t keycode, bool pressed);

#endif // COMBOS_H
