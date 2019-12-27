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
    
    for(int i = 0; i < TEST_COUNT; i++)
    {
        int child_pid = fork();

        if(child_pid != 0)
        {
            printf(1, "%d", child_pid);
            set_sleep(1);
            printf(1, "RAtteR BitCH\n");
        }
        else
            break;
        
    }
    int pid = getpid();
    printf(1,"%d", pid);
    acquirebarrierlock(bid);
    printf(1,"%d", pid);

    exit();
}