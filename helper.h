#ifndef HELPER_INCLUDED
#define HELPER_H_INCLUDED

#include "global.h"

#define NUMBER_OF_FIELDS 8

struct alertMessageStruct{ 
    int rank; 
	int coordinates[2];
	float random_height;
    float moving_average; 
    
	int match;
	int neighbours_rank[4];	// Top, Bottom, Left, Right
	int neighbours_coordinates[4][2];
	float neighbours_moving_average[4];
	
};

static void create_alert_message_type(struct alertMessageStruct alert, MPI_Datatype* alertMessageType){
	MPI_Datatype datatype[NUMBER_OF_FIELDS] = {MPI_INT, MPI_INT, MPI_FLOAT, MPI_FLOAT, 
								MPI_INT, MPI_INT, MPI_INT, MPI_FLOAT};
	int blocklen[NUMBER_OF_FIELDS] = {1, 2, 1, 1, 1, 4, 8, 4};
	MPI_Aint displacement[NUMBER_OF_FIELDS];

	MPI_Get_address(&alert.rank, &displacement[0]);
	MPI_Get_address(&alert.coordinates, &displacement[1]);
	MPI_Get_address(&alert.random_height, &displacement[2]);
    MPI_Get_address(&alert.moving_average, &displacement[3]);

	MPI_Get_address(&alert.match, &displacement[4]);
	MPI_Get_address(&alert.neighbours_rank, &displacement[5]);
	MPI_Get_address(&alert.neighbours_coordinates, &displacement[6]);
	MPI_Get_address(&alert.neighbours_moving_average, &displacement[7]);

	//Make relative
	// get the displacement of an element in bytes from the base address of the structure variable.
	for (int i = NUMBER_OF_FIELDS - 1; i >= 0; i--){
		displacement[i] = displacement[i] - displacement[0];
	}
	// displacement[0] = 0;

	// Create MPI struct
	MPI_Type_create_struct(NUMBER_OF_FIELDS, blocklen, displacement, datatype, alertMessageType);
	MPI_Type_commit(alertMessageType);
}

#endif