#include "command.h"

struct command * genCommand(struct wordlist * words)
{
	struct command * cmd;

	if (words->count)
	{
		int i = 0;
		int bg;
		int arg_count = 0;
		struct word * wtmp = words->first;
		char * tmpStr;

		/*Analyzing for background execution*/
		wtmp = words->first;
		bg = 0;
		while (wtmp != NULL)
		{
			if (!bg && strcmp(wtmp->str, "&") == 0)
				bg = 1;
			else if (bg)
				return NULL;
			else
				++arg_count;
			
		wtmp = wtmp->next;
		}

		wtmp = words->first;
		cmd = (struct command *)malloc(sizeof(struct command));
		cmd->argv = (char **)malloc(sizeof(char *)*(arg_count + 1));

		while (wtmp != NULL)
		{
			if (strcmp(wtmp->str, "&") != 0)
			{
				tmpStr = (char *)malloc(sizeof(char)*(wtmp->len)+1);
				cmd->argv[i++] = strcpy(tmpStr, wtmp->str);
			}
			else
				break;

			wtmp = wtmp->next;
		}

		cmd->argc = i;
		cmd->argv[i] = NULL;
		cmd->file = cmd->argv[0];
		cmd->bg = bg;
		cmd->words = words;
	}
	else
		return NULL;

	return cmd;
}

void delCommand(struct command ** com)
{
	/*if (com != NULL)*/
		/*echoCommand(*com);*/

	if (com != NULL && *com != NULL)
	{
		char ** pStr = (*com)->argv;

		while (*pStr != NULL)
			free(*pStr++);

		free((*com)->argv);
		free(*com);

		*com = NULL;
	}
}

void echoCommand(struct command * com)
{
	char ** argp;
	
	if (com == NULL)
	{
		fprintf(stderr, "No command(null)\n\n");
		return;
	}
	fprintf(stderr, "Command lies at %p\n", com);

	fprintf(stderr, "file: ");
	if (com->file == NULL)
		fprintf(stderr, "(null)\n");
	else
		fprintf(stderr, "%p \"%s\"\n", com->file, com->file);

	fprintf(stderr, "bg: %s\n", (com->bg ? "YES" : "NO"));

	fprintf(stderr, "argc: %d\n", com->argc);

	if (com->argv == NULL)
		fprintf(stderr, "argv: (null)\n");
	else
	{
		fprintf(stderr, "argv: %p\n", com->argv);
		argp = com->argv;
		while (*argp != NULL)
		{
			fprintf(stderr, "%p \"%s\"\n", *argp, *argp);
			argp++;
		}
		fprintf(stderr, "(null)\n");
	}

	if (com->words == NULL)
		fprintf(stderr, "words: (null)\n");
	else
	{
		fprintf(stderr, "words: %p\n", com->words);
		echoWordList(com->words);
	}
	fprintf(stderr, "\n\n");
}
