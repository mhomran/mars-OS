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
#include <time.h>

#define SHM_SIZE_KEY 3000
#define SHM_BUFF_KEY 4000
#define SHM_READ_INDEX_KEY 5000
#define SHM_WRITE_INDEX_KEY 6000
#define FULL_SEM_KEY 300
#define EMPTY_SEM_KEY 400
#define MUTEX_SEM_KEY 500
#define BUFFSIZE 5

int *memSizeAddr, *buff, *readIndex, *writeIndex;
int full, empty, mutex, buffId, readIndexId, writeIndexId, value;

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
int *InitBuff(int memsize);
int CreateSem(int semkey, int value);
void DestroySem(int sem);
void FreeResources();
void SignalHandler(int signum);
int ProduceItem();
void InsertItem(int item);
void InitSem(int sem, int value);

int main()
{
	int memSize;

	// Memory size in bytes
	memSize = BUFFSIZE * sizeof(int);

	// Initialize read and write indices
	InitIndices();

	/* Initialize the buffer */
	buff = InitBuff(memSize);

	/* Create full, empty, and mutex semaphores or get them if they already exist */
	full = CreateSem(FULL_SEM_KEY, 0);
	empty = CreateSem(EMPTY_SEM_KEY, BUFFSIZE);
	mutex = CreateSem(MUTEX_SEM_KEY, 1);

	/* Bind the SIGINT signal handler */
	signal(SIGINT, SignalHandler);

	while (1) {
		int item = ProduceItem(); /* generate something to put in buffer */
		Down(empty);              /* decrement empty count */
		Down(mutex);              /* enter critical region */
		InsertItem(item);         /* put new item in buffer */
		Up(mutex);                /* leave critical region */
		Up(full);                 /* increment count of full slots */
		sleep(10);		  /* for debugging */
	}

	FreeResources();

	return 0;
}

/**
 * @brief Decrease semaphore by 1
 * 
 * @param sem Semaphore id
 */
void Down(int sem)
{
	struct sembuf p_op;

	p_op.sem_num = 0;
	p_op.sem_op = -1;
	p_op.sem_flg = !IPC_NOWAIT;

	if (semop(sem, &p_op, 1) == -1)	{
		perror("\n\nProducer: Error in down()\n");
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
		perror("\n\nProducer: Error in up()\n");
		exit(-1);
	}
}

/**
 * @brief Create or get the shared memory for passing reading and writing indices
 */
void InitIndices()
{
	// Check if they already exist
	int readInitalized = shmget(SHM_READ_INDEX_KEY, sizeof(int), 0644);
	int writeInitalized = shmget(SHM_WRITE_INDEX_KEY, sizeof(int), 0644);
	
	// Get or create shared memories for indices
	readIndexId = shmget(SHM_READ_INDEX_KEY, sizeof(int), IPC_CREAT|0644);
	writeIndexId = shmget(SHM_WRITE_INDEX_KEY, sizeof(int), IPC_CREAT|0644);

	if (readIndexId == -1 || writeIndexId == -1) {
		perror("\n\nProducer: Error in create the shared memory for indices\n");
		exit(-1);
	}

	// Attach this producer to the shared memories
	readIndex = (int *)shmat(readIndexId, (void *)0, 0);
	writeIndex = (int *)shmat(writeIndexId, (void*) 0, 0);

	if (readIndex == (int *)-1 || writeIndex == (int *)-1) {
		perror("\n\nProducer: Error in attach the for indices shared memory\n");
		exit(-1);
	}

	// If this producer created them (first producer) initialize them to 0
	if (readInitalized == -1) *readIndex = 0;
	if (writeInitalized == -1) *writeIndex = 0;
}

/**
 * @brief Create the shared memory buffer
 * 
 * @param memSize 
 * @return int* 
 */
int *InitBuff(int memSize)
{
	// Create or get shared memory for buffer
	buffId = shmget(SHM_BUFF_KEY, memSize, IPC_CREAT | 0644);
	if (buffId == -1) {
		perror("\n\nProducer: Error in create the shared memory for buffer size\n");
		exit(-1);
	}
	else printf("\n\nProducer: Buffer id: %d\n", buffId);

	// Attach to the buffer
	int *buff;
	buff = (int *)shmat(buffId, (void *)0, 0);
	
	if (buff == (int *)-1) {
		perror("\n\nProducer: Error in attach the for buffer\n");
		exit(-1);
	}
	else printf("\n\nProducer: Buffer attached successfuly\n");

	return buff;
}

/**
 * @brief Create a Sem object and initalize it
 * 
 * @param semkey	key of the semphore 
 * @param value		initial value for the semaphore
 * @return int 
 */
int CreateSem(int semkey, int value)
{
	// Check if it already exists
	int initialized = semget(semkey, 1, 0666);

	// Get or create the semaphore
	int sem = semget(semkey, 1, 0666 | IPC_CREAT);
	if (sem == -1) {
		perror("\n\nProducer: Error in create sem\n");
		exit(-1);
	}

	// If this producer created the semaphore initialize it
	if (initialized == -1) InitSem(sem, value);

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
		perror("\n\nProducer: Error in destroy sem\n");
		exit(-1);
	}
}

/**
 * @brief Before terminating free the used resources
 * for the producers this means detaching from shared memories
 * 
 */
void FreeResources()
{
	// Detach from all shared memories
	shmdt((void *)buff);
	shmdt((void *)memSizeAddr);
	shmdt((void *) readIndex);
	shmdt((void *) writeIndex);
}

/**
 * @brief Handles SIGINT signal
 * 
 * @param signum 
 */
void SignalHandler(int signum)
{
	printf("\n\nProducer: terminating...\n");
	FreeResources();
	exit(0);
}

/**
 * @brief Create one item to be inserted in the buffer
 * 
 * @return int 
 */
int ProduceItem()
{
	printf("\n\nProducer: generating something to put in buffer...\n");
	int returnValue = value;
	value++;
	return returnValue;
}

/**
 * @brief Insert item in the shared buffer
 * 
 * @param item 
 */
void InsertItem(int item)
{
	// Insert item in the buffer
	buff[*writeIndex] = item;
	
	printf("\n\nProducer: inserted %d in the buffer at location: %d\n", item, *writeIndex);
	
	// Increase write index and wrap if it is more than buffer size
	*writeIndex = (*writeIndex + 1) % BUFFSIZE;
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
	semun.val = value; /* Initial value of the semaphore, Binary semaphore */
	if (semctl(sem, 0, SETVAL, semun) == -1) {
		perror("\n\nProducer: Error in initializing semaphore\n");
		exit(-1);
	}
}