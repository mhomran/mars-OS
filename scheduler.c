/**
 * @file scheduler.c
 * @author Ahmed Ashraf (ahmed.ashraf.cmp@gmail.com)
 * @brief Keeps track of the processes and their states and it decides which process will run and for how long.
 * @version 0.1
 * @date 2020-12-30
 */

#include "headers.h"
#include "porcess.h"
#include "PCB.h"

key_t msgqid;

struct msgbuff
{
    long mtype;
    process proc;
};

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
void CreateEntry(process entry);

int main(int argc, char * argv[])
{
    initClk();
    msgqid = msgget(MSGQKEY, 0644);
    if(msgqid == -1){
        perror("\n\nScheduler: Failed to get the message queue\n");
        exit(-1);
    }
    signal(SIGMSGQ, ReadProcess);

    // 1.  Start a new process. (Fork it and give it its parameters.)
    if(fork() == 0){
        execl("process.out", "process.out", NULL);
    }
    
    
    //upon termination release the clock resources.
    
    destroyClk(true);
}

void ReadProcess(int signum){
    while(1){
        int recVal;
        struct msgbuff msg;

        recVal = msgrcv(msgqid, &msg, sizeof(msg.proc), 0, IPC_NOWAIT);  // Try to recieve the new process
        if(recVal == -1){
            // If there is no process recieved then break
            break;
        }
        // If successfuly recieved the new process add it to the ready queue
        CreateEntry(msg.proc);
    }
}

void CreateEntry(process entry){

}