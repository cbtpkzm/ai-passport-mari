#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "pet_app.h"

static const char *TAG = "main";

static void on_key(bsp_btn_t btn, bsp_btn_ev_t event, void *user)
{
    (void)user;
    if (!bsp_lvgl_lock(250)) return;
    pet_app_key(btn, event);
    bsp_lvgl_unlock();
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting SANABI pet");
    ESP_ERROR_CHECK(bsp_i2c_init());
    esp_err_t nvs_result = nvs_flash_init();
    if (nvs_result != ESP_OK) {
        ESP_LOGW(TAG, "Pet progress persistence unavailable: %s",
                 esp_err_to_name(nvs_result));
    }

    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "Display init failed (MOSI=%d SCLK=%d CS=%d DC=%d BL=%d)",
                 BSP_LCD_MOSI, BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL);
        return;
    }
    bsp_display_backlight(100);

    esp_err_t battery_result = bsp_battery_init();
    if (battery_result != ESP_OK) {
        ESP_LOGW(TAG, "Battery gauge unavailable: %s", esp_err_to_name(battery_result));
    }

    esp_err_t button_result = bsp_button_init(on_key, NULL);
    if (button_result != ESP_OK) {
        ESP_LOGE(TAG, "Button init failed: %s", esp_err_to_name(button_result));
    }

    if (bsp_lvgl_lock(1000)) {
        pet_app_enter();
        bsp_lvgl_unlock();
    }
}
