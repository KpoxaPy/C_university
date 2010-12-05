#include "buffer.h"

struct bufferlist * newBuffer()
{
	struct bufferlist * buf;

	buf = (struct bufferlist *)malloc(sizeof(struct bufferlist));
	if (buf != NULL)
	{
		buf->first = buf->last = NULL;
		buf->count = 0;
	}

	return buf;
}

void addChar(struct bufferlist * buffer, char c)
{
	if (buffer->last == NULL)
		extendBuffer(buffer);
	if (buffer->last->count >= (BUF_PIECE_SIZE))
		extendBuffer(buffer);
	buffer->last->str[buffer->last->count] = c;
	buffer->last->str[buffer->last->count + 1] = '\0';
	++buffer->last->count;
	++buffer->count;
}

void addStr(struct bufferlist * buffer, char * str)
{
	if (str == NULL || buffer == NULL)
		return;

	while (*str != '\0')
		addChar(buffer, *str++);
}

int getChar(struct bufferlist * buffer)
{
	char c;

	if (buffer->count == 0)
		return EOF;
	else
	{
		c = buffer->last->str[buffer->last->count - 1];
		buffer->last->str[buffer->last->count - 1] = '\0';

		--buffer->last->count;
		--buffer->count;

		if (buffer->last->count == 0)
		{
			struct buffer * toRem = buffer->last;

			if (buffer->count != 0)
				buffer->last = toRem->prev;
			else
				buffer->last = buffer->first = NULL;

			free(toRem);
		}
	}
	
	return c;
}

void clearBuffer(struct bufferlist * buffer)
{
	struct buffer * tmp;

	while (buffer->first != NULL)
	{
		tmp = buffer->first->next;
		free(buffer->first);
		buffer->first = tmp;
	}

	buffer->last = NULL;
	buffer->count = 0;
}

char * flushBuffer(struct bufferlist * buffer)
{
	char * str; 
	struct buffer * tmp; 
	unsigned i, j;

	str = (char *)malloc(sizeof(char)*(buffer->count + 1));
	tmp = buffer->first;
	j = 0;

	while (tmp != NULL)
	{
		for (i = 0; i < tmp->count; i++)
			str[j++] = tmp->str[i];
		tmp = tmp->next;
	}
	str[j] = '\0';

	clearBuffer(buffer);

	return str;
}

void extendBuffer(struct bufferlist * buffer)
{
	struct buffer * tmp;

	tmp = (struct buffer *)malloc(sizeof(struct buffer));
	tmp->count = 0;
	tmp->str[0] = '\0';

	if (buffer->first == NULL)
	{
		tmp->next = NULL;
		tmp->prev = NULL;
		buffer->first = tmp;
		buffer->last = tmp;
	}
	else
	{
		tmp->next = buffer->last->next;
		tmp->prev = buffer->last;
		buffer->last->next = tmp;
		buffer->last = tmp;
	}
}
