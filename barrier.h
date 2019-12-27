// Barrier locks for processes
struct barrierlock {
    uint locked; // Is the lock held?
    struct spinlock lk; 

    int max_processes_count;
    int cur_processes_count;
};