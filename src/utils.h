#ifndef _UTILS_H_
#define _UTILS_H_

struct config {
    char *addr;
    char *proc;
    char *path;
    char *stdout;
    char *stderr;
    int port;
    int debug;
    // time for usleep
    int utime;
};

struct config *conf;

void parseArgs(int argc, char *argv[]);
void freeArgs();
int processPid(const char *proc);
void stopProcess(const char *proc);
void tcpSend(char *addr, int port);
void run(char *path, int argc, char *argv[], int optbind);
void crash();

#endif
