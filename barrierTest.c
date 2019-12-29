#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "spinlock.h"
#include "barrier.h"

const int TEST_COUNT = 5;

int main()
{
    int bid = initbarrierlock(TEST_COUNT + 1);
    int child_pid = 0;
    for(int i = 0; i < TEST_COUNT; i++)
    {
        child_pid = fork();

        if(child_pid != 0)
            set_sleep(1);
        else
            break;
        
    }
    int pid = getpid();
    printf(1,"locking before barrier: %d\n", pid);
    acquirebarrierlock(bid);
    printf(1,"after barrier locking: %d\n", pid);

    if (child_pid)
        for (int i = 0; i <TEST_COUNT; i++)
            wait();
    exit();
}