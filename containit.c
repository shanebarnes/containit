#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

enum term_resp {
    TERM_RESPONSE_NULL    = 0,
    TERM_RESPONSE_RESTART = 1,
    TERM_RESPONSE_STOP    = 2,
    TERM_RESPONSE_INVALID = 3
};

int tpid = 0;

void signal_handler(int sig) {
    int status = 0;

    if (sig == SIGCHLD) {
        tpid = waitpid(-1, &status, WNOHANG);
        if (WIFEXITED(status)) {
            fprintf(stderr,
                    "%d: Process terminated with exit status %d\n",
                    tpid,
                    WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr,
                    "%d: Process terminated by signal %d\n",
                    tpid,
                    WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            fprintf(stderr,
                    "%d: Process stopped by signal %d\n",
                    tpid,
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
    int ret = argc > 2 ? EXIT_SUCCESS : EXIT_FAILURE;
    enum term_resp resp = TERM_RESPONSE_NULL;
    int i, pid, pidc = 0, pidv[ARG_MAX];

    signal(SIGCHLD, signal_handler);

    do {
        if ((resp == TERM_RESPONSE_NULL) && (argc > 1)) {
            if (strncasecmp("stop-on-term",
                            argv[1],
                            strlen(argv[1])) == 0) {
                resp = TERM_RESPONSE_STOP;
            } else if (strncasecmp("restart-on-term",
                                   argv[1],
                                   strlen(argv[1])) == 0) {
                resp = TERM_RESPONSE_RESTART;
            } else {
                fprintf(stderr,
                        "\nError: Invalid signal termination response\n");
                resp = TERM_RESPONSE_INVALID;
            }
        }

        for (i = 2; resp != TERM_RESPONSE_INVALID && i < argc; i++) {
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
            fprintf(stderr,
                    "%d: Destroying remaining child processes\n",
                    getpid());
        } else {
            fprintf(stderr,
                    "\n%d: No child processes were created\n",
                    getpid());
            fprintf(stderr,
                    "\nusage: %s [restart-on-term | stop-on-term] "
                    "[program 1] [program 2] ...\n\n",
                    argv[0]);
        }

        while (pidc > 0) {
            if (pidv[pidc-1] != tpid ) {
                kill(pidv[pidc-1], SIGKILL);
                waitpid(pidv[pidc-1], NULL, 0);
            }
            pidc--;
        }

        if (resp == TERM_RESPONSE_RESTART) {
            usleep(1000*1000);
        }
    } while (resp == TERM_RESPONSE_RESTART);

    return ret;
}
