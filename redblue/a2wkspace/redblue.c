#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include "debuggrid.h"
#include <math.h>
#include <time.h>
#include "redblueprocedure.h"

void board_init(int** grid, int size);
int malloc2darray(int ***array, int x, int y);
void setemptycells(int **subgrid, int height, int width, int intcolor); 
void setemptybuffercells(int *buf, int size, int color);
int free2darray(int ***array);
void updatetoprow(int *toprow, int *tempbuffer,  int size);
int counttiles(int **localgrid, int height, int width, int toprowindex, int tilesize, int tilesperrow, int numtiles, int maxcells);
void get2dprocdimensions(int *xdim, int *ydim, int worldsize);

int main(char argc, char** argv) {
	if ((argc + 0) != 5) {	
		printf("Required arguments missing");
		return -1;
	}

	MPI_Init(NULL, NULL);
	int rank, worldsize;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &worldsize);	

	int n 				= strtol(argv[1], NULL, 10);	// Grid size
	int t 				= strtol(argv[2], NULL, 10);	// Tile size
	float  c 			= atof(argv[3]);				// Terminating threshold
	int maxiters 		= strtol(argv[4], NULL, 10);	// Max iterations
	int curriter 		= 0;							// Current iteration
	int **grid;											// Master grid
	int **localgrid;									// For storing local grids
	int numtoexceedc	= (int)(t * t * c + 1);			// Cells to exceed the threshold
	int tilesperrow 	= n / t;
	int numtiles		= tilesperrow * tilesperrow;
	clock_t start, end;
	double elapsed;	
	
	malloc2darray(&grid, n, n);
	
	if (rank == 0) {
		printf("Initializing board of size %d with tile size %d, threshold %f and max iterations %d, num to exceed %d \n", n, t, c, maxiters, numtoexceedc);
		board_init(grid, n);		
	}
	
	start = clock();
	if (worldsize > 1 && t != n) {
		int mynumrows, mynumcols, xremaindertiles, yremaindertiles;

		// Max number of processes in each dimension
		int maxprocs = tilesperrow;

		// Get the size of the 2D torus in each dimension
		int xprocs, yprocs;
		xprocs = yprocs = 0;
		get2dprocdimensions(&xprocs, &yprocs, worldsize);

		// If the number of processes in x or y > number of tiles, limit the dimension.
		if (xprocs > maxprocs) {
			xprocs = maxprocs;
		}
		if (yprocs > maxprocs) {
			yprocs = maxprocs;
		}
		if (rank == 0) {
			printf("Sizes are: %d, %d. Max is %d\n", xprocs, yprocs, maxprocs);
		}

		if (worldsize > tilesperrow * tilesperrow) {
			xremaindertiles = yremaindertiles = 0;
		}
		else {
			xremaindertiles = tilesperrow % xprocs;
			yremaindertiles = tilesperrow % yprocs;
		}
		
		// Make the active process communicator
		MPI_Comm activecomm;
		int color = rank / (xprocs * yprocs);
		MPI_Comm_split(MPI_COMM_WORLD, color, rank, &activecomm);

		if (rank >= xprocs * yprocs) {							// Quit if the process isn't needed
			printf("Unused process %d, exiting...\n", rank);
			MPI_Finalize();
			exit(0);
		}
		
		// Make the 2D torus topology
		MPI_Comm cartcomm;
		int dims[2] 	= { xprocs, yprocs };
		int period[2] 	= { 1, 1 };
		int reorder 	= 0;
		MPI_Cart_create(activecomm, 2, dims, period, reorder, &cartcomm);
		
		int grank, gsize;
		int mycoords[2];

		MPI_Comm_rank(cartcomm, &grank);
		MPI_Comm_size(cartcomm, &gsize);
		MPI_Cart_coords(cartcomm, grank, 2, mycoords);
		printf("mycoords for rank %d: %d, %d\n", grank, mycoords[0], mycoords[1]);
		int mycoordx = mycoords[0];
		int mycoordy = mycoords[1];
		
		//MPI_Finalize();
		//return;

		int xtilesperproc			= tilesperrow / xprocs;			// How many tiles per proc in x dimension?
		int ytilesperproc			= tilesperrow / yprocs;

		int xprocswithextratiles 	= xremaindertiles % xprocs;		// How many processes get extra tiles in x dim?
		int yprocswithextratiles 	= yremaindertiles % yprocs;		// How many in y dim?
		int rowsperproc				= xtilesperproc * t;			// Translate tiles into row counts
		int colsperproc				= ytilesperproc * t;

		// Assign the rows for this process
		if (mycoordx < xprocswithextratiles) {
			mynumrows = (xtilesperproc + xremaindertiles / xprocswithextratiles) * t;
		} else {
			mynumrows = xtilesperproc * t;
		}

		// Assign the columns for this process
		if (mycoordy < yprocswithextratiles) {
			mynumcols = (ytilesperproc + yremaindertiles / yprocswithextratiles) * t;
		} else {
			mynumcols = ytilesperproc * t;
		}

		malloc2darray(&localgrid, mynumrows, mynumcols);
		if (grank == 0) {
			for (int x = 0; x < mynumrows; x++) {
				for (int y = 0; y < mynumcols; y++) {
					localgrid[x][y] = grid[x][y];
				}
			}			
			int dest = -1;
			int xdest = 0, ydest = 0, sendcols = 0;
			int counter = 0;
			//int colstarttile = mynumcols / t;			// How many tiles did the master send?
			int colproc = 0, rowproc = 0;
			int destcoords[2];
			printf("Procs with extra rows %d, %d \n", xprocswithextratiles, yprocswithextratiles);

			for (int x = 0; x < n; x++) {						//Send rows from master to worker processes
				for (int y = 0; y < n;) {
					int tilecolindex = y / t;			// Get the index of this tile columnwise
					int tilerowindex = x / t;			

					printf("Tile indices for [%d, %d]: %d, %d\n", x, y, tilerowindex, tilecolindex);
					if (y < mynumcols * yprocswithextratiles) {
						colproc = y / mynumcols;
						sendcols = mynumcols;
					} else {
						colproc = (y - (mynumcols * yprocswithextratiles)) / colsperproc;
						colproc += yprocswithextratiles;
						sendcols = colsperproc;
					}
					if (tilerowindex < mynumrows * xprocswithextratiles) {
						rowproc = tilerowindex * t / mynumrows;
					} 
					else {
						rowproc = (tilerowindex * t - (mynumrows * xprocswithextratiles)) / rowsperproc;
						rowproc += xprocswithextratiles;
					}	

					destcoords[0] = rowproc;
					destcoords[1] = colproc;
					MPI_Cart_rank(cartcomm, destcoords, &dest);
					printf("owner proc for tile %d,%d is: [%d, %d] = rank %d\n", tilerowindex, tilecolindex, rowproc, colproc, dest);					
					if (dest == 0) {
						y += sendcols;
						continue;
					}
					MPI_Send(&grid[x][y], sendcols, MPI_INT, dest, 0, activecomm);
					y += sendcols;
				}
				printf("\n");
			}
		}
		else {
			for (int x = 0; x < mynumrows; x++) {
				MPI_Recv(&localgrid[x][0], mynumcols, MPI_INT, 0, 0, activecomm, MPI_STATUS_IGNORE); 
			}
		}

		int* temprightbuffer = (int*) malloc (mynumrows * sizeof (int));
		int* rightbuffer = (int*) malloc (mynumrows * sizeof(int));
		int* tempbotbuffer =  (int*) malloc (mynumcols * sizeof (int));
		int* botbuffer =  (int*) malloc (mynumcols * sizeof (int)); 
		int src, dest;	

		while (curriter < maxiters) {	
			solveredturn(localgrid, mynumrows, mynumcols);
			setemptycells(localgrid, mynumrows, mynumcols,  1);
			// For each process, get the row number it needs for the bottom buffer
			// Each process sends its top row to the previous process bot row
			for (int i = 0; i < gsize; i++) {
				if (grank == i) {
					if (grank == 0) {
						dest = gsize - 1;
						src = grank + 1;
					}
					else if (grank == gsize - 1) {
						dest = grank - 1;
						src = 0;
					}
					else {
						dest = grank - 1;
						src = grank + 1; 
					}
					MPI_Sendrecv(&localgrid[0][0], mynumcols, MPI_INT, dest, 1, botbuffer, mynumcols, MPI_INT, src, 1, activecomm, MPI_STATUS_IGNORE);
				}					
			}	
			solveblueturn(localgrid, botbuffer, mynumrows, mynumcols);
			for (int i = 0; i < gsize; i++) {
				if (grank == i) {	
					// Flip the src and dest - send to the src and recv from the dest as we are sending the bot buffer back		
					MPI_Sendrecv(botbuffer, mynumcols, MPI_INT, src, 2, tempbotbuffer, mynumcols, MPI_INT, dest, 2, activecomm, MPI_STATUS_IGNORE);
				}
			}
			updatetoprow(&localgrid[0][0], tempbotbuffer,  mynumcols);
			setemptycells(localgrid, mynumrows, mynumcols, 2);
			setemptybuffercells(botbuffer, mynumcols, 2);
			
			// Now check if tiles exceed c. If not, proceed with the next iteration.
			int tileresult 		= 0;
			int toprowindex 	= -1;				// The index in the whole grid of the top local row
			int allresult 		= 0;

			// Get the index of the top row
			if (grank >= xprocswithextratiles) {
				toprowindex = (xremaindertiles * t) + (grank * xtilesperproc * t);
			}
			else {
				toprowindex = grank * xprocswithextratiles * (xremaindertiles + t);
			}
			tileresult = counttiles(localgrid, mynumrows, mynumcols, toprowindex, t, tilesperrow, numtiles, numtoexceedc);
			MPI_Allreduce(&tileresult, &allresult, 1, MPI_INT, MPI_MIN, activecomm);
			if (allresult == -1) {
				break;
			}
			curriter++;
		}
		printf("Grid for process %d\n", rank);
		print_grid(localgrid, mynumrows, mynumcols);
	}
	else
	{
		if (rank > 0) {
			MPI_Finalize();
			exit(0);
		}
		while (curriter < maxiters) {
			solveredturn(grid, n, n);
			setemptycells(grid, n, n, 1);
			solveblueturn(grid, NULL, n, n);
			setemptycells(grid, n, n, 2);
			if (counttiles(grid, n, n, 0, t, tilesperrow, numtiles, numtoexceedc) == -1) {
				break;
			}
			curriter++;	
		}
		printf("Final grid \n============ \n");
		print_grid(grid, n, n);
	}
	end = clock();
	elapsed = (double)(end - start) / CLOCKS_PER_SEC;
	printf("Execution time for p%d: %f\n", rank, elapsed);
	MPI_Finalize();	
}

void get2dprocdimensions(int *xdim, int *ydim, int worldsize) {

	for (int size = worldsize; size > 0; size--) {
		int numfactors = 0, lastfactor = -1;
		// Get the factors of np
		for (int i = 1; i <= worldsize; i++) {
			if (worldsize % i == 0) {
				if (i * i == worldsize) {
					*xdim = i;
					*ydim = i;
					return;
				} else {
					numfactors++;
					if (i * lastfactor == worldsize) {
						*xdim = i;
						*ydim = lastfactor;
						return;
					}
					lastfactor = i;
				}
				printf("%d ", i);
			}
		}
		printf("\n");
	}
}

		
/* Checks the row buffer to see if any new values should be updated 
	for the top row in this process.
 */
void updatetoprow(int *toprow, int *tempbuffer, int size) {
	for (int i = 0; i < size; i++) {
		if (tempbuffer[i] == 3) {
			toprow[i] = 2;
		}	
	}
}

/* Counts the number of cells in each tile, checking if it exceeds the threshold. */
int counttiles(int **localgrid, int height, int width, int toprowindex, int tilesize, int tilesperrow, int numtiles, int maxcells) {
	
	int* numred				= (int*) malloc (numtiles * sizeof(int));
	int* numblue			= (int*) malloc (numtiles * sizeof(int));

	// Zero out arrays - just in case to get rid of old values
	for (int i = 0; i < numtiles; i++) {
		numred[i] = 0;
		numblue[i] = 0;
	}
	int result = 0;
	for (int x = 0; x < height; x++) {
		int rowindex = toprowindex + x;
		for (int y = 0; y < width; y++) {
			int tilenum = (rowindex / tilesize) *  tilesperrow + (y / tilesize);
			if (localgrid[x][y] == 1) {
				numred[tilenum]++;
			}
			else if (localgrid[x][y] == 2) {
				numblue[tilenum]++;
			}
			if ((x + 1) % tilesize == 0 && (y + 1) % tilesize == 0) {
				if (numred[tilenum] >= maxcells || numblue[tilenum] >= maxcells) {
					printf("Tile %d exceeded max @ red:%d, blue:%d\n", tilenum, numred[tilenum], numblue[tilenum]);
					result = -1;
				} 
			}
		}
	}
	return result;
}

/* Takes the subgrid and changes any "moved" values (ie 3 and 4) and turns it into an empty cell (ie 0) or a cell of the given color respectively.
 Done at the end of each half-turn. */
void setemptycells(int **subgrid, int height, int width, int intcolor) {
	for (int x = 0; x < height; x++) {
		for (int y = 0; y < width; y++) {
			if (subgrid[x][y] == 4) {
				subgrid[x][y] = 0;
			}
			else if (subgrid[x][y] == 3) {
				subgrid[x][y] = intcolor;
			}	
		} 
	}
}

/* Clears the buffer of moved cell flags. */
void setemptybuffercells(int *buf, int size, int color) {
	for (int x = 0; x < size; x++) {
		if (buf[x] == 4) {
			buf[x] = 0;
		}
		else if (buf[x] == 3) {
			buf[x] = color;
		}
	}
}

/* Allocates memory for a 2D array. */
int malloc2darray(int ***array, int x, int y) {
	int *i = (int *) malloc (x * y * sizeof(int));
	if (!i) {
		return -1;
	}
	// Allocate the row pointers
	(*array) = malloc (x * sizeof(int*));
	if (!(*array)) {
		free(i);
		return -1;
	}
	for (int a = 0; a < x; a++) {
		(*array)[a] = &(i[a * y]);
	}
	return 0;
}

int free2darray(int ***array) {
	free(&((*array)[0][0]));
	free(*array);
	return 0;
}

/* Initialises values for the grid randomly */
void  board_init(int** grid, int size) {
	float max = 1.0;
	srand(time(NULL));			// Set the random number seed
	for (int x = 0; x < size; x++) {
		for (int y = 0; y < size; y++) {
			float val = ((float)rand()/(float)(RAND_MAX)) * max;
			if (val <= 0.33) {
				grid[x][y] = 0;
			}
			else if (val <= 0.66) {
				grid[x][y] = 1; 
			}
			else {
				grid[x][y] = 2;
			}
			printf("%d", grid[x][y]); 
		}
		printf("\n");							
	}			
}
