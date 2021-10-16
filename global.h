#ifndef GLOBAL_INCLUDED
#define GLOBAL_H_INCLUDED

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_HEIGHT 8000
#define MIN_HEIGHT 5000
#define DEFAULT_THRESHOLD 6000      // default sensor threshold
#define TOLERANCE_RANGE 500     // tolerance range between nodes reading
#define DEFAULT_ITERATION 100

#define REQUEST_VALUE_TAG 2
#define SEND_VALUE_TAG 3
#define EXIT 4
#define ALERT_TAG 5
#define THREAD_EXIT 6

#define TRUE 1
#define FALSE 0

/*  Function Prototypes */
int base_station(MPI_Comm world_comm, MPI_Comm comm, int nrows, int ncols, int num_of_iterations,int threshold);
int sensor_node(MPI_Comm world_comm, MPI_Comm comm, int nrows, int ncols, int threshold);


#endif