#include "mes.h"

Queue queue = {NULL, NULL, 0};

void sendMessage(Message * mes)
{
	mMessage * mm = (mMessage *)malloc(sizeof(mMessage));

	mm->mes = (Message *)malloc(sizeof(Message));
	memcpy(mm->mes, mes, sizeof(Message));

	mm->next = queue.in;
	mm->prev = NULL;
	if (queue.in != NULL)
		(queue.in)->prev = mm;
	else
		queue.out = mm;
	queue.in = mm;
	++queue.count;
}

int getMessage(Message * mes)
{
	mMessage * mm;

	if (queue.count == 0)
		return -1;

	mm = queue.out;
	memcpy(mes, mm->mes, sizeof(Message));
	free(mm->mes);

	if (mm->prev != NULL)
		mm->prev->next = NULL;
	else
		queue.in = NULL;
	queue.out = mm->prev;
	free(mm);
	--queue.count;

	return 0;
}
