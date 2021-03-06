#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <process.h>
#include <ha/ham.h>
#include <sys/netmgr.h>

#define OK 100500

int registerTask(char* taskName, char* executableParam) {
	ham_connect(0);

	char* relaunchPath = calloc(150, sizeof(char));
	sprintf(relaunchPath, "/tmp/qnx-text-project_g %s", executableParam);

	ham_entity_t *ehdl = ham_attach(taskName, ND_LOCAL_NODE, -1, relaunchPath, 0);
	if (ehdl == NULL) {
		char* errMsg = calloc(100, sizeof(char));
		sprintf(errMsg, "ham_attach error: %s", strerror(errno));
		perror(errMsg);
		return OK;
	}

	ham_condition_t *chdl = ham_condition(ehdl, CONDDEATH, "death", HREARMAFTERRESTART);
	if (chdl == NULL) {
		char* errMsg = calloc(100, sizeof(char));
		sprintf(errMsg, "ham_condition error: %s", strerror(errno));
		perror(errMsg);
		return EXIT_FAILURE;
	}

	ham_action_t *ahdl = ham_action_restart(chdl, "restart", relaunchPath, HREARMAFTERRESTART);
	if (ahdl == NULL) {
		char* errMsg = calloc(100, sizeof(char));
		sprintf(errMsg, "ham_action_restart error: %s", strerror(errno));
		perror(errMsg);
		return EXIT_FAILURE;
	}

	ham_disconnect(0);

	return OK;
}

int main(int argc, char *argv[]) {
	int returnVal;
	if ((returnVal = registerTask("taskAirbag", "taskAirbag")) != OK) {
		return returnVal;
	}
//	if ((returnVal = registerTask("taskMemoryCheck", "taskMemoryCheck")) != OK) {
//		return returnVal;
//	}
//	if ((returnVal = registerTask("taskSerialLine", "taskSerialLine")) != OK) {
//		return returnVal;
//	}

	return EXIT_SUCCESS;
}
