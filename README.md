## About <a name = "about"></a>

This project consists of two parts. The first part is a simulation for the CPU scheduler and the memory manager. The second part is the synchronizer which is a solution for the popular producer-consumer/shared buffer problem.

The scheduler determines an order for the execution of its scheduled processes; it decides which process will run according to a certain data structure that keeps track of the processes in the system and their status with memory allocation capabilities using the buddy memory allocation system.

The synchronizer solves the producer-consumer problem using semaphores and shared memory and supports that multiple producers and multiple consumers can work at the same time.

<p align="center">
  <a href="" rel="noopener">
 <img src="https://github.com/mhomran/mars-OS/blob/master/demo/system.png" alt="System Visualization"></a>
</p>

We used the concept of the “tick” in the real OSes. So, we make the scheduler executes every tick which equals to one second. The scheduler supports the follwing algorithms:

- Round Robin (RR): Every tick the quantum of the running process is decremented. Whenever it finishes its quantum, the scheduler blocks it, puts it in the ready queue (if it still has some work to do and not finished yet), then chooses the next one from the ready queue (if exists) and gives it a full quantum. If the running process finishes before it finishes its quantum, then the scheduler will pick the next process from the ready queue (if exists).
- Non preemptive Highest Priority First (NHPF): The scheduler chooses the process with the highest priority from the priority queue which has a no complexity of O(1). Then this process runs to completion. At every tick if the scheduler sees that there's no running process, then it chooses the one with the highest priority from the priority queue.
- Shortest Remaining Time Next (SRTN): The scheduler at any tick chooses the process with the shortest remaining time from the priority queue. This operation has complexity of O(1). At any tick, if a new process arrived with a run time shorter than the running time, it will preempt the running process.

We represented the buddy system by a binary tree and its leaves represent the allocated parts of the memory.

<p align="center">
  <a href="" rel="noopener">
 <img src="https://github.com/mhomran/mars-OS/blob/master/demo/buddy.png" alt="Buddy Visualization"></a>
</p>

## Install <a name = "install"></a>

- You can use Makefile to build and run your project

- To compile your project, use the command: `make`

- To generate a random test case in the scheduler run `make generate_test`. You can change it as you want in the `processes.txt` file.

- To run your project:

  - For the scheduler use the command: `make run`
  - For the synchronizer use the command: `make <name>` where `name` is the producer `run_producer` or the consumer `run_consumer`

- If you added a file to your project add it to the build section in the Makefile

- Always start the line with a tab in Makefile, it is its syntax
