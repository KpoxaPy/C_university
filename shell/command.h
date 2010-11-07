#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "parser/lexlist.h"

#define CMD_ACTION_NEXT 0
#define CMD_ACTION_BG 1
#define CMD_ACTION_AND_CHAIN 2
#define CMD_ACTION_OR_CHAIN 3

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

/* Command actions */
simpleCmd * genCommand(lexList *);
void delCommand(simpleCmd **);
void echoCommand(simpleCmd *);

/* Cmd tree actions */
tCmd * genTCmd(simpleCmd *);
void genRelation(tCmd *, int, tCmd *);
void delTCmd(tCmd **);

#endif
