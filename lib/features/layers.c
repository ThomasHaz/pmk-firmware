#include "layers.h"
#include "timer.h"

static uint8_t layer_state = 1;  // Bitmask, layer 0 always active
static struct {
    uint32_t timer;
    uint8_t count;
    uint8_t layer;
} tap_toggle_state = {0};

void layers_init(void) {
    layer_state = 1;  // Enable layer 0
}

void layer_on(uint8_t layer) {
    if (layer < MAX_LAYERS) {
        layer_state |= (1 << layer);
    }
}

void layer_off(uint8_t layer) {
    if (layer < MAX_LAYERS && layer != 0) {  // Never turn off layer 0
        layer_state &= ~(1 << layer);
    }
}

void layer_toggle(uint8_t layer) {
    if (layer < MAX_LAYERS && layer != 0) {
        layer_state ^= (1 << layer);
    }
}

void layer_clear(void) {
    layer_state = 1;  // Keep only layer 0
}

uint8_t get_highest_layer(void) {
    for (int8_t i = MAX_LAYERS - 1; i >= 0; i--) {
        if (layer_state & (1 << i)) {
            return i;
        }
    }
    return 0;
}

uint8_t get_layer_state(void) {
    return layer_state;
}

uint16_t get_keycode_at(uint8_t layer, uint8_t row, uint8_t col) {
    if (layer >= MAX_LAYERS) return KC_NO;
    
    // Combine column for split keyboard
    uint8_t actual_col = col;
    
    uint16_t keycode = keymaps[layer][row][actual_col];
    
    // Handle transparent keys
    if (keycode == KC_TRNS && layer > 0) {
        // Check lower layers
        for (int8_t i = layer - 1; i >= 0; i--) {
            if (layer_state & (1 << i)) {
                keycode = keymaps[i][row][actual_col];
                if (keycode != KC_TRNS) break;
            }
        }
    }
    
    return keycode;
}

bool process_layer_keycode(uint16_t keycode, bool pressed) {
    uint16_t type = keycode & 0xFF00;
    uint8_t layer = keycode & 0x00FF;
    
    switch (type) {
        case 0x5100:  // MO - Momentary
            if (pressed) {
                layer_on(layer);
            } else {
                layer_off(layer);
            }
            return false;
            
        case 0x5200:  // TG - Toggle
            if (pressed) {
                layer_toggle(layer);
            }
            return false;
            
        case 0x5300:  // TO - Move to layer
            if (pressed) {
                layer_clear();
                layer_on(layer);
            }
            return false;
            
        case 0x5400:  // TT - Tap Toggle
            if (pressed) {
                if (timer_elapsed(tap_toggle_state.timer) < TAPPING_TERM && 
                    tap_toggle_state.layer == layer) {
                    tap_toggle_state.count++;
                    if (tap_toggle_state.count >= 2) {
                        layer_toggle(layer);
                        tap_toggle_state.count = 0;
                    }
                } else {
                    tap_toggle_state.count = 1;
                    tap_toggle_state.layer = layer;
                    layer_on(layer);
                }
                tap_toggle_state.timer = timer_read();
            } else {
                if (!(layer_state & (1 << layer))) {
                    layer_off(layer);
                }
            }
            return false;
    }
    
    return true;
}
