#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include "debuggrid.h"

void board_init(int** grid, int size);

int malloc2darray(int ***array, int x, int y);

void solveredturn(int **subgrid, int height, int width);

void solveblueturn(int **subgrid, int height, int width);

void setemptycells(int **subgrid, int height, int width, int intcolor); 

int free2darray(int ***array);

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
	//int subgridsperproc = (n /  worldsize);		// The number of subgrids each process gets

	//printf("subgrid size: %d, making local grid: %d \n", subgridsize, subgridsize * subgridsperproc);
	//int **localgrid = (int **) malloc (subgridsize  *  sizeof(int) * subgridsperproc);	
	//for (int i = 0; i < subgridsize * subgridsperproc; i++)
	//{
	//	localgrid[i] = (int *)malloc(subgridsize * sizeof(int));
	//}

	int **localgrid;

	// Divide up grid by rows for red turn - ie width of the grid stays the same
	malloc2darray(&localgrid, subgridsize, n); 

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
	int subgridsizesred[2]	= { subgridsize, n };
	int subgridsizesblue[2] = { n, subgridsize };
	int startindices[2] 	= { 0, 0 };

	//printf("Datatype array sizes: %d, %d, %d \n", sizeof(globalsizes), sizeof(subgridsizes), sizeof(startindices));
	printf("Subgrid sizes: %d %d \n", n, subgridsize);	
	
	MPI_Datatype redtype, bluetype, subgridtypered, subgridtypeblue, tiletype;
	MPI_Type_create_subarray(2, globalsizes, subgridsizesred, startindices, MPI_ORDER_C, MPI_INT, &redtype);
	MPI_Type_create_resized(redtype, 0, n  * subgridsize *  sizeof(int), &subgridtypered);
	MPI_Type_commit(&subgridtypered);

	MPI_Type_create_subarray(2, globalsizes, subgridsizesblue, startindices, MPI_ORDER_C, MPI_INT, &bluetype);
	MPI_Type_create_resized(bluetype, 0, n * subgridsize * sizeof(int), &subgridtypeblue);
	MPI_Type_commit(&subgridtypeblue); 
	
	// Send values to process subgrids;	
	int sendcounts[worldsize];					// The number of SENDTYPE items to send
	int displacements[worldsize];					// The displacement of an item from sendbuf
	//printf("Subgrids per proc: %d \n", subgridsperproc);

	int *gridptr = NULL;
	if (rank == 0)
	{
		gridptr = &(grid[0][0]);
		int displ = 0;
		for (int i = 0; i < worldsize; i++)
		{
			sendcounts[i] = 1;	
			displacements[i] = displ;
			displ += 1;
		}
		//displ += ((n / subgridsize) - 1) * (n / worldsize);
	}	
	
	printf("Localgrid size: %d, %d \n", sizeof(localgrid), sizeof(localgrid[0][0]));
	printf("Receiving %d items \n",  subgridsize * n);

	MPI_Scatterv(gridptr, sendcounts, displacements, subgridtypered, &localgrid[0][0], n *  subgridsize, MPI_INT, 0, MPI_COMM_WORLD);

	solveredturn(localgrid, subgridsize, n);
	/*
	for (int prank = 0; prank < worldsize; prank++)
	{
		if (prank == rank)
		{
			printf("Local process rank %d \n", rank);
			for (int x = 0; x < subgridsize; x++)
			{
				for (int y = 0; y < n;  y++)
				{
					printf("%d ", localgrid[x][y]);
				}
				printf("\n");
			}
		}
	}*/
	
	setemptycells(localgrid, subgridsize, n,  1);
	MPI_Gatherv(&localgrid[0][0], n * subgridsize, MPI_INT, gridptr, sendcounts, displacements, subgridtypered, 0, MPI_COMM_WORLD);
	
	/*
	// Now do blue cells
	free2darray(&localgrid);
	malloc2darray(&localgrid, n, subgridsize); 	
	MPI_Scatterv(gridptr, sendcounts, displacements, subgridtypeblue, &localgrid[0][0], n * subgridsize, MPI_INT, 0, MPI_COMM_WORLD);	
	solveblueturn(localgrid, n, subgridsize);
	setemptycells(localgrid, n, subgridsize, 2);
	MPI_Gatherv(&localgrid[0][0], n * subgridsize, MPI_INT, gridptr, sendcounts, displacements, subgridtypeblue, 0, MPI_COMM_WORLD);
	*/
	MPI_Finalize();

	if (rank == 0)
	{
		printf("Result grid \n");
		for (int x = 0; x < n; x++)
		{
			for (int y = 0; y < n;  y++)
			{
				printf("%d ", grid[x][y]);
			}
			printf("\n");
		}
	}
}		

void solveredturn(int **subgrid, int height, int width)
{
	for (int x = 0; x < height; x++)
	{
		for (int y = 0; y < width; y++)
		{
			if (subgrid[x][y] == 1)
			{
				if (y < width - 1)			// If this isn't the edge cell
				{
					if (subgrid[x][y + 1] == 0)	// If the cell to the right is white
					{
						subgrid[x][y + 1] = 3;
						subgrid[x][y] = 4; 	
					}
				}
				else
				{
					if (subgrid[x][0] == 0)
					{
						subgrid[x][0] = 3;	// Mark the new cell
						subgrid[x][y] = 4;	// Show this cell was left this turn
					}
				}	
			}
		} 
	}	
}


void solveblueturn(int **subgrid, int height, int width)
{
	for (int x = 0; x < height; x++)
	{
		for (int y = 0; y < width; y++)
		{
			if (subgrid[x][y] == 2)
			{
				if (x < height - 1)			// If this isn't the edge cell
				{
					if (subgrid[x + 1][y] == 0)	// If the cell below is white
					{
						subgrid[x + 1][y] = 3;
						subgrid[x][y] = 4; 	
					}
				}
				else
				{
					if (subgrid[0][y] == 0)		// Check the top row cell for wraparound
					{
						subgrid[0][y] = 3;
						subgrid[x][y] = 4;
					}
				}	
			}
		} 
	}
}

// Check if a given tile exceeds the threshold
void checktiles(int **tile, float threshold)
{
	
}

// Takes the subgrid and changes any "moved" values (ie 3 and 4) and turns it into a red or blue (ie 1 or 2).
// Done at the end of each half-turn.
void setemptycells(int **subgrid, int height, int width, int intcolor)
{
	for (int x = 0; x < height; x++)
	{
		for (int y = 0; y < width; y++)
		{
			if (subgrid[x][y] == 4)
			{
				subgrid[x][y] = 0;
			}
			else if (subgrid[x][y] == 3)
			{
				subgrid[x][y] = intcolor;
			}	
		} 
	}
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

int free2darray(int ***array)
{
	free(&((*array)[0][0]));
	free(*array);
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
