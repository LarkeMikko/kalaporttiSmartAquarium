#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_sntp.h"

#include "systemTiming.h"

const char *TAG = "System time";

void init_system_time(char server[30]){
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, server);
	sntp_init();
	obtain_time();
}

void obtain_time(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());

    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
}

struct tm get_time(time_t now, char timezone[6]){
	struct tm timeinfo;
	time(&now);
	setenv("TZ", timezone, 1);
    tzset();
    localtime_r(&now, &timeinfo);
    return timeinfo;
}

