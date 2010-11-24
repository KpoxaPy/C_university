#ifndef _BUFFER_H_
#define _BUFFER_H_

#include "main.h"

#define BUF_PIECE_SIZE 32

struct buffer {
	unsigned count;
	char str[BUF_PIECE_SIZE + 1];
	struct buffer * prev, * next;
};

struct bufferlist {
	struct buffer * first;
	struct buffer * last; 
	unsigned count;
};

struct bufferlist * newBuffer();
void clearBuffer(struct bufferlist *);
void extendBuffer(struct bufferlist *);
char * flushBuffer(struct bufferlist *);

void addChar(struct bufferlist *, char);
void addStr(struct bufferlist *, char *);
int getChar(struct bufferlist *);

#endif
