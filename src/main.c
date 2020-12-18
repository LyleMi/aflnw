#include <stdlib.h>
#include <unistd.h>

#include "utils.h"

int main(int argc, char *argv[], char *const envp[])
{
    int pid;
    struct config *conf = NULL;
    conf = (struct config *)malloc(sizeof(struct config));
    parseArgs(argc, argv, conf);
    pid = processPid(conf->proc);
    if (pid <= 0) {
        run(conf->path, argc, argv, optind);
        pid = processPid(conf->proc);
    }
    tcpSend(conf->addr, conf->port);
    if (processPid(conf->proc) != pid) {
        crash();
    }
    freeArgs(conf);
    return 0;
}
