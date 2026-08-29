#include <inttypes.h>
#include <stdio.h>

#include "esp_efuse.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PROTOCOL_VERSION 1
#define FIRMWARE_VERSION "0.1.0"
#define BOARD_NAME "waveshare-esp32-p4-wifi6-dev-kit"
#define HEARTBEAT_PERIOD_MS 2000
#define IDENTITY_PERIOD_HEARTBEATS 15

static void emit_identity(const uint8_t mac[6])
{
    printf(
        "{\"protocol\":%d,\"type\":\"identity\","
        "\"device_id\":\"proctorscan-%02x%02x%02x%02x%02x%02x\","
        "\"board\":\"%s\",\"firmware\":\"%s\"}\n",
        PROTOCOL_VERSION,
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
        BOARD_NAME,
        FIRMWARE_VERSION
    );
    fflush(stdout);
}

static void emit_heartbeat(uint32_t sequence)
{
    const int64_t uptime_ms = esp_timer_get_time() / 1000;
    printf(
        "{\"protocol\":%d,\"type\":\"heartbeat\","
        "\"sequence\":%" PRIu32 ",\"uptime_ms\":%" PRId64 "}\n",
        PROTOCOL_VERSION,
        sequence,
        uptime_ms
    );
    fflush(stdout);
}

void app_main(void)
{
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);

    emit_identity(mac);

    uint32_t sequence = 0;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_PERIOD_MS));
        sequence++;
        emit_heartbeat(sequence);
        if ((sequence % IDENTITY_PERIOD_HEARTBEATS) == 0) {
            emit_identity(mac);
        }
    }
}

