/**
 * @file scheduler.c
 * @author Ahmed Ashraf (ahmed.ashraf.cmp@gmail.com)
 * @brief Keeps track of the processes and their states and it decides which process will run and for how long.
 * @version 0.1
 * @date 2020-12-30
 */

#define RQSZ 1000

#include <math.h>
#include "headers.h"
#include "process_generator.h"
#include "priority_queue.h"

/**
 * \struct 
 * @brief struct for messages of the message queue
 * 
 */
struct msgbuff
{
        long mtype;
        process_t proc;
};


key_t mqProcesses;
PCB *running;
struct Queue *readyQueue;
int *shmRemainingTimeAd;
int procGenFinished = 0;

FILE *outputFile;

// Remaining time for current quantum
int currQuantum, nproc, schedulerType, quantum;
float avgWTA = 0, *WTAs, avgWaiting = 0;
int numProcesses;

// Functions declaration
void ReadMSGQ(short wait);
void CreateEntry(process_t entry);
void HPFSheduler();
void SRTNSheduler();
void RRSheduler(int q);
char *myItoa(int number);

void ReadProcess(int signum);
void ProcFinished(int signum);

/**
 * @brief the main program of the schulder.c
 * 
 * @param argc the number of the arguments passed
 * @param argv array of string containing the arguments
 * @return int 0 if everything is okay
 */
int main(int argc, char *argv[])
{
        //attach to clock
        initClk();

        // Create output file
        outputFile = fopen("Scheduler.log", "w");
        if (outputFile == NULL) {
                perror("Schedular: Can not create output file\n");
                exit(EXIT_FAILURE);
        }
        fprintf(outputFile, "#At time x process y state arr w total z remain y wait k\n");

        //initialize variables
        readyQueue = CreateQueue(RQSZ);
        running = NULL;

        //parse arguments
        if (argc < 4) {
                perror("Scheduler: Not enough argument\n");
                exit(EXIT_FAILURE);
        }

        schedulerType = atoi(argv[1]);
        nproc = atoi(argv[2]);
        numProcesses = nproc;
        WTAs = (float *)malloc(sizeof(float) * numProcesses);
        quantum = atoi(argv[3]);

        
        //bind used signals
        signal(SIGMSGQ, ReadProcess);
        signal(SIGPF, ProcFinished);

        // message queue
        mqProcesses = msgget(MSGQKEY, 0644);
        if (mqProcesses == -1) {
                perror("\n\nScheduler: Failed to get the message queue\n");
                exit(EXIT_FAILURE);
        }

        // shared memory remaining time 
        key_t shmRemainingTime = shmget(PRSHKEY, 4, IPC_CREAT | 0644);
        if (shmRemainingTime == -1) {
                perror("Scheduler: Failed to get the shared memory\n");
                exit(EXIT_FAILURE);
        }

        // attach to it
        shmRemainingTimeAd = (int *)shmat(shmRemainingTime, (void *)0, 0);
        if ((long)shmRemainingTimeAd == -1) {
                perror("\n\nError in attaching the shm in scheduler!\n");
                exit(EXIT_FAILURE);
        }
        
        //Semaphore semSchedProc
        int semSchedProc = semget(SEM_SCHED_PROC_KEY, 1, 0666 | IPC_CREAT);

        if (semSchedProc == -1){
                perror("Error in create sem");
                exit(EXIT_FAILURE);
        }

        union Semun semun;
        semun.val = 0;
        if (semctl(semSchedProc, 0, SETVAL, semun) == -1){
                perror("Error in semctl");
                exit(-1);
        }

        //Semaphore semSchedGen
        int semSchedGen = semget(SEM_SCHED_GEN_KEY, 1, 0666);

        if (semSchedGen == -1){
                perror("Error in create sem");
                exit(EXIT_FAILURE);
        }


        int curTime = -1;
        while (nproc) {

                if (getClk() == curTime) continue;
                curTime = getClk();
                if (procGenFinished == 0) down(semSchedGen);
                if (running != NULL) down(semSchedProc);
                
                
                ReadMSGQ(0);
                
                if(!IsEmpty(readyQueue)) {
                        switch (schedulerType){
                        case 0:
                        SRTNSheduler();
                        break;

                        case 1:
                        RRSheduler(quantum);
                        break;

                        default:
                        HPFSheduler();
                        break;
                        }
                }
        }


        // upon termination release the clock resources.
        destroyClk(false);
        if (semctl(semSchedProc, 1, IPC_RMID) == -1) {
                perror("scheduler: can't remove semaphore semSchedProc \n");
        }

        if (shmdt(shmRemainingTimeAd) == -1) {
              printf("scheduler: error in detaching a shared memory\n");  
        }

        if (shmctl(shmRemainingTime, IPC_RMID, (struct shmid_ds*) 0) == -1) {
                perror("scheduler: can't remove remaining time schared memory \n");
        }
        fclose(outputFile);

        // Create output file
        outputFile = fopen("Scheduler.perf", "w");
        if (outputFile == NULL) {
                perror("Schedular: Can not create performance file\n");
                exit(EXIT_FAILURE);
        }

        avgWTA /= numProcesses;
        avgWaiting /= numProcesses;

        float stdDev = 0;
        for (int i = 0; i < numProcesses; i++)
        {
                stdDev = pow((double) (WTAs[i] - avgWTA), 2);     
        }

        stdDev = sqrt(stdDev);        

        fprintf(outputFile, "CPU utilization = 100%% \n");
        fprintf(outputFile, "avg WTA: %g\navgWaiting:%g\nstd WTA:%g\n", round(avgWTA * 100.0) / 100.0, round(avgWaiting * 100.0) / 100.0, round(stdDev * 100.0) / 100.0);

        #ifdef DEBUG
        printf("avg WTA = %g\navgWaiting = %g\nstd WTA = %g\n", round(avgWTA * 100.0) / 100.0, round(avgWaiting * 100.0) / 100.0, round(stdDev * 100.0) / 100.0);
        #endif
        fclose(outputFile);
        free(WTAs);
}

/**
 * @brief Signal handler for the SIGUSR to handle arriving of a new process
 *
 * @param signum SIGUSR flag
 */
void ReadProcess(int signum)
{        
        procGenFinished = 1;
}

/**
 * @brief this a handler of SIGPF signal of this process to handle
 * when a process is finished.
 * 
 * @param signum SIGPF
 */
void ProcFinished(int signum)
{
        int ta = getClk()-running->arrivalTime;
        float wta = ((float)ta) / running->runTime;
        
        avgWaiting += running->waitingTime;
        avgWTA += wta;

        WTAs[running->id - 1] = wta;

        running->remainingTime = *shmRemainingTimeAd;
        
        

        fprintf(outputFile, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %g\n", 
        getClk(), running->id, running->arrivalTime, running->runTime, running->remainingTime, running->waitingTime,
        ta, round(wta * 100.0) / 100.0);

        #ifdef DEBUG
        printf("At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %0.2g\n", 
        getClk(), running->id, running->arrivalTime, running->runTime, running->remainingTime, running->waitingTime,
        ta, wta);
        #endif

        int stat;
        waitpid(running->pid, &stat, 0);
        free(running);
        running = NULL;
        nproc--;

        //rebind the signal handler
        signal(SIGPF, ProcFinished);
}

/**
 * @brief Reads the message queue and push the new processes
 * to the ready queue
 * @param wait 1 if it has to wait 0 otherwise
 */
void ReadMSGQ(short wait)
{
        while (1) {
                int recVal;
                struct msgbuff msg;

                // Try to recieve the new process
                recVal = msgrcv(mqProcesses, &msg, sizeof(msg.proc), 0, wait ? !IPC_NOWAIT : IPC_NOWAIT); 
                if (recVal == -1) {
                        // If there is no process recieved then break
                        break;
                }

                // If successfuly recieved the new process add it to the ready queue
                CreateEntry(msg.proc);
        }
}


/**
 * @brief Create a PCB object and insert it in the ready queue
 * 
 * @param entry process object
 */
void CreateEntry(process_t proc)
{
        PCB *entry = (PCB *)malloc(sizeof(PCB));
        entry->id = proc.id;
        entry->arrivalTime = proc.arrivalTime;
        entry->runTime = proc.runTime;
        entry->priority = proc.priority;
        entry->state = READY;
        entry->remainingTime = proc.runTime;
        entry->waitingTime = 0;
        
        switch (schedulerType)
        {
        case 0:
        entry->priority = entry->remainingTime;
        InsertValue(readyQueue, entry);
        // if(running) SRTNSheduler();  // Should be called again to check that the current runnning proc is the SRTN
        break;
        case 1:
        Enqueue(readyQueue, entry);
        break;
        case 2:
        InsertValue(readyQueue, entry);
        break;
        default:
        break;
        }
}

/**
 * @brief Schedule the processes using Non-preemptive Highest Priority First 
 * 
 */
void HPFSheduler()
{
        if (running == NULL) {
                running = ExtractMin(readyQueue);
                

                // Start a new process. (Fork it and give it its parameters.)
                *shmRemainingTimeAd = running->remainingTime;
                
                // Setting waiting time
                running->waitingTime = getClk() - running->arrivalTime;

                fprintf(outputFile, "At time %d process %d started arr %d total %d remain %d wait %d\n", 
                getClk(), running->id, running->arrivalTime, running->runTime, running->remainingTime, running->waitingTime);

                #ifdef DEBUG
                printf("At time %d process %d started arr %d total %d remain %d wait %d\n", 
                getClk(), running->id, running->arrivalTime, running->runTime, running->remainingTime, running->waitingTime);
                #endif

                int pid;
                if ((pid = fork()) == 0) {
                        int rt = execl("build/process.out", "process.out", NULL);

                        if (rt == -1) {
                                perror("\n\nScheduler: couldn't run scheduler.out\n");
                                exit(EXIT_FAILURE);
                        }
                }
                else {
                        running->pid = pid;
                }
        }
        else    running->remainingTime = *shmRemainingTimeAd;
        
        
}

/**
 * @brief Schedule the processes using Shortest Remaining time Next
 * 
 */
void SRTNSheduler()
{
        if(running) {
                PCB* nextProc = Minimum(readyQueue);
                running ->remainingTime = *shmRemainingTimeAd;
                running ->priority = *shmRemainingTimeAd;

                if(running->remainingTime > nextProc->remainingTime)  // Context Switching
                {
                        
                        fprintf(outputFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n", 
                        getClk(), running->id, running->arrivalTime, running->runTime, running->remainingTime, running->waitingTime);
                        
                        #ifdef DEBUG
                        printf("At time %d process %d stopped arr %d total %d remain %d wait %d\n", 
                        getClk(), running->id, running->arrivalTime, running->runTime, running->remainingTime, running->waitingTime);
                        #endif
                        running->state = BLOCKED;
                        running->waitStart = getClk();
                        InsertValue(readyQueue, running);
                        kill(running->pid, SIGSLP);
                }
                else return;
        }

        if(running == NULL || running->state == BLOCKED) {
                running = ExtractMin(readyQueue); 
                if(running->state == READY){

                        int pid;

                        *shmRemainingTimeAd = running ->remainingTime;
                        
                        // Setting initial waiting time
                        running->waitingTime = getClk() - running->arrivalTime;
                        
                        fprintf(outputFile, "At time %d process %d started arr %d total %d remain %d wait %d\n", 
                        getClk(), running->id, running->arrivalTime, running->runTime, running->remainingTime, running->waitingTime);
                        
                        #ifdef DEBUG
                        printf("At time %d process %d started arr %d total %d remain %d wait %d\n", 
                        getClk(), running->id, running->arrivalTime, running->runTime, running->remainingTime, running->waitingTime);
                        #endif
                        
                        if((pid = fork()) == 0){
                                int rt = execl("build/process.out", "process.out", NULL);
                                if (rt == -1)
                                {
                                        perror("\n\nScheduler: couldn't run scheduler.out\n");
                                        exit(EXIT_FAILURE);
                                }
                        }
                        else {
                          running -> pid = pid;
                        }
                }
                else if (running->state == BLOCKED)
                {
                        kill(running->pid, SIGSLP);
                        running -> state = READY;
                        running->waitingTime += getClk() - running->waitStart;
                        
                        fprintf(outputFile, "At time %d process %d resumed arr %d total %d remain %d wait %d\n", 
                        getClk(), running->id, running->arrivalTime, running->runTime, running->remainingTime, running->waitingTime);
                        
                        #ifdef DEBUG
                        printf("At time %d process %d resumed arr %d total %d remain %d wait %d\n", 
                        getClk(), running->id, running->arrivalTime, running->runTime, running->remainingTime, running->waitingTime);
                        #endif
                }
        }
}

/**
 * @brief Schedule the processes using Round Robin
 * 
 */

void RRSheduler(int quantum)
{
        if (running){
                currQuantum--;
                running->remainingTime = *shmRemainingTimeAd;
                if (currQuantum == 0){
                        Enqueue(readyQueue, running);
                        kill(running->pid, SIGSLP);
                        running->state = BLOCKED;
                        running->waitStart = getClk();
                        
                        fprintf(outputFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n", 
                        getClk(), running->id, running->arrivalTime, running->runTime, running->remainingTime, running->waitingTime);
                        
                        #ifdef DEBUG
                        printf("At time %d process %d stopped arr %d total %d remain %d wait %d\n", 
                        getClk(), running->id, running->arrivalTime, running->runTime, running->remainingTime, running->waitingTime);
                        #endif
                }
                else
                return;
        }

        currQuantum = quantum;
        running = Dequeue(readyQueue);
        if (running->state == READY){ 
                // Start a new process. (Fork it and give it its parameters.)
                int pid;
                *shmRemainingTimeAd = running ->remainingTime;
                
                // Setting initial waiting time
                running->waitingTime = getClk() - running->arrivalTime;
                
                fprintf(outputFile, "At time %d process %d started arr %d total %d remain %d wait %d\n", 
                getClk(), running->id, running->arrivalTime, running->runTime, running->remainingTime, running->waitingTime);
                
                #ifdef DEBUG
                printf("At time %d process %d started arr %d total %d remain %d wait %d\n", 
                getClk(), running->id, running->arrivalTime, running->runTime, running->remainingTime, running->waitingTime);
                #endif
                
                if ((pid = fork()) == 0){
                        int rt = execl("build/process.out", "process.out", NULL);
                        if (rt == -1){
                                perror("scheduler: couldn't run scheduler.out\n");
                                exit(EXIT_FAILURE);
                        }
                }
                else {
                        running -> pid = pid;
                }
        }
        else if (running->state == BLOCKED){
                kill(running->pid, SIGSLP);
                running->state = READY;
                running->waitingTime += getClk() - running->waitStart;
                
                fprintf(outputFile, "At time %d process %d resumed arr %d total %d remain %d wait %d\n", 
                getClk(), running->id, running->arrivalTime, running->runTime, running->remainingTime, running->waitingTime);
                
                #ifdef DEBUG
                printf("At time %d process %d resumed arr %d total %d remain %d wait %d\n", 
                getClk(), running->id, running->arrivalTime, running->runTime, running->remainingTime, running->waitingTime);
                #endif
        }
  
}


/**
 * @brief convert an integer to a null terminated string.
 * 
 * @param number the number to convert.
 * @return char* a pointer to the converted string.
 */
char *myItoa(int number)
{
  uint8_t ch;
  int numberSize = 0;
  char *numberStr = NULL;
  if (number != 0)
  {
    while ((ch = number % 10))
    {
      number /= 10;

      numberSize++;
      numberStr = (char *)realloc(numberStr, numberSize);
      numberStr[numberSize - 1] = ch + '0';
    }
  }
  else
  {
    numberSize++;
    numberStr = (char *)realloc(numberStr, numberSize);
    numberStr[numberSize - 1] = '0';
  }

  //null terminate the string
  numberSize++;
  numberStr = (char *)realloc(numberStr, numberSize);
  numberStr[numberSize - 1] = '\0';

  return numberStr;
}