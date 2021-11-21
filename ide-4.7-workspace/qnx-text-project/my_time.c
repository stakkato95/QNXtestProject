/*
 * my_time.c
 *
 *  Created on: Nov 6, 2021
 *      Author: user
 */

#include <time.h>

#define NANOSEC 		  1000 * 1000 * 1000 //1   sec
#define AIRBAG_SLEEP         1 * 1000 * 1000 //1   ms
#define FLASH_CHECK_SLEEP  500 * 1000 * 1000 //500 ms
#define SERIAL_LINE_SLEEP  100 * 1000 * 1000 //100 ms

//addNanoSec adds a nanosecond delay and checks whether
//an overflow of timespec.tv_nsec happens (it can't be
//greater than 1 000 000 000).
void addNanoSec(struct timespec* t, long int nanosec) {
	long int addedNanoSec = t->tv_nsec + nanosec;
	if (addedNanoSec == NANOSEC) {
		t->tv_sec += 1;
		t->tv_nsec = 0;
	} else if (addedNanoSec > NANOSEC) {
		t->tv_sec += 1;
		t->tv_nsec = addedNanoSec - NANOSEC;
	} else {
		t->tv_nsec = addedNanoSec;
	}
}

void airbagSleep() {
	mySleep(AIRBAG_SLEEP);
}

void flashCheckSleep() {
	mySleep(FLASH_CHECK_SLEEP);
}

void serialLineSleep() {
	mySleep(SERIAL_LINE_SLEEP);
}

//mySleep reads current timestamp and adds a nanosecond delay to it
void mySleep(long int sleep) {
	struct timespec next;
	clock_gettime(CLOCK_REALTIME, &next);
	addNanoSec(&next, sleep);
	clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next, NULL);
}
