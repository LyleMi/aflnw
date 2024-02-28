#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

struct config *conf;

void run_net_mode(int argc, char *argv[])
{
    int pid;

    if (conf->checksrv) {
        pid = process_pid(conf->proc);
        if (pid <= 0) {
            run_net(conf->path, argc, argv, optind);
            pid = process_pid(conf->proc);
        }
    }

    tcp_send(conf->addr, conf->port);

    if (conf->checksrv) {
        if (process_pid(conf->proc) != pid) {
            crash();
        }
    }
}

int main(int argc, char *argv[], char *const envp[])
{
    parse_args(argc, argv);
    init_env();

    if (strcmp(conf->mode, "net") == 0) {
        run_net_mode(argc, argv);
    } else if (strcmp(conf->mode, "cgi") == 0) {
        run_cgi(conf->path, argc, argv, envp, optind);
    }

    finish_env();
    return 0;
}
