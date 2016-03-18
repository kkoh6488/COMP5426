#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include "debuggrid.h"

#define TRUE 1
#define FALSE 0

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
	
	malloc2darray(&grid, n, n);
	
	if (rank == 0)
	{
		printf("Initializing board of size %d with tile size %d, threshold %f and max iterations %d\n", n, t, c, max_iters);
		board_init(grid, n);		
	}
	
	if (worldsize > 1)
	{
		// Init the local 2D array for this process
		int rowsperproc = n / worldsize;
		int extrarowsforproc = n % worldsize;		
		int mynumrows;		
		int procswithextrarows = n - worldsize;		

		// Local arrays
		int **localgrid;
		
		// Assign the rows for this process
		// Split up the rows evenly
		/*
		if (rank == 0)
		{
			mynumrows = rowsperproc + extrarowsforproc;	
		}
		else
		{
			mynumrows = rowsperproc;	
		}
		*/
		if (rank < procswithextrarows)
		{
			mynumrows = rowsperproc + extrarowsforproc / procswithextrarows;
		}
		else
		{
			mynumrows = rowsperproc;
		}

		malloc2darray(&localgrid, mynumrows, n);
		
		if (rank == 0)
		{
			printf("Master num rows %d \n", mynumrows);
			for (int x = 0; x < mynumrows; x++)
			{
				for (int y = 0; y < n; y++)
				{
					localgrid[x][y] = grid[x][y];
					printf("%d ", localgrid[x][y]);		
				}
				printf("\n");
			}			
				
			int dest = 1;
			int counter = 0;
			//Send rows from master to worker processes
			for (int x = mynumrows; x < n; x++)
			{
				MPI_Send(&grid[x][0], n, MPI_INT, dest, 0, MPI_COMM_WORLD);
				counter++;
				if (dest < procswithextrarows)
				{
					if (counter == mynumrows)
					{
						dest++;
						counter = 0;
					}
				} 
				else if (counter == rowsperproc)
				{
					dest++;
					counter = 0;
				}	
			}
		}
		else
		{
			for (int x = 0; x < mynumrows; x++)
			{
				MPI_Recv(&localgrid[x][0], n, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
			}
			
			printf("Proc %d \n", rank);	
			for (int x = 0; x < mynumrows; x++)
			{
				for (int y = 0; y < n; y++)
				{
					printf("%d ", localgrid[x][y]);
				}
				printf("\n");
			}	
		}

		solveredturn(localgrid, mynumrows, n);
		setemptycells(localgrid, mynumrows, n,  1);
		
		int* topbuffer =  (int*) malloc ((n + 1) * sizeof (int));
		int* botbuffer =  (int*) malloc ((n + 1) * sizeof (int)); 

		//printf("Len of botbuffer: %d \n", (n * sizeof(int)));		

	
		// For each process, get the row number it needs for the bottom buffer
		// Each process sends its top row to the previous process bot row
		for (int i = 0; i < worldsize; i++)
		{
			if (rank == i)
			{
				if (i == 0)
				{
					MPI_Sendrecv(&localgrid[0][0], n, MPI_INT, worldsize - 1, 0, botbuffer, n, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					//MPI_Send(&localgrid[0][0], n, MPI_INT, 1, 0, MPI_COMM_WORLD);
					printf("Sent from master\n");
					
				}
				else if (i == worldsize - 1)
				{
					//MPI_Recv(botbuffer, n, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					//int startrow = mynumrows * rank + extrarowsforproc;
					//printf("Sending last proc, starting from row %d \n", startrow);
					printf("Sending from rank %d to %d, %d ints\n", rank, rank - 1, n);
					MPI_Sendrecv(&localgrid[0][0], n, MPI_INT, rank - 1, 0, botbuffer, n, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					for (int a = 0; a < n ; a++)
					{
						printf("%d ", botbuffer[a]);
					}
					printf("\n");
				}
				else
				{
					//int startrow = mynumrows * rank + extrarowsforproc;
					MPI_Sendrecv(&localgrid[0][0], n, MPI_INT, rank - 1, 0, botbuffer, n, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				}
			}					
		}	
		printf("Received rows: proc %d \n", rank);			
		/*
		free(botbuffer);
		free(topbuffer);
		free2darray(&localgrid);
		if (rank == 0)
		{
			free2darray(&grid);
		}
		printf("Freed memory \n");
		*/	
	}
	else
	{
		
	}

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
	
	/*
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
	*/
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

// Takes the subgrid and changes any "moved" values (ie 3 and 4) and turns it into an empty cell (ie 0) or a cell of the given color respectively.
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
	//printf("Init grid OK\n");
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
