#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/netif.h"

#include "config.h"
#include "protocol.h"
#include "timer.h"
#include "matrix.h"

#define DEVICE_ID DEVICE_LEFT
#define BUFFER_SIZE 16

static struct udp_pcb *udp_pcb = NULL;
static keyboard_packet_t tx_packet;
static uint8_t previous_matrix[MATRIX_ROWS] = {0};

// Transmission buffer for reliable delivery
typedef struct {
    matrix_state_t data;
    uint32_t timestamp;
    uint8_t retry_count;
    bool active;
} tx_buffer_entry_t;

static tx_buffer_entry_t tx_buffer[BUFFER_SIZE];
static uint8_t tx_buffer_head = 0;
static uint8_t tx_buffer_tail = 0;
static uint16_t packet_sequence = 0;

void blink(int times, int ms) {
    for (int i = 0; i < times; i++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(ms);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(ms);
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

void send_matrix_update(matrix_state_t *matrix) {
    if (udp_pcb == NULL) return;
    
    tx_packet.type = PACKET_MATRIX_UPDATE;
    tx_packet.device_id = DEVICE_ID;
    tx_packet.sequence = packet_sequence++;
    tx_packet.timestamp = timer_read();
    memcpy(tx_packet.data, matrix, sizeof(matrix_state_t));
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

void add_to_buffer(matrix_state_t *matrix, bool is_release) {
    // Check buffer usage first
    int usage = get_buffer_usage();
    
    // If buffer is getting full (>75%), clear old entries aggressively
    if (usage > 12) {
        uint32_t now = timer_read();
        for (int i = 0; i < BUFFER_SIZE; i++) {
            if (tx_buffer[i].active && (now - tx_buffer[i].timestamp > 10)) {
                tx_buffer[i].active = false;
            }
        }
    }
    
    // Find next available slot
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (!tx_buffer[i].active) {
            tx_buffer[i].data = *matrix;
            tx_buffer[i].timestamp = timer_read();
            tx_buffer[i].retry_count = 0;
            tx_buffer[i].active = true;
            return;
        }
    }
    
    // Buffer completely full - force clear oldest and use that slot
    uint32_t oldest_time = 0xFFFFFFFF;
    int oldest_idx = 0;
    uint32_t now = timer_read();
    
    for (int i = 0; i < BUFFER_SIZE; i++) {
        uint32_t age = now - tx_buffer[i].timestamp;
        if (age > oldest_time) {
            oldest_time = age;
            oldest_idx = i;
        }
    }
    
    tx_buffer[oldest_idx].data = *matrix;
    tx_buffer[oldest_idx].timestamp = now;
    tx_buffer[oldest_idx].retry_count = 0;
    tx_buffer[oldest_idx].active = true;
}

void process_tx_buffer() {
    uint32_t now = timer_read();
    
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (tx_buffer[i].active) {
            uint32_t age = now - tx_buffer[i].timestamp;
            
            // Send immediately on first add
            if (tx_buffer[i].retry_count == 0) {
                send_matrix_update(&tx_buffer[i].data);
                tx_buffer[i].retry_count++;
                tx_buffer[i].timestamp = now;
            }
            // Retry after 3ms (even faster)
            else if (age > 3 && tx_buffer[i].retry_count < 3) {
                send_matrix_update(&tx_buffer[i].data);
                tx_buffer[i].retry_count++;
                tx_buffer[i].timestamp = now;
            }
            // Clear from buffer after 3 retries or 20ms (much shorter)
            else if (tx_buffer[i].retry_count >= 3 || age > 20) {
                tx_buffer[i].active = false;
            }
        }
    }
}

int get_buffer_usage() {
    int count = 0;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (tx_buffer[i].active) count++;
    }
    return count;
}

int main() {
    // Stage 1: Init WiFi chip
    cyw43_arch_init();
    blink(1, 200);  // 1 blink
    
    // Stage 2: Enable station mode  
    cyw43_arch_enable_sta_mode();
    cyw43_wifi_pm(&cyw43_state, CYW43_NO_POWERSAVE_MODE);
    sleep_ms(2000);
    blink(2, 200);  // 2 blinks
    
    // Stage 3: Connect to AP (async with manual polling)
    cyw43_arch_wifi_connect_async(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK);
    
    // Poll for connection
    for (int i = 0; i < 900; i++) {  // 90 seconds max
        cyw43_arch_poll();
        int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
        
        if (status == CYW43_LINK_UP) {
            break;  // Connected!
        }
        sleep_ms(100);
    }
    
    blink(3, 200);  // 3 blinks = connected
    sleep_ms(1000);
    
    // Stage 4: Set static IP
    ip4_addr_t ip, mask, gw;
    ipaddr_aton(LEFT_IP, &ip);
    ipaddr_aton(SUBNET_MASK, &mask);
    ipaddr_aton(DONGLE_IP, &gw);
    
    struct netif *netif = &cyw43_state.netif[CYW43_ITF_STA];
    dhcp_stop(netif);
    netif_set_addr(netif, &ip, &mask, &gw);
    sleep_ms(2000);
    
    blink(4, 200);  // 4 blinks = IP set
    sleep_ms(1000);
    
    // Stage 5: Create UDP socket
    udp_pcb = udp_new();
    udp_bind(udp_pcb, IP_ADDR_ANY, KB_PORT);
    
    blink(5, 200);  // 5 blinks = UDP ready
    sleep_ms(1000);
    
    // SUCCESS! 10 fast blinks
    for (int i = 0; i < 10; i++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(50);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(50);
    }
    sleep_ms(2000);
    
    // Initialize matrix scanning
    matrix_init();
    
    // Initialize transmission buffer
    for (int i = 0; i < BUFFER_SIZE; i++) {
        tx_buffer[i].active = false;
    }
    
    // Main loop - scan matrix and send changes
    uint32_t last_heartbeat = 0;
    uint32_t poll_counter = 0;
    uint32_t last_buffer_check = 0;
    
    while (1) {
        uint32_t now = timer_read();
        
        // Monitor buffer usage every second (for debugging via LED)
        if (now - last_buffer_check > 1000) {
            int usage = get_buffer_usage();
            // If buffer is over 50% full, do a longer blink as warning
            if (usage > 8) {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                sleep_ms(50);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            }
            last_buffer_check = now;
        }
        
        // Scan the matrix FIRST - highest priority
        matrix_scan();
        
        // Check for changes
        bool has_changes = false;
        matrix_state_t current_matrix = {0};
        
        for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
            current_matrix.rows[row] = matrix_get_row(row);
            
            if (current_matrix.rows[row] != previous_matrix[row]) {
                current_matrix.changed_mask[row] = 
                    current_matrix.rows[row] ^ previous_matrix[row];
                has_changes = true;
            } else {
                current_matrix.changed_mask[row] = 0;
            }
        }
        
        // Send IMMEDIATELY if anything changed
        if (has_changes) {
            send_matrix_update(&current_matrix);
            
            // Update previous state
            for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
                previous_matrix[row] = current_matrix.rows[row];
            }
            
            // Brief LED flash
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            sleep_us(500);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        }
        
        // Poll WiFi regularly
        poll_counter++;
        if (poll_counter >= 5) {
            cyw43_arch_poll();
            poll_counter = 0;
        }
        
        // Heartbeat every 500ms
        if (now - last_heartbeat > 500) {
            tx_packet.type = PACKET_HEARTBEAT;
            tx_packet.device_id = DEVICE_ID;
            tx_packet.timestamp = now;
            tx_packet.checksum = calculate_checksum(&tx_packet);
            
            struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, sizeof(tx_packet), PBUF_RAM);
            if (p != NULL) {
                memcpy(p->payload, &tx_packet, sizeof(tx_packet));
                ip_addr_t dongle_addr;
                ipaddr_aton(DONGLE_IP, &dongle_addr);
                udp_sendto(udp_pcb, p, &dongle_addr, KB_PORT);
                pbuf_free(p);
            }
            last_heartbeat = now;
        }
        
        // Small delay to prevent overwhelming the system
        sleep_us(500);  // 500 microseconds = ~2000 Hz scan rate
    }
    
    return 0;
}