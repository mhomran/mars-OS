#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
bool blocked = 0;
void SigSleepHandler(int signum);

int main(int argc, char * argv[])
{
        key_t shmR_id;
        int* shmRaddr;
        union Semun semun;

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


        int semSchedProc = semget(SEM_SCHED_PROC_KEY, 1, 0666 | IPC_CREAT);

        if (semSchedProc == -1)
        {
		perror("Error in create sem");
		exit(-1);
        }

        semun.val = 0; /* initial value of the semaphore, Binary semaphore */
        if (semctl(semSchedProc, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
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
			
			if (remainingtime == 0){
				break;
			}


			up(semSchedProc);
                }
                while (blocked);
        }
        
        shmdt(shmRaddr);

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