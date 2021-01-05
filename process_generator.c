/**
 * @file process_generator.c
 * @author Mohamed Hassanin
 * @brief Create processes objects and initialize scheduler and clock processes.
 * @version 0.1
 * @date 2020-12-29 
 */

#define DEBUG

#include <stdio.h>			 
#include <stdlib.h>
#include <inttypes.h>			
#include "headers.h"			/**< for dealing with clk module */
#include "process_generator.h"		/**< for process_t */

key_t msgqid;

struct msgbuff {
    long mtype;
    char* mtext;
    process_t proc;
};

void clearResources(int);
process_t* CreateProcesses(const char* fileName, int* numberOfProcesses);
char* myItoa(int number);

process_t* processes = NULL;

int main(int argc, char * argv[]) {
	int numberOfProcesses;
	int schedOption;
	int quantum;
        int curTime = -1;
        int semSchedGen;
        union Semun semun;

        

	pid_t schedPid;

	signal(SIGINT, clearResources);
	// TODO Initialization
	// 1. Read the input files.
	processes = CreateProcesses("processes.txt", &numberOfProcesses);
	// 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
	printf("please enter the scheduling algorithm:\n");
	printf("0: shortest remaining time next (SRTN)\n");
	printf("1: Round robin (RR)\n");
	printf("2: Non-preemptive Highest Priority First (NHPF)\n");

	if ((schedOption = fgetc(stdin)) == EOF)
	{
		fprintf(stderr, "processe generator: error when reading sched option\n");
		exit(EXIT_FAILURE);
	}
	fgetc(stdin); // take the newline out of the stdin

	schedOption -= '0';

	if (schedOption == 1)
	{
		printf("processe generator: please enter the quantum\n");

		char value[100];
		if(fgets(value, 100, stdin) == NULL) {
			fprintf(stderr, "processe generator: error when reading quantum option\n");
			exit(EXIT_FAILURE);
		}

		quantum = atoi(value);
	}

	// 3. Initiate and create the scheduler and clock processes.
	msgqid = msgget(MSGQKEY, 0644 | IPC_CREAT);
    	if (msgqid == -1) {
        	perror("processe generator: Failed to get the message queue\n");
		exit(EXIT_FAILURE);
    	}
	
        semSchedGen = semget(SEM_SCHED_GEN_KEY, 1, 0666 | IPC_CREAT);
        
        if (semSchedGen == -1){
                perror("Error in create sem");
                exit(-1);
        }

        semun.val = 0; /* initial value of the semaphore, Binary semaphore */
        if (semctl(semSchedGen, 0, SETVAL, semun) == -1){
                perror("Error in semctl");
                exit(-1);
        }

	if ((schedPid = fork()) == 0) {
		free(processes);

		if (execl("build/scheduler.out", "scheduler.out", myItoa(schedOption), myItoa(numberOfProcesses), myItoa(quantum), NULL) == -1) {
			perror("process_generator: couldn't run scheduler.out\n");
			exit(EXIT_FAILURE);
		}
	}
	
	if (fork() == 0) {
		free(processes);

		if (execl("build/clk.out", "clk.out", NULL) == -1) {
			perror("process_generator: couldn't run clk.out\n");
			exit(EXIT_FAILURE);
		}
		
	}
	
	// 4. Use this function after creating the clock process to initialize clock
	initClk();
	

	// TODO Generation Main Loop
	// 5. Create a data structure for processes and provide it with its parameters.
	///DONE
	while (1) {
		
		if (getClk() == curTime) continue;
		curTime = getClk();
		
		// 6. Send the information to the scheduler at the appropriate time.
		uint8_t exitFlag = 1;
		for(int i = 0; i < numberOfProcesses; i++) {
			if (processes[i].arrived == 0) {
				exitFlag = 0;

				if (processes[i].arrivalTime <= curTime) {
					processes[i].arrived = 1;
					
					struct msgbuff message;
					message.mtext = NULL;
					message.mtype = 1;
					message.proc = processes[i];

					if(msgsnd(msgqid, &message, sizeof(process_t) + sizeof(message.mtext), 0) == -1) {
						printf("process_generator: problem in sending to msg queue\n");
						exit(EXIT_FAILURE);
					}
					

					#ifdef DEBUG
					printf("process %d arrived at %d\n", processes[i].id, curTime);
					#endif
				}
			}
		}
		
		//notify the scheduler to work
		if (exitFlag == 1) {
                        kill(SIGMSGQ, schedPid);
                        up(semSchedGen);
                        break;
                }
		//up(semSchedGen);
	}

	wait(0);
	// 7. Clear clock resources
	destroyClk(true);
}

void clearResources(int signum)
{	
	//TODO Clears all resources in case of interruption
	free(processes);
    	msgctl(msgqid, IPC_RMID, (struct msqid_ds*) NULL);

	exit(EXIT_SUCCESS);
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
		perror("processe generator: Error while opening the file.\n");
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
			
			//null terminate the string passed to atoi
			numberSize++;
			number = (char*)realloc(number, numberSize);
			number[numberSize-1] = '\0';

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
		processes[processesNo - 1].arrived = 0;
	}
	free(line);
	
	fclose(fp);

	*numberOfProcesses = processesNo;
	return processes;
}

/**
 * @brief convert an integer to a null terminated string.
 * 
 * @param number the number to convert.
 * @return char* a pointer to the converted string.
 */
char* myItoa(int number)
{
	uint8_t ch;
	int numberSize = 0;
	char* numberStr = NULL;
	if(number != 0) {
		while((ch = number % 10)) {
			number /= 10;

			numberSize++;
			numberStr = (char*)realloc(numberStr, numberSize);
			numberStr[numberSize-1] = ch + '0';
		}
	}
	else {
		numberSize++;
		numberStr = (char*)realloc(numberStr, numberSize);
		numberStr[numberSize-1] = '0';		
	}

	//null terminate the string 
	numberSize++;
	numberStr = (char*)realloc(numberStr, numberSize);
	numberStr[numberSize-1] = '\0';
	
	return numberStr;
}
