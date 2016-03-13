#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <debuggrid.h>

void board_init(int** grid, int size);

int main(char argc, char** argv)
{
	MPI_Init(NULL, NULL);
	int rank, worldsize;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &worldsize);
	
	int n = strtol(argv[0], NULL, 10);			// Grid size
	int t = strtol(argv[1], NULL, 10);			// Tile size
	float  c = atof(argv[2]);			// Terminating threshold
	int max_iters = strtol(argv[3], NULL, 10);		// Max iterations
	//int grid[n][n];
	int **grid;		
	grid = malloc(n * n * sizeof(int));	

	printf("Initializing board of size %d with tile size %d, threshold %f and max iterations %d", n, t, c, max_iters);
	board_init(grid, n);
	print_grid(grid, n);	
	MPI_Finalize();
}

void  board_init(int** grid, int size)
{
	srand( (unsigned)time( NULL));
	float max = 1.0;
	for (int x = 0; x < size; x++)
	{
		for (int y = 0; y < size; y++)
		{
			float val = ((float)rand()/(float)(RAND_MAX)) * max;
			if (val <= 0.33)
			{
				grid[x][y] = 0;
			}
			else if (val <= 0.66)
			{
				grid[x][y] = 1; 

			}
			else
			{
				grid[x][y] = 2;
			} 
		}					
	}			
}
