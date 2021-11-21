/*
 * my_time.h
 *
 *  Created on: Nov 6, 2021
 *      Author: user
 */

#ifndef MY_TIME_H_
#define MY_TIME_H_

#include <time.h>

#define AIRBAG_SLEEP         1 * 1000 * 1000 //1   ms
#define FLASH_CHECK_SLEEP  500 * 1000 * 1000 //500 ms
#define SERIAL_LINE_SLEEP  100 * 1000 * 1000 //100 ms

void addNanoSec(struct timespec* t, long int nanosec);

void airbagSleep();

void flashCheckSleep();

void serialLineSleep();

#endif /* MY_TIME_H_ */
