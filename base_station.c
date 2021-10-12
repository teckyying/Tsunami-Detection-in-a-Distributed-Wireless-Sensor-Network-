#include "global.h"

void base_station(MPI_Comm world_comm, MPI_Comm comm, int nrows, int ncols, int num_of_iterations){
    FILE *logFile;
    int iteration, alert, world_size;
    int alert_size = 11;
    float alert_data[alert_size];
    int exit = FALSE;

    MPI_Comm_size(world_comm, &world_size); // size of the world communicator
    MPI_Status status;
    MPI_Request broadcast_request;
    
    
    iteration = 0;
    while (iteration < num_of_iterations){
        // Check it there's any incoming message
		MPI_Iprobe(MPI_ANY_SOURCE, ALERT_TAG, world_comm, &alert, &status);

        if (alert){
            MPI_Recv(alert_data, 11, MPI_FLOAT, MPI_ANY_SOURCE, ALERT_TAG, world_comm, &status);
            printf("IN BASE STATION: Rank %d", (int)alert_data[0]);
        }
        else{
            printf("NOTHING");
        }
        iteration += 1;
        sleep(3);

    }
    // Send termination message to all the other processor
    exit = TRUE;
    MPI_Ibcast(&exit, 1, MPI_INT, world_size - 1, MPI_COMM_WORLD, &broadcast_request);

}