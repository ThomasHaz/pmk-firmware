#include "oneshot.h"
#include "layers.h"
#include "timer.h"
#include "config.h"

typedef struct {
    bool active;
    uint8_t layer;
    uint32_t timer;
    bool used;
} oneshot_state_t;

static oneshot_state_t oneshot = {0};

bool process_oneshot_layer(uint16_t keycode, bool pressed) {
    if ((keycode & 0xFF00) == 0x5500) {  // OSL keycode
        uint8_t layer = keycode & 0x00FF;
        
        if (pressed) {
            oneshot.active = true;
            oneshot.layer = layer;
            oneshot.timer = timer_read();
            oneshot.used = false;
            layer_on(layer);
        }
        return false;
    }
    
    // Check if we should deactivate oneshot
    if (oneshot.active && pressed && keycode != KC_NO) {
        if (!oneshot.used) {
            oneshot.used = true;
        } else {
            // Second key pressed, deactivate
            layer_off(oneshot.layer);
            oneshot.active = false;
        }
    }
    
    return true;
}

void oneshot_task(void) {
    if (oneshot.active && timer_elapsed(oneshot.timer) > ONESHOT_TIMEOUT) {
        layer_off(oneshot.layer);
        oneshot.active = false;
    }
}

uint8_t get_oneshot_layer(void) {
    return oneshot.active ? oneshot.layer : 0;
}

bool is_oneshot_active(void) {
    return oneshot.active;
}
