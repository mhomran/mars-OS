#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
bool sleeped = 0;
void SigSleepHandler(int signum);

int main(int agrc, char * argv[])
{
    key_t shmR_id = shmget(PRSHKEY, 4, IPC_CREAT | 0644);
    if (shmR_id == -1)
    {
        perror("\n\nScheduler: Failed to get the shared memory\n");
        exit(-1);
    }
    int* shmRaddr = (int *) shmat(shmR_id, (void *)0, 0);
    
    if ((long)shmRaddr == -1)
    {
        perror("Error in attaching the shm in scheduler!");
        exit(-1);
    }

    initClk();
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    remainingtime = *shmRaddr;
    int curTime = getClk();

    signal(SIGSLP, SigSleepHandler);

    while (remainingtime > 0)
    {
        if (getClk() != curTime)
        {
          remainingtime -= 1;
          curTime = getClk();
          *shmRaddr = remainingtime;
        }
        while (sleeped);
    }
    shmdt(shmRaddr);
    destroyClk(false);
    kill(getppid(), SIGPF);
    
    return 0;
}


void SigSleepHandler(int signum)
{
  sleeped = !sleeped;
  signal(SIGSLP, SigSleepHandler);
}