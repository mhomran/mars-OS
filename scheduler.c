/**
 * @file scheduler.c
 * @author Ahmed Ashraf (ahmed.ashraf.cmp@gmail.com)
 * @brief Keeps track of the processes and their states and it decides which process will run and for how long.
 * @version 0.1
 * @date 2020-12-30
 */

#include "headers.h"
#include "process_generator.h"
#include "PCB.h"
#include "RQ.h"

#define RQSZ 1000

key_t msgqid;
int *shmRaddr;
struct Queue *rq;
PCB *running;

// Remaining time for current quantum
int currQ, nproc, schedulerType;

struct msgbuff
{
  long mtype;
  process_t proc;
};

// Functions declaration
void ReadMSGQ(short wait);
void ReadProcess(int signum);
void CreateEntry(process_t entry);
void HPFSheduler();
void SRTNSheduler();
void RRSheduler(int q);
char *myItoa(int number);
void ProcFinished(int signum);

int main(int argc, char *argv[])
{
  initClk();

  if (argc < 3)
  {
    perror("\n\nScheduler: Not enough argument\n");
    exit(-1);
  }
  schedulerType = atoi(argv[1]);
  nproc = atoi(argv[2]);

  msgqid = msgget(MSGQKEY, 0644);
  if (msgqid == -1)
  {
    perror("\n\nScheduler: Failed to get the message queue\n");
    exit(-1);
  }

  // Create shared memory for remaining time of running process
  key_t shmR_id = shmget(PRSHKEY, 4, IPC_CREAT | 0644);
  if (shmR_id == -1)
  {
    perror("\n\nScheduler: Failed to get the shared memory\n");
    exit(-1);
  }

  // attach to shared memory of runnign process
  shmRaddr = (int *)shmat(shmR_id, (void *)0, 0);
  if ((long)shmRaddr == -1)
  {
    perror("Error in attaching the shm in scheduler!");
    exit(-1);
  }

  signal(SIGMSGQ, ReadProcess);
  signal(SIGPF, ProcFinished);
  rq = createQueue(RQSZ);
  running = NULL;

  int currTime = getClk();
  while (nproc)
  {
    // If the ready queue is empty, and still there are processes, wait
    while (nproc && isEmpty(rq));

    while (!isEmpty(rq))
    {
      if (getClk() == currTime && running)
        continue;
      currTime = getClk();
      switch (schedulerType)
      {
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
  destroyClk(true);
}

/**
 * @brief Reads the message queue and push the new processes
 * to the ready queue
 * @param wait 1 if it has to wait 0 otherwise
 */
void ReadMSGQ(short wait)
{
  while (1)
  {
    int recVal;
    struct msgbuff msg;

    recVal = msgrcv(msgqid, &msg, sizeof(msg.proc), 0, wait ? !IPC_NOWAIT : IPC_NOWAIT); // Try to recieve the new process
    if (recVal == -1)
    {
      // If there is no process recieved then break
      break;
    }

    // If successfuly recieved the new process add it to the ready queue
    CreateEntry(msg.proc);
  }
}

/**
 * @brief Signal handler for the SIGUSR to handle arriving of a new process
 *
 * @param signum SIGUSR flag
 */
void ReadProcess(int signum)
{
  ReadMSGQ(0);
  switch (schedulerType)
      {
      case 0:
        SRTNSheduler();
        break;

      case 2:
        HPFSheduler();
        break;
      default:
      break;
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
    insert_value(rq, entry);
    break;
  case 1:
    enqueue(rq, entry);
    break;
  case 2:
    insert_value(rq, entry);
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
  if (running)
  {
    if (running -> remainingTime != *shmRaddr)
      running ->remainingTime = *shmRaddr;
    return;
  }

  running = extract_min(rq);
  if (running->state == READY)
  { // Start a new process. (Fork it and give it its parameters.)
    *shmRaddr = running ->remainingTime;
    int pid;
    if ((pid = fork()) == 0)
    {
      int rt = execl("build/process.out", "process.out", myItoa(running->id),
                     myItoa(running->arrivalTime), myItoa(running->runTime), myItoa(running->priority), NULL);
      if (rt == -1)
      {
        perror("scheduler: couldn't run scheduler.out\n");
        exit(EXIT_FAILURE);
      }
    }
    else
    {
      running ->pid = pid;
    }
    
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

/**
 * @brief Schedule the processes using Shortest Remaining time Next
 * 
 */
void SRTNSheduler()
{
  //STUB
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
      enqueue(rq, running);
      kill(running->pid, SIGSLP);
      running->state = BLOCKED;
    }
    else
      return;
  }

  currQ = quantum;
  running = dequeue(rq);
  if (running->state == READY)
  { // Start a new process. (Fork it and give it its parameters.)
    *shmRaddr = running ->remainingTime;
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

void ProcFinished(int signum)
{
  running -> state = FINISHED;
  free(running);
  running = NULL;
  nproc--;
  signal(SIGPF, ProcFinished);
}