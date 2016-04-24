/* 
   white = 0, red = 1, blue = 2, 
   red or blue just moved in = 3 and 
   red or blue (in the first row or column) just moved out = 4
*/

int grid[n][n]; 	/* grid[row][col] */
bool finished = false;
int n_itrs = 0;
int redcount, bluecount;
int i, j;


...

while (!finished || n_itrs < MAX_ITRS){
   n_itrs++; 

   /* red color movement */
   for (i = 0; i < n; i++){
	if (grid[i][0] == 1 && grid[i][1] == 0){
		grid[i][0] = 4;
		grid[i][1] = 3;
	}
	for (j = 1; j < n; j++){
		if (grid[i][j] == 1 && (grid[i][(j+1)%n] == 0){
			grid[i][j] = 0;
			grid[i][(j+1)%n] = 3;
		}
		else if (grid[i][j] == 3)
			grid[i][j] = 1;
	}
	if (grid[i][0] == 3)
		grid[i][0] = 1;
	else if (grid[i][0] == 4)
		grid[i][0] = 0;
   }

   /* blue color movement */
   for (j = 0; j < n; j++){
	if (grid[0][j] == 1 && grid[1][j] == 0){
		grid[0][j] = 4;
		grid[1][j] = 3;
	}
	for (i = 1; i < n; i++){
		if (grid[i][j] == 2 && grid[(i+1)%n][j]==0){
			grid[i][j] = 0;
			grid[(i+1)%n][j] = 3;
		}
		else if (grid[i][j] == 3)
			grid[i][j] = 2;
	}
	if (grid[0][j] == 3)
		grid[0][j] = 2;
 	else if (grid[0][j] == 4)
		grid[0][j] = 0;
  }

   /* count the number of red and blue in each tile and check if the computation can be terminated*/

   ...

}
