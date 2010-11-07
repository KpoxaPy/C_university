#ifndef _WORDS_H_
#define _WORDS_H_

#include "lexer.h"

typedef struct lexListElem {
	Lex * lex;
	unsigned strlen;
	struct lexListElem * next;
} lexListElem;

typedef struct lexList {
	lexListElem * first;
	lexListElem * last;
	unsigned count;
} lexList;

lexList * newLexList();
void initLexList(lexList *);
void clearLexList(lexList *);
void addLex(lexList *, Lex *);
Lex * getLex(lexList *);

void echoLexList(lexList *);

#endif
