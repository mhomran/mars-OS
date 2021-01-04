#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
bool blocked = 0;
void SigSleepHandler(int signum);

int main(int argc, char * argv[])
{
        key_t shmR_id;
        int* shmRaddr;

        shmR_id = shmget(PRSHKEY, 4, IPC_CREAT | 0644);
        if (shmR_id == -1) {
                perror("Process: Failed to get the shared memory\n");
                exit(EXIT_FAILURE);
        }

        shmRaddr = (int *) shmat(shmR_id, (void *)0, 0);
        if ((long)shmRaddr == -1) {
                perror("Process: Error in attaching the shm in scheduler!\n");
                exit(EXIT_FAILURE);
        }

        if (argc < 5) {
                perror("Process: Not enough argument\n");
                exit(EXIT_FAILURE);
        }

        remainingtime = atoi(argv[3]);

        initClk();

        //TODO it needs to get the remaining time from somewhere
        //remainingtime = ??;
        *shmRaddr = remainingtime;
        int curTime = getClk();

        signal(SIGSLP, SigSleepHandler);

        while (remainingtime > 0) {
                if (getClk() != curTime) {
                        remainingtime -= 1;
                        curTime = getClk();
                        *shmRaddr = remainingtime;
                }
                while (blocked);
        }
        
        shmdt(shmRaddr);

        destroyClk(false);

        //notify the scheduler that this process is finished
        kill(getppid(), SIGPF);

        return 0;
}


void SigSleepHandler(int signum)
{
        blocked = !blocked;
        signal(SIGSLP, SigSleepHandler);
}