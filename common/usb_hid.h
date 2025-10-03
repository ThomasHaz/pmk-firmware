// /common/usb_hid.h

#ifndef USB_HID_H
#define USB_HID_H

#include <stdint.h>
#include <stdbool.h>
#include "keycodes.h"

// Initialize USB HID
void usb_hid_init(void);

// Key registration functions
void register_key(uint16_t keycode);
void unregister_key(uint16_t keycode);
void register_modifier(uint8_t mod);
void unregister_modifier(uint8_t mod);

// These were missing - add them
void add_key_to_report(uint16_t keycode);    // Alias for register_key
void remove_key_from_report(uint16_t keycode); // Alias for unregister_key

// HID report management
void send_hid_report(void);
void clear_keyboard(void);

// Utility functions
uint8_t qmk_to_hid(uint16_t keycode);
uint8_t get_modifier_state(void);
bool is_key_pressed(uint16_t keycode);
uint8_t get_key_count(void);

// Function prototype needed by other files
void process_key_event(uint8_t row, uint8_t col, bool pressed);

#endif // USB_HID_H