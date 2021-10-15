#include "global.h"

struct Queue {
    time_t timestamp;   
    int x;   
    int y;   
    int height; 
    struct Queue* next;
};

struct Queue* newQueue(time_t timestamp, int x,int y,int height){
    struct Queue* Queue = (struct Queue*)malloc(sizeof(struct Queue));
    Queue -> timestamp = timestamp;
    Queue -> x = x;
    Queue -> y = y;
    Queue->height = height;
    Queue -> next = NULL;
    return Queue;
}


int show_me(struct Queue* head_ref){
    int count = 0;
    struct Queue* Queue = head_ref;
    while(Queue != NULL){
        count += 1;
        // printf("The timestamp :%s",asctime( localtime(&Queue->timestamp) ) );
        printf("The height is : %d\n",Queue->height);
        // printf("The x is : %d\n",Queue->x);
        // printf("The y is : %d\n",Queue->y);
        Queue = Queue -> next;
    }
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    return count;
}