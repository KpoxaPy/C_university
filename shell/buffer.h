#ifndef _BUFFER_H_
#define _BUFFER_H_

#define BUF_PIECE_SIZE 32

struct buffer {
	unsigned count;
	char str[BUF_PIECE_SIZE + 1];
	struct buffer * next;
};

struct bufferlist {
	struct buffer * first;
	struct buffer * last; 
	unsigned count;
};

void clearBuffer(struct bufferlist *);
void extendBuffer(struct bufferlist *);
char * flushBuffer(struct bufferlist *);

void addChar(struct bufferlist *, char);

#endif
