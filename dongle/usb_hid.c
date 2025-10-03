#include "usb_hid.h"
#include "tusb.h"
#include "keycodes.h"
#include <string.h>

// HID Report structure
static struct {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keycodes[6];
} hid_report = {0};

static bool report_dirty = false;

// Convert QMK keycode to HID keycode
uint8_t qmk_to_hid(uint16_t keycode) {
    if (keycode >= KC_A && keycode <= KC_0) {
        return (uint8_t)keycode;
    }
    
    switch (keycode) {
        case KC_ENT:  return 0x28;
        case KC_ESC:  return 0x29;
        case KC_BSPC: return 0x2A;
        case KC_TAB:  return 0x2B;
        case KC_SPC:  return 0x2C;
        case KC_MINS: return 0x2D;
        case KC_EQL:  return 0x2E;
        case KC_LBRC: return 0x2F;
        case KC_RBRC: return 0x30;
        case KC_BSLS: return 0x31;
        case KC_SCLN: return 0x33;
        case KC_QUOT: return 0x34;
        case KC_GRV:  return 0x35;
        case KC_COMM: return 0x36;
        case KC_DOT:  return 0x37;
        case KC_SLSH: return 0x38;
        case KC_CAPS: return 0x39;
        
        case KC_F1:   return 0x3A;
        case KC_F2:   return 0x3B;
        case KC_F3:   return 0x3C;
        case KC_F4:   return 0x3D;
        case KC_F5:   return 0x3E;
        case KC_F6:   return 0x3F;
        case KC_F7:   return 0x40;
        case KC_F8:   return 0x41;
        case KC_F9:   return 0x42;
        case KC_F10:  return 0x43;
        case KC_F11:  return 0x44;
        case KC_F12:  return 0x45;
        
        case KC_INS:  return 0x49;
        case KC_HOME: return 0x4A;
        case KC_PGUP: return 0x4B;
        case KC_DEL:  return 0x4C;
        case KC_END:  return 0x4D;
        case KC_PGDN: return 0x4E;
        case KC_RIGHT:return 0x4F;
        case KC_LEFT: return 0x50;
        case KC_DOWN: return 0x51;
        case KC_UP:   return 0x52;
        
        default:
            if (keycode < 0xFF) {
                return (uint8_t)keycode;
            }
            return 0;
    }
}

void register_key(uint16_t keycode) {
    if (keycode >= KC_LCTL && keycode <= KC_RGUI) {
        register_modifier(keycode);
        return;
    }
    
    uint8_t hid_keycode = qmk_to_hid(keycode);
    if (hid_keycode == 0) return;
    
    for (int i = 0; i < 6; i++) {
        if (hid_report.keycodes[i] == hid_keycode) {
            return;
        }
    }
    
    for (int i = 0; i < 6; i++) {
        if (hid_report.keycodes[i] == 0) {
            hid_report.keycodes[i] = hid_keycode;
            report_dirty = true;
            break;
        }
    }
}

void unregister_key(uint16_t keycode) {
    if (keycode >= KC_LCTL && keycode <= KC_RGUI) {
        unregister_modifier(keycode);
        return;
    }
    
    uint8_t hid_keycode = qmk_to_hid(keycode);
    if (hid_keycode == 0) return;
    
    for (int i = 0; i < 6; i++) {
        if (hid_report.keycodes[i] == hid_keycode) {
            for (int j = i; j < 5; j++) {
                hid_report.keycodes[j] = hid_report.keycodes[j + 1];
            }
            hid_report.keycodes[5] = 0;
            report_dirty = true;
            break;
        }
    }
}

void register_modifier(uint8_t mod) {
    uint8_t mod_bit = 0;
    
    switch (mod) {
        case KC_LCTL: mod_bit = 0x01; break;
        case KC_LSFT: mod_bit = 0x02; break;
        case KC_LALT: mod_bit = 0x04; break;
        case KC_LGUI: mod_bit = 0x08; break;
        case KC_RCTL: mod_bit = 0x10; break;
        case KC_RSFT: mod_bit = 0x20; break;
        case KC_RALT: mod_bit = 0x40; break;
        case KC_RGUI: mod_bit = 0x80; break;
        default: return;
    }
    
    hid_report.modifiers |= mod_bit;
    report_dirty = true;
}

void unregister_modifier(uint8_t mod) {
    uint8_t mod_bit = 0;
    
    switch (mod) {
        case KC_LCTL: mod_bit = 0x01; break;
        case KC_LSFT: mod_bit = 0x02; break;
        case KC_LALT: mod_bit = 0x04; break;
        case KC_LGUI: mod_bit = 0x08; break;
        case KC_RCTL: mod_bit = 0x10; break;
        case KC_RSFT: mod_bit = 0x20; break;
        case KC_RALT: mod_bit = 0x40; break;
        case KC_RGUI: mod_bit = 0x80; break;
        default: return;
    }
    
    hid_report.modifiers &= ~mod_bit;
    report_dirty = true;
}

void send_hid_report(void) {
    if (!report_dirty) return;
    if (!tud_hid_ready()) return;
    if (!tud_mounted()) return;
    
    tud_hid_keyboard_report(1, hid_report.modifiers, hid_report.keycodes);
    report_dirty = false;
}

void clear_keyboard(void) {
    hid_report.modifiers = 0;
    memset(hid_report.keycodes, 0, 6);
    report_dirty = true;
}

uint8_t get_modifier_state(void) {
    return hid_report.modifiers;
}

bool is_key_pressed(uint16_t keycode) {
    uint8_t hid_keycode = qmk_to_hid(keycode);
    
    for (int i = 0; i < 6; i++) {
        if (hid_report.keycodes[i] == hid_keycode) {
            return true;
        }
    }
    return false;
}

uint8_t get_key_count(void) {
    uint8_t count = 0;
    for (int i = 0; i < 6; i++) {
        if (hid_report.keycodes[i] != 0) {
            count++;
        }
    }
    return count;
}

void add_key_to_report(uint16_t keycode) {
    register_key(keycode);
}

void remove_key_from_report(uint16_t keycode) {
    unregister_key(keycode);
}

// TinyUSB callbacks
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len) {
    (void) instance;
    (void) report;
    (void) len;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, 
                                hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) instance;
    (void) report_type;
    
    if (report_id == 1 && reqlen >= sizeof(hid_report)) {
        memcpy(buffer, &hid_report, sizeof(hid_report));
        return sizeof(hid_report);
    }
    
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, 
                           hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}