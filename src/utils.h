#ifndef _UTILS_H_
#define _UTILS_H_

#define PIPE_READ 0
#define PIPE_WRITE 1

struct config {
    char *addr;
    char *proc;     /* process name of the target program */
    char *path;     /* path to execute the service(target program) */
    char *chroot;
    char *stdin;
    char *stdout;
    char *stderr;
    char *mode;     /* "net" mode or "cgi" mode */
    char *shm;      /* address of shared memory */
    int shmid;      /* shared memory identifier */
    int port;
    int debug;
    int checksrv;   /* whether to check if the service is running or not */
    int asforksrv;
    // time for usleep
    int utime;
};

struct config *conf;

void parse_args(int argc, char *argv[]);
void free_args();

int process_pid(const char *proc);
void stop_process(const char *proc);

void tcp_send(char *addr, int port);

void run_net(char *path, int argc, char *argv[], int optbind);
void run_cgi(char *path, int argc, char *argv[], char *const envp[], int optbind);

void crash();
char **stdin_to_env(char *const envp[]);
void set_fd();

void init_shm();
void set_shm();
int count_shm();

void init_env();
void finish_env();

#endif
