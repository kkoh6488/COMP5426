#include "debuggrid.h"
#include <stdio.h>

void print_grid(int** grid, int height, int width)
{
	for (int x = 0; x < height; x++)
	{
		for (int y = 0; y < width; y++)
		{
			printf("%d", grid[x][y]);
		}
		printf("\n");
	}
}

void print_array(int* arr, int size)
{
	for (int i = 0; i < size; i++)
	{
		printf("%d", arr[i]);
	}
	printf("\n");
}
