#ifndef _PRIORITY_QUEUE_H
#define _PRIORITY_QUEUE_H


void swap(PCB **x, PCB **y);


PCB* minimum(struct Queue *hp);


PCB* extract_min(struct Queue *hp);


void dec_priority(struct Queue *hp, int i, int val);



void insert_value(struct Queue *hp, PCB *pc);





#endif