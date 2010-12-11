#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lexlist.h"

lexList * newLexList()
{
	lexList * list = (lexList *)malloc(sizeof(lexList));

	initLexList(list);

	return list;
}

void initLexList(lexList * list)
{
	if (list == NULL)
		return;

	list->first = NULL;
	list->last = NULL;
	list->count = 0;
}

void clearLexList(lexList * list)
{
	lexListElem * tmp;

	if (list == NULL)
		return;

	while (list->first != NULL)
	{
		tmp = list->first->next;
		delLex(list->first->lex);
		free(list->first);
		list->first = tmp;
	}

	list->last = NULL;
	list->count = 0;
}

void addLex(lexList * list, Lex * lex)
{
	lexListElem * tmp;

	tmp = (lexListElem *)malloc(sizeof(lexListElem));
	tmp->lex = lex;
	
	if (lex != NULL)
	{
		if (lex->str != NULL)
			tmp->strlen = strlen(lex->str);
		else
			tmp->strlen = 0;
	}

	if (list->first == NULL)
	{
		tmp->next = NULL;
		list->first = tmp;
		list->last = tmp;
	}
	else
	{
		tmp->next = list->last->next;
		list->last->next = tmp;
		list->last = tmp;
	}

	++list->count;
}

Lex * getLex(lexList * list)
{
	Lex * lex;
	lexListElem * tmp;

	if (list == NULL)
		return NULL;

	if (list->count == 0)
		return NULL;

	lex = list->first->lex;
	tmp = list->first;
	list->first = list->first->next;
	free(tmp);

	--list->count;

	return lex;
}

void echoLexList(lexList * list)
{
	lexListElem * tmp;

	tmp = list->first;

	while (tmp != NULL)
	{
		if (tmp->lex != NULL)
		{
			if (tmp->lex->str != NULL)
				printf(" %s:[%s]\n",
					lexTypeStr[tmp->lex->type],
					tmp->lex->str);
			else
				printf(" %s\n", lexTypeStr[tmp->lex->type]);
		}
		else
			printf("-Empty lex-\n");
		tmp = tmp->next;
	}
}
