#include <debuggrid.h>
#include <stdio.h>

int print_grid(int** grid, int size)
{
	for (int x = 0; x < size; x++)
	{
		for (int y = 0; y < size; y++)
		{
			printf("&d", grid[x][y]);
		}
		printf("\n");
	}
}
