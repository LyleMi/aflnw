#ifndef _UTILS_H_
#define _UTILS_H_

struct config {
    char *addr;
    char *proc;
    char *path;
    int port;
};

void parseArgs(int argc, char *argv[], struct config *conf);
void freeArgs(struct config *conf);
int processPid(const char *proc);
void stopProcess(const char *proc);
void tcpSend(char *addr, int port);
void run(char *path, int argc, char *argv[], int optbind);
void crash();

#endif
