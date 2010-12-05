#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "words.h"

void initWordList(struct wordlist * words)
{
	words->first = NULL;
	words->last = NULL;
	words->count = 0;
}

void clearWordList(struct wordlist * words)
{
	struct word * tmp;

	while (words->first != NULL)
	{
		tmp = words->first->next;
		free(words->first->str);
		free(words->first);
		words->first = tmp;
	}

	words->last = NULL;
	words->count = 0;
}

void addWord(struct wordlist * words, char * str)
{
	struct word * tmp;

	tmp = (struct word *)malloc(sizeof(struct word));
	tmp->str = str;
	tmp->len = strlen(str);

	if (words->first == NULL)
	{
		tmp->next = NULL;
		words->first = tmp;
		words->last = tmp;
	}
	else
	{
		tmp->next = words->last->next;
		words->last->next = tmp;
		words->last = tmp;
	}

	words->count++;
}

void echoWordList(struct wordlist * words)
{
	struct word * tmp;

	tmp = words->first;

	while (tmp != NULL)
	{
		printf("[%s]\n", tmp->str);
		tmp = tmp->next;
	}
}
