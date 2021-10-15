#include "global.h"
#include "helper.h"

int base_station(MPI_Comm world_comm, MPI_Comm comm, int nrows, int ncols, int num_of_iterations){
    FILE *logFile;
    int iteration, flag, world_size;
    int exit = FALSE;

    MPI_Comm_size(world_comm, &world_size); // size of the world communicator
    MPI_Status probe_status;
    MPI_Status status;
    MPI_Request broadcast_request;
    
    struct alertMessageStruct alert;
	MPI_Datatype alertMessageType;
    create_alert_message_type(alert, &alertMessageType);

    logFile = fopen("logFile.txt", "w");
    
    iteration = 0;
    while (iteration < num_of_iterations){
        // Check it there's any incoming message
		MPI_Iprobe(MPI_ANY_SOURCE, ALERT_TAG, world_comm, &flag, &probe_status);

        if (flag){  
            MPI_Recv(&alert, 1, alertMessageType, probe_status.MPI_SOURCE, ALERT_TAG, world_comm, &status);
            printf("alert message: %d  %.3f   %.3f \n", alert.rank, alert.random_height, alert.moving_average);
            printf("IN BASE STATION: Rank %d \n", alert.rank);

			fprintf(logFile, "Iteration: %d\n", iteration);
			fprintf(logFile, "Rank: %d\t\t\t\t\t\tCoordinates: (%d, %d) \n", alert.rank, alert.coordinates[0], alert.coordinates[1]);
            fprintf(logFile, "Random height: %.3f\t\tMoving Average: %.3f \n", alert.random_height, alert.moving_average);

            fprintf(logFile, "Adjacent Nodes\t\tCoord\t\t\tHeight(m)\t\t\tIPv4\n");
            for (int i = 0; i < 4; i++){
                if (alert.neighbours_rank[i] != -2){
                    fprintf(logFile, "%d\t\t\t\t\t(%d, %d)\t\t\t%.3f\t\t\t\tIPV4\n", 
                    alert.neighbours_rank[i], alert.neighbours_coordinates[i][0], alert.neighbours_coordinates[i][1], alert.neighbours_moving_average[i]);
                }
                
            }
            
            fprintf(logFile, "---------------------------------------------------------------------------------\n");

            fflush(stdout);
        }
        iteration += 1;
        sleep(1);
 

    }
    // Send termination message to all the other processor
    exit = TRUE;
    printf("Base should send exit message\n");
    fflush(stdout);
    MPI_Ibcast(&exit, 1, MPI_INT, world_size - 1, world_comm, &broadcast_request);
    MPI_Wait(&broadcast_request, MPI_STATUS_IGNORE);

	fprintf(logFile, "SUMMARY: \n");
    fclose(logFile);
    return 0;
}