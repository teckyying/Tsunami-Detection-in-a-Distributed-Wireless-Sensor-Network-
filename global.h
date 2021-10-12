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

#define REQUEST_VALUE_TAG 2
#define EXIT 3
#define ALERT_TAG 4

#define TRUE 1
#define FALSE 0


/*  Function Prototypes */
void base_station(MPI_Comm world_comm, MPI_Comm comm, int nrows, int ncols, int num_of_iterations);
void sensor_node(MPI_Comm world_comm, MPI_Comm comm, int nrows, int ncols, int threshold);


#endif