#include <stdlib.h>
#include <stdio.h>

struct list {
  int num;
  struct list * next;
};

void addNum(struct list ** first, int num)
{
  struct list * tmp, *p, **prev;

  tmp = (struct list *)malloc(sizeof(struct list));
  tmp->num = num;

  if (*first == NULL)
  {
    tmp->next = NULL;
    *first = tmp;
  }
  else
  {
    prev = first;
    p = *prev;
    while ((p != NULL) && (p->num < num))
    {
      prev = &(p->next);
      p = p->next;
    }

    tmp->next = *prev;
    *prev = tmp;
  }
}

void printList(struct list * first)
{
  while (first != NULL)
  {
    printf("[%d]  ", first->num);
    first = first->next;
  }
}

int main()
{
  struct list * list = NULL;
  int c;

  while ((c = getchar()) != EOF)
    addNum(&list, c);
  
  printList(list);

  return 0;
}
