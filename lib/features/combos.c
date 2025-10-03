#include "combos.h"
#include "timer.h"
#include "config.h"
#include "usb_hid.h"  // This include

extern const combo_t key_combos[];
extern const uint8_t combo_count;

typedef struct {
    bool active;
    uint32_t timer;
    uint16_t pressed_keys[8];
    uint8_t pressed_count;
} combo_state_t;

static combo_state_t combo_state = {0};

bool process_combo(uint16_t keycode, bool pressed) {
    if (!pressed) {
        // Check if releasing a combo key
        for (uint8_t i = 0; i < combo_state.pressed_count; i++) {
            if (combo_state.pressed_keys[i] == keycode) {
                // Combo interrupted by release
                combo_state.active = false;
                combo_state.pressed_count = 0;
                return true;
            }
        }
        return true;
    }
    
    // Key pressed
    if (!combo_state.active) {
        combo_state.active = true;
        combo_state.timer = timer_read();
        combo_state.pressed_count = 0;
    } else if (timer_elapsed(combo_state.timer) > COMBO_TERM) {
        // Combo window expired
        combo_state.active = false;
        combo_state.pressed_count = 0;
        return true;
    }
    
    // Add key to combo state
    if (combo_state.pressed_count < 8) {
        combo_state.pressed_keys[combo_state.pressed_count++] = keycode;
    }
    
    // Check if this matches any combo
    for (uint8_t c = 0; c < combo_count; c++) {
        const combo_t *combo = &key_combos[c];
        
        if (combo->key_count != combo_state.pressed_count) continue;
        
        bool match = true;
        for (uint8_t i = 0; i < combo->key_count; i++) {
            bool found = false;
            for (uint8_t j = 0; j < combo_state.pressed_count; j++) {
                if (combo->keys[i] == combo_state.pressed_keys[j]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                match = false;
                break;
            }
        }
        
        if (match) {
            // Combo matched!
            register_key(combo->keycode);
            unregister_key(combo->keycode);
            combo_state.active = false;
            combo_state.pressed_count = 0;
            return false;  // Don't process original keys
        }
    }
    
    return true;
}

void init_combos(void) {
    // Initialization if needed
}