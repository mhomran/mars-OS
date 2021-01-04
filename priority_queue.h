#ifndef _PRIORITY_QUEUE_H
#define _PRIORITY_QUEUE_H

#include "ready_queue.h"

void Swap(PCB **x, PCB **y);
PCB* Minimum(struct Queue *hp);
PCB* ExtractMin(struct Queue *hp);
void DecPriority(struct Queue *hp, int i, int val);
void InsertValue(struct Queue *hp, PCB *pc);
void MinHeapify(struct Queue *hp, int parent);


#endif