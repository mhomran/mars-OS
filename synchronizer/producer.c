#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h> 
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/file.h>

#define SHM_SIZE_KEY 3000

/* arg for semctl system calls. */
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

void Down(int sem)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &p_op, 1) == -1)
    {
        perror("\n\nProducer: Error in down()\n");
        exit(-1);
    }
}

void Up(int sem)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &v_op, 1) == -1)
    {
        perror("\n\nProducer: Error in up()\n");
        exit(-1);
    }
}

void init(int buffsize){
    int shmid;
    shmid = shmget(SHM_SIZE_KEY, sizeof(int), IPC_CREAT|0644);
    if (shmid == -1)
    {
        perror("\n\nProducer: Error in create the shared memory for buffer size\n");
        exit(-1);
    }
    
    int *shmaddr = (int *)shmat(shmid, (void *)0, 0);
    if (shmaddr == -1)
    {
        perror("\n\nProducer: Error in attach the for buffer size shared memory\n");
        exit(-1);
    }
    
    *shmaddr = buffsize;
}

int main(){
    int buffsize;
    int memsize;

    printf("\n\nProducer: please enter the buffer size: ");
    scanf("%d", &buffsize);
    if(buffsize <= 0){
        perror("\n\nProducer: invalid buffer size\n");
        exit(-1);
    }
    buffsize--;
    memsize = buffsize*sizeof(int);

    init(buffsize);

    return 0;
}