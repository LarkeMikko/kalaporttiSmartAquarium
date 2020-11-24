/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
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

#include "esp_sntp.h"

void time_sync_notification_cb(struct timeval *tv)
{
    //ESP_LOGI(TAG, "Notification of a time synchronization event");
}

SemaphoreHandle_t feederSem;

/* Can use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define ACTIVITY_LED_GPIO 26
#define SW1 34

#define REED_LOWER_GPIO 25
#define REED_UPPER_GPIO 33

#define LED_RED_GPIO 18
#define LED_GREEN_GPIO 19
#define LED_BLUE_GPIO 21
#define PUMP_GPIO 22

#define FEEDER_SERVO_GPIO 23

#define TEMP_GPIO 14

TaskHandle_t task1handle = NULL;
TaskHandle_t task2handle = NULL;
TaskHandle_t task3handle = NULL;
TaskHandle_t task4handle = NULL;


void task1(void *arg){
    gpio_pad_select_gpio(ACTIVITY_LED_GPIO);   
    //gpio_pad_select_gpio(PUMP_GPIO);
	    
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(ACTIVITY_LED_GPIO, GPIO_MODE_OUTPUT);
    //gpio_set_direction(PUMP_GPIO, GPIO_MODE_OUTPUT);

	RGB_init();
	while(1){

		
		printf("red\n");
		set_color(255,0,0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		
		printf("green\n");
		set_color(0,255,0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		
		printf("blue\n");
		set_color(0,0,255);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		
		printf("hot pink\n");
		set_color(255,105,180);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		
		printf("orange\n");
		set_color(255,165,0);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
		
		/*
		printf("fade red\n");
		fade_color(255,0,0,500,1200);
		//vTaskDelay(3000 / portTICK_PERIOD_MS);
		
		printf("fade green\n");
		fade_color(0,255,0,500,1200);
		//vTaskDelay(3000 / portTICK_PERIOD_MS);
		
		printf("fade blue\n");
		fade_color(0,0,255,500,1200);
		//vTaskDelay(3000 / portTICK_PERIOD_MS);
		
		printf("fade hot pink\n");
		fade_color(255,105,180,500,1200);
		//vTaskDelay(3000 / portTICK_PERIOD_MS);
		
		printf("fade orange\n");
		fade_color(255,165,0,500,3000);
		//vTaskDelay(3000 / portTICK_PERIOD_MS);
		*/
		/*
		gpio_set_level(PUMP_GPIO, 0);
		vTaskDelay(2000 / portTICK_PERIOD_MS); 
		gpio_set_level(PUMP_GPIO, 1);
		vTaskDelay(2000 / portTICK_PERIOD_MS); 
		*/
		
		//gpio_set_level(ACTIVITY_LED_GPIO, 0);
        
		
        //gpio_set_level(ACTIVITY_LED_GPIO, 1);
        
          
	}
}

void task2(void *arg){
	ds18b20_init(TEMP_GPIO);
	gpio_set_direction(TEMP_GPIO, GPIO_MODE_INPUT);
	float temp = 0;
	while(1){
		temp = ds18b20_get_temp(); 
		printf("tempperature: %f\n",temp);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}


void task3(void *arg)
{	
    servo_init(FEEDER_SERVO_GPIO,50);
    while (1) {
    	//if(xSemaphoreTake(feederSem, portMAX_DELAY) == pdTRUE){

		//}    		   		
    		for(int i=1000; i<2000; i++){
    			rotate(i);
    			printf("%d\n",i);
    			vTaskDelay(2 / portTICK_PERIOD_MS); 
			}
			vTaskDelay(200 / portTICK_PERIOD_MS); 
			for(int i=2000; i>1000; i--){
    			rotate(i);
    			printf("%d\n",i);
    			vTaskDelay(2 / portTICK_PERIOD_MS); 
			} 
    }
}

void task4(void *arg){
	
	gpio_pullup_en(REED_LOWER_GPIO);
	gpio_pullup_en(REED_UPPER_GPIO);
	
	gpio_pad_select_gpio(REED_LOWER_GPIO);
	gpio_pad_select_gpio(REED_UPPER_GPIO);
	
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(REED_LOWER_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(REED_UPPER_GPIO, GPIO_MODE_INPUT);
	while(1){

        if(gpio_get_level(REED_UPPER_GPIO) == 1){
        	//printf("Magnet away upper limitswitch\n");
		}
		else if(gpio_get_level(REED_UPPER_GPIO) == 0){
			printf("Magnet near upper limitswitch\n");
		}
		else{
			printf("Something went wrong\n");
		}
		if(gpio_get_level(REED_LOWER_GPIO) == 1){
        	//printf("Magnet away lower limitswitch\n");
		}
		else if(gpio_get_level(REED_LOWER_GPIO) == 0){
			printf("Magnet near lower limitswitch\n");
		}
		else{
			printf("Something went wrong\n");
		}	
		vTaskDelay(100 / portTICK_PERIOD_MS);  
   
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
	
    //wifi_init_sta();
	//init_system_time("pool.ntp.org");
	
	feederSem = xSemaphoreCreateBinary();
   	
    //xTaskCreate(task1, "ledTask", 4096, NULL, 1, &task1handle);
    //xTaskCreate(task2, "temp task", 4096, NULL, 1, &task2handle);
    xTaskCreate(task3, "servo task", 4096, NULL, 1, &task3handle);
    //xTaskCreate(task4, "Reed task", 4096, NULL, 1, &task4handle); 
    //xTaskCreate(task5, "time task", 4096, NULL, 1, NULL);
}
