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

//int initHam(char* taskName, char* executableName) {
//	//ham_connect(0);
//
//	ham_entity_t *ehdl;
//	ham_condition_t *chdl;
//	ham_action_t *ahdl;
//
//	char* relaunchPath = calloc(100, sizeof(char));
//	sprintf(relaunchPath, "/tmp/%s %s", executableName, taskName);
//
//	ehdl = ham_attach(taskName, ND_LOCAL_NODE, getpid(), relaunchPath, 0);
//	if (ehdl == NULL) {
//		perror("initHam() ham_attach error");
//		return EXIT_FAILURE;
//	}
//
//	chdl = ham_condition(ehdl, CONDDEATH, "death", HREARMAFTERRESTART);
//	if (chdl == NULL) {
//		perror("initHam() ham_condition error");
//		return EXIT_FAILURE;
//	}
//
//	ahdl = ham_action_restart(chdl, "restart", relaunchPath, HREARMAFTERRESTART);
//	if (ahdl == NULL) {
//		perror("initHam() ham_action_restart error");
//		return EXIT_FAILURE;
//	}
//
//	//ham_disconnect(0);
//
//	free(relaunchPath);
//
//	return OK;
//}

int initHamAndHeartbit(char* taskName, char* executableName) {
	char* killCmd = calloc(25, sizeof(char));
	sprintf(killCmd, "/bin/kill -9 %d", getpid());

	char* relaunchPath = calloc(150, sizeof(char));
//	sprintf(relaunchPath, "cd /tmp; %s %s", executableName, taskName);
//	sprintf(relaunchPath, "/tmp/qnx-text-project_g", executableName, taskName);

	ham_connect(0);

//	ham_entity_t *ehdl = ham_attach_self(taskName, 4000000000ULL /*AIRBAG_SLEEP * 3*/, 1, 4, 0);
//	if (ehdl == NULL) {
//		perror("initHamAndHeartbit() ham_attach_self error - OK");
//		return OK;
//	}
//
//	ham_condition_t *chdl = ham_condition(ehdl, CONDHBEATMISSEDLOW, "heartbeatLow", HREARMAFTERRESTART);
//	if (chdl == NULL) {
//		perror("initHamAndHeartbit() ham_condition(heartbeatLow) error");
//		return EXIT_FAILURE;
//	}
//
//	ham_action_t *ahdl = ham_action_execute(chdl, "kill", killCmd, HREARMAFTERRESTART);
//	if (ahdl == NULL) {
//		perror("initHamAndHeartbit() ham_action_execute(kill) error");
//		return EXIT_FAILURE;
//	}

	//free(killCmd);

	char* taskNameWithPid = calloc(50, sizeof(char));
	sprintf(taskNameWithPid, "%s-%d", taskName, getpid());

	ham_entity_t *ehdl = ham_attach(taskNameWithPid, ND_LOCAL_NODE, -1, "/tmp/airbag", 0);
	ham_entity_t *ehdl = ham_attach(taskNameWithPid, ND_LOCAL_NODE, -1, "/tmp/memory", 0);
	ham_entity_t *ehdl = ham_attach(taskNameWithPid, ND_LOCAL_NODE, -1, "/tmp/serial", 0);
	if (ehdl == NULL) {
		char* errMsg = calloc(100, sizeof(char));
		sprintf(errMsg, "initHam() ham_attach error - OK %d", getpid());
		perror(errMsg);
		return OK;
	}

	/////////////////////////////////////////////
	ham_condition_t *chdl = ham_condition(ehdl, CONDDEATH, "death", HREARMAFTERRESTART);
	if (chdl == NULL) {
		perror("initHam() ham_condition(death) error");
		return EXIT_FAILURE;
	}

	ham_action_t *ahdl = ham_action_restart(chdl, "restart", "/tmp/qnx-text-project_g taskAirbag", HREARMAFTERRESTART);
	if (ahdl == NULL) {
		perror("initHam() ham_action_restart(restart) error");
		return EXIT_FAILURE;
	}
	/////////////////////////////////

//	ham_action_handle_free(ahdl);
//	ham_condition_handle_free(chdl);
//
//	chdl = ham_condition(ehdl, CONDABNORMALDEATH, "abnormal", HREARMAFTERRESTART);
//	if (chdl == NULL) {
//		perror("initHam() ham_condition(deathAbnormal) error");
//		return EXIT_FAILURE;
//	}
//
//	ahdl = ham_action_restart(chdl, "whatever", relaunchPath, HREARMAFTERRESTART);
//	if (ahdl == NULL) {
//		perror("initHam() ham_action_restart(restartAfterAbnormalDeath) error");
//		return EXIT_FAILURE;
//	}

	//ham_disconnect(0);

	ham_disconnect(0);
	free(relaunchPath);


	return OK;
}

//int initHamHeartbeat(char* taskName) {
//	char* killCmd = calloc(25, sizeof(char));
//	sprintf(killCmd, "/bin/kill -9 %d", getpid());
//
//	char* heartbeatTaskName = calloc(50, sizeof(char));
//	sprintf(heartbeatTaskName, "%sHeartbeat", taskName);
//
//	ham_entity_t *ehdl = ham_attach_self(heartbeatTaskName, 1000000000ULL /*AIRBAG_SLEEP * 3*/, 4, 8, 0);
//	if (ehdl == NULL) {
//		perror("initHamHeartbeat() ham_attach_self error");
//		return EXIT_FAILURE;
//	}
//
//	ham_condition_t *chdl = ham_condition(ehdl, CONDHBEATMISSEDLOW, "heartbeatLow", 0);
//	if (chdl == NULL) {
//		perror("initHamHeartbeat() ham_condition error");
//		return EXIT_FAILURE;
//	}
//
//	ham_action_t *ahdl = ham_action_execute(chdl, "kill", killCmd, HREARMAFTERRESTART);
//	if (ahdl == NULL) {
//		perror("initHamHeartbeat() ham_action_execute error");
//		return EXIT_FAILURE;
//	}
//
//	free(killCmd);
//	free(heartbeatTaskName);
//
//	return OK;
//}

int main(int argc, char *argv[]) {
	char* executableName = argv[0];
	char* taskName = argv[1];

//	if (fork() == FORK_ERROR) {
//		char* errMsg = calloc(100, sizeof(char));
//		sprintf(errMsg, "fork err: %s", strerror(errno));
//		perror(errMsg);
//		return EXIT_FAILURE;
//	} else {
//		return EXIT_SUCCESS;
//	}

	int returnValue = OK;
	if (strcmp(taskName, TASK_AIRBAG) == 0) {
		if ((returnValue = initHamAndHeartbit(TASK_AIRBAG, executableName)) != OK) {
			return returnValue;
		}
//		if ((returnValue = initHam(TASK_AIRBAG, executableName)) != OK) {
//			return returnValue;
//		}
		taskAirbag(NULL);
	} else if (strcmp(taskName, TASK_MEMORY) == 0) {
//		if ((returnValue = initHam(TASK_MEMORY, executableName)) != OK) {
//			return returnValue;
//		}
//		if ((returnValue = initHamHeartbeat(TASK_MEMORY)) != OK) {
//			return returnValue;
//		}
		taskMemoryCheck(NULL);
	} else if (strcmp(taskName, TASK_SERIAL) == 0) {
//		if ((returnValue = initHam(TASK_SERIAL, executableName)) != OK) {
//			return returnValue;
//		}
//		if ((returnValue = initHamHeartbeat(TASK_SERIAL)) != OK) {
//			return returnValue;
//		}
		taskSerialLine(NULL);
	} else {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
