/** 
 * @file priority_queue.h
 * @author Mohamed Abo-Bakr
 * @brief Implementation of priority queue data structure using min-heap
 **/ 

#include <stdio.h>
#include <limits.h>
#include "PCB.h"
#include "RQ.h"


/**
 * \fn 
 * @brief function to swap PCB pointers
 */
void swap(PCB **x, PCB **y)
{
	PCB* temp = *x;
	*x = *y;
	*y = temp;
}

/** 
 * @brief Returns top element (root) without removing it
 */
PCB* minimum(struct Queue *hp)
{
	return hp->array[0];
}


/**
 * @brief Extract top element and remove it
 */
PCB* extract_min(struct Queue *hp)
{
	int N = hp->size;
	if (N == 0){
		printf("Can’t remove element as queue is empt");
		return 0;
	}

	// save the value
	PCB *min = hp->array[0];

	// replace it with last element in heap
	hp->array[0] = hp->array[N - 1];

	// Decrease the size and call min-heapify for root
	hp->size = N - 1;
	min_heapify(hp, 0);

	// Return old top
	return min;
}


/**
 * @brief change priority of element
 * @param i index of element that we want to change
 * @param val new value
 */
void dec_priority(struct Queue *hp, int i, int val)
{
	PCB **array = hp->array;
	if (val > array[i]->priority){
		printf("Error: New value is more than current value\n");
		return;
	}

	// Set the priority
	array[i]->priority = val;

	// while parent's priority is higher, swap
	while (i > 0 && array[(i - 1) / 2]->priority > array[i]->priority){
		swap(&array[(i - 1) / 2], &array[i]);
		i = (i - 1) / 2;
	}
}

/**
 * @brief Inserts a new PCB to the heap
 * @param pc pointer to the PCB
 */ 
void insert_value(struct Queue *hp, PCB *pc)
{
	if (hp->size == hp-> capacity){
		printf("Heap overflow");
		return;
	}

	// assign the new pointer to first empty position
	hp -> array[hp -> size] = pc;

	// Insert with high priority then decrease priority to its value
	int val = pc ->priority;
	hp->array[hp->size] ->priority = INT_MAX;
	hp->size = hp->size + 1;
	dec_priority(hp, hp ->size - 1, val);
}
