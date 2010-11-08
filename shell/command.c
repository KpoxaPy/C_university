#include <stdlib.h>
#include <stdio.h>
#include "command.h"

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

char ** genArgVector(lexList * list)
{
	Lex * lex;
	char ** argv;
	int i;

	if (list == NULL)
		return NULL;

	argv = (char **)malloc(sizeof(char *)*
		(list->count + 1));

	for (i = 0; i < list->count + 1; i++)
	{
		lex = getLex(list);

		if (lex == NULL)
			break;

		argv[i] = lex->str;
		free(lex);
	}

	argv[i] = NULL;

	return argv;
}

simpleCmd * newCommand()
{
	simpleCmd * cmd;

	cmd = (simpleCmd *)malloc(sizeof(simpleCmd));

	if (cmd == NULL)
		return NULL;

	cmd->file = NULL;
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
