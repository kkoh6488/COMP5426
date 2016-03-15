#include "redblueprocedure.h"

void solveredturn(int **subgrid, int height, int width)
{
	for (int x = 0; x < height; x++)
	{
		for (int y = 0; y < width; y++)
		{
			if (subgrid[x][y] == 1)				// If this cell is red
			{
				if (y < width - 1)			// If this isn't the right edge cell
				{
					if (subgrid[x][y + 1] == 0)	// If the cell to the right is white
					{
						subgrid[x][y + 1] = 3;	// Mark it as just moved in
						subgrid[x][y] = 0; 	
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

void solveblueturn(int **subgrid, int *botbuffer, int height, int width)
{
	for (int x = 0; x < height; x++)
	{
		for (int y = 0; y < width; y++)
		{
			if (subgrid[x][y] == 2)
			{
				if (x < height - 1)			// If this isn't the bottom edge cell
				{
					if (subgrid[x + 1][y] == 0)	// If the cell below is white
					{
						subgrid[x + 1][y] = 3;
						subgrid[x][y] = 0; 	
					}
				}
				else					// This row is the bottom row of the localgrid
				{
					if (!botbuffer)
					{
						
						if (subgrid[0][y] == 0)		// Check the top row cell for wraparound
						{
							subgrid[0][y] = 3;
							subgrid[x][y] = 4;
						}	
					}
					else
					{
						if (botbuffer[y] == 0)
						{
							subgrid[x][y] = 4;
							botbuffer[y] = 3;
						}
					}
				}	
			}
		} 
	}
}

