#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tusb.h"

/* Local includes */
#include "vendor_packet_protocol.h"

static const char *TAG = "esp32s3_tinyusb";

static uint8_t rx_buf[CONFIG_TINYUSB_VENDOR_RX_BUFSIZE];
static uint8_t tx_buf[VENDOR_PACKET_MAX_FRAME_SIZE];
static vendor_packet_parser_t packet_parser;

typedef struct {
    uint8_t itf;
} vendor_link_context_t;

static void handle_packet(const vendor_packet_t *packet, void *user_data)
{
    vendor_link_context_t *context = user_data;

    ESP_LOGI(TAG, "Packet type=0x%02x payload_len=%u", packet->type, (unsigned)packet->payload_len);

    size_t frame_len = vendor_packet_build(packet->type, packet->payload, packet->payload_len, tx_buf, sizeof(tx_buf));
    if (frame_len == 0) {
        ESP_LOGE(TAG, "Failed to encode response frame");
        return;
    }

    tud_vendor_n_write(context->itf, tx_buf, frame_len);
    tud_vendor_n_write_flush(context->itf);
}

static void handle_vendor_rx(uint8_t itf)
{
    vendor_link_context_t context = {
        .itf = itf,
    };

    while (tud_vendor_n_available(itf) > 0) {
        uint32_t chunk = tud_vendor_n_available(itf);
        if (chunk > CONFIG_TINYUSB_VENDOR_RX_BUFSIZE) {
            chunk = CONFIG_TINYUSB_VENDOR_RX_BUFSIZE;
        }

        uint32_t read_len = tud_vendor_n_read(itf, rx_buf, chunk);
        if (read_len == 0) {
            break;
        }

        vendor_packet_parser_push(&packet_parser, rx_buf, read_len, handle_packet, &context);
    }
}

#if (TUSB_VERSION_MINOR >= 17)
void tud_vendor_rx_cb(uint8_t itf, uint8_t const *buffer, uint16_t bufsize)
{
    (void)buffer;
    (void)bufsize;
    handle_vendor_rx(itf);
}
#else
void tud_vendor_rx_cb(uint8_t itf)
{
    handle_vendor_rx(itf);
}
#endif

void app_main(void)
{
    ESP_LOGI(TAG, "Installing TinyUSB driver");
    const tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    vendor_packet_parser_init(&packet_parser);

    ESP_LOGI(TAG, "USB vendor interface ready. Send framed packets over raw bulk transport.");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
