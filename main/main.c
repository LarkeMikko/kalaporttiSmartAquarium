/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "esp_attr.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"
#include "driver/ledc.h"

#include "ds18b20.h"
#include "feederServo.h"

/* Can use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define ACTIVITY_LED_GPIO 26
#define SW1 34

#define REED_LOWER_GPIO 16
#define REED_UPPER_GPIO 15

#define LED_RED_GPIO 18
#define LED_GREEN_GPIO 19
#define LED_BLUE_GPIO 21
#define PUMP_GPIO 22

#define FEEDER_SERVO_GPIO 23

#define TEMP_GPIO 14

#define LEDC_TEST_DUTY         (4000)
#define LEDC_TEST_FADE_TIME    (1000)

/*
//You can get these value from the datasheet of servo you use, in general pulse width varies between 1000 to 2000 mocrosecond
#define SERVO_MIN_PULSEWIDTH 1000 //Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH 2000 //Maximum pulse width in microsecond
#define SERVO_MAX_DEGREE 90 //Maximum angle in degree upto which servo can rotate
*/

/*
static uint32_t servo_per_degree_init(uint32_t degree_of_rotation)
{
    uint32_t cal_pulsewidth = 0;
    cal_pulsewidth = (SERVO_MIN_PULSEWIDTH + (((SERVO_MAX_PULSEWIDTH - SERVO_MIN_PULSEWIDTH) * (degree_of_rotation)) / (SERVO_MAX_DEGREE)));
    return cal_pulsewidth;
}
*/

TaskHandle_t task1handle = NULL;
TaskHandle_t task2handle = NULL;
TaskHandle_t task3handle = NULL;
TaskHandle_t task4handle = NULL;
TaskHandle_t task5handle = NULL;

void task1(void *arg){
	    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_pad_select_gpio(ACTIVITY_LED_GPIO);   
    gpio_pad_select_gpio(PUMP_GPIO);
    gpio_pad_select_gpio(LED_GREEN_GPIO);
    
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(ACTIVITY_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(PUMP_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_RED_GPIO, GPIO_MODE_OUTPUT);
	while(1){
		/* Blink off (output low) */
		//printf("Turning off the LED\n");
        gpio_set_level(ACTIVITY_LED_GPIO, 0);
        gpio_set_level(PUMP_GPIO, 0);
        gpio_set_level(LED_GREEN_GPIO, 0);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        /* Blink on (output high) */
		//printf("Turning on the LED\n");
        gpio_set_level(ACTIVITY_LED_GPIO, 1);
        gpio_set_level(PUMP_GPIO, 1);
        gpio_set_level(LED_GREEN_GPIO, 1);
        vTaskDelay(2000 / portTICK_PERIOD_MS);   
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
    //servo_init(FEEDER_SERVO_GPIO,50);
    while (1) {
		//feed();
    }
}

void task4(void *arg){
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
        	printf("Magnet away upper limitswitch\n");
        	resetServo=false;
		}
		else if(gpio_get_level(REED_UPPER_GPIO) == 0){
			printf("Magnet near upper limitswitch\n");
			if(resetServo==false){
				feed();
			}
			resetServo=true;
			
		}
		else{
			printf("Something went wrong\n");
		}
		
		
		if(gpio_get_level(REED_LOWER_GPIO) == 1){
        	printf("Magnet away lower limitswitch\n");
		}
		else if(gpio_get_level(REED_LOWER_GPIO) == 0){
			printf("Magnet near lower limitswitch\n");
		}
		else{
			printf("Something went wrong\n");
		}
		printf("\n");
		
		vTaskDelay(1000 / portTICK_PERIOD_MS);      
	}
}

void task5(void *arg)
{
    ledc_channel_config_t ledc_channel_red = {0};
    ledc_channel_red.gpio_num = LED_RED_GPIO;
    ledc_channel_red.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_channel_red.channel = LEDC_CHANNEL_1;
    ledc_channel_red.intr_type = LEDC_INTR_DISABLE;
    ledc_channel_red.timer_sel = LEDC_TIMER_1;
    ledc_channel_red.duty = 0;
    
    ledc_timer_config_t ledc_timer = {0};
    ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_timer.duty_resolution = LEDC_TIMER_13_BIT;
    ledc_timer.timer_num = LEDC_TIMER_1;
    ledc_timer.freq_hz = 1000;
    
    ESP_ERROR_CHECK( ledc_channel_config(&ledc_channel_red) );
	ESP_ERROR_CHECK( ledc_timer_config(&ledc_timer) );
	
    while (1) {
    	for(int i=0;i<100;i++){
    		ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, i);
    		printf("%d\n",i);
    		vTaskDelay(10 / portTICK_PERIOD_MS);
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		for(int i=100;i>0;i--){
    		ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, i);
    		printf("%d\n",i);
    		vTaskDelay(10 / portTICK_PERIOD_MS);
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}



void app_main(void)
{

   	
    //xTaskCreate(task1, "ledTask", 4096, NULL, 1, &task1handle);
    //xTaskCreate(task2, "temp task", 4096, NULL, 1, &task2handle);
    //xTaskCreate(task3, "servo task", 4096, NULL, 1, &task3handle);
    xTaskCreate(task4, "Reed task", 4096, NULL, 1, &task4handle); 
    //xTaskCreate(task5, "ledStrip task", 4096, NULL, 1, &task5handle);
}




