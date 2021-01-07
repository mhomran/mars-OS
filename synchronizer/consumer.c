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
#define SHM_BUFF_KEY 4000

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
        perror("\n\nConsumer: Error in down()\n");
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
        perror("\n\nConsumer: Error in up()\n");
        exit(-1);
    }
}

int Init(){
    int shmid;
    shmid = shmget(SHM_SIZE_KEY, sizeof(int), IPC_CREAT|0644);
    if (shmid == -1)
    {
        perror("\n\nConsumer: Error in create the shared memory for buffer size\n");
        exit(-1);
    }
    
    int *shmaddr = (int *)shmat(shmid, (void *)0, 0);
    if (shmaddr == -1)
    {
        perror("\n\nConsumer: Error in attach the for buffer size shared memory\n");
        exit(-1);
    }
    
    int buffsize = *shmaddr;
    return buffsize;
}

int* InitBuff(int memsize){
    int shmid;
    shmid = shmget(SHM_BUFF_KEY, memsize, IPC_CREAT|0644);
    if (shmid == -1)
    {
        perror("\n\nConsumer: Error in create the shared memory for buffer size\n");
        exit(-1);
    }
    else printf("\n\nConsumer: Buffer id: %d\n", shmid);
    
    int* buff;
    buff = (int *)shmat(shmid, (void *)0, 0);
    if (buff == -1)
    {
        perror("\n\nConsumer: Error in attach the for buffer\n");
        exit(-1);
    }
    else printf("\n\nConsumer: Buffer attached successfuly\n");

    return buff;
    
}

int main(){
    int memsize, buffsize;
    
    buffsize = Init();
    memsize = buffsize*sizeof(int);

    int* buff = InitBuff(memsize);

    return 0;
}