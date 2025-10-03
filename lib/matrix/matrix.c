#include "matrix.h"
#include "hardware/gpio.h"
#include "timer.h"

static matrix_t matrix = {0};
static const uint row_pins[] = ROW_PINS;
static const uint col_pins[] = COL_PINS;

void matrix_init(void) {
    // Initialize row pins as outputs (high)
    for (int i = 0; i < MATRIX_ROWS; i++) {
        gpio_init(row_pins[i]);
        gpio_set_dir(row_pins[i], GPIO_OUT);
        gpio_put(row_pins[i], 1);
    }
    
    // Initialize column pins as inputs with pull-up
    for (int i = 0; i < MATRIX_COLS; i++) {
        gpio_init(col_pins[i]);
        gpio_set_dir(col_pins[i], GPIO_IN);
        gpio_pull_up(col_pins[i]);
    }
}

void matrix_scan(void) {
    uint32_t now = timer_read();
    
    // Save previous state
    for (int i = 0; i < MATRIX_ROWS; i++) {
        matrix.previous[i] = matrix.current[i];
    }
    
    // Scan matrix with debouncing
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        gpio_put(row_pins[row], 0);  // Drive row low
        sleep_us(30);  // Let it settle
        
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            bool pressed = !gpio_get(col_pins[col]);
            uint8_t *state = &matrix.debounce_state[row][col];
            
            if (pressed != (*state & 0x01)) {
                // State changed, start debouncing
                if ((*state & 0x80) == 0) {
                    // Not debouncing, start timer
                    matrix.debounce_timer[row][col] = now;
                    *state |= 0x80;  // Set debouncing flag
                } else if (timer_elapsed(matrix.debounce_timer[row][col]) >= DEBOUNCE_MS) {
                    // Debounce complete
                    *state = pressed ? 0x01 : 0x00;
                    
                    // Update matrix state
                    if (pressed) {
                        matrix.current[row] |= (1 << col);
                    } else {
                        matrix.current[row] &= ~(1 << col);
                    }
                }
            } else {
                // State stable, clear debouncing flag
                *state &= ~0x80;
            }
        }
        
        gpio_put(row_pins[row], 1);  // Drive row high again
    }
}

bool matrix_is_pressed(uint8_t row, uint8_t col) {
    return (matrix.current[row] & (1 << col)) != 0;
}

bool matrix_has_changed(uint8_t row, uint8_t col) {
    uint8_t mask = 1 << col;
    return (matrix.current[row] & mask) != (matrix.previous[row] & mask);
}

uint8_t matrix_get_row(uint8_t row) {
    return matrix.current[row];
}

void matrix_clear_changed(void) {
    for (int i = 0; i < MATRIX_ROWS; i++) {
        matrix.previous[i] = matrix.current[i];
    }
}