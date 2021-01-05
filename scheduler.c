/**
 * @file scheduler.c
 * @author Ahmed Ashraf (ahmed.ashraf.cmp@gmail.com)
 * @brief Keeps track of the processes and their states and it decides which process will run and for how long.
 * @version 0.1
 * @date 2020-12-30
 */

#define DEBUG
#define RQSZ 1000

#include "headers.h"
#include "process_generator.h"
#include "priority_queue.h"


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

// Remaining time for current quantum
int currQ, nproc, schedulerType, quantum;

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
        quantum = atoi(argv[3]);

        
        //bind used signals
        signal(SIGMSGQ, ReadProcess);
        signal(SIGPF, ProcFinished);


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

        while (nproc) {

                if (procGenFinished == 0) down(semSchedGen);


                if (running != NULL) down(semSchedProc);
                
                printf("scheduler: #%d tick.\n", getClk());

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
        #ifdef DEBUG
        printf("Scheduler: process %d finished running at %d.\n", running->id, getClk());
        #endif

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
        printf("Scheduler: process %d arrived at %d\n", entry->id, getClk());
}

/**
 * @brief Schedule the processes using Non-preemptive Highest Priority First 
 * 
 */
void HPFSheduler()
{
        if (running == NULL) {
                running = ExtractMin(readyQueue);
                
                #ifdef DEBUG
                printf("process %d started running at %d.\n", running->id, getClk());
                #endif

                // Start a new process. (Fork it and give it its parameters.)
                *shmRemainingTimeAd = running->remainingTime;

                int pid;
                if ((pid = fork()) == 0) {
                        int rt = execl("build/process.out", "process.out", myItoa(running->id),
                                myItoa(running->arrivalTime), myItoa(running->runTime), myItoa(running->priority), NULL);

                        if (rt == -1) {
                                perror("\n\nScheduler: couldn't run scheduler.out\n");
                                exit(EXIT_FAILURE);
                        }
                }
                else {
                        running->pid = pid;
                }
        }
}

/**
 * @brief Schedule the processes using Shortest Remaining time Next
 * 
 */
void SRTNSheduler()
{
        if(running) {
                PCB* nextProc = Minimum(readyQueue);
                if(running->remainingTime > nextProc->remainingTime)  // Context Switching
                {
                        printf("scheduler: process %d has blocked at time %d", running->id, getClk());
                        running->state = BLOCKED;
                        InsertValue(readyQueue, running);
                        kill(running->pid, SIGSLP);
                }
                else return;
        }
        else if(running == NULL || running->state == BLOCKED) {
                running = ExtractMin(readyQueue); 
                if(running == READY){

                        #ifdef DEBUG
                        printf("process %d started running at %d.\n", running->id, getClk());
                        #endif

                        *shmRemainingTimeAd = running ->remainingTime;
                        if(fork() == 0){
                                int rt = execl("build/process.out", "process.out", myItoa(running->id),
                                myItoa(running->arrivalTime), myItoa(running->runTime), myItoa(running->priority), NULL);
                                if (rt == -1)
                                {
                                        perror("\n\nScheduler: couldn't run scheduler.out\n");
                                        exit(EXIT_FAILURE);
                                }
                        }
                }
                else if (running->state == BLOCKED)
                {
                        kill(running->pid, SIGSLP);
                }
        }
}

/**
 * @brief Schedule the processes using Round Robin
 * 
 */

void RRSheduler(int quantum)
{
  if (running)
  {
    currQ--;
    if (currQ == 0)
    {
      Enqueue(readyQueue, running);
      kill(running->pid, SIGSLP);
      running->state = BLOCKED;
    }
    else
      return;
  }

  currQ = quantum;
  running = Dequeue(readyQueue);
  if (running->state == READY)
  { // Start a new process. (Fork it and give it its parameters.)
    *shmRemainingTimeAd = running ->remainingTime;
    if (fork() == 0)
    {
      int rt = execl("build/process.out", "process.out", myItoa(running->id),
                     myItoa(running->arrivalTime), myItoa(running->runTime), myItoa(running->priority), NULL);
      if (rt == -1)
      {
        perror("scheduler: couldn't run scheduler.out\n");
        exit(EXIT_FAILURE);
      }
    }
  }
  else if (running->state == BLOCKED)
  {
    kill(running->pid, SIGSLP);
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