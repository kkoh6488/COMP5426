#include <stdio.h>
#include <mpi.h>

int main()
{
	MPI_Init(NULL, NULL);
	int rank, size;
	MPI_Comm_size(MPI_COMM_WORLD, &rank);
	MPI_Comm_rank(MPI_COMM_WORLD, &size);

	int buf[4];
	buf[0] = buf[3] = -1;
	buf[1] = rank;
	buf[2] = rank + 1;

	if (rank < (size - 1) && rank != 0)
	{
		MPI_Sendrecv(&buf[2], 1,  MPI_INT, rank + 1, 0, buf[0], 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
	else
	{
		if (rank == 0)
		{
			MPI_Send(&buf[1], 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
		}
		else if (rank == size - 1)
		{
			MPI_Recv(&buf[0], 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}	
	}

	printf("Buf of %d is : %d %d %d %d \n", rank, buf[0], buf[1], buf[2], buf[3]);	

	MPI_Finalize();

}

