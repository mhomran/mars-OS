/**
 * @file process_generator.c
 * @author Mohamed Hassanin
 * @brief Create processes objects and initialize scheduler and clock processes.
 * @version 0.1
 * @date 2020-12-29 
 */

#include <stdio.h>			 
#include <stdlib.h>			
#include "headers.h"			/**< for dealing with clk module */
#include "process_generator.h"		/**< for process_t */



void clearResources(int);
process_t* CreateProcesses(const char* fileName, int* numberOfProcesses);

int main(int argc, char * argv[]) {
	process_t* processes;
	int numberOfProcesses;
	int schedOption;
	int quantum;

	signal(SIGINT, clearResources);
	// TODO Initialization
	// 1. Read the input files.
	processes = CreateProcesses("processes.txt", &numberOfProcesses);
	// 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
	printf("please enter the scheduling algorithm:\n");
	printf("0: Non-preemptive Highest Priority First (NHPF)\n");
	printf("1: shortest remaining time next (SRTN)\n");
	printf("2: Round robin (RR)\n");

	if ((schedOption = fgetc(stdin)) == EOF)
	{
		fprintf(stderr, "error when reading sched option\n");
		exit(EXIT_FAILURE);
	}
	fgetc(stdin); // take the newline out of the stdin

	
	if (schedOption == '2')
	{
		printf("please enter the quantum\n");

		char value[100];
		if(fgets(value, 100, stdin) == NULL)
		{
			fprintf(stderr, "error when reading quantum option\n");
			exit(EXIT_FAILURE);
		}

		quantum = atoi(value);
	}
	// 3. Initiate and create the scheduler and clock processes.
	if (fork() == 0) {
		free(processes);

		execl("scheduler.out", "scheduler.out", NULL);
	}
	if (fork() == 0) {
		free(processes);

		execl("clk.out", "clk.out", NULL);
	}
	// 4. Use this function after creating the clock process to initialize clock
	initClk();
	// To get time use this
	int x = getClk();
	printf("current time is %d\n", x);

	// TODO Generation Main Loop
	// 5. Create a data structure for processes and provide it with its parameters.
	while (1) {
	// 6. Send the information to the scheduler at the appropriate time.
	}
	// 7. Clear clock resources
	destroyClk(true);
	
	free(processes);
	return EXIT_SUCCESS;
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
}

/**
 * @brief Create a Processes objects array after parsing the processes file.
 * 
 * @param fileName the name of the processes file
 * @param numberOfProcesses the number of processes in the array.
 * @return process* array of processes
 */
process_t* CreateProcesses(const char* fileName, int* numberOfProcesses)
{
	char* number = NULL;
	int numberSize = 0;

	char * line = NULL;
	size_t len = 0;
	ssize_t lineLength;

	FILE* fp;

	size_t processesNo = 0;
	process_t* processes = NULL;

	fp = fopen(fileName, "r"); 

	if (fp == NULL) {
		perror("Error while opening the file.\n");
		exit(EXIT_FAILURE);
	}

	
	while ((lineLength = getline(&line, &len, fp)) != -1) {
		int chIndex = 0;
		if (line[chIndex] == '#') continue;
		
		int numbers[4];
		for(int member = 0; member < 4; member++) {
			while(line[chIndex] != '\t' && line[chIndex] != '\n') {
				numberSize++;
				number = (char*)realloc(number, numberSize);
				number[numberSize-1] = line[chIndex];
				
				chIndex++;
			}
			chIndex++;

			#ifdef DEBUG
			printf("%d\t", atoi(number));
			#endif
			numbers[member] = atoi(number);
			numberSize = 0;
			free(number);
			number = NULL;	
		}
		#ifdef DEBUG
		printf("\n");
		#endif
		processesNo++;
		processes = (process_t*)realloc(processes, sizeof(process_t) * processesNo);
		processes[processesNo - 1].id = numbers[0];
		processes[processesNo - 1].arrivalTime = numbers[1];
		processes[processesNo - 1].runTime = numbers[2];
		processes[processesNo - 1].priority = numbers[3];
	}
	free(line);
	
	fclose(fp);

	*numberOfProcesses = processesNo;
	return processes;
}
