/**
 * mpicc -Wall -o main.o main.c base_station.c sensor_node.c
 * mpirun -np 5 main.o 2 2 6000 20
 * mpirun -oversubscribe -np 10 main.o 3 3 6000 20
 *  mpirun -np 2 main.o
 * */

#include "global.h"

int main(int argc, char *argv[]) {
    int rank, size;
	int nrows, ncols;
	int threshold;
	int num_of_iterations;
	MPI_Comm comm;
	
	/* Initialize MPI environment */
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	if (argc == 5) {
		nrows = atoi (argv[1]);
		ncols = atoi (argv[2]);
		threshold = atoi (argv[3]);
		num_of_iterations = atoi(argv[4]);
		if( (nrows * ncols) != size - 1) {
			if (rank == 0){
                printf("ERROR: nrows*ncols)= %d*%d = %d != %d\nnrows*ncols should be one lesser than the number of processors\n", nrows, ncols, nrows * ncols, size - 1);
            } 
			if (threshold < MIN_HEIGHT || threshold > MAX_HEIGHT){
				printf("ERROR: Threshold should be between %d and %d", MIN_HEIGHT, MAX_HEIGHT);
			}
			MPI_Finalize(); 
			return 0;
		}
	} else {
		nrows = ncols = (int)sqrt(size - 1);
		threshold = DEFAULT_THRESHOLD;
	}
	
	MPI_Comm_split(MPI_COMM_WORLD, rank == size - 1, 0, &comm);
	
	if (rank == size - 1){
        base_station(MPI_COMM_WORLD, comm, nrows, ncols, num_of_iterations);
    }
	else
		sensor_node(MPI_COMM_WORLD, comm, nrows, ncols, threshold);
	

    // MPI_Comm_free(&comm);
	MPI_Finalize();
	return 0;
}
