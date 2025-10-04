// /keymaps/default/keymap.c
#include <stddef.h>  // Add this for NULL
#include <string.h>  // Add this for memcpy
#include "keycodes.h"
#include "config.h"
#include "layers.h"
#include "combos.h"
#include "tapdance.h"
#include "features.h"
#include "usb_hid.h"

// Define layers
enum layers {
    _BASE,
    _QWERT,
    _LOWER,
    _RAISE,
    _ADJUST,
    _MOUSE
};

// Define tap dance indices
enum tap_dances {
    TD_ESC_CAPS,
    TD_SPC_ENT,
    TD_Q_ESC
};

// Custom keycodes (if needed)
enum custom_keycodes {
    CUSTOM_1 = 0x7000,
    CUSTOM_2,
    CUSTOM_3
};

// Keymap - 4 rows x 12 columns (6 per half)
const uint16_t keymaps[][MATRIX_ROWS][MATRIX_COLS * 2] = {
    [_BASE] = {
        // Left Half                                    // Right Half
        {KC_A, KC_B, KC_C, KC_D, KC_E, KC_F,       KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_BSPC},
        {KC_G, KC_H, KC_I, KC_J, KC_K, KC_L,              KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT},
        {KC_M, KC_N, KC_O, KC_P, KC_Q, KC_R,       KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT},
        {KC_S, KC_T, KC_U, KC_V, KC_W, KC_X,    KC_ENT, MO(_RAISE), KC_RALT, KC_RGUI, OSL(_ADJUST), TD(TD_ESC_CAPS)}
    },

    [_QWERT] = {
        // Left Half                                    // Right Half
        {KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,       KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_BSPC},
        {CTL_T(KC_ESC), KC_A, KC_S, KC_D, KC_F, KC_G,              KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT},
        {KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,       KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT},
        {KC_LGUI, KC_LALT, MO(_LOWER), KC_SPC, TD(TD_SPC_ENT), MO(_RAISE),    KC_ENT, MO(_RAISE), KC_RALT, KC_RGUI, OSL(_ADJUST), TD(TD_ESC_CAPS)}
    },
    
    [_LOWER] = {
        {KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,       KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_DEL},
        {_______, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,      KC_F6,   KC_MINS, KC_EQL,  KC_LBRC, KC_RBRC, KC_BSLS},
        {_______, KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,     KC_F12,  KC_HOME, KC_END,  KC_PGUP, KC_PGDN, _______},
        {_______, _______, _______, _______, _______, _______,    _______, MO(_ADJUST), _______, _______, _______, _______}
    },
    
    [_RAISE] = {
        {KC_GRV,  KC_EXLM, KC_AT,   KC_HASH, KC_DLR,  KC_PERC,    KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN, KC_DEL},
        {_______, _______, _______, _______, _______, _______,    KC_LEFT, KC_DOWN, KC_UP,   KC_RIGHT,_______, _______},
        {_______, _______, KC_CUT,  KC_COPY, KC_PASTE,_______,    KC_HOME, KC_PGDN, KC_PGUP, KC_END,  _______, _______},
        {_______, _______, MO(_ADJUST), _______, _______, _______,    _______, _______, _______, _______, _______, _______}
    },
    
    [_ADJUST] = {
        {RESET,   _______, _______, _______, _______, _______,    _______, _______, _______, _______, _______, RESET},
        {_______, _______, _______, _______, _______, _______,    _______, _______, _______, _______, _______, _______},
        {_______, _______, _______, _______, _______, _______,    _______, _______, _______, _______, _______, _______},
        {_______, _______, _______, _______, _______, _______,    _______, _______, _______, _______, _______, _______}
    },

    // Mouse layer - WASD for movement, Q/E for clicks, R/F for auto-clicker
    [_MOUSE] = {
        // Left Half                                              // Right Half
        {KC_MS_ACCEL0, KC_MS_BTN1, KC_MS_WH_UP, KC_MS_BTN2, KC_AC_LEFT, KC_AC_TOGGLE,    KC_MS_WH_UP, KC_MS_WH_LEFT, KC_MS_UP, KC_MS_WH_RIGHT, KC_MS_BTN4, KC_MS_ACCEL2},
        {KC_MS_ACCEL1, KC_MS_BTN3, KC_MS_WH_DOWN, KC_AC_RIGHT, KC_AC_FASTER, KC_AC_SLOWER,  KC_MS_WH_DOWN, KC_MS_LEFT, KC_MS_DOWN, KC_MS_RIGHT, KC_MS_BTN5, _______},
        {_______,    _______,   _______,   _______,   _______,   _______,    _______,  _______,  _______,  _______,  _______,  _______},
        {_______,    _______,   _______,   KC_MS_BTN1, KC_MS_BTN2, _______,    KC_MS_BTN1, _______, _______, _______, _______, _______}
    }
};

// Combo definitions - define the actual key combinations
// const uint16_t PROGMEM combo_jk[] = {KC_J, KC_K, COMBO_END};
// const uint16_t PROGMEM combo_df[] = {KC_D, KC_F, COMBO_END};
// const uint16_t PROGMEM combo_sd[] = {KC_S, KC_D, COMBO_END};
// const uint16_t PROGMEM combo_kl[] = {KC_K, KC_L, COMBO_END};
// const uint16_t PROGMEM combo_as[] = {KC_A, KC_S, COMBO_END};
// const uint16_t PROGMEM combo_qw[] = {KC_Q, KC_W, COMBO_END};
// const uint16_t PROGMEM combo_op[] = {KC_O, KC_P, COMBO_END};

const uint16_t combo_jk[] = {KC_J, KC_K, COMBO_END};
const uint16_t combo_df[] = {KC_D, KC_F, COMBO_END};
const uint16_t combo_sd[] = {KC_S, KC_D, COMBO_END};
const uint16_t combo_kl[] = {KC_K, KC_L, COMBO_END};
const uint16_t combo_as[] = {KC_A, KC_S, COMBO_END};
const uint16_t combo_qw[] = {KC_Q, KC_W, COMBO_END};
const uint16_t combo_op[] = {KC_O, KC_P, COMBO_END};

// Combo array - maps combinations to output keycodes
const combo_t key_combos[] = {
    {combo_jk, 2, KC_ESC},    // J+K = ESC
    {combo_df, 2, KC_TAB},    // D+F = TAB
    {combo_sd, 2, KC_CAPS},   // S+D = CAPS LOCK
    {combo_kl, 2, KC_ENT},    // K+L = ENTER
    {combo_as, 2, KC_LGUI},   // A+S = Windows/Cmd key
    {combo_qw, 2, KC_ESC},    // Q+W = ESC (alternative)
    {combo_op, 2, KC_BSPC},   // O+P = BACKSPACE
};

const uint8_t combo_count = sizeof(key_combos) / sizeof(combo_t);

// Tap Dance implementations
void td_esc_caps_finished(uint8_t count) {
    switch(count) {
        case 1:
            register_key(KC_ESC);
            break;
        case 2:
            register_key(KC_CAPS);
            break;
        case 3:
            // Triple tap - could add another function
            register_key(KC_GRV);
            break;
        default:
            break;
    }
}

void td_esc_caps_reset(uint8_t count) {
    switch(count) {
        case 1:
            unregister_key(KC_ESC);
            break;
        case 2:
            unregister_key(KC_CAPS);
            break;
        case 3:
            unregister_key(KC_GRV);
            break;
        default:
            break;
    }
}

void td_spc_ent_finished(uint8_t count) {
    switch(count) {
        case 1:
            register_key(KC_SPC);
            break;
        case 2:
            register_key(KC_ENT);
            break;
        default:
            break;
    }
}

void td_spc_ent_reset(uint8_t count) {
    switch(count) {
        case 1:
            unregister_key(KC_SPC);
            break;
        case 2:
            unregister_key(KC_ENT);
            break;
        default:
            break;
    }
}

// Tap Dance definitions array
const tap_dance_action_t tap_dance_actions[] = {
    [TD_ESC_CAPS] = {
        .kc_single = KC_ESC,
        .kc_double = KC_CAPS,
        .on_dance_finished = td_esc_caps_finished,
        .on_reset = td_esc_caps_reset
    },
    [TD_SPC_ENT] = {
        .kc_single = KC_SPC,
        .kc_double = KC_ENT,
        .on_dance_finished = td_spc_ent_finished,
        .on_reset = td_spc_ent_reset
    },
    [TD_Q_ESC] = {
        .kc_single = KC_Q,
        .kc_double = KC_ESC,
        .on_dance_finished = NULL,  // Use default behavior
        .on_reset = NULL
    }
};

// Process custom keycodes
bool process_custom_keycode(uint16_t keycode, bool pressed) {
    switch (keycode) {
        case CUSTOM_1:
            if (pressed) {
                // Custom macro example - types "Hello World"
                register_key(KC_H);
                unregister_key(KC_H);
                register_key(KC_E);
                unregister_key(KC_E);
                register_key(KC_L);
                unregister_key(KC_L);
                register_key(KC_L);
                unregister_key(KC_L);
                register_key(KC_O);
                unregister_key(KC_O);
            }
            return false;
            
        case CUSTOM_2:
            if (pressed) {
                // Example: Switch to specific layer
                layer_clear();
                layer_on(_RAISE);
            }
            return false;
            
        case CUSTOM_3:
            if (pressed) {
                // Example: Complex macro
                register_modifier(KC_LCTL);
                register_key(KC_C);
                unregister_key(KC_C);
                unregister_modifier(KC_LCTL);
            }
            return false;
            
        default:
            return true;  // Process normally
    }
}

// Main key processing function (called from dongle/main.c)
void process_key_event(uint8_t row, uint8_t col, bool pressed) {
    uint8_t layer = get_highest_layer();
    uint16_t keycode = get_keycode_at(layer, row, col);
    
    // Process custom keycodes first
    if ((keycode & 0xFF00) == 0x7000) {
        if (!process_custom_keycode(keycode, pressed)) {
            return;
        }
    }
    
    // Process mouse keycodes
    if ((keycode & 0xFF00) == 0x7100) {
        if (!process_mouse_keycode(keycode, pressed)) {
            return;
        }
    }
    
    // Process auto-clicker keycodes
    if ((keycode & 0xFF00) == 0x7200) {
        if (!process_autoclicker_keycode(keycode, pressed)) {
            return;
        }
    }
    
    // Process through feature pipeline
    if (!process_combo(keycode, pressed)) return;
    if (!process_tap_dance(keycode, pressed, row, col)) return;
    if (!process_modtap(keycode, pressed, row, col)) return;
    if (!process_layer_keycode(keycode, pressed)) return;
    if (!process_oneshot_layer(keycode, pressed)) return;
    
    // Handle special keycodes
    if (keycode == RESET && pressed) {
        clear_keyboard();
        return;
    }
    
    // Finally, send normal key
    if (pressed) {
        register_key(keycode);
    } else {
        unregister_key(keycode);
    }
    
    send_hid_report();
}

// Feature flags getter
uint32_t get_feature_flags(void) {
    return FEATURE_MODTAP | FEATURE_LAYERS | FEATURE_ONESHOT | 
           FEATURE_COMBOS | FEATURE_TAPDANCE;
}

// Initialize all features
void init_features(void) {
    layers_init();
    init_combos();
    // Other feature inits handled by their respective modules
}

// Run periodic feature tasks
void features_task(void) {
    modtap_task();
    tap_dance_task();
    oneshot_task();
}

// Optional: LED indicator update based on layer
void update_layer_leds(uint8_t layer_state) {
    // This would be called on the keyboard halves to update LED indicators
    // Example implementation:
    /*
    if (layer_state & (1 << _LOWER)) {
        gpio_put(LED_LOWER_PIN, 1);
    } else {
        gpio_put(LED_LOWER_PIN, 0);
    }
    
    if (layer_state & (1 << _RAISE)) {
        gpio_put(LED_RAISE_PIN, 1);
    } else {
        gpio_put(LED_RAISE_PIN, 0);
    }
    */
}

// Optional: Per-layer RGB colors (if using RGB LEDs)
uint32_t get_layer_color(uint8_t layer) {
    switch(layer) {
        case _BASE:
            return 0x001000;  // Green
        case _LOWER:
            return 0x100000;  // Red
        case _RAISE:
            return 0x000010;  // Blue
        case _ADJUST:
            return 0x100010;  // Purple
        default:
            return 0x000000;  // Off
    }
}

// Helper function for RESET keycode
#define RESET 0x5C00

// Additional helper macros for common key combinations
#define KC_COPY  LCTL(KC_C)
#define KC_PASTE LCTL(KC_V)
#define KC_CUT   LCTL(KC_X)
#define KC_UNDO  LCTL(KC_Z)
#define KC_REDO  LCTL(KC_Y)

// Simplified modifier key combinations
#define LCTL(kc) (KC_LCTL | ((kc) << 8))
#define LSFT(kc) (KC_LSFT | ((kc) << 8))
#define LALT(kc) (KC_LALT | ((kc) << 8))
#define LGUI(kc) (KC_LGUI | ((kc) << 8))
