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
#define BUFFSIZE 3

int *memsizeAddr, *buff, *readIndex, *writeIndex;
int full, empty, mutex, buffid, memsizeid, buffIter, value; //, buffsize;

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
void Init();
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
  int memsize;

  memsize = BUFFSIZE * sizeof(int);

  /*Initialize the buffer*/
  buff = InitBuff(memsize);

  /*Create full, empty, and mutex semaphores or get them if they already exist*/
  full = CreateSem(FULL_SEM_KEY, 0);
  empty = CreateSem(EMPTY_SEM_KEY, BUFFSIZE);
  mutex = CreateSem(MUTEX_SEM_KEY, 1);

  /*Bind the SIGINT signal handler*/
  signal(SIGINT, SignalHandler);

  while (1)
  {
    int item = ProduceItem(); /* generate something to put in buffer */
    Down(empty);              /* decrement empty count */
    Down(mutex);              /* enter critical region */
    InsertItem(item);         /* put new item in buffer */
    Up(mutex);                /* leave critical region */
    Up(full);                 /* increment count of full slots */
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

  if (semop(sem, &p_op, 1) == -1)
  {
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

  if (semop(sem, &v_op, 1) == -1)
  {
    perror("\n\nProducer: Error in up()\n");
    exit(-1);
  }
}

/**
 * @brief Create or get the shared memory for passing reading and writing indices
 */
void Init()
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
int *InitBuff(int memsize)
{
  buffid = shmget(SHM_BUFF_KEY, memsize, IPC_CREAT | 0644);
  if (buffid == -1)
  {
    perror("\n\nProducer: Error in create the shared memory for buffer size\n");
    exit(-1);
  }
  else
    printf("\n\nProducer: Buffer id: %d\n", buffid);

  int *buff;
  buff = (int *)shmat(buffid, (void *)0, 0);
  if (buff == -1)
  {
    perror("\n\nProducer: Error in attach the for buffer\n");
    exit(-1);
  }
  else
    printf("\n\nProducer: Buffer attached successfuly\n");

  return buff;
}

/**
 * @brief Create a Sem object
 * 
 * @param semkey 
 * @return int 
 */
int CreateSem(int semkey, int value)
{
  int initialized = semget(semkey, 1, 0666);
  
  int sem = semget(semkey, 1, 0666 | IPC_CREAT);
  if (sem == -1)
  {
    perror("\n\nProducer: Error in create sem\n");
    exit(-1);
  }

  /* If this producer created the semaphore initialize it*/
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
  if (semctl(sem, 0, IPC_RMID) == -1)
  {
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
  shmdt((void *)buff);
  shmdt((void *)memsizeAddr);

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
  int returnValue = value;
  value++;
  return returnValue;
  // srand(time(0));
  // return rand() % 11;
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
  buffIter = (buffIter + 1) % BUFFSIZE;
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
  if (semctl(sem, 0, SETVAL, semun) == -1)
  {
    perror("\n\nProducer: Error in initializing semaphore\n");
    exit(-1);
  }
}