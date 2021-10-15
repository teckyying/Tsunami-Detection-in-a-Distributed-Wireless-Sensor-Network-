#ifndef HELPER_INCLUDED
#define HELPER_H_INCLUDED

#include "global.h"

#define NUMBER_OF_FIELDS 14

struct alertMessageStruct{ 
    int rank; 
	int coordinates[2];
	float random_height;
    float moving_average; 
    
	int match;
	int neighbours_rank[4];	// Top, Bottom, Left, Right
	int neighbours_coordinates[4][2];
	float neighbours_moving_average[4];

	char ip_address[15];
	char neighbours_ip_address[4][15];
	char process_name[MPI_MAX_PROCESSOR_NAME]; // Stack array
	char neighbours_process_name[4][MPI_MAX_PROCESSOR_NAME];\

	double send_time;	// For calculating communication time
	char send_datetime[25];	// To get current timestamp
};

static void create_alert_message_type(struct alertMessageStruct alert, MPI_Datatype* alertMessageType){
	MPI_Datatype datatype[NUMBER_OF_FIELDS] = {MPI_INT, MPI_INT, MPI_FLOAT, MPI_FLOAT, 
								                MPI_INT, MPI_INT, MPI_INT, MPI_FLOAT, 
												MPI_CHAR, MPI_CHAR, MPI_CHAR, MPI_CHAR,
												MPI_DOUBLE, MPI_CHAR};
	int blocklen[NUMBER_OF_FIELDS] = {1, 2, 1, 1, 1, 4, 4 * 2, 4, 15, 4 * 15, MPI_MAX_PROCESSOR_NAME, 4 * MPI_MAX_PROCESSOR_NAME, 1, 25};
	MPI_Aint displacement[NUMBER_OF_FIELDS];

	MPI_Get_address(&alert.rank, &displacement[0]);
	MPI_Get_address(&alert.coordinates, &displacement[1]);
	MPI_Get_address(&alert.random_height, &displacement[2]);
    MPI_Get_address(&alert.moving_average, &displacement[3]);

	MPI_Get_address(&alert.match, &displacement[4]);
	MPI_Get_address(&alert.neighbours_rank, &displacement[5]);
	MPI_Get_address(&alert.neighbours_coordinates, &displacement[6]);
	MPI_Get_address(&alert.neighbours_moving_average, &displacement[7]);
	
	MPI_Get_address(&alert.ip_address, &displacement[8]);
	MPI_Get_address(&alert.neighbours_ip_address, &displacement[9]);
	MPI_Get_address(&alert.process_name, &displacement[10]);
	MPI_Get_address(&alert.neighbours_process_name, &displacement[11]);

	MPI_Get_address(&alert.send_time, &displacement[12]);
	MPI_Get_address(&alert.send_datetime, &displacement[13]);

	/* Get the displacement of an element in bytes from the base address of the structure variable */
	for (int i = NUMBER_OF_FIELDS - 1; i >= 0; i--){
		displacement[i] = displacement[i] - displacement[0];
	}
	displacement[0] = 0;

	// Create MPI struct
	MPI_Type_create_struct(NUMBER_OF_FIELDS, blocklen, displacement, datatype, alertMessageType);
	MPI_Type_commit(alertMessageType);
}

static void get_current_time(char *s){
    // Reference: https://stackoverflow.com/questions/1442116/how-to-get-the-date-and-time-values-in-a-c-program
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(s, 25, "%a %Y-%m-%d %H:%M:%S", tm);
}

#endif