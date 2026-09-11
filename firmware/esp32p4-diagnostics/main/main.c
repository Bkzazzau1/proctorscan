#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_efuse.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdmmc_cmd.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#define PROTOCOL_VERSION 1
#define FIRMWARE_VERSION "0.7.0"
#define BOARD_NAME "waveshare-esp32-p4-wifi6-dev-kit"
#define HEARTBEAT_PERIOD_MS 2000
#define IDENTITY_PERIOD_HEARTBEATS 15
#define SDMMC_CLK_GPIO 43
#define SDMMC_CMD_GPIO 44
#define SDMMC_D0_GPIO 39
#define SDMMC_D1_GPIO 40
#define SDMMC_D2_GPIO 41
#define SDMMC_D3_GPIO 42
#define SDMMC_LDO_CHANNEL 4
#define TAMPER_SWITCH_GPIO GPIO_NUM_5
#define RADAR_PRESENCE_GPIO GPIO_NUM_4

static const char *storage_state = "INIT_ERROR";
static uint64_t storage_capacity_bytes = 0;
static const char *log_state = "NOT_STARTED";
static char log_path[80] = "";
static FILE *log_file = NULL;

static void initialize_tamper_switch(void)
{
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << TAMPER_SWITCH_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&config);
}

static int read_tamper_switch_level(void)
{
    return gpio_get_level(TAMPER_SWITCH_GPIO);
}

static void emit_tamper_switch(int level)
{
    printf(
        "{\"protocol\":%d,\"type\":\"tamper_switch\","
        "\"gpio\":%d,\"physical_pin\":13,\"level\":%d,"
        "\"contact\":\"%s\"}\n",
        PROTOCOL_VERSION,
        TAMPER_SWITCH_GPIO,
        level,
        level == 0 ? "CLOSED" : "OPEN"
    );
    fflush(stdout);
}

static void initialize_radar_presence(void)
{
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << RADAR_PRESENCE_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&config);
}

static int read_radar_presence_level(void)
{
    return gpio_get_level(RADAR_PRESENCE_GPIO);
}

static void emit_radar_presence(int level)
{
    printf(
        "{\"protocol\":%d,\"type\":\"radar_presence\","
        "\"component\":\"hlk-ld2420-v2.1\",\"signal\":\"OT2\","
        "\"gpio\":%d,\"physical_pin\":16,\"level\":%d,"
        "\"presence\":%s}\n",
        PROTOCOL_VERSION, RADAR_PRESENCE_GPIO, level,
        level == 1 ? "true" : "false"
    );
    fflush(stdout);
}

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
        "\"component\":\"microsd\",\"mode\":\"new_file_only\","
        "\"state\":\"%s\",\"capacity_bytes\":%" PRIu64 "}\n",
        PROTOCOL_VERSION,
        storage_state,
        storage_capacity_bytes
    );
    fflush(stdout);
}

static void emit_log_status(void)
{
    printf(
        "{\"protocol\":%d,\"type\":\"log_status\","
        "\"component\":\"microsd\",\"state\":\"%s\",\"path\":\"%s\"}\n",
        PROTOCOL_VERSION,
        log_state,
        log_path
    );
    fflush(stdout);
}

static bool create_unique_log(const uint8_t mac[6])
{
    if (mkdir("/sdcard/proctorscan", 0755) != 0 && errno != EEXIST) {
        log_state = "CREATE_ERROR";
        return false;
    }

    for (unsigned int suffix = 0; suffix < 1000; suffix++) {
        snprintf(
            log_path,
            sizeof(log_path),
            "/sdcard/proctorscan/bringup-%02x%02x%02x-%03u.log",
            mac[3], mac[4], mac[5], suffix
        );
        int descriptor = open(log_path, O_WRONLY | O_CREAT | O_EXCL, 0644);
        if (descriptor >= 0) {
            log_file = fdopen(descriptor, "w");
            if (log_file == NULL) {
                close(descriptor);
                log_state = "CREATE_ERROR";
                return false;
            }
            fprintf(
                log_file,
                "PROCTORSCAN_DIAGNOSTIC_LOG_V1\n"
                "device=proctorscan-%02x%02x%02x%02x%02x%02x\n"
                "board=%s\nfirmware=%s\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                BOARD_NAME,
                FIRMWARE_VERSION
            );
            fflush(log_file);
            fsync(fileno(log_file));
            log_state = "WRITING";
            return true;
        }
        if (errno != EEXIST) {
            log_state = "CREATE_ERROR";
            return false;
        }
    }

    log_state = "NO_FREE_FILENAME";
    return false;
}

static void append_log_heartbeat(uint32_t sequence)
{
    if (log_file == NULL || sequence > 3) {
        return;
    }
    int64_t uptime_ms = esp_timer_get_time() / 1000;
    fprintf(log_file, "heartbeat=%" PRIu32 ",uptime_ms=%" PRId64 "\n", sequence, uptime_ms);
    fflush(log_file);
    fsync(fileno(log_file));

    if (sequence == 3) {
        fclose(log_file);
        log_file = NULL;

        FILE *verification_file = fopen(log_path, "r");
        char contents[512] = {0};
        if (verification_file == NULL) {
            log_state = "VERIFY_ERROR";
        } else {
            size_t bytes_read = fread(contents, 1, sizeof(contents) - 1, verification_file);
            fclose(verification_file);
            contents[bytes_read] = '\0';
            log_state = strstr(contents, "heartbeat=3") != NULL ? "VERIFIED" : "VERIFY_ERROR";
        }
        emit_log_status();
    }
}

static void initialize_microsd_logging(const uint8_t mac[6])
{
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;

    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = SDMMC_LDO_CHANNEL,
    };
    sd_pwr_ctrl_handle_t power_handle = NULL;
    esp_err_t result = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &power_handle);
    if (result != ESP_OK) {
        storage_state = "INIT_ERROR";
        emit_storage_status();
        return;
    }
    host.pwr_ctrl_handle = power_handle;

    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 4;
    slot.clk = SDMMC_CLK_GPIO;
    slot.cmd = SDMMC_CMD_GPIO;
    slot.d0 = SDMMC_D0_GPIO;
    slot.d1 = SDMMC_D1_GPIO;
    slot.d2 = SDMMC_D2_GPIO;
    slot.d3 = SDMMC_D3_GPIO;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 3,
        .allocation_unit_size = 16 * 1024,
    };
    sdmmc_card_t *card = NULL;
    result = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot, &mount_config, &card);
    if (result != ESP_OK) {
        storage_state = "MOUNT_ERROR";
        emit_storage_status();
        return;
    }

    storage_capacity_bytes =
        (uint64_t)card->csd.capacity * (uint64_t)card->csd.sector_size;
    storage_state = "DETECTED";
    emit_storage_status();
    create_unique_log(mac);
    emit_log_status();
}

void app_main(void)
{
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);

    emit_identity(mac);
    initialize_tamper_switch();
    int switch_level = read_tamper_switch_level();
    emit_tamper_switch(switch_level);
    initialize_radar_presence();
    int radar_level = read_radar_presence_level();
    emit_radar_presence(radar_level);
    initialize_microsd_logging(mac);

    uint32_t sequence = 0;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_PERIOD_MS));
        sequence++;
        emit_heartbeat(sequence);
        int current_switch_level = read_tamper_switch_level();
        if (current_switch_level != switch_level) {
            vTaskDelay(pdMS_TO_TICKS(25));
            current_switch_level = read_tamper_switch_level();
            if (current_switch_level != switch_level) {
                switch_level = current_switch_level;
                emit_tamper_switch(switch_level);
            }
        }
        int current_radar_level = read_radar_presence_level();
        if (current_radar_level != radar_level) {
            vTaskDelay(pdMS_TO_TICKS(25));
            current_radar_level = read_radar_presence_level();
            if (current_radar_level != radar_level) {
                radar_level = current_radar_level;
                emit_radar_presence(radar_level);
            }
        }
        append_log_heartbeat(sequence);
        if ((sequence % IDENTITY_PERIOD_HEARTBEATS) == 0) {
            emit_identity(mac);
            emit_storage_status();
            emit_log_status();
            emit_tamper_switch(switch_level);
            emit_radar_presence(radar_level);
        }
    }
}
