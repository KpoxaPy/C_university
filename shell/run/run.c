#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "run.h"
#include "internals.h"
#include "jobmanager.h"

typedef struct managedTask {
	Task * task;

	jid_t curJob;

	struct managedTask * prev;
	struct managedTask * next;
} managedTask;
typedef managedTask mTask;

struct TaskManager {
	mTask * first;
	mTask * last;

	int count;
} tman = {NULL, NULL, 0};


void runTask(Task * task)
{
/*
 *  status.internal = checkInternalCommands(&status, cmd);
 *
 *  if (status.internal == INTERNAL_COMMAND_BREAK)
 *    break;
 *  else if (status.internal == INTERNAL_COMMAND_OK)
 *    run(&status, cmdmand);
 *  [> else if internalStatus == INTERNAL_COMMAND_CONTINUE <]
 *
 *  if (!cmdmand->bg)
 *    delCommand(&cmdmand);
 */
	if (task == NULL)
		return;

}
