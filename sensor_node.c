#include "global.h"
#include "node.h"
#include "helper.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <string.h>
#include <time.h>

#define SHIFT_ROW 0
#define SHIFT_COL 1
#define DISP 1
#define DIMENSION 2     // 2D topology
#define NUM_NBR 4    // maximum number of adjacent neighbourhood nodes
#define INTERVAL 10     // time interval in seconds
#define MOVING_AVG_WINDOW 5

// Function Prototype
float generate_random_values(int rank, int iter);
float calculate_average(struct Node* head_node, struct Node* last_node);
int compare_with_neighbours(float moving_average, float receive[]);
void get_ip_address(char ip_addr[15]);

void *SensorHelperFunc(void *pArg);
MPI_Comm comm2D;
struct alertMessageStruct alert;

int sensor_node(MPI_Comm world_comm, MPI_Comm comm, int nrows, int ncols, int threshold){
    // MPI_Comm comm2D;
    int ndims = DIMENSION;
    int size, reorder, ierr, world_size;
    int dims[ndims];
    int wrap_around[ndims];
    struct Node* head_node = NULL;   // first element for moving average
    struct Node* last_node = NULL;   // current element aka the last element for moving average
    int node_list_count = 0;    // ensure the number of node list doesnt exceed the moving average window
    int iter = 0;
    int exit = FALSE;
    int request_from_neighbour;

    // struct alertMessageStruct alert;
	MPI_Datatype alertMessageType;
    create_alert_message_type(alert, &alertMessageType);

    MPI_Comm_size(world_comm, &world_size); // size of the world communicator
    MPI_Comm_size(comm, &size);         // size of the slave(sensor_nodes) communicator
    MPI_Comm_rank(comm, &alert.rank);         // rank of the slave(sensor_nodes) communicator

    dims[0] = nrows;    // specify number of rows
    dims[1] = ncols;    // specify number of columns

    /* Create cartesian topology for processes */
    MPI_Dims_create(size, ndims, dims);

    /* create cartesian mapping */
    wrap_around[0] = wrap_around[1] = 0; // periodic shift is false
    reorder = 1;
    ierr = 0;
    ierr = MPI_Cart_create(comm, ndims, dims, wrap_around, reorder, &comm2D);
    if (ierr != 0) {
        printf("ERROR[%d] creating CART\n", ierr);
    }
 
	/* Get coordinates */
	MPI_Cart_coords(comm2D, alert.rank, ndims, alert.coordinates); // coordinates is returned into the coord array

    /* Get rank of neighbours */
    MPI_Cart_shift(comm2D, SHIFT_ROW, DISP, &alert.neighbours_rank[0], &alert.neighbours_rank[1]); // top, bottom
    MPI_Cart_shift(comm2D, SHIFT_COL, DISP, &alert.neighbours_rank[2],&alert.neighbours_rank[3]); // left, right

    /* Get coordinates of neighbours */
    MPI_Neighbor_allgather(alert.coordinates, 2, MPI_INT, alert.neighbours_coordinates, 2, MPI_INT, comm2D);
    for (int i = 0; i < NUM_NBR; i++){
        if (alert.neighbours_rank[i] == -2){
            alert.neighbours_coordinates[i][0] = -2;
            alert.neighbours_coordinates[i][1] = -2;
        }
    } 

    /** Get and exhange IPv4 address with neighbours */
    get_ip_address(alert.ip_address);
    MPI_Neighbor_allgather(alert.ip_address, 15, MPI_CHAR, alert.neighbours_ip_address, 15, MPI_CHAR, comm2D);

    /** Get and exhange processor name address with neighbours */
    int length;
    MPI_Get_processor_name(alert.process_name, &length);
    MPI_Neighbor_allgather(alert.process_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, alert.neighbours_process_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, comm2D);

	pthread_t tid[1];	// create pthreads
	int threadNum[1];	// to store unique id of the thread
	threadNum[0] = alert.rank;
	pthread_create(&tid[0], 0, SensorHelperFunc, &threadNum[0]);

    MPI_Request send_request[NUM_NBR];
    MPI_Request receive_request[NUM_NBR];
    MPI_Status send_status[NUM_NBR];
    MPI_Status receive_status[NUM_NBR];
    MPI_Request send_alert_request[1];
    MPI_Request broadcast_request;

    MPI_Irecv(&exit, 1, MPI_INT, world_size - 1, EXIT, world_comm, &broadcast_request);
    
    while (!exit){
        request_from_neighbour = FALSE;
        /* Generate random height */
        alert.random_height = generate_random_values(alert.rank, iter);
        // printf("Rank %d: %.3f \n", rank, random_height);

        if (head_node == NULL){
            head_node = newNode(alert.rank, alert.random_height);
            head_node -> moving_average = alert.random_height;
            last_node = head_node;
            node_list_count += 1;
            // printf("Case 1, moving average is %.3f \n", last_node -> moving_average);
        } 
        else{
            if (node_list_count < MOVING_AVG_WINDOW) {
                node_list_count += 1;
            }
            else {  // node_list_count >= MOVING_AVG_WINDOW
                /* Delete the head node and assign the next element as the new head */
                struct Node* temp = head_node;
                head_node = head_node->next;
                free(temp);
            }
            last_node -> next = newNode(alert.rank, alert.random_height);
            last_node = last_node -> next;
            last_node -> moving_average = calculate_average(head_node, last_node);
            // printf("Case 2, moving average is %.3f \n", last_node -> moving_average);
        }
        
        alert.moving_average = last_node->moving_average;
        fflush(stdout);

        /* Gather neighbours' values */ // CHANGE THIS LATER. SHOULD ONLY RECEIVE VALUE IF MOVING AVERAGE > THRESHOLD
        /* Request values from its adjacent process */
        // for (int i = 0; i < NUM_NBR; i++){
        //     MPI_Isend(&alert.moving_average, 1, MPI_FLOAT, alert.neighbours_rank[i], 0, comm2D, &send_request[i]);
        //     MPI_Irecv(&alert.neighbours_moving_average[i], 1, MPI_FLOAT, alert.neighbours_rank[i], 0, comm2D, &receive_request[i]);
        // }
        // MPI_Waitall(NUM_NBR, send_request, send_status);
        // MPI_Waitall(NUM_NBR, receive_request, receive_status);  

        // for (int i = 0; i < NUM_NBR; i++){  // if neighbour does't exist, change the value to -2.
        //     alert.neighbours_moving_average[i] = (alert.neighbours_rank[i] == -2) ? -2 : alert.neighbours_moving_average[i];
        // }

        if (alert.moving_average > threshold){ // if moving average exceeds the threshold
            request_from_neighbour = TRUE;  // doesnt't matter what we send here
            for (int i = 0; i < NUM_NBR; i++){
                int neighbour_rank = alert.neighbours_rank[i] == -2 ? MPI_PROC_NULL : alert.neighbours_rank[i];
                MPI_Isend(&request_from_neighbour, 1, MPI_INT, neighbour_rank, REQUEST_VALUE_TAG, comm2D, &send_request[i]);
                MPI_Irecv(&alert.neighbours_moving_average[i], 1, MPI_FLOAT, neighbour_rank, SEND_VALUE_TAG, comm2D, &receive_request[i]);
            }
            /* Wait for all requests to complete to make sure all neighbours have sent their value */
            MPI_Waitall(NUM_NBR, send_request, send_status);
            MPI_Waitall(NUM_NBR, receive_request, receive_status);
    
            alert.match = compare_with_neighbours(alert.moving_average, alert.neighbours_moving_average);   // Compare moving average with adjacent nodes
            
            if (alert.match >= 2) {       // if at least two or more adjacent nodes match the reading of the local node 
                // printf("height: %.3f    average:%.3f\n", alert.random_height, alert.moving_average);
                get_current_time(alert.send_datetime);
                alert.send_time = MPI_Wtime();
                MPI_Isend(&alert, 1, alertMessageType, world_size - 1, ALERT_TAG, world_comm, &send_alert_request[1]);
            }
        }
        sleep(3);
        if (exit == TRUE){
            printf("\nRank %d receive exit message\n", alert.rank);
            fflush(stdout);
         }
         iter++;
    }

    MPI_Send(&exit, 1, MPI_INT, alert.rank, THREAD_EXIT, comm2D);   // signal thread to exit too
    pthread_join(tid[0], NULL);

	/* Clean up the type */
    MPI_Type_free(&alertMessageType);
    MPI_Comm_free(&comm2D);
    return 0;
}

float generate_random_values(int rank, int iter){
    unsigned int seed = time(NULL) + rank + iter;  // seed
    float randomNumber = sin(rand_r(&seed)* rand_r(&seed));
    return (MIN_HEIGHT + (MAX_HEIGHT - MIN_HEIGHT) * fabs(randomNumber));
}

float calculate_average(struct Node* head_node, struct Node* last_node){
    int count = 0;
    float sum = 0;
    struct Node* node = head_node;
    while(node != NULL){
        sum += node -> height;
        count += 1;
        node = node -> next;
    }
    return (float)(sum/count);
}

int compare_with_neighbours(float moving_average, float receive[]){
    int match = 0;        
    /* Compare with adjacent neighbouring nodes */
    for (int i = 0; i < NUM_NBR; i++){
        if (receive[i] != -2) {    // if value is -2, it means that the neighbour doesn't exist
            if (abs(moving_average - receive[i]) <= TOLERANCE_RANGE){    // if within predefined tolerance range
                match++;
            }   
        }  
    }
    // printf("%d   %.3f     %.3f   %.3f     %.3f    %.3f\n", match, moving_average, receive[0], receive[1], receive[2], receive[3]);
    return match;
}

void get_ip_address(char ip_addr[15]) {
    // Reference: https://stackoverflow.com/a/4139893
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    char *tmp_ip_addr;
  
    getifaddrs (&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family==AF_INET) {
            sa = (struct sockaddr_in *) ifa->ifa_addr;
            tmp_ip_addr = (char *)inet_ntoa(sa->sin_addr);
            memcpy(ip_addr, tmp_ip_addr, 15 * sizeof(char));
        }
    }
    freeifaddrs(ifap);
}

void *SensorHelperFunc(void *pArg)
{
    int thread_rank = *((int*)pArg); // same rank as main process
    int exit = FALSE;
    int request = FALSE;

    int flag;
    MPI_Status probe_status;
    MPI_Status receive_status;
    MPI_Request broadcast_request;
    MPI_Request send_request;

    /* Check if main process sends an exit notification */
    MPI_Irecv(&exit, 1, MPI_INT, thread_rank, THREAD_EXIT, comm2D, &broadcast_request); 

    while (exit == FALSE){
        request = FALSE;
        /* Check if there's any incoming request from neightbours */
        MPI_Iprobe(MPI_ANY_SOURCE, REQUEST_VALUE_TAG, comm2D, &flag, &probe_status);
        if (flag){ // if TRUE
            /* Receive the request */
            MPI_Recv(&request, 1, MPI_INT, probe_status.MPI_SOURCE, REQUEST_VALUE_TAG, comm2D, &receive_status);
            if (request == TRUE){   // Not really needed. But adding this condition just to confirm that it is a valid request.
                /* Send moving average to neighbour that requested it */
                MPI_Isend(&alert.moving_average, 1, MPI_FLOAT, receive_status.MPI_SOURCE, SEND_VALUE_TAG, comm2D, &send_request);
            }
        }
    }
    return NULL;
}
