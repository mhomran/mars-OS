/**
 * @file process_generator.h
 * @author Mohamed Hassanin
 * @brief This header file contains all the shared data structures
 * between this module and the scheduler module.
 * @version 0.1
 * @date 2020-12-30
 */
#ifndef _PROCESS_GENERATOR_H
#define _PROCESS_GENERATOR_H

#include <inttypes.h>

/**
 * @brief this a type for every process read from a file
 */
typedef struct process
{
	int id;			 /**< The id of the process to use in logging*/
	int arrivalTime; /**< Store the time when the proccess arive */
	int runTime;	 /**< Store the runtime/bursttime of the process */
	int priority;	 /**< The priority of the process */
	uint8_t arrived; /**< flag to track if the process arrived or not */
	int memSize;
} process_t;

#endif /* _PROCESS_GENERATOR_H */
