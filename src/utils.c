#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>

#include "afl.h"
#include "config.h"
#include "utils.h"

#ifdef DEBUG
static void debug_printf(const char *format, ...)
{
    va_list arg_ptr;
    va_start(arg_ptr, format);
    fflush(stdout);
    vfprintf(stderr, format, arg_ptr);
    va_end(arg_ptr);
}
#else
static inline void debug_printf(const char *format, ...)
{

}
#endif


void parse_args(int argc, char *argv[])
{
    conf = (struct config *)malloc(sizeof(struct config));
    bzero(conf, sizeof(struct config));
    if (argc == 1) {
        printf("Usage: %s -p <port> -a <addr>\n", argv[0]);
        exit(1);
    }
    // conf->checksrv = 1;
    // shmid 有可能被设置为 0
    conf->shmid = -1;
    char ch;
    while ((ch = getopt(argc, argv, "p:a:n:i:o:e:m:t:s:r:dcf")) != -1)
    {
        switch (ch)
        {
        case 'p':
        // 端口
            conf->port = atoi(optarg);
            break;
        case 'a':
        // 地址
            conf->addr = strdup(optarg);
            break;
        case 'n':
        // 程序
            conf->proc = strdup(optarg);
            break;
        case 'i':
        // 标准输入
            conf->stdin = strdup(optarg);
            break;
        case 'o':
        // 标准输出
            conf->stdout = strdup(optarg);
            break;
        case 'e':
        // 标准报错
            conf->stderr = strdup(optarg);
            break;
        case 'm':
        // 测试模式
            conf->mode = strdup(optarg);
            break;
        case 'r':
            conf->chroot = strdup(optarg);
            break;
        case 't':
        // 时间
            conf->utime = atoi(optarg);
            break;
        case 's':
        // shmid
            conf->shmid = atoi(optarg);
            break;
        case 'd':
            conf->debug = 1;
            break;
        case 'c':
            conf->checksrv = 1;
            break;
        case 'f':
            conf->asforksrv = 1;
            break;
        default:
            // break;
            fprintf(
                stderr, "%s [-a addr] [-p port] [-n proc] "
                "[-i stdin] [-o stdout] [-m mode] [-t utime]"
                "[-s shmid] [-d debug] [-c checksrv]\n"
                "version %s\n",
                argv[0], VERSION
            );
            exit(EXIT_FAILURE);
        }
    }

    if (!conf->mode) {
        conf->mode = strdup("net");
    }

    if (optind < argc) {
        conf->path = strdup(argv[optind]);
    }

    if (!conf->proc && conf->path) {
        conf->proc = strrchr(conf->path, '/');
        conf->proc++;
    }

    debug_printf("argc %d optindex %d\n", argc, optind);
    debug_printf("elf path %s, proc %s\n", conf->path, conf->proc);
}

void free_args()
{
    free(conf->addr);
    free(conf->path);
    free(conf->chroot);
    free(conf->stdin);
    free(conf->stdout);
    free(conf->stderr);
    free(conf->mode);
    free(conf);
}


void init_env()
{
    int ret;
    if (conf->shmid != -1) {
        init_shm();
    }
    if (conf->chroot) {
        ret = chroot(conf->chroot);
        if (ret == -1) {
            perror("chroot");
            exit(1);
        }
    }
}

void finish_env()
{
    if (conf->shmid != -1) {
        set_shm();
    }
    free_args();
}

int process_pid(const char *proc)
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

void stop_process(const char *proc)
{
    char command[150];
    sprintf(command, "pkill %s", proc);
    system(command);
}

void tcp_send(char *addr, int port)
{
    int sock_fd;
    int ret;
    char *buf;
    struct sockaddr_in server_addr;

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    /*
    if (conf->utime > 0) {
        timeout.tv_sec = conf->utime / 1000;
    } else {
        timeout.tv_sec = 1;
    }
    */
    debug_printf("send tcp network to %s:%d", addr, port);

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, addr, (void*)&server_addr.sin_addr);
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    ret = setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (ret < 0) {
        perror("setsockopt RCVTIMEO");
        close(sock_fd);
        return;
    }
    ret = setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    if (ret < 0) {
        perror("setsockopt SNDTIMEO");
        close(sock_fd);
        return;
    }
    ret = connect(
        sock_fd,
        (const struct sockaddr *)&server_addr,
        sizeof(struct sockaddr)
    );
    if (ret < 0)
    {
        // crash();
        close(sock_fd);
        return;
    }
    buf = (char*)malloc(BUF_SZIE);
    while (fread(buf, 1, BUF_SZIE, stdin)) {
        ret = send(sock_fd, (void*)buf, strlen(buf), 0);
        if (ret < 0) {
            crash();
            close(sock_fd);
            return;
        }
        if (conf->debug) {
            debug_printf("send bytes[%d][%s]\n", ret, buf);
            recv(sock_fd, buf, 4096, 0);
            debug_printf("recv bytes %s\n", buf);
        }
    }
    close(sock_fd);
    free(buf);
    buf = NULL;
    // usleep(conf->utime);
}

char **stdin_to_env(char *const envp[])
{
    char *buf;
    size_t size = 0;
    ssize_t len = 0;
    int count = 0;
    char **envs = malloc(sizeof(char *) * (count + 1));
    while (envp[count]) {
        envs[count] = strdup(envp[count]);
        count += 1;
        envs = realloc(envs, sizeof(char *) * (count + 1));
    }
    while (1) {
        len = getline(&buf, &size, stdin);
        if (len <= 0) {
            break;
        }
        buf[len - 1] = '\x00';
        if (strncmp(buf, "end", 3) == 0) {
            break;
        }
        envs[count] = buf;
        if (conf->debug) {
            printf(
                "[%d] stdin size(%ld): %s\n",
                count, len, buf
            );
        }
        count += 1;
        envs = realloc(envs, sizeof(char *) * (count + 1));
        buf = NULL;
    }
    envs[count] = NULL;
    return envs;
}


void run_cgi(char *path, int argc, char *argv[], char *const envp[], int optbind)
{
    int ret;
    int status;
    int count = 0;
    int pipefd[2];
    char **envs = stdin_to_env(envp);

    char *chargs[argc - optind + 1];
    while (optind + count < argc) {
        chargs[count] = argv[optind + count];
        count++;
    }
    chargs[count] = NULL;

    ret = pipe(pipefd);
    if (ret == -1) {
        perror("pipe");
        exit(1);
    }
    int child = fork();
    if (child > 0) {

        char *buf;
        buf = (char*)malloc(BUF_SZIE);
        while (fread(buf, 1, BUF_SZIE, stdin)) {
            write(pipefd[PIPE_WRITE], buf, strlen(buf));
        }
        close(pipefd[PIPE_WRITE]);
        free(buf);
        buf = NULL;
        wait(NULL);
        printf("non zero count %d\n", count_shm());

    } else if (child == 0) {

        if (dup2(pipefd[PIPE_READ], fileno(stdin)) == -1) {
            perror("dup stdin");
            exit(1);
        }
        close(pipefd[PIPE_WRITE]);
        ret = execve(path, chargs, envs);
        if (ret) {
            perror("execve");
            exit(1);
        }
        exit(0);
    } else {
        perror("fork");
        exit(1);
    }
}

void run_net(char *path, int argc, char *argv[], int optbind)
{
    int child = fork();
    int status;
    int ret;

    if (child < 0) {
        perror("fork");
        exit(1);
    }

    if (child == 0) {
        // printf("argc %d optind %d\n", argc, optind);
        int count;
        pid_t sid = 0;

        char* chargs[argc - optind + 1];
        count = 0;
        while (optind + count < argc) {
            chargs[count] = argv[optind + count];
            // printf("chargs %d %s\n", count, chargs[count]);
            count++;
        }
        chargs[count] = NULL;

        sid = setsid();

        if (sid < 0) {
            perror("setsid");
        }

        signal(SIGHUP, SIG_IGN);

        child = fork();
        if (child < 0) {
            exit(1);
        }
        if (child > 0) {
            // ret = waitpid(child, NULL, 0);
            exit(0);
        }

        if (chdir("/") < 0) {
            perror("chdir");
            exit(1);
        }

        umask(0);
        set_fd();
        execv(path, chargs);
        exit(0);
    } else {
        ret = waitpid(child, &status, 0);
        if (!ret) {
            fprintf(stderr, "wait pid error with code (%d)\n", status);
        }
        // wait for the server to start
        usleep(conf->utime);
        return;
    }
}

void set_fd()
{
    int fd;

    if (conf->stdin) {
        fd = open(conf->stdin, O_RDONLY);
        if (dup2(fd, fileno(stdin))) {
            perror("dup stdin");
        }
        close(fd);
        debug_printf("dup stdin to %s\n", conf->stdin);
    } else {
        fd = open("/dev/null", O_RDONLY);
        if (fd != fileno(stdin)) {
            dup2(fd, fileno(stdin));
            close(fd);
        }
    }

    if (conf->stdout) {
        fd = open(conf->stdout, O_WRONLY | O_CREAT | O_APPEND, 0600);
    } else {
        fd = open("/dev/null", O_WRONLY);
    }
    if (fd != fileno(stdout)) {
        dup2(fd, fileno(stdout));
        close(fd);
    }

    if (conf->stderr) {
        fd = open(conf->stderr, O_WRONLY);
    } else {
        fd = open("/dev/null", O_WRONLY);
    }
    if (fd != fileno(stderr)) {
        dup2(fd, fileno(stderr));
        close(fd);
    }
}

void crash()
{
    if (conf->shm) {
        set_shm();
    }
    void (*x)();
    x();
}

void init_shm()
{
    void *shm = NULL;
    shm = shmat(conf->shmid, 0, 0);
    if (shm == (void*) - 1) {
        return;
    }
    memset(shm, 0, MAP_SIZE);
    conf->shm = shm;
}

void set_shm()
{
    if (!conf->shm) {
        return;
    }
    int shmid;
    void *shm = NULL;
    char *id_str = getenv(SHM_ENV_VAR);
    if (!id_str) {
        return;
    }
    shmid = atoi(id_str);
    shm = shmat(shmid, NULL, 0);
    if (shm == (void*) - 1) {
        return;
    }
    memcpy(shm, conf->shm, MAP_SIZE);
}

int count_shm()
{
    if (!conf->shm) {
        return 0;
    }
    return count_non_zero_bytes((u8 *)conf->shm);
}
