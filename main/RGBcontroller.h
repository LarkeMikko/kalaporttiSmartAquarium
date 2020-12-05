/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef RGBCONTROLLER_H_  
#define RGBCONTROLLER_H_

void RGB_init();

void set_update_duty(ledc_channel_config_t *channel, int duty);
void set_color(uint8_t r, uint8_t g, uint8_t b);

void fade_update_duty(ledc_channel_config_t *channel, int duty, int fadetime);
void fade_color(uint8_t r, uint8_t g, uint8_t b, int fadetime, int staytime);
void rainbowFade(int fadetime,int staytime);

void str_tolower(char *str);
int extract_rgb_values(const char *data, uint8_t *red, uint8_t *green, uint8_t *blue);
void setOverMQTT(const char *data, int fadetime, int staytime);
#endif
