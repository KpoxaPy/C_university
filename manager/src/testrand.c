#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TESTS 10000000

int changeState(int o)
{
	static int lvl[5][4] = {
		{4, 8, 10, 11},
		{3, 7, 10, 11},
		{1, 4, 8, 11},
		{1, 2, 5, 9},
		{1, 2, 4, 8}
	};
	int r;
	int s;

	r = 1 + (int)(12.0*rand()/(RAND_MAX+1.0));

	for (s = 0; s < 4 && lvl[o][s] < r; s++)
		;

	return s;
}

int main()
{
	int sm[5][5];
	int sc[5];
	int i, j;

	memset(sm, 0, sizeof(int)*25);
	memset(sc, 0, sizeof(int)*5);

	for (j = 0; j < 5 ; j++)
		for (i = 0; i < TESTS; i++)
		{
			sc[j]++;
			sm[j][changeState(j)]++;
		}

	for (j = 0; j < 5; j++)
	{
		for (i = 0; i < 5; i++)
			printf("%f%s", sm[j][i]*(12.0/TESTS), (i == 4 ? "" : " "));
		printf("\n");
	}

	return EXIT_SUCCESS;
}
