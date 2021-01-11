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
#define SHM_READ_INDEX_KEY 5000
#define SHM_WRITE_INDEX_KEY 6000
#define FULL_SEM_KEY 300
#define EMPTY_SEM_KEY 400
#define MUTEX_SEM_KEY 500
#define BUFFSIZE 5


int *memSizeAddr, *buff, *readIndex, *writeIndex;
int full, empty, mutex, buffId, readIndexId, writeIndexId; 

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
void InitIndices();
int *InitBuff(int memSize);
int GetSem(int semkey);
void DestroySem(int sem);
void FreeResources();
void SignalHandler(int signum);
int RemoverItem();
void ConsumeItem(int item);
void InitSem(int sem, int value);


int main()
{
	int memSize;

	/* Buffer size in bytes */
	memSize = BUFFSIZE * sizeof(int);

	/* Initialize the buffer */
	buff = InitBuff(memSize);

	/* Get full, empty, and mutex semaphores if they exist and wait until a producer creates them if not */
	full = GetSem(FULL_SEM_KEY);
	empty = GetSem(EMPTY_SEM_KEY);
	mutex = GetSem(MUTEX_SEM_KEY);

	// Get reading and writing indices 
	InitIndices();

	/* Bind the SIGINT signal handler */
	signal(SIGINT, SignalHandler);

	while (1) {
		Down(full);               /* decrement full count */
		Down(mutex);              /* enter critical region */
		int item = RemoverItem(); /* take item from buffer */
		Up(mutex);                /* leave critical region */
		Up(empty);                /* increment count of empty slots */
		ConsumeItem(item);
		sleep(5);		  /* for debugging */
	}

	return 0;
}

/**
 * @brief Decrease semaphore by 1
 * 
 * @param sem Demaphore id
 */
void Down(int sem)
{
	struct sembuf p_op;

	p_op.sem_num = 0;
	p_op.sem_op = -1;
	p_op.sem_flg = !IPC_NOWAIT;

	if (semop(sem, &p_op, 1) == -1) {
		perror("\n\nConsumer: Error in down()\n");
		exit(-1);
	}
}

/**
 * @brief Increase semaphore by 1
 * 
 * @param sem Semaphore id
 */
void Up(int sem)
{
	struct sembuf v_op;

	v_op.sem_num = 0;
	v_op.sem_op = 1;
	v_op.sem_flg = !IPC_NOWAIT;

	if (semop(sem, &v_op, 1) == -1) {
		perror("\n\nConsumer: Error in up()\n");
		exit(-1);
	}
}

/**
 * @brief Get indices for reading and writing in the buffer
 * 
 */
void InitIndices()
{
	// Get shared memories for indices
	readIndexId = shmget(SHM_READ_INDEX_KEY, sizeof(int), 0644);
	writeIndexId = shmget(SHM_WRITE_INDEX_KEY, sizeof(int), 0644);
  
	if (readIndexId == -1 || writeIndexId == -1) {
		perror("\n\nConsumer: Error in getting the shared memory for indices\n"); // somehow 
		exit(-1);
	}

	// Attach this consumer to the shared memories
	readIndex = (int *)shmat(readIndexId, (void *)0, 0);
	writeIndex = (int *)shmat(writeIndexId, (void*) 0, 0);

	if (readIndex == (int *)-1 || writeIndex == (int *)-1) {
		perror("\n\nProducer: Error in attach the for indices shared memory\n");
		exit(-1);
	}
}

/**
 * @brief Get the shared memory buffer
 * 
 * @param memSize 
 * @return int* 
 */
int *InitBuff(int memSize)
{
	buffId = shmget(SHM_BUFF_KEY, memSize, IPC_CREAT | 0644);
	if (buffId == (int)-1) {
		perror("\n\nConsumer: Error in create the shared memory for buffer size\n");
		exit(-1);
  	}
  	else printf("\n\nConsumer: Buffer id: %d\n", buffId);

	int *buff;
	buff = (int *)shmat(buffId, (void *)0, 0);
	if (buff == (int *)-1) {
		perror("\n\nConsumer: Error in attach the for buffer\n");
		exit(-1);
	} 
	else printf("\n\nConsumer: Buffer attached successfuly\n");

	return buff;
}

/**
 * @brief Get semaphore with semkey
 * 
 * @param semkey 
 * @return int
 */
int GetSem(int semkey)
{
	int sem = semget(semkey, 1, 0666);
	while (sem == -1) {
		printf("Wait until a producer is created\n");
		sleep(1);
		sem = semget(semkey, 1, 0666);
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
	if (semctl(sem, 0, IPC_RMID) == -1) {
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
	shmdt((void *)buff);
	shmdt((void *)memSizeAddr);
	shmdt((void *) readIndex);
	shmdt((void *) writeIndex);

	DestroySem(full);
	DestroySem(empty);
	DestroySem(mutex);

	
	shmctl(readIndexId, IPC_RMID, NULL);
	shmctl(writeIndexId, IPC_RMID, NULL);
	shmctl(buffId, IPC_RMID, NULL);
	shmctl(buffId, IPC_RMID, NULL);
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
	item = buff[*readIndex];
	printf("\n\nConsumer: removed item from buffer index %d\n", *readIndex);
	*readIndex = (*readIndex + 1) % BUFFSIZE;
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
	if (semctl(sem, 0, SETVAL, semun) == -1) {
		perror("\n\nConsumer: Error in initializing semaphore\n");
		exit(-1);
	}
}