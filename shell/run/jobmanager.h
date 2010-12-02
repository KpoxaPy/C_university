#ifndef _BGMANAGER_H_
#define _BGMANAGER_H_

#include "../main.h"
#include "../stuff/echoes.h"
#include "task.h"
#include "run.h"
#include "job.h"

#define JM_ST_NONE 0
#define JM_ST_RUNNING 1
#define JM_ST_CONTINUED 2
#define JM_ST_STOPPED 3
#define JM_ST_COMPLETED 4


typedef struct managedJob {
	jid_t jid;
	Job * job;
	Task * task;

	int status;
	int notified;

	struct managedJob * next;
	struct managedJob * prev;
} managedJob;

typedef managedJob mJob;

/* Managing jobs:
 *  - adding in manager;
 *  - deleting from manager.
 */
jid_t addJob(Task *);
void delJobByJid(jid_t);

/* Changing the last and the penult jid in the list. */
void setPenultByLastJid();
void setLastJid(jid_t);

/* Base executing and control functions */
void launchJobByJid(jid_t);
int makeFG(jid_t, int);
int makeBG(jid_t, int);
void waitJob(jid_t);

/* Periodicaly called functions to take actual
 * information about jobs */
void checkJobs(void);

/* Working with list of jobs */
mJob * getJob(jid_t);
mJob * getJobByGpid(pid_t);
mJob * getJobByPid(pid_t, Process **);
pid_t getGpidByJid(jid_t);

/* Echoing jobs */
void echoJobList(void);

#endif
