#include "tapdance.h"
#include "timer.h"
#include "config.h"
#include "usb_hid.h"

extern const tap_dance_action_t tap_dance_actions[];

typedef struct {
    uint8_t count;
    uint32_t timer;
    uint16_t keycode;
    bool interrupted;
    bool pressed;
} tap_dance_state_t;

static tap_dance_state_t tap_dance_state[MAX_TAP_DANCE] = {0};

static void finish_tap_dance(uint8_t index) {
    tap_dance_state_t *state = &tap_dance_state[index];
    const tap_dance_action_t *action = &tap_dance_actions[index];
    
    if (action->on_dance_finished) {
        action->on_dance_finished(state->count);
    } else {
        // Default behavior
        switch (state->count) {
            case 1:
                register_key(action->kc_single);
                unregister_key(action->kc_single);
                break;
            case 2:
                register_key(action->kc_double);
                unregister_key(action->kc_double);
                break;
            default:
                // More taps - could extend functionality
                break;
        }
    }
    
    if (action->on_reset) {
        action->on_reset(state->count);
    }
    
    state->count = 0;
}

bool process_tap_dance(uint16_t keycode, bool pressed, uint8_t row, uint8_t col) {
    if ((keycode & 0xFF00) != 0x5600) {  // TD() keycode
        // Not a tap dance key, but might interrupt active tap dances
        for (int i = 0; i < MAX_TAP_DANCE; i++) {
            if (tap_dance_state[i].count > 0) {
                tap_dance_state[i].interrupted = true;
                finish_tap_dance(i);
            }
        }
        return true;
    }
    
    uint8_t td_index = keycode & 0xFF;
    if (td_index >= MAX_TAP_DANCE) return true;
    
    tap_dance_state_t *state = &tap_dance_state[td_index];
    
    if (pressed) {
        if (state->count == 0 || timer_elapsed(state->timer) > TAP_DANCE_TERM) {
            state->count = 1;
        } else {
            state->count++;
        }
        state->timer = timer_read();
        state->pressed = true;
        state->interrupted = false;
    } else {
        state->pressed = false;
        
        // If this is a quick tap after another tap, wait for more
        if (timer_elapsed(state->timer) >= TAP_DANCE_TERM) {
            finish_tap_dance(td_index);
        }
    }
    
    return false;
}

void tap_dance_task(void) {
    for (int i = 0; i < MAX_TAP_DANCE; i++) {
        tap_dance_state_t *state = &tap_dance_state[i];
        
        if (state->count > 0 && !state->pressed) {
            if (timer_elapsed(state->timer) > TAP_DANCE_TERM) {
                finish_tap_dance(i);
            }
        }
    }
}
