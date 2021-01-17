/**
 * @file ready_queue.c
 * @author Ahmed Ashraf (ahmed.ashraf.cmp@gmail.com)
 * @brief A mocking of the ready queue which contains the porcess that have already arrived to the scheduler
 * @version 0.1
 * @date 2020-12-30
 */

#include <limits.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include "ready_queue.h"

struct Queue* CreateQueue(unsigned capacity) 
{ 
        struct Queue* queue = (struct Queue*)malloc( 
        sizeof(struct Queue)); 
        queue->capacity = capacity; 
        queue->front = queue->size = 0; 

        queue->rear = capacity - 1; 
        queue->array = malloc(queue->capacity * sizeof(PCB*)); 
        return queue; 
} 

int IsFull(struct Queue* queue) 
{ 
        return (queue->size == queue->capacity); 
} 
  
int IsEmpty(struct Queue* queue) 
{ 
        return (queue->size == 0); 
} 

void Enqueue(struct Queue* queue, PCB* item) 
{ 
        if (IsFull(queue)) return; 
        queue->rear = (queue->rear + 1) % queue->capacity; 
        queue->array[queue->rear] = item; 
        queue->size = queue->size + 1; 
} 

PCB* Dequeue(struct Queue* queue) 
{ 
        if (IsEmpty(queue)) return NULL; 
        PCB* item = queue->array[queue->front]; 
        queue->front = (queue->front + 1) % queue->capacity; 
        queue->size = queue->size - 1; 
        return item; 
} 

PCB* Front(struct Queue* queue) 
{ 
    if (IsEmpty(queue)) return NULL; 
    return queue->array[queue->front]; 
} 

PCB* Rear(struct Queue* queue) 
{ 
    if (IsEmpty(queue)) return NULL; 
    return queue->array[queue->rear]; 
} 