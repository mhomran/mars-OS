
/**
 * @file PCB.h
 * @author Ahmed Ashraf (ahmed.ashraf.cmp@gmail.com)
 * @brief PCB data structure
 * @version 0.1
 * @date 2020-12-30
 */

typedef struct PCB
{
    int pid;                // Process ID
    int arrivalTime;        // Process arrival time in the read queue
    int runTime;            // Estimated running time
    int priority;           // Priority. 0 is the heighest priority
    int remainingTime;      // Remaining time to finish
    int state;              // 1: running, 0: stopped
    int waitingTime;        // Total time from creation to first run
};
