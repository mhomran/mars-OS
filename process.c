#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
bool blocked = 0;
void SigSleepHandler(int signum);

int main(int argc, char * argv[])
{
        signal(SIGSLP, SigSleepHandler);

        key_t shmRemainingTime;
        int* shmRemainingTimeAd;

        shmRemainingTime = shmget(PRSHKEY, 4, 0644);
        if (shmRemainingTime == -1) {
                perror("Process: Failed to get the shared memory\n");
                exit(EXIT_FAILURE);
        }

        shmRemainingTimeAd = (int *) shmat(shmRemainingTime, (void *)0, 0);
        if ((long)shmRemainingTimeAd == -1) {
                perror("Process: Error in attaching the shm in scheduler!\n");
                exit(EXIT_FAILURE);
        }


        int semSchedProc = semget(SEM_SCHED_PROC_KEY, 1, 0666);

        if (semSchedProc == -1)
        {
            perror("Error in create sem");
            exit(-1);
        }

        
        initClk();

        //TODO it needs to get the remaining time from somewhere
        //remainingtime = ??;

        remainingtime = *shmRemainingTimeAd;
        int curTime = getClk();


        while (remainingtime > 0) {
                if (getClk() != curTime) {
                        remainingtime -= 1;
                        curTime = getClk();
                        *shmRemainingTimeAd = remainingtime;
			if (remainingtime == 0) break;
			
			up(semSchedProc);
                }
                while (blocked);
        }
        
        //clear resources
        if (shmdt(shmRemainingTimeAd) == -1) {
              printf("process: error in detaching a shared memory\n");  
        }
        //detach the clock
        destroyClk(false);

        //notify the scheduler that this process is finished
        kill(getppid(), SIGPF);

	up(semSchedProc);
        
        return 0;
}


void SigSleepHandler(int signum)
{
        blocked = !blocked;
        signal(SIGSLP, SigSleepHandler);
}