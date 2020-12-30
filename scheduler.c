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
struct Queue* rq;
PCB* running;

struct msgbuff
{
    long mtype;
    process_t proc;
};

/**
 * @brief Reads the message queue and push the new processes
 * to the ready queue
 * @param wait 1 if it has to wait 0 otherwise
 */
void ReadMSGQ(short wait);
/**
 * @brief Signal handler for the SIGUSR to handle arriving of a new process
 *
 * @param signum SIGUSR flag
 */
void ReadProcess(int signum);
/**
 * @brief Create a PCB object and insert it in the ready queue
 * 
 * @param entry process object
 */
void CreateEntry(process_t entry);
/**
 * @brief Schedule the processes using Non-preemptive Highest Priority First 
 * 
 */
void HPFSheduler();
/**
 * @brief Schedule the processes using Shortest Remaining time Next
 * 
 */
void SRTNSheduler();
/**
 * @brief Schedule the processes using Round Robin
 * 
 */
void RRSheduler();

int main(int argc, char * argv[])
{
    initClk();

    if(argc < 3){
        perror("\n\nScheduler: Not enough argument\n");
        exit(-1);
    }
    int schedulerType = atoi(argv[1]);
    int nproc = atoi(argv[2]);

    msgqid = msgget(MSGQKEY, 0644);
    if(msgqid == -1){
        perror("\n\nScheduler: Failed to get the message queue\n");
        exit(-1);
    }

    signal(SIGMSGQ, ReadProcess);
    rq = createQueue(RQSZ);
    running = NULL;

    while(nproc){
        // If the ready queue is empty, and still there are processes, wait 
        while(nproc && isEmpty(rq));

        while(!isEmpty(rq)){
            switch (schedulerType)
            {
            case 1: SRTNSheduler();
                break;
            case 2: RRSheduler();
                break;
            default: HPFSheduler();
                break;
            }
        }
    }


    // upon termination release the clock resources.
    destroyClk(true);
}

void ReadMSGQ(short wait){
    while(1){
        int recVal;
        struct msgbuff msg;

        recVal = msgrcv(msgqid, &msg, sizeof(msg.proc), 0, wait ? !IPC_NOWAIT : IPC_NOWAIT);  // Try to recieve the new process
        if(recVal == -1){
            // If there is no process recieved then break
            break;
        }

        // If successfuly recieved the new process add it to the ready queue
        CreateEntry(msg.proc);
    }
}

void ReadProcess(int signum){
    ReadMSGQ(0);
}

void CreateEntry(process_t proc){
    PCB* entry = (PCB*)malloc(sizeof(PCB));
    entry->pid = proc.id;
    entry->arrivalTime = proc.arrivalTime;
    entry->runTime = proc.runTime;
    entry->priority = proc.priority;
    entry->state = READY;
    enqueue(rq, entry);
}

void HPFSheduler(){
    if(running) enqueue(rq, running);
    running = dequeue(rq);
    if(running->state = READY){ // Start a new process. (Fork it and give it its parameters.)
        if(fork() == 0){
            execl("process.out", "process.out", running->pid, running->arrivalTime, running->runTime, running->priority, NULL);
        }
    }
    else if(running->state == FINISHED) {
        // Release the PCB resources
        free(running);
    }
}