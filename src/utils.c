#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/wait.h>

#include "utils.h"

void parseArgs(int argc, char *argv[], struct config *conf)
{
    if (argc == 1) {
        printf("Usage: %s -p <port> -a <addr>\n", argv[0]);
        exit(1);
    }
    char ch;
    while ((ch = getopt(argc, argv, "p:a:")) != -1)
    {
        switch (ch)
        {
        case 'p':
            conf->port = atoi(optarg);
            break;
        case 'a':
            conf->addr = strdup(optarg);
            break;

        default:
            break;
        }
    }
    conf->path = strdup(argv[optind]);
    conf->proc = strrchr(conf->path, '/');
    conf->proc++;
}

void freeArgs(struct config *conf)
{
    free(conf->addr);
    free(conf->path);
}

int processPid(const char *proc)
{
    FILE* fp = NULL;
    int count;
    int BUFSZ = 100;
    char buf[BUFSZ];
    char command[150];

    sprintf(command, "pidof %s", proc);

    if ((fp = popen(command, "r")) == NULL)
    {
        printf("popen err\n");
        return -1;
    }
    if ((fgets(buf, BUFSZ, fp)) != NULL)
    {
        count = atoi(buf);
    }
    pclose(fp);
    fp = NULL;
    return count;
}

void stopProcess(const char *proc)
{
    char command[150];
    sprintf(command, "pkill %s", proc);
    system(command);
}

void tcpSend(char *addr, int port)
{
    int server_fd;
    char *buf;
    struct sockaddr_in server_listen_addr;
    bzero(&server_listen_addr, sizeof(server_listen_addr));
    server_listen_addr.sin_family = AF_INET;
    server_listen_addr.sin_port = htons(port);
    inet_pton(AF_INET, addr, (void*)&server_listen_addr.sin_addr);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int ret = connect(
                  server_fd, (const struct sockaddr *)&server_listen_addr,
                  sizeof(struct sockaddr)
              );
    if (ret < 0)
    {
        // for debug
        // printf("send to %s %d\n", addr, port);
        crash();
    }
    buf = (char*)malloc(4096);
    while (fread(buf, 1, sizeof(buf), stdin)) {
        int num = send(server_fd, (void*)buf, strlen(buf), 0);
        printf("send bytes[%d][%s]\n", num, buf);
    }
    close(server_fd);
    free(buf);
    buf = NULL;
}

void run(char *path, int argc, char *argv[], int optbind)
{
    int child = fork();
    int ret;

    if (child == 0) {
        // printf("argc %d optind %d\n", argc, optind);
        int count;
        char* chargs[argc - optind + 1];
        count = 0;
        while (optind + count < argc) {
            chargs[count] = argv[optind + count];
            // printf("chargs %d %s\n", count, chargs[count]);
            count++;
        }
        chargs[count] = NULL;
        execv(path, chargs);
        exit(0);
    } else {
        // wait for the server to start
        usleep(200);
        ret = waitpid(child, NULL, 0);
        return;
    }
}

void crash()
{
    void (*x)();
    x();
}
