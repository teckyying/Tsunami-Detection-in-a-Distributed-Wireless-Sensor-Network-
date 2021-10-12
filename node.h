/**
 * Node class
 * 
 * References: 
 * https://www.geeksforgeeks.org/linked-list-set-3-deleting-node/ 
 * https://www.techiedelight.com/linked-list-implementation-part-1/
*/


struct Node {
    int rank;   // Rank of processor
    float height;   // readings from the sensor
    float moving_average;   // current moving average of height
    struct Node* next;

    // only store these values if height exceeds the threshold
    int match;  // number of neighbours with matching values
    int coords[2];
    int neighbour_ranks[4];
    int neighbour_coords[4][2]; 
    int neighbour_readings[4];
    int time;
};

struct Node* newNode(int rank, float height){
    struct Node* node = (struct Node*)malloc(sizeof(struct Node));
    node -> rank = rank;
    node -> height = height;
    node -> next = NULL;

    return node;
}


int count(struct Node* head_ref){
    int count = 0;
    struct Node* node = head_ref;
    while(node != NULL){
        count += 1;
        node = node -> next;
    }
    return count;
}