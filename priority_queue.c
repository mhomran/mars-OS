/** 
 * @file priority_queue.h
 * @author Mohamed Abo-Bakr
 * @brief Implementation of priority queue data structure using min-heap
 **/ 

#include <stdio.h>
#include <limits.h>
#include "pcb.h"
#include "priority_queue.h"


/**
 * \fn 
 * @brief function to Swap PCB pointers
 */
void Swap(PCB **x, PCB **y)
{
	PCB* temp = *x;
	*x = *y;
	*y = temp;
}

/** 
 * @brief Returns top element (root) without removing it
 */
PCB* Minimum(struct Queue *hp)
{
	return hp->array[0];
}


/**
 * @brief Extract top element and remove it
 */
PCB* ExtractMin(struct Queue *hp)
{
	int N = hp->size;
	if (N == 0){
		printf("Can’t remove element as queue is empty");
		return 0;
	}

	// save the value
	PCB *min = hp->array[0];

	// replace it with last element in heap
	hp->array[0] = hp->array[N - 1];

	// Decrease the size and call min-heapify for root
	hp->size = N - 1;
	MinHeapify(hp, 0);

	// Return old top
	return min;
}



void MinHeapify(struct Queue *hp, int parent)
{
	PCB **arr = hp->array;
	int size = hp->size, min;
	int left = 2 * parent + 1;
	int right = 2 * parent + 2;

	// min = minimum of left, right and parent
	if (left < size && arr[left]->priority < arr[parent]->priority)
		min = left;
	else
		min = parent;

	if (right < size && arr[right]->priority < arr[min]->priority)
		min = right;


	if (min != parent){
		// Swap pointers
		Swap(&arr[parent], &arr[min]);

		// call heapify for the new min
		MinHeapify(hp, min);
	}
}

/**
 * @brief change priority of element
 * @param i index of element that we want to change
 * @param val new value
 */
void DecPriority(struct Queue *hp, int i, int val)
{
	PCB **array = hp->array;
	if (val > array[i]->priority){
		printf("Error: New value is more than current value\n");
		return;
	}

	// Set the priority
	array[i]->priority = val;

	// while parent's priority is higher, Swap
	while (i > 0 && array[(i - 1) / 2]->priority > array[i]->priority){
		Swap(&array[(i - 1) / 2], &array[i]);
		i = (i - 1) / 2;
	}
}

/**
 * @brief Inserts a new PCB to the heap
 * @param pc pointer to the PCB
 */ 
void InsertValue(struct Queue *hp, PCB *pc)
{
	if (hp->size == hp-> capacity) {
		printf("Heap overflow");
		return;
	}

	// assign the new pointer to first empty position
	hp -> array[hp -> size] = pc;

	// Insert with high priority then decrease priority to its value
	int val = pc ->priority;
	hp->array[hp->size] ->priority = INT_MAX;
	hp->size = hp->size + 1;
	DecPriority(hp, hp ->size - 1, val);
}
