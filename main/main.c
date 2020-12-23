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

#include "ds18b20.h"
#include "feederServo.h"
#include "systemTiming.h"
#include "ConnectWifi.h"
#include "RGBcontroller.h"

#include <stdlib.h>

#include "mqtt_client.h"

esp_mqtt_client_handle_t mqtt_client;
int mqtt_connected = 0;
static const char *TAG = "MQTT_EXAMPLE";

SemaphoreHandle_t feederSem;
SemaphoreHandle_t submitSem;
SemaphoreHandle_t resetSem;

#define REED_LOWER_GPIO 26
#define REED_UPPER_GPIO 27

#define LED_RED_GPIO 5
#define LED_GREEN_GPIO 18
#define LED_BLUE_GPIO 19
#define PUMP_GPIO 21

#define FEEDER_SERVO_GPIO 23

#define TEMP_GPIO 25

/*
authentication for adafruit IO MQTT broker
change [username] to your username and [key] to your active key
*/
#define BROKER_URL "mqtt://[username]:[key]@io.adafruit.com"

/*
connection to adafruit IO feeds
uses form [username]/feeds/[feedname]
*/
#define IO_TOPIC_TEMP "[username]/feeds/temperature"
#define IO_TOPIC_COLOR "[username]/feeds/Color"
#define IO_TOPIC_PUMP "[username]/feeds/toggle"
#define IO_TOPIC_FEEDHOUR "[username]/feeds/hour"
#define IO_TOPIC_FEEDMINUTE "[username]/feeds/minute"
#define IO_TOPIC_FEEDSECOND "[username]/feeds/second"
#define IO_TOPIC_BUTTON "[username]/feeds/button"
#define IO_TOPIC_TEXTBOX "[username]/feeds/textbox"
#define IO_TOPIC_INDICATOR "[username]/feeds/waterlevel"
#define IO_TOPIC_RESET "[username]/feeds/reset"

TaskHandle_t task1handle = NULL;
TaskHandle_t task2handle = NULL;
TaskHandle_t task3handle = NULL;
TaskHandle_t task4handle = NULL;

//Static variables for MQTT data
static char hexColor[8]="#ffffff";
static char pumpValue[4]="ON";
static char feedHour[3]="12";
static char feedMinute[3]="0";
static char feedSecond[3]="0";

//Event handler for MQTT event
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    mqtt_client = event->client;
    int msg_id;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

			//PUBLISH STARTING VALUES TO MQTT SERVER
			msg_id = esp_mqtt_client_publish(client, IO_TOPIC_PUMP, pumpValue, 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

			msg_id = esp_mqtt_client_publish(client, IO_TOPIC_COLOR, hexColor, 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

			msg_id = esp_mqtt_client_publish(client, IO_TOPIC_TEXTBOX, "Water level is good", 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

			msg_id = esp_mqtt_client_publish(client, IO_TOPIC_INDICATOR, "0", 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

			msg_id = esp_mqtt_client_publish(client, IO_TOPIC_FEEDHOUR, feedHour, 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

			msg_id = esp_mqtt_client_publish(client, IO_TOPIC_FEEDMINUTE, feedMinute, 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

			msg_id = esp_mqtt_client_publish(client, IO_TOPIC_FEEDSECOND, feedSecond, 0, 1, 0);
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
			
			//Convert data received from MQTT to useable form and store data to proper variables or send semaphore flags
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

//Function for starting MQTT connection
static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = BROKER_URL,
        .event_handle = mqtt_event_handler,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

/*
freeRTOS tasks
*/

//RGB LED
void task1(void *arg){
	RGB_init();
	while(1){
		setOverMQTT(hexColor,1000,0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

//Temperature sensor
void task2(void *arg){
	ds18b20_init(TEMP_GPIO);
	gpio_set_direction(TEMP_GPIO, GPIO_MODE_INPUT);
	gpio_pullup_en(TEMP_GPIO);
	gpio_pad_select_gpio(TEMP_GPIO);
	
	float temp = 0;
	char buf[100];

	while(1){
		temp = ds18b20_get_temp();
		
		if(temp<0 || temp>84){
			printf("ERROR tempperature: %f\n",temp);
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

//Feeder
void task3(void *arg)
{	
    servo_init(FEEDER_SERVO_GPIO,50);
	int i;
    while (1) {
		
    	if(xSemaphoreTake(feederSem, portMAX_DELAY) == pdTRUE){
			printf("spin\n");
			rotate(1000);
			vTaskDelay(1500 / portTICK_PERIOD_MS);
			rotate(1440);
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

//Water level
void task4(void *arg){
	
	gpio_pullup_en(REED_LOWER_GPIO);
	gpio_pullup_en(REED_UPPER_GPIO);
	
	gpio_pad_select_gpio(REED_LOWER_GPIO);
	gpio_pad_select_gpio(REED_UPPER_GPIO);
	
    gpio_set_direction(REED_LOWER_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(REED_UPPER_GPIO, GPIO_MODE_INPUT);
	bool levelNotGood=false;
	while(1){

		printf("switch1 = %d, switch2 = %d\n\n", gpio_get_level(REED_LOWER_GPIO), gpio_get_level(REED_UPPER_GPIO)); 

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
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}

//System time
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
			printf("Feed time set %d  %d  %d\n",updatedFeedHH,updatedFeedMM,updatedFeedSS);
		}
		timeinfo = get_time(&now, "CST-2");    	
    	printf("Time in Finland: %d:%d:%d\n",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);	
    	if(timeinfo.tm_hour==updatedFeedHH && timeinfo.tm_min==updatedFeedMM && timeinfo.tm_sec== updatedFeedSS){
			printf("test\n");
    		xSemaphoreGive(feederSem);
		}
    	vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

//Water pump
void task6(void *arg)
{ 
    gpio_pad_select_gpio(PUMP_GPIO);
	    
    gpio_set_direction(PUMP_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(PUMP_GPIO, 0);
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

	//Set log topics
	esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
	
	//Initialize internet based services
    wifi_init_sta();
	init_system_time("pool.ntp.org");
	mqtt_app_start();
	
	feederSem = xSemaphoreCreateBinary();
	submitSem = xSemaphoreCreateBinary();
	resetSem = xSemaphoreCreateBinary();
   	
    xTaskCreate(task1, "ledTask", 4096, NULL, 1, NULL);
    xTaskCreate(task2, "temp task", 4096, NULL, 1, NULL);
    xTaskCreate(task3, "servo task", 4096, NULL, 1, NULL);
    xTaskCreate(task4, "Reed task", 4096, NULL, 1, NULL); 
    xTaskCreate(task5, "time task", 4096, NULL, 1, NULL);
	xTaskCreate(task6, "pump task", 4096, NULL, 1, NULL);
}
