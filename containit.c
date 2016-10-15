#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void signal_handler(int sig) {
    switch (sig) {
        case SIGCHLD:
            fprintf(stdout,
                    "Child process %d has terminated\n",
                    waitpid(-1, NULL, WNOHANG));
            break;
        default:
            break;
    }
}

void split_arg(char *arg, int argc, char **argv) {
    char *pch = NULL, *tok = " \t\n";
    int i = 0;

    if (argc > i) {
        argv[i] = strtok(arg, tok);
        while (argv[i] != NULL && i < argc) {
            argv[++i] = strtok(NULL, tok);
        }
    }
}

void exec_cmd(char *cmd) {
    char *argv[64];

    split_arg(cmd, 64, argv);
    execvp(*argv, argv);
    exit(EXIT_FAILURE);
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
            exec_cmd(argv[i]);
        } else {
            fprintf(stdout, "Creating child process %d (%s)\n", pid, argv[i]);
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
        fprintf(stdout, "Destroying child process %d\n", pidv[pidc-1]);
        kill(pidv[pidc-1], SIGKILL);
        pidc--;
    }

    return ret;
}
