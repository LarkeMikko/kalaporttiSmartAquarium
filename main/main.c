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

#define REED_LOWER_GPIO 15
#define REED_UPPER_GPIO 16

#define LED_RED_GPIO 18
#define LED_GREEN_GPIO 19
#define LED_BLUE_GPIO 21
#define PUMP_GPIO 22

#define FEEDER_SERVO_GPIO 23

#define TEMP_GPIO 14

#define LEDC_TEST_DUTY         (8150)
#define LEDC_TEST_FADE_TIME    (5000)


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

	RGB_init(LED_RED_GPIO, LED_GREEN_GPIO, LED_BLUE_GPIO);
	while(1){
		/*
		printf("red\n");
		set_RGB(255,0,0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		
		printf("green\n");
		set_RGB(0,255,0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		
		printf("blue\n");
		set_RGB(0,0,255);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		
		printf("hot pink\n");
		set_RGB(255,105,180);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		
		printf("orange\n");
		set_RGB(255,165,0);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
		*/
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
		
		//gpio_set_level(ACTIVITY_LED_GPIO, 0);
        //gpio_set_level(PUMP_GPIO, 0);
		//vTaskDelay(2000 / portTICK_PERIOD_MS); 
        //gpio_set_level(ACTIVITY_LED_GPIO, 1);
        //gpio_set_level(PUMP_GPIO, 1);
        //vTaskDelay(2000 / portTICK_PERIOD_MS);   
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
    	if(xSemaphoreTake(feederSem, portMAX_DELAY) == pdTRUE){
    		feed(180,1000);
    		printf("FEED\n");
    		vTaskDelay(10 / portTICK_PERIOD_MS); 
		} 

    }
}

void task4(void *arg){
	gpio_pad_select_gpio(ACTIVITY_LED_GPIO);
	gpio_set_direction(ACTIVITY_LED_GPIO, GPIO_MODE_OUTPUT);
	
	gpio_pullup_en(REED_LOWER_GPIO);
	
	gpio_pad_select_gpio(REED_LOWER_GPIO);
	gpio_pad_select_gpio(REED_UPPER_GPIO);
	
	servo_init(FEEDER_SERVO_GPIO,50);
	bool resetServo=false;
	
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(REED_LOWER_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(REED_UPPER_GPIO, GPIO_MODE_INPUT);
	while(1){
		
        if(gpio_get_level(REED_UPPER_GPIO) == 1){
        	//printf("Magnet away upper limitswitch\n");
        	resetServo=false;
		}
		else if(gpio_get_level(REED_UPPER_GPIO) == 0){
			//printf("Magnet near upper limitswitch\n");
			if(resetServo==false){
				feed(20000,1000);
				printf("FEED\n");
			}
			resetServo=true;
			
		}
		else{
			printf("Something went wrong\n");
		}
		if(gpio_get_level(REED_LOWER_GPIO) == 1){
			gpio_set_level(ACTIVITY_LED_GPIO, 0);
        	//printf("Magnet away lower limitswitch\n");
		}
		else if(gpio_get_level(REED_LOWER_GPIO) == 0){
			gpio_set_level(ACTIVITY_LED_GPIO, 1);
			//printf("Magnet near lower limitswitch\n");
		}
		else{
			printf("Something went wrong\n");
		}
		printf("\n");
		
		vTaskDelay(100 / portTICK_PERIOD_MS);      
	}
}

void task5(void *arg)
{
    time_t now;
    struct tm timeinfo;

    while (1) {
		timeinfo = get_time(&now, "CST-2");    	
    	//printf("hours: %d minutes: %d seconds %d\n",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
    	printf("Time in Finland: %d:%d:%d\n",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
    	
    	//timeinfo = get_time(&now, "CST-1");    	
    	//printf("Time in Sweden: %d:%d:%d\n",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
    	
    	//if(timeinfo.tm_hour==11 && timeinfo.tm_min==47 && timeinfo.tm_sec== 11){
		if(timeinfo.tm_sec % 10 == 0){
    		xSemaphoreGive(feederSem);
		}
    	vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void task6(void *arg)
{
    	ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_HIGH_SPEED_MODE,           // timer mode
        .timer_num = LEDC_TIMER_1,            // timer index
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };
    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t RED_channel = {

        .channel    = LEDC_CHANNEL_0,
        .duty       = 0,
        .gpio_num   = LED_RED_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_1
    };
    
        ledc_channel_config_t GREEN_channel = {

        .channel    = LEDC_CHANNEL_1,
        .duty       = 0,
        .gpio_num   = LED_GREEN_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_1
    };
    
        ledc_channel_config_t BLUE_channel = {

        .channel    = LEDC_CHANNEL_2,
        .duty       = 0,
        .gpio_num   = LED_BLUE_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_1
    };
    /////////////
    /*
        ledc_channel_config_t ledc_channel[3] = {
        {
            .channel    = LEDC_CHANNEL_0,
            .duty       = 0,
            .gpio_num   = LED_GREEN_GPIO,
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_1
        },
        {
            .channel    = LEDC_CHANNEL_0,
            .duty       = 0,
            .gpio_num   = LED_BLUE_GPIO,
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_1
        },

        {
            .channel    = LEDC_CHANNEL_0,
            .duty       = 0,
            .gpio_num   = LED_RED_GPIO,
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_1
        },
    };
    */
    
        
    ledc_channel_config(&RED_channel);
    ledc_channel_config(&GREEN_channel);
    ledc_channel_config(&BLUE_channel);
    ledc_fade_func_install(0);

    while (1) {
    	
    	printf("1. LEDC fade up to duty = %d\n", LEDC_TEST_DUTY);
            ledc_set_fade_with_time(RED_channel.speed_mode,
                    RED_channel.channel, LEDC_TEST_DUTY, LEDC_TEST_FADE_TIME);
                    
            ledc_fade_start(RED_channel.speed_mode,
                    RED_channel.channel, LEDC_FADE_NO_WAIT);
        vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);

        printf("2. LEDC fade down to duty = 0\n");
            ledc_set_fade_with_time(RED_channel.speed_mode,
                    RED_channel.channel, 0, LEDC_TEST_FADE_TIME);
                    
            ledc_fade_start(RED_channel.speed_mode,
                    RED_channel.channel, LEDC_FADE_NO_WAIT);
        vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
            printf("3. LEDC fade up to duty = %d\n", LEDC_TEST_DUTY);
            ledc_set_fade_with_time(GREEN_channel.speed_mode,
                    GREEN_channel.channel, 8150, LEDC_TEST_FADE_TIME);
                    
            ledc_fade_start(GREEN_channel.speed_mode,
                    GREEN_channel.channel, LEDC_FADE_NO_WAIT);
        vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);

        printf("4. LEDC fade down to duty = 0\n");
            ledc_set_fade_with_time(GREEN_channel.speed_mode,
                    GREEN_channel.channel, 0, LEDC_TEST_FADE_TIME);
                    
            ledc_fade_start(GREEN_channel.speed_mode,
                    GREEN_channel.channel, LEDC_FADE_NO_WAIT);
        vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
            printf("5. LEDC fade up to duty = %d\n", LEDC_TEST_DUTY);
            ledc_set_fade_with_time(BLUE_channel.speed_mode,
                    BLUE_channel.channel, LEDC_TEST_DUTY, LEDC_TEST_FADE_TIME);
                    
            ledc_fade_start(BLUE_channel.speed_mode,
                    BLUE_channel.channel, LEDC_FADE_NO_WAIT);
        vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);

        printf("6. LEDC fade down to duty = 0\n");
            ledc_set_fade_with_time(BLUE_channel.speed_mode,
                    BLUE_channel.channel, 0, LEDC_TEST_FADE_TIME);
                    
            ledc_fade_start(RED_channel.speed_mode,
                    BLUE_channel.channel, LEDC_FADE_NO_WAIT);
        vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);
        
/*
        printf("3. LEDC set duty = %d without fade\n", LEDC_TEST_DUTY);

            ledc_set_duty(RED_channel.speed_mode, RED_channel.channel, LEDC_TEST_DUTY);
            ledc_update_duty(RED_channel.speed_mode, RED_channel.channel);

        vTaskDelay(1000 / portTICK_PERIOD_MS);

        printf("4. LEDC set duty = 0 without fade\n");

            ledc_set_duty(RED_channel.speed_mode, RED_channel.channel, 0);
            ledc_update_duty(RED_channel.speed_mode, RED_channel.channel);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
*/
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
   	
    xTaskCreate(task1, "ledTask", 4096, NULL, 1, &task1handle);
    //xTaskCreate(task2, "temp task", 4096, NULL, 1, &task2handle);
    //xTaskCreate(task3, "servo task", 4096, NULL, 1, &task3handle);
    //xTaskCreate(task4, "Reed task", 4096, NULL, 1, &task4handle); 
    //xTaskCreate(task5, "time task", 4096, NULL, 1, NULL);
    //xTaskCreate(task6, "ledStrip task", 4096, NULL, 1, NULL);
}
