#include <inttypes.h>
#include <stdio.h>

#include "esp_efuse.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#if CONFIG_PROCTORSCAN_ADXL345_PROBE
#include "driver/i2c_master.h"
#endif

#define PROTOCOL_VERSION 1
#define FIRMWARE_VERSION "0.2.0"
#define BOARD_NAME "waveshare-esp32-p4-wifi6-dev-kit"
#define HEARTBEAT_PERIOD_MS 2000
#define IDENTITY_PERIOD_HEARTBEATS 15

#if CONFIG_PROCTORSCAN_ADXL345_PROBE
#define ADXL345_I2C_ADDRESS 0x53
#define ADXL345_DEVID_REGISTER 0x00
#define ADXL345_EXPECTED_DEVID 0xE5
#define ADXL345_SDA_GPIO 7
#define ADXL345_SCL_GPIO 8
#define I2C_TIMEOUT_MS 250

static i2c_master_bus_handle_t adxl_bus;
static i2c_master_dev_handle_t adxl_device;
#endif

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

#if CONFIG_PROCTORSCAN_ADXL345_SIMULATOR
static void emit_simulated_acceleration(uint32_t sequence)
{
    const double x_g = 0.01 * (double)sequence;
    const double y_g = -0.005 * (double)sequence;
    printf(
        "{\"protocol\":%d,\"type\":\"accelerometer\","
        "\"source\":\"simulation\",\"x_g\":%.3f,\"y_g\":%.3f,\"z_g\":1.000}\n",
        PROTOCOL_VERSION,
        x_g,
        y_g
    );
    fflush(stdout);
}
#endif

#if CONFIG_PROCTORSCAN_ADXL345_PROBE
static void emit_adxl_status(const char *state, int device_id)
{
    printf(
        "{\"protocol\":%d,\"type\":\"peripheral_status\","
        "\"component\":\"adxl345\",\"source\":\"hardware\","
        "\"state\":\"%s\",\"address\":83,\"device_id\":%d}\n",
        PROTOCOL_VERSION,
        state,
        device_id
    );
    fflush(stdout);
}

static bool init_adxl_probe(void)
{
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = ADXL345_SDA_GPIO,
        .scl_io_num = ADXL345_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };
    if (i2c_new_master_bus(&bus_config, &adxl_bus) != ESP_OK) {
        emit_adxl_status("BUS_ERROR", -1);
        return false;
    }

    const i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ADXL345_I2C_ADDRESS,
        .scl_speed_hz = 100000,
    };
    if (i2c_master_bus_add_device(adxl_bus, &device_config, &adxl_device) != ESP_OK) {
        emit_adxl_status("BUS_ERROR", -1);
        return false;
    }
    return true;
}

static void probe_adxl(void)
{
    uint8_t register_address = ADXL345_DEVID_REGISTER;
    uint8_t device_id = 0;
    esp_err_t result = i2c_master_transmit_receive(
        adxl_device,
        &register_address,
        sizeof(register_address),
        &device_id,
        sizeof(device_id),
        I2C_TIMEOUT_MS
    );
    if (result != ESP_OK) {
        emit_adxl_status("NOT_DETECTED", -1);
    } else if (device_id != ADXL345_EXPECTED_DEVID) {
        emit_adxl_status("ID_MISMATCH", device_id);
    } else {
        emit_adxl_status("DETECTED", device_id);
    }
}
#endif

void app_main(void)
{
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);

    emit_identity(mac);

#if CONFIG_PROCTORSCAN_ADXL345_PROBE
    bool adxl_probe_ready = init_adxl_probe();
    if (adxl_probe_ready) {
        probe_adxl();
    }
#endif

    uint32_t sequence = 0;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_PERIOD_MS));
        sequence++;
        emit_heartbeat(sequence);
#if CONFIG_PROCTORSCAN_ADXL345_SIMULATOR
        emit_simulated_acceleration(sequence);
#endif
#if CONFIG_PROCTORSCAN_ADXL345_PROBE
        if (adxl_probe_ready && (sequence % IDENTITY_PERIOD_HEARTBEATS) == 0) {
            probe_adxl();
        }
#endif
        if ((sequence % IDENTITY_PERIOD_HEARTBEATS) == 0) {
            emit_identity(mac);
        }
    }
}
