/* 
	Kalaportti smart aquarium
*/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_attr.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"
#include "driver/ledc.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_client.h"

#include "ds18b20.h"
#include "feederServo.h"
#include "systemTiming.h"
#include "ConnectWifi.h"
#include "RGBcontroller.h"

SemaphoreHandle_t feederSem;
SemaphoreHandle_t submitSem;
SemaphoreHandle_t resetSem;

esp_mqtt_client_handle_t mqtt_client;
int mqtt_connected = 0;
static const char *TAG = "MQTT_EXAMPLE";

#define REED_LOWER_GPIO 32
#define REED_UPPER_GPIO 33

#define LED_RED_GPIO 18
#define LED_GREEN_GPIO 19
#define LED_BLUE_GPIO 21

#define PUMP_GPIO 22

#define FEEDER_SERVO_GPIO 23

#define TEMP_GPIO 14

#define BROKER_URL "mqtt://largist:aio_ZmVe53sldKLMRfyvYaqwJ5CecOv1@io.adafruit.com"
#define IO_TOPIC_TEMP "largist/feeds/temperature"
#define IO_TOPIC_COLOR "largist/feeds/Color"
#define IO_TOPIC_PUMP "largist/feeds/toggle"
#define IO_TOPIC_FEEDHOUR "largist/feeds/hour"
#define IO_TOPIC_FEEDMINUTE "largist/feeds/minute"
#define IO_TOPIC_FEEDSECOND "largist/feeds/second"
#define IO_TOPIC_BUTTON "largist/feeds/button"
#define IO_TOPIC_TEXTBOX "largist/feeds/textbox"
#define IO_TOPIC_INDICATOR "largist/feeds/waterlevel"
#define IO_TOPIC_RESET "largist/feeds/reset"

TaskHandle_t task1handle = NULL;
TaskHandle_t task2handle = NULL;
TaskHandle_t task3handle = NULL;
TaskHandle_t task4handle = NULL;

static char hexColor[8]="#ffffff";
static char pumpValue[4]="ON";
static char feedHour[3]="12";
static char feedMinute[3]="0";
static char feedSecond[3]="0";

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    mqtt_client = event->client;
    int msg_id;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

			//SET STARTING VALUES ON MQTT SERVER
			msg_id = esp_mqtt_client_publish(client, IO_TOPIC_PUMP, "ON", 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

			msg_id = esp_mqtt_client_publish(client, IO_TOPIC_COLOR, "#ffffff", 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

			//msg_id = esp_mqtt_client_publish(client, IO_TOPIC_TEXTBOX, "Water level is good", 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

			msg_id = esp_mqtt_client_publish(client, IO_TOPIC_INDICATOR, "0", 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

            mqtt_connected = 1;

			//SET SUBSCRIBTIONS

            msg_id = esp_mqtt_client_subscribe(client, IO_TOPIC_COLOR, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

			msg_id = esp_mqtt_client_subscribe(client, IO_TOPIC_PUMP, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, IO_TOPIC_FEEDHOUR, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

			msg_id = esp_mqtt_client_subscribe(client, IO_TOPIC_FEEDMINUTE, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

			msg_id = esp_mqtt_client_subscribe(client, IO_TOPIC_FEEDSECOND, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

			msg_id = esp_mqtt_client_subscribe(client, IO_TOPIC_BUTTON, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

			msg_id = esp_mqtt_client_subscribe(client, IO_TOPIC_RESET, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            mqtt_connected = 0;
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
			char dataBuf[25];
			char topicBuf[25];
			
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
			strncpy(dataBuf,event->data,event->data_len);
			strncpy(topicBuf,event->topic,event->topic_len);
			dataBuf[event->data_len] = '\0';
			topicBuf[event->topic_len] = '\0';
			
			if(strcmp(topicBuf,IO_TOPIC_COLOR)==0){	
				strcpy(hexColor,dataBuf);
			}
			else if(strcmp(topicBuf,IO_TOPIC_PUMP)==0){
				strcpy(pumpValue,dataBuf);
			}
			
			else if(strcmp(topicBuf,IO_TOPIC_FEEDHOUR)==0){
				strcpy(feedHour,dataBuf);
			}
			else if(strcmp(topicBuf,IO_TOPIC_FEEDMINUTE)==0){
				strcpy(feedMinute,dataBuf);
			}
			else if(strcmp(topicBuf,IO_TOPIC_FEEDSECOND)==0){
				strcpy(feedSecond,dataBuf);
			}
			else if(strcmp(topicBuf,IO_TOPIC_BUTTON)==0){
				if(strcmp(dataBuf,"1")==0){
					xSemaphoreGive(submitSem);
				}
			}
			else if(strcmp(topicBuf,IO_TOPIC_RESET)==0){
				if(strcmp(dataBuf,"1")==0){
					xSemaphoreGive(resetSem);
				}
			}
			else{
				printf("Error\n");
			}
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
	vTaskDelay(500 / portTICK_PERIOD_MS);
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = BROKER_URL,
        .event_handle = mqtt_event_handler,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

void task1(void *arg){
	RGB_init();
	while(1){
		//rainbowFade(1500,1000);
		setOverMQTT(hexColor,1000,0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void task2(void *arg){
	ds18b20_init(TEMP_GPIO);
	gpio_set_direction(TEMP_GPIO, GPIO_MODE_INPUT);
	float temp = 0;
	char buf[100];

	while(1){
		temp = ds18b20_get_temp();
		if(temp<0 || temp>84){
			temp = ds18b20_get_temp();
		} 
		else{
			sprintf(buf, "%.2f", temp); 	
			printf("tempperature: %f    %s\n",temp,buf);
			if(mqtt_connected){
        		esp_mqtt_client_publish(mqtt_client, IO_TOPIC_TEMP, buf, 0, 1, 0);
    		}
		}	
		vTaskDelay(10000 / portTICK_PERIOD_MS);
	}
}

void task3(void *arg)
{	
    servo_init(FEEDER_SERVO_GPIO,50);
    while (1) {
    	if(xSemaphoreTake(feederSem, portMAX_DELAY) == pdTRUE){
			rotate(710);
			vTaskDelay(3000 / portTICK_PERIOD_MS);
			rotate(1210);
		}  
    }
}

void task4(void *arg){
	
	gpio_pullup_en(REED_LOWER_GPIO);
	gpio_pullup_en(REED_UPPER_GPIO);
	
	gpio_pad_select_gpio(REED_LOWER_GPIO);
	gpio_pad_select_gpio(REED_UPPER_GPIO);
	
    gpio_set_direction(REED_LOWER_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(REED_UPPER_GPIO, GPIO_MODE_INPUT);
	bool levelNotGood=false;
	while(1){
		if(gpio_get_level(REED_UPPER_GPIO) == 0 && !levelNotGood){
			esp_mqtt_client_publish(mqtt_client, IO_TOPIC_INDICATOR, "1", 0, 1, 0);
			esp_mqtt_client_publish(mqtt_client, IO_TOPIC_TEXTBOX, "Water level is too high", 0, 1, 0);
			levelNotGood=true;
		}
		if(gpio_get_level(REED_LOWER_GPIO) == 0 && !levelNotGood){
			esp_mqtt_client_publish(mqtt_client, IO_TOPIC_INDICATOR, "1", 0, 1, 0);
			esp_mqtt_client_publish(mqtt_client, IO_TOPIC_TEXTBOX, "Water level is too low", 0, 1, 0);
			levelNotGood=true;
		}
		if(xSemaphoreTake(resetSem, 0) == pdTRUE){
			esp_mqtt_client_publish(mqtt_client, IO_TOPIC_INDICATOR, "0", 0, 1, 0);
			esp_mqtt_client_publish(mqtt_client, IO_TOPIC_TEXTBOX, "Water level is good", 0, 1, 0);
			levelNotGood=false;
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void task5(void *arg)
{
    time_t now;
    struct tm timeinfo;
	int updatedFeedHH=12,updatedFeedMM=0,updatedFeedSS=0;	
    while (1) {		
		if(xSemaphoreTake(submitSem, 0) == pdTRUE){
			sscanf(feedHour, "%d", &updatedFeedHH);
			sscanf(feedMinute, "%d", &updatedFeedMM);
			sscanf(feedSecond, "%d", &updatedFeedSS);
		}
		timeinfo = get_time(&now, "CST-2");    	
    	printf("Time in Finland: %d:%d:%d\n",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);	
    	if(timeinfo.tm_hour==updatedFeedHH && timeinfo.tm_min==updatedFeedMM && timeinfo.tm_sec== updatedFeedSS){
    		xSemaphoreGive(feederSem);
		}
    	vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void task6(void *arg)
{ 
    gpio_pad_select_gpio(PUMP_GPIO);
	    
    gpio_set_direction(PUMP_GPIO, GPIO_MODE_OUTPUT);
    while (1) {
		if(strcmp(pumpValue,"ON")==0){
			gpio_set_level(PUMP_GPIO, 1);
		}
		else if(strcmp(pumpValue,"OFF")==0){
			gpio_set_level(PUMP_GPIO, 0);
		}
	   vTaskDelay(200 / portTICK_PERIOD_MS); 
    }
}

void app_main(void)
{
	//Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
	
    wifi_init_sta();
	init_system_time("pool.ntp.org");
	mqtt_app_start();
	
	feederSem = xSemaphoreCreateBinary();
	submitSem = xSemaphoreCreateBinary();
	resetSem = xSemaphoreCreateBinary();
   	
    xTaskCreate(task1, "ledTask", 4096, NULL, 1, &task1handle);
    xTaskCreate(task2, "temp task", 4096, NULL, 1, &task2handle);
    xTaskCreate(task3, "servo task", 4096, NULL, 1, &task3handle);
    xTaskCreate(task4, "Reed task", 4096, NULL, 1, &task4handle); 
    xTaskCreate(task5, "time task", 4096, NULL, 1, NULL);
	xTaskCreate(task6, "pump task", 4096, NULL, 1, NULL);
}
