#ifndef MATRIX_H
#define MATRIX_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

typedef struct {
    uint8_t current[MATRIX_ROWS];
    uint8_t previous[MATRIX_ROWS];
    uint32_t debounce_timer[MATRIX_ROWS][MATRIX_COLS];
    uint8_t debounce_state[MATRIX_ROWS][MATRIX_COLS];
} matrix_t;

void matrix_init(void);
void matrix_scan(void);
bool matrix_is_pressed(uint8_t row, uint8_t col);
bool matrix_has_changed(uint8_t row, uint8_t col);
uint8_t matrix_get_row(uint8_t row);
void matrix_clear_changed(void);

#endif // MATRIX_H