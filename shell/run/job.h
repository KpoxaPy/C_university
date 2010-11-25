#ifndef _JOB_H_
#define _JOB_H_

#include "../main.h"
#include "command.h"

typedef struct process {
	simpleCmd * cmd;
	pid_t pid;

	int retStatus;
	int status;

	struct process * next;
} Process;

typedef struct job {
	pid_t pgid;

	Process * firstProc;

	int retStatus;

	struct termios tmodes;
	int stdin, stdout, stderr;
} Job;


Job * takeJob(void);
Job * newJob(void);
void freeJob(Job *);

Job * fillJob(Job *, tCmd *);

void launchJob(Job *, int);

#endif
