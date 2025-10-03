#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

// Simple packet protocol for UART communication
// Format: [START][DEVICE_ID][ROW][COL][PRESSED][CHECKSUM][END]

#define UART_START_BYTE 0xAA
#define UART_END_BYTE   0x55
#define UART_PACKET_SIZE 7
#define UART_BAUD_RATE  115200

typedef struct __attribute__((packed)) {
    uint8_t start;       // 0xAA
    uint8_t device_id;   // DEVICE_LEFT or DEVICE_RIGHT
    uint8_t row;
    uint8_t col;
    uint8_t pressed;     // 1 = pressed, 0 = released
    uint8_t checksum;    // XOR of all previous bytes
    uint8_t end;         // 0x55
} uart_packet_t;

static inline uint8_t uart_calc_checksum(const uart_packet_t *pkt) {
    return pkt->start ^ pkt->device_id ^ pkt->row ^ pkt->col ^ pkt->pressed;
}

static inline bool uart_validate(const uart_packet_t *pkt) {
    return (pkt->start == UART_START_BYTE) && 
           (pkt->end == UART_END_BYTE) &&
           (pkt->checksum == uart_calc_checksum(pkt));
}

#endif // UART_PROTOCOL_H