/**
 * @file pcb.h
 * @author Ahmed Ashraf (ahmed.ashraf.cmp@gmail.com)
 * @brief PCB data structure
 * @version 0.1
 * @date 2020-12-30
 */

#ifndef _PCB_H
#define _PCB_H
#include "buddy.h"
typedef enum
{
    READY,
    PAUSED,
    BLOCKED,
    FINISHED
} STATE;

typedef struct
{
    int id;            // ID
    int pid;           // Process ID
    int arrivalTime;   // Process arrival time in the read queue
    int runTime;       // Estimated running time
    int priority;      // Priority. 0 is the heighest priority
    int remainingTime; // Remaining time to finish
    STATE state;
    int waitingTime; // Total time from creation to first run
    int waitStart;   // Start time for waiting
    node *memoryNode;

} PCB;

#endif /* _PCB_H */