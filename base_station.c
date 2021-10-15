#include "global.h"
#include "queue.h"
#include "helper.h"

struct Queue* head_node = NULL;   // first element for moving average
struct Queue* current_node = NULL;   // current element aka the last element for moving average
int queue_count =0;    // ensure the number of node list doesnt exceed the moving average window


void *ThreadFunc(void *pArg);
int shutdown;

int base_station(MPI_Comm world_comm, MPI_Comm comm, int nrows, int ncols, int num_of_iterations,int threshold){
    FILE *logFile;
    int iteration, flag, world_size;
    // shutdown = FALSE;
    int exit = FALSE;
    double alert_received_time;
    int *receive_count= NULL;
    char loggedTime[25];
    
    pthread_t tid[1];	// create pthreads
    int argArray[3];
    argArray[0]=nrows;
    argArray[1]=ncols;
    argArray[2]=threshold;

    MPI_Comm_size(world_comm, &world_size); // size of the world communicator
    MPI_Status probe_status;
    MPI_Status status;

    receive_count = (int*)malloc((world_size-1) * sizeof(int));
    for (int i = 0; i < world_size - 1; i++){
        receive_count[i] = 0;
    }

    struct alertMessageStruct alert;
	MPI_Datatype alertMessageType;
    create_alert_message_type(alert, &alertMessageType);

    logFile = fopen("logFile.txt", "w");

    iteration = 0;
    pthread_create(&tid[0], 0, ThreadFunc, argArray);
    while (iteration < num_of_iterations){
        // Check it there's any incoming message
		MPI_Iprobe(MPI_ANY_SOURCE, ALERT_TAG, world_comm, &flag, &probe_status);

        if (flag){  
            MPI_Recv(&alert, 1, alertMessageType, probe_status.MPI_SOURCE, ALERT_TAG, world_comm, &status);
            alert_received_time = MPI_Wtime();
            receive_count[status.MPI_SOURCE] += 1;
            // printf("alert message: %d  %.3f   %.3f \n", alert.rank, alert.random_height, alert.moving_average);
            // printf("IN BASE STATION: Rank %s \n", alert.process_name);
            fprintf(logFile, "------------------------------------------------------------------------------------------------------\n");
			fprintf(logFile, "Iteration: %d\n", iteration);
            get_current_time(loggedTime);
            fprintf(logFile, "Logged time: %s\t\t\t\n", loggedTime);
            fprintf(logFile, "Alert reported time: %s\t\t\t\n", alert.send_datetime);
            fprintf(logFile, "Alert type:\t\t\t\n\n");
            
			fprintf(logFile, "Reporting Node\t\tCoord\t\t\tHeight(m)\t\t\tIPv4\n");
            fprintf(logFile, "%d\t\t\t\t\t(%d, %d)\t\t\t%.3f\t\t\t%s (%s)\n\n", alert.rank, alert.coordinates[0], alert.coordinates[1], alert.moving_average, alert.ip_address, alert.process_name);

            fprintf(logFile, "Adjacent Nodes\t\tCoord\t\t\tHeight(m)\t\t\tIPv4\n");
            for (int i = 0; i < 4; i++){
                if (alert.neighbours_rank[i] != -2){
                    fprintf(logFile, "%d\t\t\t\t\t(%d, %d)\t\t\t%.3f\t\t\t%s (%s)\n", 
                    alert.neighbours_rank[i], alert.neighbours_coordinates[i][0], alert.neighbours_coordinates[i][1], alert.neighbours_moving_average[i], alert.ip_address, alert.process_name);
                }
                
            }

            fprintf(logFile, "\nSatellite altimeter reporting time: \n");
            fprintf(logFile, "Satellite altimeter reporting height (m): \n");
            fprintf(logFile, "Satellite altimeter reporting Coord: \n\n");

            fprintf(logFile, "Communication Time (seconds): %f\n", alert_received_time - alert.send_time);
            fprintf(logFile, "Total Messages send between reporting node and base station: %d\n", receive_count[status.MPI_SOURCE]);
            fprintf(logFile, "Number of adjacent matches to reporting node: %d\n", alert.match);
            fprintf(logFile, "Max. tolerance range between nodes readings (m): %d\n", TOLERANCE_RANGE);
            fprintf(logFile, "Max. tolerance range between satellite altimeter and reporting node readings (m): %d\n", TOLERANCE_RANGE);
            fprintf(logFile, "------------------------------------------------------------------------------------------------------\n");

            fflush(stdout);
        }
        iteration += 1;
        sleep(1);
    }
    // Send termination message to all the other processor
    shutdown = TRUE;
    exit = TRUE;

    for (int i = 0; i < world_size - 1; i++ ){
        MPI_Send(&exit, 1, MPI_INT, i, EXIT, world_comm);
        // MPI_Send(&shutdown, 1, MPI_INT, i, EXIT, world_comm);
    }

    pthread_join(tid[0], NULL);
  

	fprintf(logFile, "SUMMARY: \n");
    fclose(logFile);

	
    free(receive_count);
    /* Clean up the type */
    MPI_Type_free(&alertMessageType);
    return 0;
}

void *ThreadFunc(void *pArg){
    int x,y;
    int *val_p = (int *) pArg;
    time_t timestamp; /* calendar time */
    // struct threadArgs *args = pArg;
    int height = 0;
    while (shutdown == FALSE) { 
        srand(time(0));
        height =  (rand() % (10000 - val_p[2] + 1)) + val_p[2];
        x = rand() % val_p[1];
        y = rand() % val_p[0];
        timestamp = time(NULL); /* get current cal time */
        if (head_node == NULL){
            //create node
            head_node = newQueue(timestamp,x,y,height);
            current_node = head_node;
            queue_count += 1;
        }
        else if (queue_count < 10){
            struct Queue* newqueue = newQueue(timestamp,x,y,height);
            current_node->next = newqueue;
            current_node = newqueue;
            //printf(current_node->next);
            queue_count += 1;
        }
        else{
            //remove first 
            struct Queue* temp = head_node;
            head_node = head_node->next;
            free(temp);
            queue_count -= 1;
            struct Queue* newqueue = newQueue(timestamp,x,y,height);
            current_node->next = newqueue;
            current_node = newqueue;
            queue_count += 1;
        }
        // printf("the number of item in node is: %d",queue_count);
        // printf("The time :%s",asctime( localtime(&timestamp) ) );
        // printf("row%d\n",val_p[0]);
        // printf("col%d\n",val_p[1] );
        // printf("cordinatex%d\n",x );
        // printf("cordinatey%d\n",y );
        sleep(1);
    } 
    pthread_exit(NULL);
    return NULL;
       
}