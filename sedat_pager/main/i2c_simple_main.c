#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "i2c-lcd.h"
#include "client_socket.h"

static const char *TAG = "sedat_pager_main";

char buffer[10];
float num = 12.34;

static esp_err_t i2c_master_init(void) // initialize the i2c master 
{
    int i2c_master_port = I2C_NUM_0;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER, // set the I2C mode to master
        .sda_io_num = GPIO_NUM_21, // SDA (serial data) pin is GPIO21
        .scl_io_num = GPIO_NUM_22, // SCL (serial clock) pin is GPIO 22 
        .sda_pullup_en = GPIO_PULLUP_ENABLE, // GPIO 21 pullup enabled
        .scl_pullup_en = GPIO_PULLUP_ENABLE, // GPIO 22 pullup enabled
        .master.clk_speed = 100000, // set the serial clock speed to 100 kiloherz
    };

    i2c_param_config(i2c_master_port, &conf); // shove them configs into the I2C

    return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0); // install I2C drivers
}


void app_main(void) {
	esp_err_t status = WIFI_FAILURE;
	// init storage
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	
	status = connect_wifi((uint8_t*)"idealab");
	if (WIFI_SUCCESS != status)
	{
		ESP_LOGI(TAG, "Failed to associate to AP, dying...");
		return;
	}
	
	status = connect_tcp_server();
	if (TCP_SUCCESS != status)
	{
		ESP_LOGI(TAG, "Failed to connect to remote server, dying...");
		return;
	}
	/*ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");
    lcd_init();
    lcd_clear();

	lcd_put_cur(0, 0);
    lcd_send_string("Hello world!");

    lcd_put_cur(1, 0);
    lcd_send_string("from ESP32");
	*/


}
