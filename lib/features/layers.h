#ifndef LAYERS_H
#define LAYERS_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "keycodes.h"

void layers_init(void);
void layer_on(uint8_t layer);
void layer_off(uint8_t layer);
void layer_toggle(uint8_t layer);
void layer_clear(void);
uint8_t get_highest_layer(void);
uint8_t get_layer_state(void);
bool process_layer_keycode(uint16_t keycode, bool pressed);
uint16_t get_keycode_at(uint8_t layer, uint8_t row, uint8_t col);

// Keymap is defined externally
extern const uint16_t keymaps[][MATRIX_ROWS][MATRIX_COLS * 2];

#endif // LAYERS_H