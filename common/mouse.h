#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>
#include <stdbool.h>

// Mouse button masks
#define MOUSE_BTN1 (1 << 0)  // Left
#define MOUSE_BTN2 (1 << 1)  // Right
#define MOUSE_BTN3 (1 << 2)  // Middle
#define MOUSE_BTN4 (1 << 3)  // Back
#define MOUSE_BTN5 (1 << 4)  // Forward

// Mouse state structure
typedef struct {
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t wheel_v;  // Vertical wheel
    int8_t wheel_h;  // Horizontal wheel
    uint8_t speed_multiplier;
} mouse_state_t;

// Auto-clicker state structure
typedef struct {
    bool active;
    uint8_t button;  // Which button to auto-click
    uint32_t interval_ms;
    uint32_t last_click_time;
    bool click_state;  // true = pressed, false = released
} autoclicker_state_t;

// Mouse functions
void mouse_init(void);
void mouse_task(void);
void mouse_move(int8_t x, int8_t y);
void mouse_scroll(int8_t vertical, int8_t horizontal);
void mouse_press_button(uint8_t button);
void mouse_release_button(uint8_t button);
void mouse_click_button(uint8_t button);
void mouse_set_speed(uint8_t speed);
uint8_t mouse_get_speed(void);
void mouse_send_report(void);
void mouse_clear(void);

// Auto-clicker functions
void autoclicker_init(void);
void autoclicker_task(void);
void autoclicker_toggle(void);
void autoclicker_start(uint8_t button);
void autoclicker_stop(void);
void autoclicker_set_interval(uint32_t interval_ms);
void autoclicker_adjust_speed(int8_t adjustment);
bool autoclicker_is_active(void);
uint32_t autoclicker_get_interval(void);

// Process mouse keycodes
bool process_mouse_keycode(uint16_t keycode, bool pressed);
bool process_autoclicker_keycode(uint16_t keycode, bool pressed);

#endif // MOUSE_H