#include "mouse.h"
#include "keycodes.h"
#include "timer.h"
#include "tusb.h"
#include <string.h>

static mouse_state_t mouse_state = {
    .buttons = 0,
    .x = 0,
    .y = 0,
    .wheel_v = 0,
    .wheel_h = 0,
    .speed_multiplier = MOUSE_SPEED_NORMAL
};

static autoclicker_state_t autoclicker = {
    .active = false,
    .button = MOUSE_BTN1,
    .interval_ms = AC_INTERVAL_DEFAULT,
    .last_click_time = 0,
    .click_state = false
};

static bool mouse_report_dirty = false;

// Mouse Report structure (matches TinyUSB HID_REPORT_ID(2))
typedef struct {
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t wheel;
    int8_t pan;
} mouse_report_t;

static mouse_report_t mouse_report = {0};

// Initialize mouse
void mouse_init(void) {
    memset(&mouse_state, 0, sizeof(mouse_state));
    mouse_state.speed_multiplier = MOUSE_SPEED_NORMAL;
    mouse_report_dirty = false;
}

// Mouse movement
void mouse_move(int8_t x, int8_t y) {
    // Apply speed multiplier
    mouse_state.x += (x * mouse_state.speed_multiplier) / 8;
    mouse_state.y += (y * mouse_state.speed_multiplier) / 8;
    
    // Clamp to int8_t range
    if (mouse_state.x > 127) mouse_state.x = 127;
    if (mouse_state.x < -127) mouse_state.x = -127;
    if (mouse_state.y > 127) mouse_state.y = 127;
    if (mouse_state.y < -127) mouse_state.y = -127;
    
    mouse_report_dirty = true;
}

// Mouse scrolling
void mouse_scroll(int8_t vertical, int8_t horizontal) {
    mouse_state.wheel_v = vertical;
    mouse_state.wheel_h = horizontal;
    mouse_report_dirty = true;
}

// Press mouse button
void mouse_press_button(uint8_t button) {
    mouse_state.buttons |= button;
    mouse_report_dirty = true;
}

// Release mouse button
void mouse_release_button(uint8_t button) {
    mouse_state.buttons &= ~button;
    mouse_report_dirty = true;
}

// Click mouse button (press and release)
void mouse_click_button(uint8_t button) {
    mouse_press_button(button);
    mouse_send_report();
    sleep_ms(5);
    mouse_release_button(button);
    mouse_send_report();
}

// Set mouse speed
void mouse_set_speed(uint8_t speed) {
    mouse_state.speed_multiplier = speed;
}

// Get mouse speed
uint8_t mouse_get_speed(void) {
    return mouse_state.speed_multiplier;
}

// Send mouse HID report
void mouse_send_report(void) {
    if (!tud_hid_ready()) return;
    
    // Prepare report
    mouse_report.buttons = mouse_state.buttons;
    mouse_report.x = mouse_state.x;
    mouse_report.y = mouse_state.y;
    mouse_report.wheel = mouse_state.wheel_v;
    mouse_report.pan = mouse_state.wheel_h;
    
    // Send report with report ID 2
    tud_hid_report(2, &mouse_report, sizeof(mouse_report));
    
    // Clear movement and wheel after sending
    mouse_state.x = 0;
    mouse_state.y = 0;
    mouse_state.wheel_v = 0;
    mouse_state.wheel_h = 0;
    
    mouse_report_dirty = false;
}

// Clear mouse state
void mouse_clear(void) {
    mouse_state.buttons = 0;
    mouse_state.x = 0;
    mouse_state.y = 0;
    mouse_state.wheel_v = 0;
    mouse_state.wheel_h = 0;
    mouse_report_dirty = true;
}

// Mouse task - called periodically
void mouse_task(void) {
    if (mouse_report_dirty) {
        mouse_send_report();
    }
}

// Auto-clicker initialization
void autoclicker_init(void) {
    autoclicker.active = false;
    autoclicker.button = MOUSE_BTN1;
    autoclicker.interval_ms = AC_INTERVAL_DEFAULT;
    autoclicker.last_click_time = 0;
    autoclicker.click_state = false;
}

// Toggle auto-clicker
void autoclicker_toggle(void) {
    if (autoclicker.active) {
        autoclicker_stop();
    } else {
        autoclicker_start(autoclicker.button);
    }
}

// Start auto-clicker
void autoclicker_start(uint8_t button) {
    autoclicker.active = true;
    autoclicker.button = button;
    autoclicker.last_click_time = timer_read();
    autoclicker.click_state = false;
}

// Stop auto-clicker
void autoclicker_stop(void) {
    autoclicker.active = false;
    // Release button if currently pressed
    if (autoclicker.click_state) {
        mouse_release_button(autoclicker.button);
        mouse_send_report();
        autoclicker.click_state = false;
    }
}

// Set auto-clicker interval
void autoclicker_set_interval(uint32_t interval_ms) {
    if (interval_ms < AC_INTERVAL_MIN) interval_ms = AC_INTERVAL_MIN;
    if (interval_ms > AC_INTERVAL_MAX) interval_ms = AC_INTERVAL_MAX;
    autoclicker.interval_ms = interval_ms;
}

// Adjust auto-clicker speed
void autoclicker_adjust_speed(int8_t adjustment) {
    int32_t new_interval = (int32_t)autoclicker.interval_ms + (adjustment * AC_INTERVAL_STEP);
    
    if (new_interval < AC_INTERVAL_MIN) new_interval = AC_INTERVAL_MIN;
    if (new_interval > AC_INTERVAL_MAX) new_interval = AC_INTERVAL_MAX;
    
    autoclicker.interval_ms = (uint32_t)new_interval;
}

// Check if auto-clicker is active
bool autoclicker_is_active(void) {
    return autoclicker.active;
}

// Get current auto-clicker interval
uint32_t autoclicker_get_interval(void) {
    return autoclicker.interval_ms;
}

// Auto-clicker task - called periodically
void autoclicker_task(void) {
    if (!autoclicker.active) return;
    
    uint32_t now = timer_read();
    uint32_t elapsed = now - autoclicker.last_click_time;
    
    // Half interval for press, half for release
    uint32_t half_interval = autoclicker.interval_ms / 2;
    
    if (!autoclicker.click_state && elapsed >= half_interval) {
        // Press button
        mouse_press_button(autoclicker.button);
        mouse_send_report();
        autoclicker.click_state = true;
        autoclicker.last_click_time = now;
    } else if (autoclicker.click_state && elapsed >= half_interval) {
        // Release button
        mouse_release_button(autoclicker.button);
        mouse_send_report();
        autoclicker.click_state = false;
        autoclicker.last_click_time = now;
    }
}

// Process mouse keycodes
bool process_mouse_keycode(uint16_t keycode, bool pressed) {
    // Check if this is a mouse keycode
    if ((keycode & 0xFF00) != 0x7100) {
        return true;  // Not a mouse keycode
    }
    
    switch (keycode) {
        case KC_MS_UP:
            if (pressed) mouse_move(0, -1);
            return false;
            
        case KC_MS_DOWN:
            if (pressed) mouse_move(0, 1);
            return false;
            
        case KC_MS_LEFT:
            if (pressed) mouse_move(-1, 0);
            return false;
            
        case KC_MS_RIGHT:
            if (pressed) mouse_move(1, 0);
            return false;
            
        case KC_MS_BTN1:
            if (pressed) {
                mouse_press_button(MOUSE_BTN1);
            } else {
                mouse_release_button(MOUSE_BTN1);
            }
            return false;
            
        case KC_MS_BTN2:
            if (pressed) {
                mouse_press_button(MOUSE_BTN2);
            } else {
                mouse_release_button(MOUSE_BTN2);
            }
            return false;
            
        case KC_MS_BTN3:
            if (pressed) {
                mouse_press_button(MOUSE_BTN3);
            } else {
                mouse_release_button(MOUSE_BTN3);
            }
            return false;
            
        case KC_MS_BTN4:
            if (pressed) {
                mouse_press_button(MOUSE_BTN4);
            } else {
                mouse_release_button(MOUSE_BTN4);
            }
            return false;
            
        case KC_MS_BTN5:
            if (pressed) {
                mouse_press_button(MOUSE_BTN5);
            } else {
                mouse_release_button(MOUSE_BTN5);
            }
            return false;
            
        case KC_MS_WH_UP:
            if (pressed) mouse_scroll(1, 0);
            return false;
            
        case KC_MS_WH_DOWN:
            if (pressed) mouse_scroll(-1, 0);
            return false;
            
        case KC_MS_WH_LEFT:
            if (pressed) mouse_scroll(0, -1);
            return false;
            
        case KC_MS_WH_RIGHT:
            if (pressed) mouse_scroll(0, 1);
            return false;
            
        case KC_MS_ACCEL0:
            if (pressed) mouse_set_speed(MOUSE_SPEED_SLOW);
            return false;
            
        case KC_MS_ACCEL1:
            if (pressed) mouse_set_speed(MOUSE_SPEED_NORMAL);
            return false;
            
        case KC_MS_ACCEL2:
            if (pressed) mouse_set_speed(MOUSE_SPEED_FAST);
            return false;
    }
    
    return true;
}

// Process auto-clicker keycodes
bool process_autoclicker_keycode(uint16_t keycode, bool pressed) {
    // Check if this is an auto-clicker keycode
    if ((keycode & 0xFF00) != 0x7200) {
        return true;  // Not an auto-clicker keycode
    }
    
    if (!pressed) return false;  // Only respond to key press
    
    switch (keycode) {
        case KC_AC_TOGGLE:
            autoclicker_toggle();
            return false;
            
        case KC_AC_LEFT:
            if (autoclicker.active && autoclicker.button == MOUSE_BTN1) {
                autoclicker_stop();
            } else {
                autoclicker_start(MOUSE_BTN1);
            }
            return false;
            
        case KC_AC_RIGHT:
            if (autoclicker.active && autoclicker.button == MOUSE_BTN2) {
                autoclicker_stop();
            } else {
                autoclicker_start(MOUSE_BTN2);
            }
            return false;
            
        case KC_AC_MIDDLE:
            if (autoclicker.active && autoclicker.button == MOUSE_BTN3) {
                autoclicker_stop();
            } else {
                autoclicker_start(MOUSE_BTN3);
            }
            return false;
            
        case KC_AC_FASTER:
            autoclicker_adjust_speed(-5);  // Decrease interval = faster
            return false;
            
        case KC_AC_SLOWER:
            autoclicker_adjust_speed(5);   // Increase interval = slower
            return false;
    }
    
    return true;
}