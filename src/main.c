#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"
#include "pico/unique_id.h"
#include "secrets.h"

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#define DEVICE_ID "pico_w_01"
#define TOPIC_TELEMETRY "devices/" DEVICE_ID "/telemetry"

typedef struct {
    mqtt_client_t *client;
    ip_addr_t broker_ip;
    bool connected;
} mqtt_state_t;

static void mqtt_request_cb(void *arg, err_t err) {
    if (err != ERR_OK) {
        printf("[MQTT] Request error code: %d\n", err);
    }
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    mqtt_state_t *state = (mqtt_state_t *)arg;
    if (status == MQTT_CONNECT_ACCEPTED) {
        printf("[MQTT] Connected to Broker successfully!\n");
        state->connected = true;
    } else {
        printf("[MQTT] Connection disconnected or rejected. Status: %d\n", status);
        state->connected = false;
    }
}

static void connect_broker(mqtt_state_t *state) {
    struct mqtt_connect_client_info_t ci = {0};
    ci.client_id = DEVICE_ID;
    ci.keep_alive = 60;

    cyw43_arch_lwip_begin();
    mqtt_client_connect(state->client, &state->broker_ip, MQTT_PORT, mqtt_connection_cb, state, &ci);
    cyw43_arch_lwip_end();
}

int main() {
    stdio_init_all();
    sleep_ms(2000);

    if (cyw43_arch_init()) {
        printf("[ERROR] Failed to initialize Wi-Fi chip\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();
    printf("[WIFI] Connecting to SSID: %s...\n", WIFI_SSID);
    
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("[ERROR] Wi-Fi connection timeout\n");
        return -1;
    }
    
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    printf("[WIFI] Connected successfully! LED turned ON.\n");

    mqtt_state_t state = {0};
    ip4addr_aton(MQTT_BROKER_IP, &state.broker_ip);
    state.client = mqtt_client_new();

    connect_broker(&state);

    uint32_t uptime = 0;
    while (true) {
        if (state.connected) {
            char hardware_id[37];
            pico_get_unique_board_id_string(hardware_id, sizeof(hardware_id));

            char payload[256];
            snprintf(payload, sizeof(payload), 
                     "{\"device_id\":\"%s\",\"hardware_id\":\"%s\",\"uptime\":%lu}", 
                     DEVICE_ID, hardware_id, uptime);
            
            cyw43_arch_lwip_begin();
            mqtt_publish(state.client, TOPIC_TELEMETRY, payload, strlen(payload), 0, 0, mqtt_request_cb, &state);
            cyw43_arch_lwip_end();
            
            printf("[MQTT] Published telemetry: %s\n", payload);
        } else {
            printf("[MQTT] Reconnecting to Broker...\n");
            connect_broker(&state);
        }
        
        sleep_ms(5000);
        uptime += 5;
    }

    cyw43_arch_deinit();
    return 0;
}