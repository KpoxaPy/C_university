#ifndef _BUFFER_H_
#define _BUFFER_H_

#define BUF_PIECE_SIZE 32

struct buffer {
	unsigned count;
	char str[BUF_PIECE_SIZE + 1];
	struct buffer * prev, * next;
};

typedef struct bufferlist {
	struct buffer * first;
	struct buffer * last;
	unsigned count;
} Buffer;

struct bufferlist * newBuffer();
void clearBuffer(struct bufferlist *);
void extendBuffer(struct bufferlist *);
char * flushBuffer(struct bufferlist *);

void addChar(struct bufferlist *, char);
void addStr(struct bufferlist *, char *);
void addnStr(struct bufferlist *, void *, int);
int getChar(struct bufferlist *);

#endif
