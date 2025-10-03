#include "modtap.h"
#include "timer.h"
#include "usb_hid.h"  // This include
#include "keycodes.h"

typedef enum {
    MT_IDLE,
    MT_WAITING,
    MT_HELD,
    MT_TAPPED
} modtap_state_t;

typedef struct {
    modtap_state_t state;
    uint32_t press_time;
    uint16_t keycode;
    uint8_t row;
    uint8_t col;
} modtap_t;

static modtap_t modtap_keys[MAX_MODTAP_KEYS] = {0};

bool process_modtap(uint16_t keycode, bool pressed, uint8_t row, uint8_t col) {
    // Check if this is a mod-tap key (has modifier in upper byte)
    if ((keycode & 0xFF00) < 0xE000 || (keycode & 0xFF00) > 0xE700) {
        return true;  // Not a mod-tap key
    }
    
    uint8_t mod = (keycode & 0xFF00) >> 8;
    uint8_t kc = keycode & 0x00FF;
    
    // Find slot for this key
    modtap_t *mt = NULL;
    for (int i = 0; i < MAX_MODTAP_KEYS; i++) {
        if (modtap_keys[i].row == row && modtap_keys[i].col == col) {
            mt = &modtap_keys[i];
            break;
        } else if (modtap_keys[i].state == MT_IDLE && mt == NULL) {
            mt = &modtap_keys[i];
        }
    }
    
    if (!mt) return false;  // No slot available
    
    if (pressed) {
        mt->state = MT_WAITING;
        mt->press_time = timer_read();
        mt->keycode = keycode;
        mt->row = row;
        mt->col = col;
    } else {
        uint32_t hold_time = timer_elapsed(mt->press_time);
        
        if (mt->state == MT_WAITING && hold_time < TAPPING_TERM) {
            // It was a tap
            register_key(kc);
            unregister_key(kc);
            mt->state = MT_TAPPED;
        } else if (mt->state == MT_HELD) {
            // Release the modifier
            unregister_modifier(mod);
        }
        
        mt->state = MT_IDLE;
    }
    
    return false;
}

void modtap_task(void) {
    for (int i = 0; i < MAX_MODTAP_KEYS; i++) {
        modtap_t *mt = &modtap_keys[i];
        
        if (mt->state == MT_WAITING) {
            uint32_t hold_time = timer_elapsed(mt->press_time);
            
            if (hold_time >= TAPPING_TERM) {
                // Convert to hold
                mt->state = MT_HELD;
                uint8_t mod = (mt->keycode & 0xFF00) >> 8;
                register_modifier(mod);
            }
        }
    }
}

void modtap_interrupt(void) {
    // Called when another key is pressed (permissive hold)
    for (int i = 0; i < MAX_MODTAP_KEYS; i++) {
        modtap_t *mt = &modtap_keys[i];
        
        if (mt->state == MT_WAITING) {
            mt->state = MT_HELD;
            uint8_t mod = (mt->keycode & 0xFF00) >> 8;
            register_modifier(mod);
        }
    }
}
