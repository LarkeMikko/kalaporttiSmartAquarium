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
#ifndef FEEDERSERVO_H_  
#define FEEDERSERVO_H_

void servo_init(int GPIO, int freq);
uint32_t servo_per_degree_init(uint32_t degree_of_rotation);
void rotate(int angle);
void feed(int ammount, int ms);
void test(void);
#endif
