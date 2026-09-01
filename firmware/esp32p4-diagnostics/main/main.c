#include <inttypes.h>
#include <stdio.h>

#include "esp_efuse.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "driver/sdmmc_host.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdmmc_cmd.h"
#define PROTOCOL_VERSION 1
#define FIRMWARE_VERSION "0.4.0"
#define BOARD_NAME "waveshare-esp32-p4-wifi6-dev-kit"
#define HEARTBEAT_PERIOD_MS 2000
#define IDENTITY_PERIOD_HEARTBEATS 15
#define SDMMC_CLK_GPIO 43
#define SDMMC_CMD_GPIO 44
#define SDMMC_D0_GPIO 39
#define SDMMC_D1_GPIO 40
#define SDMMC_D2_GPIO 41
#define SDMMC_D3_GPIO 42

static const char *storage_state = "INIT_ERROR";
static uint64_t storage_capacity_bytes = 0;

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

static void emit_storage_status(void)
{
    printf(
        "{\"protocol\":%d,\"type\":\"storage_status\","
        "\"component\":\"microsd\",\"mode\":\"identification_only\","
        "\"state\":\"%s\",\"capacity_bytes\":%" PRIu64 "}\n",
        PROTOCOL_VERSION,
        storage_state,
        storage_capacity_bytes
    );
    fflush(stdout);
}

static void identify_microsd(void)
{
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;

    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 4;
    slot.clk = SDMMC_CLK_GPIO;
    slot.cmd = SDMMC_CMD_GPIO;
    slot.d0 = SDMMC_D0_GPIO;
    slot.d1 = SDMMC_D1_GPIO;
    slot.d2 = SDMMC_D2_GPIO;
    slot.d3 = SDMMC_D3_GPIO;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    esp_err_t result = sdmmc_host_init();
    if (result != ESP_OK) {
        storage_state = "INIT_ERROR";
        emit_storage_status();
        return;
    }

    result = sdmmc_host_init_slot(host.slot, &slot);
    if (result != ESP_OK) {
        storage_state = "INIT_ERROR";
        emit_storage_status();
        return;
    }

    static sdmmc_card_t card;
    result = sdmmc_card_init(&host, &card);
    if (result != ESP_OK) {
        storage_state = "NOT_DETECTED";
        emit_storage_status();
        return;
    }

    storage_capacity_bytes =
        (uint64_t)card.csd.capacity * (uint64_t)card.csd.sector_size;
    storage_state = "DETECTED";
    emit_storage_status();
}

void app_main(void)
{
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);

    emit_identity(mac);
    identify_microsd();

    uint32_t sequence = 0;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_PERIOD_MS));
        sequence++;
        emit_heartbeat(sequence);
        if ((sequence % IDENTITY_PERIOD_HEARTBEATS) == 0) {
            emit_identity(mac);
            emit_storage_status();
        }
    }
}
