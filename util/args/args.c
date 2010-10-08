#include <stdio.h>

int main(int argc, char **argv)
{
  int i;
  
  if (argc == 1)
    printf("Использование: ./args [АРГУМЕНТЫ]\n"
           "Выводит полученные программой аргументы на выходной поток.\n");
  else
    for (i = 1; i < argc; i++)
      printf("[%s]\n", argv[i]);

  return 0;
}
