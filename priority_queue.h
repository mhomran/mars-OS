/** 
 * @file priority_queue.h
 * @author Mohamed Abo-Bakr
 * @brief Implementation of priority queue data structure using min-heap
 **/ 

#include <stdio.h>
#include <limits.h>
# include "PCB.h"

#define max_cap 30

/**
 * \struct 
 * @brief Heap struct representing the min-heap
 */
typedef struct heap
{
  int size;
  PCB *arr[max_cap];
} heap;


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


void min_heapify(heap *hp, int parent)
{
  PCB **arr = hp->arr;
  int size = hp->size;
  int left = 2 * parent + 1;
  int right = 2 * parent + 2;
  int smallest;

  // smallest = min(left and parent)
  if (left < size && arr[left]->priority < arr[parent]->priority)
    smallest = left;
  else
    smallest = parent;

  // smallest = min(smallest and right)
  if (right < size && arr[right]->priority < arr[smallest]->priority)
    smallest = right;

  // if the smallest element is not the parent
  if (smallest != parent)
  {
    // swap pointers
    swap(&arr[parent], &arr[smallest]);

    // call heapify for the new smallest
    min_heapify(hp, smallest);
  }
}

/**
 * @brief Builds min-heap from array inside heap (hp)
 */
void build_minheap(heap *hp)
{
  int N = hp->size;
  int start = (N % 2 == 0) ? (N - 1) / 2 : N / 2 - 1;
  for (int i = start; i >= 0; i--)
    min_heapify(hp, i);
}

/** 
 * @brief Returns top element (root) without removing it
 */
PCB* minimum(heap *hp)
{
  return hp->arr[0];
}

/**
 * @brief Extract top element and remove it
 */
PCB* extract_min(heap *hp)
{
  int N = hp->size;
  if (N == 0)
  {
    printf("Can’t remove element as queue is empt");
    return 0;
  }

  // save the value
  PCB *min = hp->arr[0];

  // replace it with last element in heap
  hp->arr[0] = hp->arr[N - 1];
  
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
void dec_priority(heap *hp, int i, int val)
{
  PCB **arr = hp->arr;
  if (val > arr[i]->priority)
  {
    printf("New value is more than current value, can’t be inserted\n");
    return;
  }

  // Set the priority
  arr[i]->priority = val;

  // while parent's priority is higher, swap
  while (i > 0 && arr[(i - 1) / 2]->priority > arr[i]->priority)
  {
    swap(&arr[(i - 1) / 2], &arr[i]);
    i = (i - 1) / 2;
  }
}

/**
 * @brief Inserts a new PCB to the heap
 * @param pc pointer to the PCB
 */ 
void insert_value(heap *hp, PCB *pc)
{
  if (hp->size == max_cap)
  {
    printf("Heap overflow");
    return;
  }

  // assign the new pointer to first empty position
  hp -> arr[hp -> size] = pc;

  // Insert with high priority then dec priority to its value
  int val = pc ->priority;
  hp->arr[hp->size] ->priority = INT_MAX;
  hp->size = hp->size + 1;
  dec_priority(hp, hp ->size - 1, val);
}
