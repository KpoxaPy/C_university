#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "main.h"
#include "words.h"

#define CMD_ACTION_NEXT 0
#define CMD_ACTION_BG 1
#define CMD_ACTION_AND_CHAIN 2
#define CMD_ACTION_OR_CHAIN 3

struct command {
	char * file;
	int bg;
	int argc;
	char ** argv;
	struct wordlist * words;
};

struct cmdElem {
	struct cmdElem * firstChild;
	struct cmdElemAction * next;
	struct command * cmd;
};

struct cmdElemAction {
	int cmdAction;
	struct cmdElem * second;
	struct cmdElem * fir


/* Command actions */
stuct command * genCommand(struct wordlist *);
void delCommand(struct command ** );
void echoCommand(struct command *);

/* Cmd tree actions */
struct cmdElem * createCmdElem(struct command *, struct cmdElem *);
void removeCmdElem(struct cmdElem *);

struct cmdElemAction * createCmdElemAction(int,	struct cmdElem *, struct cmdElem *);
void removeCmdElemAction(struct cmdElemAction *);
