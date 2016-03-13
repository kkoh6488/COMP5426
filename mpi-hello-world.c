#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv)
{
	MPI_Init(NULL, NULL);

	// Get the number of proceses
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	
	// Get the rank of the process
	// WATCH for assigning the correct variable!
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	// Get the name of the processor
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	int name_len;
	MPI_Get_processor_name(processor_name, &name_len);
	
	// Print off hello world
	printf("Hellow world from processor %s, rank %d out of %d processes\n", processor_name, world_rank, world_size);
	
	int num;
	// If I am the master process
	if (world_rank == 0)
	{
		num = -1;
		// MPI_Send(void* buf, int count, MPI_Datatype, int destination, int tag, MPI_Comm communicator)
		MPI_Send(&num, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);	
		printf("Sent\n");
	}
	else if (world_rank == 1)
	{
		MPI_Recv(&num, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		printf("Process 1 received int with value %d from process 0\n",  num);	
	}
	MPI_Finalize();
}

