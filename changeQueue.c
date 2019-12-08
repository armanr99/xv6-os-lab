#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "date.h"

#define LOTTERY 1
#define HRRN    2
#define SRPF    3

int main(int argc, char *argv[]) 
{
    if(argc != 3)
    {
        printf(2, "Too few or too much arguments.\n");
        exit();
    }
    int pid = atoi(argv[1]);
    if(strcmp(argv[2], "lottery") == 0 || strcmp(argv[2], "l") == 0)
        set_schedule_queue(LOTTERY, pid);
    else if (strcmp(argv[2], "hrrn") == 0 || strcmp(argv[2], "h") == 0)
        set_schedule_queue(HRRN, pid);
    else if (strcmp(argv[2], "srpf") == 0|| strcmp(argv[2], "s") == 0)
        set_schedule_queue(SRPF, pid);
    exit();
}