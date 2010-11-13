#include <stdlib.h>
#include <stdio.h>
#include "command.h"
#include "buffer.h"

int echoCmdTreeIndentLevel;

char * cmdTypeStr[] = {
	"TCMD_SIMPLE",
	"TCMD_PIPE",
	"TCMD_LIST",
	"TCMD_SCRIPT"};

char * relTypeStr[] = {
	"TREL_END",
	"TREL_PIPE",
	"TREL_AND",
	"TREL_OR",
	"TREL_SCRIPT"};

void echoIndent();
void echoSimpleCmd(simpleCmd *);
void echoCmdTreeNode(tCmd *);

int genArgVector(simpleCmd * cmd, lexList * list)
{
	Lex * lex;
	char ** argv;
	int i;
	int lc;

	if (list == NULL || cmd == NULL)
		return -1;

	lc = list->count;

	argv = (char **)malloc(sizeof(char *)*
		(lc + 1));

	if (argv == NULL)
		return -1;

	for (i = 0; i < lc; i++)
	{
		lex = getLex(list);

		if (lex == NULL)
			break;

		argv[i] = lex->str;
		free(lex);
	}

	argv[i] = NULL;

	cmd->argv = argv;
	cmd->argc = lc;

	return 0;
}

simpleCmd * newCommand()
{
	simpleCmd * cmd;

	cmd = (simpleCmd *)malloc(sizeof(simpleCmd));

	if (cmd == NULL)
		return NULL;

	cmd->file = NULL;
	cmd->argc = 0;
	cmd->argv = NULL;
	cmd->rdrInputFile = NULL;
	cmd->rdrOutputFile = NULL;
	cmd->rdrOutputAppend = 0;

	return cmd;
}

void delCommand(simpleCmd ** cmd)
{
	if (cmd != NULL && *cmd != NULL)
	{
		char ** pStr = (*cmd)->argv;

		if (pStr != NULL)
			while (*pStr != NULL)
				free(*pStr++);

		if ((*cmd)->rdrInputFile != NULL)
			free((*cmd)->rdrInputFile);

		if ((*cmd)->rdrOutputFile != NULL)
			free((*cmd)->rdrOutputFile);

		free((*cmd)->argv);
		free(*cmd);

		*cmd = NULL;
	}
}

/*
 *void echoCommand(simpleCmd * cmd)
 *{
 *  char ** argp;
 *  
 *  if (com == NULL)
 *  {
 *    fprintf(stderr, "No command(null)\n\n");
 *    return;
 *  }
 *  fprintf(stderr, "Command lies at %p\n", com);
 *
 *  fprintf(stderr, "file: ");
 *  if (com->file == NULL)
 *    fprintf(stderr, "(null)\n");
 *  else
 *    fprintf(stderr, "%p \"%s\"\n", com->file, com->file);
 *
 *  fprintf(stderr, "bg: %s\n", (com->bg ? "YES" : "NO"));
 *
 *  fprintf(stderr, "argc: %d\n", com->argc);
 *
 *  if (com->argv == NULL)
 *    fprintf(stderr, "argv: (null)\n");
 *  else
 *  {
 *    fprintf(stderr, "argv: %p\n", com->argv);
 *    argp = com->argv;
 *    while (*argp != NULL)
 *    {
 *      fprintf(stderr, "%p \"%s\"\n", *argp, *argp);
 *      argp++;
 *    }
 *    fprintf(stderr, "(null)\n");
 *  }
 *
 *  if (com->list == NULL)
 *    fprintf(stderr, "list: (null)\n");
 *  else
 *  {
 *    fprintf(stderr, "list: %p\n", com->list);
 *    echoWordList(com->list);
 *  }
 *  fprintf(stderr, "\n\n");
 *}
 */

tCmd * genTCmd(simpleCmd * cmd)
{
	tCmd * tcmd;

	tcmd = (tCmd *)malloc(sizeof(tCmd));

	if (tcmd == NULL)
		return NULL;

	tcmd->cmdType = TCMD_SIMPLE;
	tcmd->modeBG = 0;
	tcmd->cmd = cmd;
	tcmd->child = NULL;
	tcmd->rel = NULL;

	return tcmd;
}

void genRelation(tCmd * from, int relType, tCmd * to)
{
	tRel * rel;

	if (from == NULL || to == NULL)
		return;

	rel = (tRel *)malloc(sizeof(tRel));

	rel->relType = relType;
	rel->next = to;

	from->rel = rel;
}

void delTCmd(tCmd ** cmd)
{
	if (cmd == NULL || *cmd == NULL)
		return;

	delCommand(&((*cmd)->cmd));
	delTCmd(&((*cmd)->child));

	if ((*cmd)->rel != NULL &&
		(*cmd)->rel->relType != TREL_END)
	{
		delTCmd(&((*cmd)->rel->next));
		free((*cmd)->rel);
	}

	free(*cmd);
	*cmd = NULL;
}

void echoIndent()
{
	int i;

	for (i = 0; i < echoCmdTreeIndentLevel; i++)
		printf("|  ");
}

void echoCmdTree(tCmd * root)
{
	echoCmdTreeIndentLevel = 0;

	echoCmdTreeNode(root);
}

void echoSimpleCmd(simpleCmd * cmd)
{
	char ** argp;

	if (cmd == NULL)
		return;

	echoIndent(); printf("%p: %s\n", cmd, cmd->file);
	argp = cmd->argv;
	while (*argp != NULL)
	{
		echoIndent(); printf("[%s]\n", *argp++);
	}
	if (cmd->rdrInputFile != NULL)
	{
		echoIndent(); printf("< %s\n", cmd->rdrInputFile);
	}
	if (cmd->rdrOutputFile != NULL)
	{
		echoIndent(); printf("%s %s\n",
			(cmd->rdrOutputAppend ? ">>": ">"),
			cmd->rdrOutputFile);
	}
}

void echoCmdTreeNode(tCmd * node)
{
	if (node == NULL)
		return;

	echoIndent(); printf("%p: %s %s\n", node,
		cmdTypeStr[node->cmdType],
		(node->modeBG ? " BG": ""));
	++echoCmdTreeIndentLevel;
	if (node->cmdType == TCMD_SIMPLE)
		echoSimpleCmd(node->cmd);
	else
		echoCmdTreeNode(node->child);
	--echoCmdTreeIndentLevel;

	if (node->rel == NULL ||
		node->rel->relType == TREL_END)
		return;

	echoIndent(); printf("|\n");
	echoIndent(); printf("|--(%s)\n",
		relTypeStr[node->rel->relType]);
	echoIndent(); printf("|\n");

	echoCmdTreeNode(node->rel->next);
}

/*
 * Getting command string from tree
 */

void putNodeStr(tCmd *, int, struct bufferlist *);
void putRelStr(tRel *, int, struct bufferlist *);
void putCmdStr(simpleCmd *, struct bufferlist *);

char * getCmdString(tCmd * node)
{
	struct bufferlist * buf = newBuffer();
	char * str;

	if (buf == NULL || node == NULL)
		return NULL;

	putNodeStr(node, TCMD_NONE, buf);
	str = flushBuffer(buf);
	free(buf);

	return str;
}

void putNodeStr(tCmd * node, int type,
	struct bufferlist * buf)
{
	if (node == NULL)
		return;

	switch (type)
	{
		case TCMD_NONE:
		case TCMD_PIPE:
		case TCMD_SCRIPT:
			if (node->cmdType == TCMD_SIMPLE)
				putCmdStr(node->cmd, buf);
			else
				putNodeStr(node->child, node->cmdType, buf);

			putRelStr(node->rel, type, buf);
			break;

		case TCMD_LIST:
			if (node->cmdType == TCMD_SIMPLE)
				putCmdStr(node->cmd, buf);
			else if (node->cmdType == TCMD_PIPE)
				putNodeStr(node->child, node->cmdType, buf);
			else if (node->cmdType == TCMD_LIST ||
				node->cmdType == TCMD_SCRIPT)
			{
				addChar(buf, '(');
				putNodeStr(node->child, node->cmdType, buf);
				addChar(buf, ')');
			}

			putRelStr(node->rel, type, buf);
			break;
	}
}

void putRelStr(tRel * rel, int type,
	struct bufferlist * buf)
{
	if (buf == NULL || rel == NULL)
		return;

	switch (rel->relType)
	{
		case TREL_PIPE:
			addStr(buf, " | ");
			break;
		case TREL_AND:
			addStr(buf, " && ");
			break;
		case TREL_OR:
			addStr(buf, " || ");
			break;
		case TREL_SCRIPT:
			addStr(buf, " ; ");
			break;
	}

	if (rel->relType != TREL_END)
		putNodeStr(rel->next, type, buf);
}

void putCmdStr(simpleCmd * cmd, struct bufferlist * buf)
{
	char ** pstr;

	if (cmd == NULL || buf == NULL)
		return;

	pstr = cmd->argv;

	while (*pstr != NULL)
	{
		addStr(buf, *pstr++);
		if (*pstr != NULL)
			addChar(buf, ' ');
	}

	if (cmd->rdrOutputFile != NULL)
	{
		if (cmd->rdrOutputAppend)
			addStr(buf, " >> ");
		else
			addStr(buf, " > ");
		addStr(buf, cmd->rdrOutputFile);
	}

	if (cmd->rdrInputFile != NULL)
	{
		addStr(buf, " < ");
		addStr(buf, cmd->rdrInputFile);
	}
}
