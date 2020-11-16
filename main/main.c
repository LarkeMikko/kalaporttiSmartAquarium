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

/* Can use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 12
#define SERVO_GPIO 18
#define REED_LOWER_GPIO 14
#define REED_UPPER_GPIO 27
#define TEMP_GPIO 19

//You can get these value from the datasheet of servo you use, in general pulse width varies between 1000 to 2000 mocrosecond
#define SERVO_MIN_PULSEWIDTH 1000 //Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH 2000 //Maximum pulse width in microsecond
#define SERVO_MAX_DEGREE 90 //Maximum angle in degree upto which servo can rotate


static uint32_t servo_per_degree_init(uint32_t degree_of_rotation)
{
    uint32_t cal_pulsewidth = 0;
    cal_pulsewidth = (SERVO_MIN_PULSEWIDTH + (((SERVO_MAX_PULSEWIDTH - SERVO_MIN_PULSEWIDTH) * (degree_of_rotation)) / (SERVO_MAX_DEGREE)));
    return cal_pulsewidth;
}



TaskHandle_t task1handle = NULL;
TaskHandle_t task2handle = NULL;
TaskHandle_t task3handle = NULL;
TaskHandle_t task4handle = NULL;

void task1(void *arg){
	    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_pad_select_gpio(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
	while(1){
		/* Blink off (output low) */
		//printf("Turning off the LED\n");
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        /* Blink on (output high) */
		//printf("Turning on the LED\n");
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(2000 / portTICK_PERIOD_MS);       
	}
}

void task2(void *arg){

	ds18b20_init(TEMP_GPIO);
	float temp = 0;
	while(1){
		temp = ds18b20_get_temp(); 
		//printf("tempperature: %f\n",temp);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

}


void task3(void *arg)
{
    uint32_t angle, count;
    printf("initializing mcpwm servo control gpio......\n");
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, SERVO_GPIO);    //Set GPIO 18 as PWM0A, to which servo is connected

    //2. initial mcpwm configuration
    printf("Configuring Initial Parameters of mcpwm......\n");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 50;    //frequency = 50Hz, i.e. for every servo motor time period should be 20ms
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings
    while (1) {
    	/*
        for (count = 0; count < SERVO_MAX_DEGREE; count++) {
            //printf("Angle of rotation: %d\n", count);
            angle = servo_per_degree_init(count);
            //printf("pulse width: %dus\n", angle);
            mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, angle);
            vTaskDelay(10);     //Add delay, since it takes time for servo to rotate, generally 100ms/60degree rotation at 5V
        }
        for (count = SERVO_MAX_DEGREE; count > 0; count--) {
            //printf("Angle of rotation: %d\n", count);
            angle = servo_per_degree_init(count);
            //printf("pulse width: %dus\n", angle);
            mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, angle);
            vTaskDelay(10);     //Add delay, since it takes time for servo to rotate, generally 100ms/60degree rotation at 5V
        }
        */
        angle = servo_per_degree_init(0);
        mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, angle);
        vTaskDelay(100);
        
        angle = servo_per_degree_init(45);
        mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, angle);
        vTaskDelay(100);
        
        angle = servo_per_degree_init(90);
        mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, angle);
        vTaskDelay(100);
        
        angle = servo_per_degree_init(45);
        mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, angle);
        vTaskDelay(100);
    }
}

void task4(void *arg){
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(REED_LOWER_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(REED_UPPER_GPIO, GPIO_MODE_INPUT);
	while(1){
		
        if(gpio_get_level(REED_UPPER_GPIO) == 1){
        	printf("Magnet near upper limitswitch\n");
		}
		else if(gpio_get_level(REED_UPPER_GPIO) == 0){
			printf("Magnet away upper limitswitch\n");
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



void app_main(void)
{

   	
    xTaskCreate(task1, "ledTask", 4096, NULL, 1, &task1handle);
    xTaskCreate(task2, "temp task", 4096, NULL, 1, &task2handle);
    xTaskCreate(task3, "servo task", 4096, NULL, 1, &task3handle);
    xTaskCreate(task4, "Reed task", 4096, NULL, 1, &task4handle);
}




