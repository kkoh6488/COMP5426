#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include "debuggrid.h"

#define TRUE 1
#define FALSE 0

void board_init(int** grid, int size);

int malloc2darray(int ***array, int x, int y);

int main(char argc, char** argv)
{
	if ((argc + 0) != 5)
	{	
		printf("Required arguments missing");
		return -1;
	}
	MPI_Init(NULL, NULL);
	int rank, worldsize;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &worldsize);	

	int n 		= strtol(argv[1], NULL, 10);			// Grid size
	int t 		= strtol(argv[2], NULL, 10);			// Tile size
	float  c 	= atof(argv[3]);			// Terminating threshold
	int max_iters 	= strtol(argv[4], NULL, 10);		// Max iterations
	//int grid[n][n];
	int **grid;
	///int **grid = (int **) malloc (n * sizeof(int *));
	//for (int i = 0; i < n; i++)
	//{
	//	grid[i] = (int *)malloc(n * sizeof(int));
	//}		
	//grid = malloc(n * n * sizeof(grid));	

	malloc2darray(&grid, n, n);

	printf("Initializing board of size %d with tile size %d, threshold %f and max iterations %d\n", n, t, c, max_iters);

	if (rank == 0)
	{
		board_init(grid, n);		
	}
	
	// Init the local 2D array for this process
	int subgridsize = n / worldsize;
	int subgridsperproc = (n /  subgridsize) * (n / subgridsize)  / worldsize;		// The number of subgrids each process gets

	printf("subgrid size: %d, making local grid: %d \n", subgridsize, subgridsize * subgridsperproc);
	//int **localgrid = (int **) malloc (subgridsize  *  sizeof(int) * subgridsperproc);	
	//for (int i = 0; i < subgridsize * subgridsperproc; i++)
	//{
	//	localgrid[i] = (int *)malloc(subgridsize * sizeof(int));
	//}

	int **localgrid;
	malloc2darray(&localgrid, subgridsize, subgridsize); 

	/*
	for (int x = 0; x < subgridsize; x++)
	{
		for (int y = 0; y < subgridsize; y++)
		{
			localgrid[x][y] = 0;
		}
	}
*/

	// Create the MPI datatype for the subgrid
	int globalsizes[2] 	= { n, n };
	int subgridsizes[2]	= { subgridsize, subgridsize };
	int startindices[2] 	= { 0, 0 };

	printf("Datatype array sizes: %d, %d, %d \n", sizeof(globalsizes), sizeof(subgridsizes), sizeof(startindices));
	
	MPI_Datatype type, subgridtype;
	MPI_Type_create_subarray(2, globalsizes, subgridsizes, startindices, MPI_ORDER_C, MPI_INT, &type);
	MPI_Type_create_resized(type, 0, subgridsize * sizeof(int), &subgridtype);
	MPI_Type_commit(&subgridtype);

	// Send values to process subgrids;	
	int sendcounts[worldsize];					// The number of SENDTYPE items to send
	int displacements[worldsize];					// The displacement of an item from sendbuf
	printf("Subgrids per proc: %d \n", subgridsperproc);

	int *gridptr = NULL;
	if (rank == 0)
	{
		gridptr = &(grid[0][0]);
		int displ = 0;
		for (int i = 0; i < worldsize; i++)
		{
			sendcounts[i] = subgridsperproc;	
			displacements[i] = displ;
			displ += subgridsperproc;
		}
	}	
	
	printf("Localgrid size: %d, %d \n", sizeof(localgrid), sizeof(localgrid[0][0]));
	printf("Receiving %d items \n", subgridsperproc * (subgridsize * subgridsize));

	MPI_Scatterv(gridptr, sendcounts, displacements, subgridtype, &localgrid[0][0], subgridsperproc * (subgridsize * subgridsize), MPI_INT, 0, MPI_COMM_WORLD);

	for (int prank = 0; prank < worldsize; prank++)
	{
		if (prank == rank)
		{
			printf("Local process rank %d \n", rank);
			for (int x = 0; x < subgridsize; x++)
			{
				for (int y = 0; y < subgridsize; y++)
				{
					printf("%d ", localgrid[x][y]);
				}
				printf("\n");
			}
		}
	}

	//print_grid(grid, n);	
	MPI_Finalize();
}

int malloc2darray(int ***array, int x, int y)
{
	int *i = (int *) malloc (x * y * sizeof(int));
	if (!i)
	{
		return -1;
	}
	
	// Allocate the row pointers
	(*array) = malloc (x * sizeof(int*));
	if (!(*array))
	{
		free(i);
		return -1;
	}
	
	for (int a = 0; a < x; a++)
	{
		(*array)[a] = &(i[a * y]);
	}
	printf("Init grid OK\n");
	return 0;
}

void  board_init(int** grid, int size)
{
	printf("Initializing\n");
	float max = 1.0;
	for (int x = 0; x < size; x++)
	{
		for (int y = 0; y < size; y++)
		{
			float val = ((float)rand()/(float)(RAND_MAX)) * max;
			//printf("val is %f\n", val);
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
			printf("%d", grid[x][y]); 
		}
		printf("\n");							
	}			
}
