#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "date.h"

int main(int argc, char *argv[]) 
{
    int num_of_child_processes = 10;
    if (argc == 2)
        num_of_child_processes = atoi(argv[1]);

    for (int i = 0; i < num_of_child_processes; i++)
        if(fork() == 0)
        {
            int dev_null = 0;
            for (;;)
                dev_null++;
            exit();
        }
    for (int i = 0; i < num_of_child_processes; i++)
        wait();
    exit();
}