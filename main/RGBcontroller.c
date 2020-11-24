
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"

#include "RGBcontroller.h"

#define LED_RED_GPIO 18
#define LED_GREEN_GPIO 19
#define LED_BLUE_GPIO 21

const int MAX_DUTY = pow(2, LEDC_TIMER_13_BIT);

static ledc_channel_config_t ledc_channel_red = {
		.channel    = LEDC_CHANNEL_0,
        .duty       = 0,
        .gpio_num   = LED_RED_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_0
};
static ledc_channel_config_t ledc_channel_green = {
		.channel    = LEDC_CHANNEL_1,
        .duty       = 0,
        .gpio_num   = LED_GREEN_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_0
};
static ledc_channel_config_t ledc_channel_blue = {
		.channel    = LEDC_CHANNEL_2,
        .duty       = 0,
        .gpio_num   = LED_BLUE_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_0
};

static ledc_timer_config_t ledc_timer0 = {
    .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
    .freq_hz = 5000,                      // frequency of PWM signal
    .speed_mode = LEDC_HIGH_SPEED_MODE,           // timer mode
    .timer_num = LEDC_TIMER_0,            // timer index
    .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
};
static ledc_timer_config_t ledc_timer1 = {
    .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
    .freq_hz = 5000,                      // frequency of PWM signal
    .speed_mode = LEDC_HIGH_SPEED_MODE,           // timer mode
    .timer_num = LEDC_TIMER_1,            // timer index
    .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
};

void RGB_init()
{
	gpio_pad_select_gpio(LED_RED_GPIO);
    gpio_pad_select_gpio(LED_GREEN_GPIO);
    gpio_pad_select_gpio(LED_BLUE_GPIO);
    
    gpio_set_direction(LED_RED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_GREEN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_BLUE_GPIO, GPIO_MODE_OUTPUT);
    
    gpio_set_level(LED_RED_GPIO, 0);
    gpio_set_level(LED_GREEN_GPIO, 0);
    gpio_set_level(LED_BLUE_GPIO, 0);
  
    ledc_timer_config(&ledc_timer0);

        
    ledc_channel_config(&ledc_channel_red);
    ledc_channel_config(&ledc_channel_green);
    ledc_channel_config(&ledc_channel_blue);
    
    ledc_fade_func_install(0);
}

void set_update_duty(ledc_channel_config_t *channel, int duty) {
            ledc_set_duty(channel->speed_mode, channel->channel, duty);
            ledc_update_duty(channel->speed_mode, channel->channel);
}


void set_color(uint8_t r, uint8_t g, uint8_t b) {
	r=255-r;
	g=255-g;
	b=255-b;
	
	set_update_duty(&ledc_channel_red, r * MAX_DUTY / 255);
	set_update_duty(&ledc_channel_green, g * MAX_DUTY / 255);
	set_update_duty(&ledc_channel_blue, b * MAX_DUTY / 255);
}

void fade_update_duty(ledc_channel_config_t *channel, int duty, int fadetime) {
        ledc_set_fade_with_time(channel->speed_mode, channel->channel, duty, fadetime);           
        ledc_fade_start(channel->speed_mode, channel->channel, LEDC_FADE_NO_WAIT); 
}

void fade_color(uint8_t r, uint8_t g, uint8_t b, int fadetime, int staytime) {
	r=255-r;
	g=255-g;
	b=255-b;
	
	if(staytime<=fadetime) staytime = fadetime;
	
	fade_update_duty(&ledc_channel_red, r * MAX_DUTY / 255, fadetime);
	fade_update_duty(&ledc_channel_green, g * MAX_DUTY / 255, fadetime);
	fade_update_duty(&ledc_channel_blue, b * MAX_DUTY / 255, fadetime);
	vTaskDelay(staytime / portTICK_PERIOD_MS);
}







