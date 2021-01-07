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
#include<time.h>

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
void Init(int buffsize);
int* InitBuff(int memsize);
int CreateSem(int semkey);
void DestroySem(int sem);
void FreeResources();
void SignalHandler(int signum);
int ProduceItem();
void InsertItem(int item);
void InitSem(int sem, int value);

int main(){
	int memsize;

	/*Read the buffer size from user*/
	printf("\n\nProducer: please enter the buffer size: ");
	scanf("%d", &buffsize);
	if(buffsize <= 0){
		perror("\n\nProducer: invalid buffer size\n");
		exit(-1);
	}
	memsize = buffsize*sizeof(int);

	/*Pass the buffer size to the consumer*/
	Init(buffsize);

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

	for(int i=0; i<20; i++){
		int item = ProduceItem();     /* generate something to put in buffer */
		Down(empty);                  /* decrement empty count */
		Down(mutex);                  /* enter critical region */
		InsertItem(item);             /* put new item in buffer */
		Up(mutex);                    /* leave critical region */
		Up(full);                     /* increment count of full slots */
	}

	FreeResources();

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
		perror("\n\nProducer: Error in down()\n");
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
		perror("\n\nProducer: Error in up()\n");
		exit(-1);
	}
}

/**
 * @brief Create or get the shared memory for passing buffer size
 * 
 * @return int 
 */
void Init(int buffsize)
{
	memsizeid = shmget(SHM_SIZE_KEY, sizeof(int), IPC_CREAT|0644);
	if (memsizeid == -1){
		perror("\n\nProducer: Error in create the shared memory for buffer size\n");
		exit(-1);
	}
	
	memsizeAddr = (int *)shmat(memsizeid, (void *)0, 0);
	if (memsizeAddr == -1){
		perror("\n\nProducer: Error in attach the for buffer size shared memory\n");
		exit(-1);
	}
	
	*memsizeAddr = buffsize;
}

/**
 * @brief Create the shared memory buffer
 * 
 * @param memsize 
 * @return int* 
 */
int* InitBuff(int memsize)
{
	buffid = shmget(SHM_BUFF_KEY, memsize, IPC_CREAT|0644);
	if (buffid == -1){
		perror("\n\nProducer: Error in create the shared memory for buffer size\n");
		exit(-1);
	}
	else printf("\n\nProducer: Buffer id: %d\n", buffid);
	
	int* buff;
	buff = (int *)shmat(buffid, (void *)0, 0);
	if (buff == -1){
		perror("\n\nProducer: Error in attach the for buffer\n");
		exit(-1);
	}
	else printf("\n\nProducer: Buffer attached successfuly\n");

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
		perror("\n\nProducer: Error in create sem\n");
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
		perror("\n\nProducer: Error in destroy sem\n");
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

	// DestroySem(full);
	// DestroySem(empty);
	// DestroySem(mutex);

	// shmctl(memsizeid, IPC_RMID, NULL);
	// shmctl(buffid, IPC_RMID, NULL);
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
 * @brief Removes one item from shared buffer
 * 
 * @return int 
 */
int ProduceItem()
{
	printf("\n\nProducer: generating something to put in buffer...\n");
	srand(time(0));
	return rand()%11;
}

/**
 * @brief Consumes the removed item from the buffer
 * 
 * @param item 
 */
void InsertItem(int item)
{
	buff[buffIter] = item;
	printf("\n\nProducer: inserted %d in the buffer at location: %d\n", item, buffIter);
	buffIter = (buffIter+1)%buffsize;
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
		perror("\n\nProducer: Error in initializing semaphore\n");
		exit(-1);
	}
}