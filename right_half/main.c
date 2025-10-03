#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

#include "config.h"
#include "protocol.h"
#include "matrix.h"
#include "timer.h"

#define DEVICE_ID DEVICE_RIGHT

static struct udp_pcb *udp_pcb = NULL;
static keyboard_packet_t tx_packet;
static matrix_state_t current_matrix = {0};
static matrix_state_t last_sent_matrix = {0};
static bool connected = false;

// Initialize WiFi and connect to dongle
static void init_wifi_client(void) {
    if (cyw43_arch_init()) {
        panic("WiFi init failed");
    }
    
    cyw43_arch_enable_sta_mode();
    
    printf("Connecting to %s...\n", WIFI_SSID);
    
    int result = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000
    );
    
    if (result != 0) {
        printf("WiFi connection failed\n");
        return;
    }
    
    printf("Connected!\n");
    connected = true;
    
    // Setup UDP
    udp_pcb = udp_new();
    if (udp_pcb == NULL) {
        panic("Failed to create UDP PCB");
    }
    
    // Bind to our port
    err_t err = udp_bind(udp_pcb, IP_ADDR_ANY, KB_PORT);
    if (err != ERR_OK) {
        panic("Failed to bind UDP");
    }
}

// Send matrix update to dongle
static void send_matrix_update(void) {
    if (!connected || udp_pcb == NULL) return;
    
    // Check for changes
    bool has_changes = false;
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        current_matrix.rows[row] = matrix_get_row(row);
        
        if (current_matrix.rows[row] != last_sent_matrix.rows[row]) {
            current_matrix.changed_mask[row] = 
                current_matrix.rows[row] ^ last_sent_matrix.rows[row];
            has_changes = true;
        } else {
            current_matrix.changed_mask[row] = 0;
        }
    }
    
    if (!has_changes) return;
    
    // Prepare packet
    tx_packet.type = PACKET_MATRIX_UPDATE;
    tx_packet.device_id = DEVICE_ID;
    tx_packet.timestamp = timer_read();
    memcpy(tx_packet.data, &current_matrix, sizeof(matrix_state_t));
    tx_packet.checksum = calculate_checksum(&tx_packet);
    
    // Send packet
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, sizeof(tx_packet), PBUF_RAM);
    if (p != NULL) {
        memcpy(p->payload, &tx_packet, sizeof(tx_packet));
        
        ip_addr_t dongle_addr;
        ipaddr_aton(DONGLE_IP, &dongle_addr);
        udp_sendto(udp_pcb, p, &dongle_addr, KB_PORT);
        
        pbuf_free(p);
    }
    
    // Update last sent state
    memcpy(&last_sent_matrix, &current_matrix, sizeof(matrix_state_t));
}

// Send heartbeat to dongle
static void send_heartbeat(void) {
    if (!connected || udp_pcb == NULL) return;
    
    tx_packet.type = PACKET_HEARTBEAT;
    tx_packet.device_id = DEVICE_ID;
    tx_packet.timestamp = timer_read();
    tx_packet.checksum = calculate_checksum(&tx_packet);
    
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, sizeof(tx_packet), PBUF_RAM);
    if (p != NULL) {
        memcpy(p->payload, &tx_packet, sizeof(tx_packet));
        
        ip_addr_t dongle_addr;
        ipaddr_aton(DONGLE_IP, &dongle_addr);
        udp_sendto(udp_pcb, p, &dongle_addr, KB_PORT);
        
        pbuf_free(p);
    }
}

// UDP receive callback for sync packets
static void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                             const ip_addr_t *addr, u16_t port) {
    if (p != NULL) {
        if (p->len >= sizeof(keyboard_packet_t)) {
            keyboard_packet_t *packet = (keyboard_packet_t *)p->payload;
            
            if (validate_packet_checksum(packet)) {
                if (packet->type == PACKET_FEATURE_STATE) {
                    feature_sync_t *sync = (feature_sync_t *)packet->data;
                    // TODO: Update LED indicators based on layer state
                }
            }
        }
        pbuf_free(p);
    }
}

uint16_t calculate_checksum(const keyboard_packet_t *packet) {
    uint16_t sum = 0;
    const uint8_t *data = (const uint8_t *)packet;
    
    for (size_t i = 0; i < offsetof(keyboard_packet_t, checksum); i++) {
        sum += data[i];
    }
    
    return sum;
}

bool validate_packet_checksum(const keyboard_packet_t *packet) {
    return calculate_checksum(packet) == packet->checksum;
}

int main() {
    stdio_init_all();
    matrix_init();
    init_wifi_client();
    
    if (udp_pcb != NULL) {
        udp_recv(udp_pcb, udp_recv_callback, NULL);
    }
    
    uint32_t last_heartbeat = 0;
    
    while (1) {
        // Scan physical matrix
        matrix_scan();
        
        // Network tasks
        cyw43_arch_poll();
        
        // Send matrix updates
        send_matrix_update();
        
        // Send periodic heartbeat
        uint32_t now = timer_read();
        if (now - last_heartbeat > 500) {
            send_heartbeat();
            last_heartbeat = now;
        }
        
        sleep_ms(1);
    }
    
    return 0;
}