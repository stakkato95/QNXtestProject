#include <stdlib.h>
#include <stdio.h>
//#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <process.h>
#include <ha/ham.h>
#include <sys/netmgr.h>

#include "my_time.h"
#include "airbag.h"
#include "serial_line.h"
#include "memory_check.h"
#include "aes.h"
#include "my_time.h"

#define FORK_ERROR -1
#define CHILD_PROZESS 0
#define OK 100500

#define TASK_AIRBAG "taskAirbag"
#define TASK_MEMORY "taskMemoryCheck"
#define TASK_SERIAL "taskSerialLine"

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
	//TODO send on serial line is disabled for Assignment 5 (since IPC is not yet implemented)
	//sendOnSerialLine(flashCheckResult, N_BLOCK, 1);
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

int attachSelfToHam(char* taskName) {
	ham_entity_t* ehdl;
	ham_condition_t* chdl;
	ham_action_t *ahdl;

	char* buffer = calloc(100, sizeof(char));
	sprintf(buffer, "/bin/kill -9 %d", getpid());

	char* uniqueTaskName = calloc(150, sizeof(char));
	sprintf(uniqueTaskName, "%s", taskName);

	ham_connect(0);

	ehdl = ham_attach_self("qnx-text-project_g", 1000000000ULL, 3, 8, 0);
	if (ehdl == NULL) {
		char* errMsg = calloc(150, sizeof(char));
		sprintf(errMsg, "test-project: ham_attach_self error: %s", strerror(errno));
		perror(errMsg);
		return EXIT_FAILURE;
	}

	chdl = ham_condition(ehdl, CONDHBEATMISSEDLOW, "HeartbeatLow", 0);
	if (chdl == NULL) {
		char* errMsg = calloc(150, sizeof(char));
		sprintf(errMsg, "test-project: ham_condition error: %s", strerror(errno));
		perror(errMsg);
		return EXIT_FAILURE;
	}

	ahdl = ham_action_execute(chdl, "Kill", buffer, HREARMAFTERRESTART);
	if (ahdl == NULL) {
		char* errMsg = calloc(150, sizeof(char));
		sprintf(errMsg, "test-project: ham_action_execute error: %s", strerror(errno));
		perror(errMsg);
		return EXIT_FAILURE;
	}

	return OK;
}

int main(int argc, char *argv[]) {
	char* taskName = argv[1];

//	int returnVal;
//	if ((returnVal = attachSelfToHam(taskName)) != OK) {
//		return returnVal;
//	}

	if (strcmp(taskName, TASK_AIRBAG) == 0) {
		taskAirbag(NULL);
	} else if (strcmp(taskName, TASK_MEMORY) == 0) {
		taskMemoryCheck(NULL);
	} else if (strcmp(taskName, TASK_SERIAL) == 0) {
		taskSerialLine(NULL);
	} else {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
