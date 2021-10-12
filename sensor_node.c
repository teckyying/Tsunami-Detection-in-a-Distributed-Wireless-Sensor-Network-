#include "global.h"
#include "node.h"

#define SHIFT_ROW 0
#define SHIFT_COL 1
#define DISP 1

#define DIMENSION 2     // 2D topology
#define NUM_NBR 4    // maximum number of adjacent neighbourhood nodes

#define INTERVAL 10     // time interval in seconds
#define TOLERANCE_RANGE 500     // tolerance range


#define MOVING_AVG_WINDOW 5

// Function Prototype
float generate_random_values(int rank, int iter);
float calculate_average(struct Node* head_node, struct Node* last_node);
int compare_with_neighbours(float moving_average, float* receive[]);

void sensor_node(MPI_Comm world_comm, MPI_Comm comm, int nrows, int ncols, int threshold){
    MPI_Comm comm2D;
    int ndims = DIMENSION;
    int size, rank, reorder, my_cart_rank, ierr, world_size;
    int nbr_i_lo, nbr_i_hi, nbr_j_lo, nbr_j_hi;
    int dims[ndims], coord[ndims];
    int wrap_around[ndims];
    struct Node* head_node = NULL;   // first element for moving average
    struct Node* last_node = NULL;   // current element aka the last element for moving average
    int node_list_count = 0;    // ensure the number of node list doesnt exceed the moving average window
    int iter = 0;
    int exit = FALSE;

    MPI_Comm_size(world_comm, &world_size); // size of the world communicator
    MPI_Comm_size(comm, &size);         // size of the slave(sensor_nodes) communicator
    MPI_Comm_rank(comm, &rank);         // rank of the slave(sensor_nodes) communicator

    // dims[0] = dims[1] = 0   // Specify the number of nodes in each dimension. A value of 0 indicates that MPI_Dims_create should fill in a suitable value.
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
 
	/* find my coordinates in the cartesian communicator group */
	MPI_Cart_coords(comm2D, rank, ndims, coord); // coordinates is returned into the coord array

    /* use my cartesian coordinates to find my rank in cartesian group*/
	MPI_Cart_rank(comm2D, coord, &my_cart_rank);

    /* get my neighbors; axis is coordinate dimension of shift */
    /* axis=0 ==> shift along the rows: P[my_row-1]: P[me] : P[my_row+1] */
    /* axis=1 ==> shift along the columns P[my_col-1]: P[me] : P[my_col+1] */
	MPI_Cart_shift(comm2D, SHIFT_ROW, DISP, &nbr_i_lo, &nbr_i_hi ); // top, bottom
	MPI_Cart_shift(comm2D, SHIFT_COL, DISP, &nbr_j_lo, &nbr_j_hi ); // left, right
     
    MPI_Request send_request[NUM_NBR];
    MPI_Request receive_request[NUM_NBR];
    MPI_Status send_status[NUM_NBR];
    MPI_Status receive_status[NUM_NBR];
    MPI_Request send_alert_request[1];
    MPI_Request broadcast_request;

    MPI_Ibcast(&exit, 1, MPI_INT, world_size - 1, MPI_COMM_WORLD, &broadcast_request);
    
    while (!exit){
        /* Generate random height */
        float random_height = generate_random_values(rank, iter);
        // printf("Rank %d: %.3f \n", rank, random_height);

        if (head_node == NULL){
            head_node = newNode(rank, random_height);
            head_node -> moving_average = random_height;
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
            last_node -> next = newNode(rank, random_height);
            last_node = last_node -> next;
            last_node -> moving_average = calculate_average(head_node, last_node);
            // printf("Case 2, moving average is %.3f \n", last_node -> moving_average);
        }
        
        float moving_average = last_node -> moving_average;

        /* Gather neighbours' values */ // CHANGE THIS LATER. SHOULD ONLY RECEIVE VALUE IF MOVING AVERAGE > THRESHOLD
        int send[] = {nbr_i_lo, nbr_i_hi, nbr_j_lo, nbr_j_hi};
        float left_height = -1, right_height = -1, top_height = -1, bottom_height = -1;
        float* receive[] = {&top_height, &bottom_height, &left_height, &right_height};

        /* Request values from its adjacent process, ie left, right, top, bottom */
        for (int i = 0; i < NUM_NBR; i++){
            MPI_Isend(&moving_average, 1, MPI_FLOAT, send[i], 0, comm2D, &send_request[i]);
            MPI_Irecv(receive[i], 1, MPI_FLOAT, send[i], 0, comm2D, &receive_request[i]);
        }
        MPI_Waitall(NUM_NBR, send_request, send_status);
        MPI_Waitall(NUM_NBR, receive_request, receive_status);

        printf("Global rank: %d. Cart rank: %d. Coord: (%d, %d). Random Height: %.3f. Recv Top: %.3f. Recv Bottom: %.3f. Recv Left: %.3f. Recv Right: %.3f.\n", rank, my_cart_rank, coord[0], coord[1], moving_average, top_height, bottom_height, left_height, right_height);

        if (moving_average > threshold){ // if moving average exceeds the threshold
            int match = compare_with_neighbours(moving_average, receive);   // Compare moving average with adjacent nodes
            if (match >= 2) {       // if at least two or more adjacent nodes match the reading of the local node
                printf("2 or more matches\n");

                // Send to base station   
                float alert_data[] = {(float)rank, (float)random_height, (float) moving_average, (float)nbr_i_lo, (float)nbr_i_hi, (float)nbr_j_lo, (float)nbr_j_hi, 
									*receive[0], *receive[1], *receive[2], *receive[3]};
				
				/* Send alert report to base station. */
				MPI_Isend(alert_data, 11, MPI_FLOAT, world_size - 1, ALERT_TAG, world_comm, &send_alert_request[1]);
            }
        }

        // iter ++;
        // if (iter == 2){
        //     exit = TRUE;
        // }
        sleep(2);
    }

    printf("EXIT IS TRUE");

 
    // every 10 seconds, generate a value, calculate moving average and exchange with adjacent nodes

    MPI_Comm_free(&comm2D);
}

float generate_random_values(int rank, int iter){
    unsigned int seed = time(NULL) + rank + iter;  // seed
    // generate a randommized value
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

int compare_with_neighbours(float moving_average, float* receive[]){
    int match = 0;        
    /* Compare with adjacent neighbouring nodes */
    for (int i = 0; i < NUM_NBR; i++){
        // printf("%.3f  %.3f \n", moving_average, *receive[i]);
        if (*receive[i] == -1) {    // if height is -1, it means that the neighbour doesn't exist
            continue;
        }
        if (abs(moving_average - *receive[i]) <= TOLERANCE_RANGE)    // if within predefined tolerance range
            match++;
    }
    return match;
}

