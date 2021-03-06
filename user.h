struct stat;
struct rtcdate;
struct barrierlock;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);

//2
int set_path(char*);
int get_parent_id();
int get_children(int, char*, int);
int get_posteriors(int, char*, int);
int set_sleep(int);
int fill_date(struct rtcdate*);

//3
int set_lottery_ticket(int, int);
int set_srpf_remaining_priority(int, int, int);
int set_schedule_queue(int, int);
void ps(void);

//4
int initbarrierlock(int);
int acquirebarrierlock(int);
void test_reentrant_lock(void);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);   
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
