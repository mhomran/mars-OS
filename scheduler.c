/**
 * @file scheduler.c
 * @author Ahmed Ashraf (ahmed.ashraf.cmp@gmail.com)
 * @brief Keeps track of the processes and their states and it decides which process will run and for how long.
 * @version 0.1
 * @date 2020-12-30
 */

#include "headers.h"


int main(int argc, char * argv[])
{
    initClk();
    
    // 1.  Start a new process. (Fork it and give it its parameters.)
    if(fork() == 0){
        execl("process.out", "process.out", NULL);
    }
    
    
    //upon termination release the clock resources.
    
    destroyClk(true);
}
