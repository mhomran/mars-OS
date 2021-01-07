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
#define FULL_SEM_KEY 300
#define EMPTY_SEM_KEY 400
#define MUTEX_SEM_KEY 500

int *memsizeAddr, *buff;
int full, empty, mutex, buffid, memsizeid, buffIter, buffsize;

/* arg for semctl system calls. */
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

void Down(int sem);
void Up(int sem);
int Init();
int* InitBuff(int memsize);
int CreateSem(int semkey);
void DestroySem(int sem);
void FreeResources();
void SignalHandler(int signum);
int RemoverItem();
void ConsumeItem(int item);
void InitSem(int sem, int value);

int main(){
	int memsize;

	/*Read buffer size from producer*/
	buffsize = Init();
	memsize = buffsize*sizeof(int);

	/*Initialize the buffer*/
	buff = InitBuff(memsize);

	/*Create full, empty, and mutex semaphores*/
	full = CreateSem(FULL_SEM_KEY);
	empty = CreateSem(EMPTY_SEM_KEY);
	mutex = CreateSem(MUTEX_SEM_KEY);

	/*Initialize the semaphores*/
	InitSem(full, 0);
	InitSem(empty, buffsize);
	InitSem(mutex, 1);

	/*Bind the SIGINT signal handler*/
	signal(SIGINT, SignalHandler);

	while(1){
		Down(full);                   /* decrement full count */
		Down(mutex);                  /* enter critical region */
		int item = RemoverItem();     /* take item from buffer */
		Up(mutex);                    /* leave critical region */
		Up(empty);                    /* increment count of empty slots */
		ConsumeItem(item);
	}

	return 0;
}

/**
 * @brief decrease semaphore by 1
 * 
 * @param sem semaphore id
 */
void Down(int sem)
{
	struct sembuf p_op;

	p_op.sem_num = 0;
	p_op.sem_op = -1;
	p_op.sem_flg = !IPC_NOWAIT;

	if (semop(sem, &p_op, 1) == -1){
		perror("\n\nConsumer: Error in down()\n");
		exit(-1);
	}
}

/**
 * @brief increase semaphore by 1
 * 
 * @param sem semaphore id
 */
void Up(int sem)
{
	struct sembuf v_op;

	v_op.sem_num = 0;
	v_op.sem_op = 1;
	v_op.sem_flg = !IPC_NOWAIT;

	if (semop(sem, &v_op, 1) == -1){
		perror("\n\nConsumer: Error in up()\n");
		exit(-1);
	}
}

/**
 * @brief Create or get the shared memory for passing buffer size
 * 
 * @return int 
 */
int Init()
{
	while ((memsizeid = shmget(SHM_SIZE_KEY, sizeof(int), 0644)) == -1){
		perror("\n\nConsumer: Shared memory for buffer size is not found, please run the producer\n");
		sleep(1);
	}
	
	memsizeAddr = (int *)shmat(memsizeid, (void *)0, 0);
	if (memsizeAddr == -1){
		perror("\n\nConsumer: Error in attach the for buffer size shared memory\n");
		exit(-1);
	}
	
	int buffsize = *memsizeAddr;
	return buffsize;
}

/**
 * @brief Get the shared memory buffer
 * 
 * @param memsize 
 * @return int* 
 */
int* InitBuff(int memsize)
{
	buffid = shmget(SHM_BUFF_KEY, memsize, IPC_CREAT|0644);
	if (buffid == -1){
		perror("\n\nConsumer: Error in create the shared memory for buffer size\n");
		exit(-1);
	}
	else printf("\n\nConsumer: Buffer id: %d\n", buffid);
	
	int* buff;
	buff = (int *)shmat(buffid, (void *)0, 0);
	if (buff == -1){
		perror("\n\nConsumer: Error in attach the for buffer\n");
		exit(-1);
	}
	else printf("\n\nConsumer: Buffer attached successfuly\n");

	return buff;
    
}

/**
 * @brief Create a Sem object
 * 
 * @param semkey 
 * @return int 
 */
int CreateSem(int semkey)
{
	int sem = semget(semkey, 1, 0666 | IPC_CREAT);
	if(sem == -1){
		perror("\n\nConsumer: Error in create sem\n");
		exit(-1);
	}
	return sem;
}

/**
 * @brief Remove the semaphore
 * 
 * @param sem 
 */
void DestroySem(int sem)
{
	if(semctl(sem, 0, IPC_RMID) == -1){
		perror("\n\nConsumer: Error in destroy sem\n");
		exit(-1);
	}
}

/**
 * @brief Before terminating free the used resources
 * 
 */
void FreeResources()
{
	shmdt((void*)buff);
	shmdt((void*)memsizeAddr);

	DestroySem(full);
	DestroySem(empty);
	DestroySem(mutex);

	shmctl(memsizeid, IPC_RMID, NULL);
	shmctl(buffid, IPC_RMID, NULL);
}

/**
 * @brief Handles SIGINT signal
 * 
 * @param signum 
 */
void SignalHandler(int signum)
{
	printf("\n\nConsumer: terminating...\n");
	FreeResources();
	exit(0);
}

/**
 * @brief Removes one item from shared buffer
 * 
 * @return int 
 */
int RemoverItem()
{
	int item;
	item = buff[buffIter];
	printf("\n\nConsumer: removed item from buffer\n");
	buffIter = (buffIter+1)%buffsize;
	return item;
}

/**
 * @brief Consumes the removed item from the buffer
 * 
 * @param item 
 */
void ConsumeItem(int item)
{
    	printf("\n\nConsumer: consumed item: %d\n", item);
}

/**
 * @brief Initialize the semaphore with a given value
 * 
 * @param sem 
 * @param value 
 */
void InitSem(int sem, int value)
{
	union Semun semun;
	semun.val = value; /* initial value of the semaphore, Binary semaphore */
	if (semctl(sem, 0, SETVAL, semun) == -1){
		perror("\n\nConsumer: Error in initializing semaphore\n");
		exit(-1);
	}
}