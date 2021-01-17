/**
 * @file ready_queue.h
 * @author Ahmed Ashraf, Mohamed Hassanin 
 * @brief A mocking of the ready queue which contains the porcess that have already arrived to the scheduler
 * @version 0.1
 * @date 2020-12-30
 */

#ifndef _READY_QUEUE_H
#define _READY_QUEUE_H

#include "pcb.h"

struct Queue { 
        int front, rear, size; 
        unsigned capacity; 
        PCB** array; 
}; 

struct Queue* CreateQueue(unsigned capacity);
int IsFull(struct Queue* queue);
int IsEmpty(struct Queue* queue);
void Enqueue(struct Queue* queue, PCB* item);
PCB* Dequeue(struct Queue* queue);
PCB* Front(struct Queue* queue);
PCB* Rear(struct Queue* queue);

#endif /* _READY_QUEUE_H */