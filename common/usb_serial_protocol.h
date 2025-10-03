#ifndef USB_SERIAL_PROTOCOL_H
#define USB_SERIAL_PROTOCOL_H

#include <stdint.h>

// Simple protocol for USB serial communication
// Format: [HEADER][DEVICE_ID][ROW][COL][PRESSED][CHECKSUM]

#define USB_SERIAL_HEADER 0xAA
#define USB_SERIAL_PACKET_SIZE 6

typedef struct __attribute__((packed)) {
    uint8_t header;      // Always 0xAA
    uint8_t device_id;   // DEVICE_LEFT or DEVICE_RIGHT
    uint8_t row;
    uint8_t col;
    uint8_t pressed;     // 1 = pressed, 0 = released
    uint8_t checksum;    // Simple sum of previous bytes
} usb_serial_packet_t;

static inline uint8_t usb_serial_calc_checksum(const usb_serial_packet_t *pkt) {
    return pkt->header + pkt->device_id + pkt->row + pkt->col + pkt->pressed;
}

static inline bool usb_serial_validate(const usb_serial_packet_t *pkt) {
    return (pkt->header == USB_SERIAL_HEADER) && 
           (pkt->checksum == usb_serial_calc_checksum(pkt));
}

#endif // USB_SERIAL_PROTOCOL_H