#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "my_time.h"
#include "airbag.h"
#include "serial_line.h"
#include "memory_check.h"
#include "aes.h"

void taskAirbagInternal() {
	//airbagLoop() is implementation of airbag algorithm from
	// Assignment 1
	airbagLoop();
	//For debug purposes airbag prints info, that outer loop
	//was called. It's needed to differentiate between calls
	//of outer and inner loop of airbag algorithm (again for
	//debug purposes).
	//printf("airbag task loop\n");
	//flushall();
	//sleep for 1 ms in outer loop
	//airbagSleep();
}

void* taskAirbag(void* args) {
	for (;;) {
		taskAirbagInternal();
	}

	return NULL;
}

void taskMemoryCheckInternal(unsigned char* flashCheckResult) {
	//calculate CRC, encrypt it with AES and wrote to flashCheckResult
	performMemoryCheck(flashCheckResult);
	//send flashCheckResult on serial line if it's currently available
	//(serial line is a shared resource protected with a mutex).
	//parameter "1" is a debug flag to indicate that sendOnSerialLine
	//was called from taskMemoryCheck. It's needed for debug purposes.
	sendOnSerialLine(flashCheckResult, N_BLOCK, 1);
	//sleep for 500 ms
	flashCheckSleep();
}

void* taskMemoryCheck(void* args) {
	//initialize array, which should contain results of flash check
	unsigned char* flashCheckResult = calloc(N_BLOCK, sizeof(unsigned char));
	//initialize AES and a dummy flash segment from memory_check.c
	initMemoryCheck();

	for (;;) {
		taskMemoryCheckInternal(flashCheckResult);
	}

	return NULL;
}

void taskSerialLineInternal(unsigned char* serialLineData, unsigned char* serialLineStatusMessage) {
	//receive a command from serial line
	int command = receiveCommand();
	//print command for debug purposes. There are two possible commands:
	//COMMAND_A and COMMAND_B
	printf("Command %c received on serial line\n", command == COMMAND_A ? 'A' : 'B');
	//encrypt dummy status message. Result is written to serialLineData
	prepareToSendOnSerialLine(serialLineStatusMessage, serialLineData);
	//send encrypted status message on serial line. This call is protected by a mutex
	sendOnSerialLine(serialLineData, N_BLOCK, 0);
	//sleep 100 ms
	serialLineSleep();
}

void* taskSerialLine(void* args) {
	//initialize array which should contain encrypted result of status message
	unsigned char* serialLineData = calloc(N_BLOCK, sizeof(unsigned char));
	//initialize dummy status message
	unsigned char* serialLineStatusMessage = "serial status";
	//initialize AES for serial line
	initSerialLine();

	for (;;) {
		taskSerialLineInternal(serialLineData, serialLineStatusMessage);
	}

	return NULL;
}

int main(int argc, char *argv[]) {
	//created IDs for threads
	pthread_t threadIDairbag;
	pthread_t threadIDmemoryCheck;
	pthread_t threadIDserialLine;

	//create threads
	pthread_create(&threadIDairbag, NULL, taskAirbag, NULL);
	pthread_create(&threadIDmemoryCheck, NULL, taskMemoryCheck, NULL);
	pthread_create(&threadIDserialLine, NULL, taskSerialLine, NULL);

	//wait till threads are finished
	pthread_join(threadIDairbag, NULL);
	pthread_join(threadIDmemoryCheck, NULL);
	pthread_join(threadIDserialLine, NULL);

	return EXIT_SUCCESS;
}
