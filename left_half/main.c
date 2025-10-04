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
#define BUFFER_SIZE 32
#define RETRANSMIT_DELAY_MS 5
#define MAX_RETRIES 3
#define RETRANSMIT_TIMEOUT_MS 50

static struct udp_pcb *udp_pcb = NULL;
static keyboard_packet_t tx_packet;
static keyboard_packet_t rx_packet;
static uint8_t previous_matrix[MATRIX_ROWS] = {0};
static uint16_t packet_sequence = 0;

// Transmission buffer for reliable delivery
typedef struct {
    matrix_state_t data;
    uint32_t timestamp;
    uint16_t sequence;
    uint8_t retry_count;
    bool active;
    bool acked;
} tx_buffer_entry_t;

static tx_buffer_entry_t tx_buffer[BUFFER_SIZE];
static bool dongle_connected = false;
static uint32_t last_ack_time = 0;

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

bool validate_packet_checksum(const keyboard_packet_t *packet) {
    return calculate_checksum(packet) == packet->checksum;
}

void send_packet(keyboard_packet_t *packet) {
    if (udp_pcb == NULL) return;
    
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, sizeof(keyboard_packet_t), PBUF_RAM);
    if (p != NULL) {
        memcpy(p->payload, packet, sizeof(keyboard_packet_t));
        ip_addr_t dongle_addr;
        ipaddr_aton(DONGLE_IP, &dongle_addr);
        udp_sendto(udp_pcb, p, &dongle_addr, KB_PORT);
        pbuf_free(p);
    }
}

void send_matrix_update(matrix_state_t *matrix, uint16_t seq) {
    tx_packet.type = PACKET_MATRIX_UPDATE;
    tx_packet.device_id = DEVICE_ID;
    tx_packet.sequence = seq;
    tx_packet.timestamp = timer_read();
    memcpy(tx_packet.data, matrix, sizeof(matrix_state_t));
    tx_packet.checksum = calculate_checksum(&tx_packet);
    
    send_packet(&tx_packet);
}

void add_to_buffer(matrix_state_t *matrix) {
    // Find a free slot
    int slot = -1;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (!tx_buffer[i].active) {
            slot = i;
            break;
        }
    }
    
    // If no free slot, clear oldest completed entry
    if (slot == -1) {
        uint32_t oldest_time = 0xFFFFFFFF;
        for (int i = 0; i < BUFFER_SIZE; i++) {
            if (tx_buffer[i].acked && tx_buffer[i].timestamp < oldest_time) {
                oldest_time = tx_buffer[i].timestamp;
                slot = i;
            }
        }
    }
    
    // If still no slot (all unacked), force clear oldest
    if (slot == -1) {
        uint32_t oldest_time = 0xFFFFFFFF;
        for (int i = 0; i < BUFFER_SIZE; i++) {
            if (tx_buffer[i].timestamp < oldest_time) {
                oldest_time = tx_buffer[i].timestamp;
                slot = i;
            }
        }
    }
    
    // Add to buffer
    if (slot >= 0) {
        tx_buffer[slot].data = *matrix;
        tx_buffer[slot].timestamp = timer_read();
        tx_buffer[slot].sequence = packet_sequence++;
        tx_buffer[slot].retry_count = 0;
        tx_buffer[slot].active = true;
        tx_buffer[slot].acked = false;
        
        // Send immediately
        send_matrix_update(&tx_buffer[slot].data, tx_buffer[slot].sequence);
    }
}

void process_tx_buffer() {
    uint32_t now = timer_read();
    
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (!tx_buffer[i].active || tx_buffer[i].acked) continue;
        
        uint32_t age = now - tx_buffer[i].timestamp;
        
        // Retransmit if needed
        if (tx_buffer[i].retry_count > 0 && age > RETRANSMIT_DELAY_MS) {
            if (tx_buffer[i].retry_count < MAX_RETRIES) {
                send_matrix_update(&tx_buffer[i].data, tx_buffer[i].sequence);
                tx_buffer[i].retry_count++;
                tx_buffer[i].timestamp = now;
            }
        }
        
        // Clear from buffer after max retries or timeout
        if (tx_buffer[i].retry_count >= MAX_RETRIES || age > RETRANSMIT_TIMEOUT_MS) {
            tx_buffer[i].active = false;
        }
    }
}

void handle_ack(uint16_t sequence) {
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (tx_buffer[i].active && tx_buffer[i].sequence == sequence) {
            tx_buffer[i].acked = true;
            tx_buffer[i].active = false;
            break;
        }
    }
    last_ack_time = timer_read();
    dongle_connected = true;
}

int get_buffer_usage() {
    int count = 0;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (tx_buffer[i].active && !tx_buffer[i].acked) count++;
    }
    return count;
}

static void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                             const ip_addr_t *addr, u16_t port) {
    (void)arg; (void)pcb; (void)addr; (void)port;
    
    if (p != NULL) {
        if (p->tot_len >= sizeof(keyboard_packet_t)) {
            pbuf_copy_partial(p, &rx_packet, sizeof(keyboard_packet_t), 0);
            
            if (validate_packet_checksum(&rx_packet)) {
                if (rx_packet.type == PACKET_SYNC_RESPONSE) {
                    // This is an ACK from the dongle
                    handle_ack(rx_packet.sequence);
                }
            }
        }
        pbuf_free(p);
    }
}

int main() {
    stdio_init_all();
    sleep_ms(1000);
    
    printf("\n=== Wireless Keyboard Left Half ===\n");
    
    // Stage 1: Init WiFi chip
    printf("1. WiFi chip init...\n");
    if (cyw43_arch_init()) {
        printf("   FAILED\n");
        while(1) {
            sleep_ms(1000);
        }
    }
    printf("   OK\n");
    blink(1, 200);
    
    // Stage 2: Enable station mode  
    printf("2. Station mode...\n");
    cyw43_arch_enable_sta_mode();
    cyw43_wifi_pm(&cyw43_state, CYW43_NO_POWERSAVE_MODE);
    sleep_ms(1000);
    printf("   OK\n");
    blink(2, 200);
    
    // Stage 3: Connect to AP
    printf("3. Connecting to AP: %s\n", WIFI_SSID);
    cyw43_arch_wifi_connect_async(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK);
    
    // Poll for connection
    int connect_attempts = 0;
    while (connect_attempts < 300) {  // 30 seconds max
        cyw43_arch_poll();
        int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
        
        if (status == CYW43_LINK_UP) {
            printf("   CONNECTED\n");
            break;
        } else if (status == CYW43_LINK_FAIL) {
            printf("   AUTH FAILED\n");
            break;
        }
        
        sleep_ms(100);
        connect_attempts++;
        
        if (connect_attempts % 10 == 0) {
            printf("   Still trying... (%d)\n", connect_attempts/10);
        }
    }
    
    blink(3, 200);
    sleep_ms(500);
    
    // Stage 4: Set static IP
    printf("4. Setting static IP: %s\n", LEFT_IP);
    ip4_addr_t ip, mask, gw;
    ipaddr_aton(LEFT_IP, &ip);
    ipaddr_aton(SUBNET_MASK, &mask);
    ipaddr_aton(DONGLE_IP, &gw);
    
    struct netif *netif = &cyw43_state.netif[CYW43_ITF_STA];
    dhcp_stop(netif);
    netif_set_addr(netif, &ip, &mask, &gw);
    sleep_ms(1000);
    printf("   OK\n");
    
    blink(4, 200);
    sleep_ms(500);
    
    // Stage 5: Create UDP socket
    printf("5. Creating UDP socket...\n");
    udp_pcb = udp_new();
    if (udp_pcb == NULL) {
        printf("   FAILED to create UDP PCB\n");
        while(1) {
            blink(1, 100);
            sleep_ms(900);
        }
    }
    
    if (udp_bind(udp_pcb, IP_ADDR_ANY, KB_PORT) != ERR_OK) {
        printf("   FAILED to bind UDP port\n");
        while(1) {
            blink(2, 100);
            sleep_ms(800);
        }
    }
    
    udp_recv(udp_pcb, udp_recv_callback, NULL);
    printf("   OK - Listening on port %d\n", KB_PORT);
    
    blink(5, 200);
    sleep_ms(500);
    
    // SUCCESS! 10 fast blinks
    printf("\n=== Setup complete! Starting matrix scan... ===\n");
    for (int i = 0; i < 10; i++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(50);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(50);
    }
    
    // Initialize matrix scanning
    matrix_init();
    
    // Initialize transmission buffer
    memset(tx_buffer, 0, sizeof(tx_buffer));
    memset(previous_matrix, 0, sizeof(previous_matrix));
    
    // Main loop variables
    uint32_t last_heartbeat = 0;
    uint32_t last_buffer_check = 0;
    uint32_t last_connection_check = 0;
    uint32_t loop_counter = 0;
    
    printf("Starting main loop...\n");
    
    // Main loop - scan matrix and send changes
    while (1) {
        uint32_t loop_start = timer_read();
        
        // Always poll WiFi first
        cyw43_arch_poll();
        
        // Scan the matrix - highest priority
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
        
        // Send immediately if anything changed
        if (has_changes) {
            // Add to buffer for reliable transmission
            add_to_buffer(&current_matrix);
            
            // Update previous state
            memcpy(previous_matrix, current_matrix.rows, sizeof(previous_matrix));
            
            // Brief LED flash for feedback
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            sleep_us(500);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        }
        
        // Process retransmissions
        process_tx_buffer();
        
        // Check connection status (every 500ms)
        uint32_t now = timer_read();
        if (now - last_connection_check > 500) {
            // Check if we're still connected to WiFi
            int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
            if (status != CYW43_LINK_UP) {
                // Lost WiFi connection - try to reconnect
                printf("WiFi connection lost, attempting reconnect...\n");
                cyw43_arch_wifi_connect_async(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK);
            }
            
            // Check if dongle is responding
            if (dongle_connected && (now - last_ack_time > 2000)) {
                dongle_connected = false;
                printf("Dongle connection timeout\n");
            }
            
            last_connection_check = now;
        }
        
        // Send heartbeat every 500ms
        if (now - last_heartbeat > 500) {
            tx_packet.type = PACKET_HEARTBEAT;
            tx_packet.device_id = DEVICE_ID;
            tx_packet.sequence = packet_sequence++;
            tx_packet.timestamp = now;
            memset(tx_packet.data, 0, sizeof(tx_packet.data));
            tx_packet.checksum = calculate_checksum(&tx_packet);
            
            send_packet(&tx_packet);
            last_heartbeat = now;
        }
        
        // Monitor buffer usage every second (for debugging)
        if (now - last_buffer_check > 1000) {
            int usage = get_buffer_usage();
            
            // Visual indicator of buffer usage
            if (usage > 20) {
                // Buffer very full - rapid blinks
                for (int i = 0; i < 3; i++) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    sleep_ms(50);
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                    sleep_ms(50);
                }
            } else if (usage > 10) {
                // Buffer getting full - double blink
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                sleep_ms(100);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                sleep_ms(100);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                sleep_ms(100);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            } else if (!dongle_connected) {
                // No connection - slow blink
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                sleep_ms(200);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            }
            
            last_buffer_check = now;
        }
        
        // Debug output every 10000 loops (approximately every 10 seconds)
        loop_counter++;
        if (loop_counter % 10000 == 0) {
            printf("Loop %lu: Buffer usage: %d, Connected: %s, Last ACK: %lums ago\n", 
                   loop_counter, 
                   get_buffer_usage(),
                   dongle_connected ? "Yes" : "No",
                   now - last_ack_time);
        }
        
        // Ensure consistent loop timing
        uint32_t loop_time = timer_read() - loop_start;
        if (loop_time < 1) {
            // Loop took less than 1ms, sleep for the remainder
            sleep_us((1 - loop_time) * 1000);
        }
        // If loop took more than 1ms, continue immediately (we're behind)
    }
    
    return 0;
}