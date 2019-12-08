#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "date.h"

int main(int argc, char *argv[]) 
{
    if(argc != 3)
    {
        printf(2, "Too few or too much arguments.\n");
        exit();
    }
    int pid = atoi(argv[1]);
    double remaining_priority = 0;
    int dot_idx = -1;
    char before_dot[100], after_dot[100];
    for (int i = 0; i < strlen(argv[2]); i++)
        if(argv[2][i] == '.')
        {
            before_dot[i] = '\0';
            dot_idx = i;
        } else if (dot_idx == -1)
            before_dot[i] = argv[2][i];
        else
            after_dot[i - dot_idx - 1] = argv[2][i];
    if (dot_idx == -1)
        remaining_priority = atoi(argv[2]);
    else
    {
        after_dot[strlen(argv[2]) - dot_idx - 1] = '\0';
        remaining_priority = atoi(before_dot);
        double decimal = atoi(after_dot);
        for (int i = 0; i < strlen(after_dot); i++)
            remaining_priority *= 10;
        remaining_priority += decimal;
    }

    set_srpf_remaining_priority(remaining_priority, strlen(after_dot), pid);
    exit();
}