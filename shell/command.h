#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "parser/lexlist.h"

#define TCMD_NONE -1
#define TCMD_SIMPLE 0
#define TCMD_PIPE 1
#define TCMD_LIST 2
#define TCMD_SCRIPT 3

#define TREL_END 0
#define TREL_PIPE 1
#define TREL_AND 2
#define TREL_OR 3
#define TREL_SCRIPT 4

typedef struct simpleCmd {
	char * file;

	int argc;
	char ** argv;

	char * rdrInputFile,
		* rdrOutputFile;
	int rdrOutputAppend;

	int pFDin, pFDout;
} simpleCmd;

typedef struct tCmd {
	int cmdType;
	int modeBG;

	simpleCmd * cmd;
	struct tCmd * child;
	struct tRel * rel;
} tCmd;

typedef struct tRel {
	int relType;

	tCmd * next;
} tRel;

/* Support functions for command */
/* This function generates argv from list of 
 * lexems.
 *
 * ATTENTION! After execution list will be
 * free. Strings that were in list of lexems
 * remain in memory, links on them will be
 * in vector of arguments. 
 */
int genArgVector(simpleCmd *, lexList *);

/* Command actions */
simpleCmd * newCommand();
void delCommand(simpleCmd **);
/*void echoCommand(simpleCmd *);*/

/* Cmd tree actions */
tCmd * genTCmd(simpleCmd *);
void genRelation(tCmd *, int, tCmd *);
void delTCmd(tCmd **);
void echoCmdTree(tCmd *);
char * getCmdString(tCmd *);

#endif
