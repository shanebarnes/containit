#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void signal_handler(int sig) {
    int status = 0, pid;

    if (sig == SIGCHLD) {
        pid = waitpid(-1, &status, WNOHANG);
        if (WIFEXITED(status)) {
            fprintf(stderr,
                    "%d: Process terminated with exit status %d\n",
                    pid,
                    WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr,
                    "%d: Process terminated by signal %d\n",
                    pid,
                    WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            fprintf(stderr,
                    "%d: Process stopped by signal %d\n",
                    pid,
                    WSTOPSIG(status));
        }
    }
}

void split_arg(char *arg, int argc, char **argv) {
    char *tok = " \t\n";
    int i = 0;

    if (argc > i) {
        argv[i] = strtok(arg, tok);
        while (argv[i] != NULL && i < argc) {
            argv[++i] = strtok(NULL, tok);
        }
    }
}

void exec_arg(char *arg) {
    char *argv[64], pid[16];

    split_arg(arg, 64, argv);
    sprintf(pid, "%d", getpid());
    execvp(*argv, argv);
    perror(pid);
    exit(errno);
}

int main(int argc, char **argv) {
    int ret = argc > 1 ? EXIT_SUCCESS : EXIT_FAILURE;
    int i, pid, pidc = 0, pidv[ARG_MAX];

    signal(SIGCHLD, signal_handler);

    for (i = 1; i < argc; i++) {
        pid = fork();

        if (pid == -1) {
            perror("Error");
        } else if (pid == 0) {
            exec_arg(argv[i]);
        } else {
            fprintf(stdout, "%d: Created child process (%s)\n", pid, argv[i]);
            pidv[pidc] = pid;
            pidc++;
        }
    }

    if (pidc > 0) {
        pause();
    } else {
        fprintf(stderr, "\nNo child processes were created\n");
        fprintf(stderr, "\nusage: %s [program 1] [program 2] ...\n\n", argv[0]);
    }

    while (pidc > 0) {
        kill(pidv[pidc-1], SIGKILL);
        fprintf(stdout, "%d: Destroyed child process\n", pidv[pidc-1]);
        pidc--;
    }

    return ret;
}
