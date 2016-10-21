#include <errno.h>
#if defined(__linux__)
    #include <dirent.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/sysctl.h>
#include <sys/wait.h>
#include <unistd.h>

enum term_resp
{
    TERM_RESPONSE_NULL    = 0,
    TERM_RESPONSE_RESTART = 1,
    TERM_RESPONSE_STOP    = 2,
    TERM_RESPONSE_INVALID = 3
};

int tpid = 0;

void signal_handler(int sig)
{
    int status = 0;

    if (sig == SIGCHLD)
    {
        tpid = waitpid(-1, &status, WNOHANG);
        if (WIFEXITED(status))
        {
            fprintf(stderr,
                    "%d: Process terminated with exit status %d\n",
                    tpid,
                    WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            fprintf(stderr,
                    "%d: Process terminated by signal %d\n",
                    tpid,
                    WTERMSIG(status));
        }
        else if (WIFSTOPPED(status))
        {
            fprintf(stderr,
                    "%d: Process stopped by signal %d\n",
                    tpid,
                    WSTOPSIG(status));
        }
    }
}

void split_arg(char *arg, int argc, char **argv)
{
    char *tok = " \t\n";
    int i = 0;

    if (argc > 0)
    {
        argv[i] = strtok(arg, tok);
        while ((argv[i] != NULL) && (i < argc))
        {
            argv[++i] = strtok(NULL, tok);
        }
    }
}

void exec_arg(char *arg)
{
    char *argv[512], copy[1024], pid[16];

    sprintf(pid, "%d", getpid());
    if (strlen(arg) < (sizeof(copy) / sizeof(copy[0])))
    {
        strncpy(copy, arg, sizeof(copy) / sizeof(copy[0]) - 1);
        split_arg(arg, sizeof(argv) / sizeof(argv[0]), argv);
        execvp(*argv, argv);

        perror(pid);
        exit(errno);
    }
    else
    {
        fprintf(stderr, "%s: Error: Argument length is too large\n", pid);
    }
}

int killchildren(pid_t cpid)
{
    int ret = -1;
#if defined(__APPLE__)
    char pid[16];
    pid_t oid = 0;
    size_t len = sizeof(ret);
    int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL };
    struct kinfo_proc *pinfo = NULL;
    int pcount = 0, i;

    sprintf(pid, "%d", cpid);
    if (sysctl(mib, sizeof(mib) / sizeof(int), NULL, &len, NULL, 0) == -1)
    {
        perror(pid);
    }
    else if ((pinfo = malloc(len)) == NULL)
    {
        perror(pid);
    }
    else if (sysctl(mib, sizeof(mib) / sizeof(int), pinfo, &len, NULL, 0) == -1)
    {
        perror(pid);
        free(pinfo);
    }
    else
    {
        ret = 0;
        pcount = len / sizeof(struct kinfo_proc);

        for (i = 0; i < pcount; i++)
        {
            if (cpid == pinfo[i].kp_eproc.e_ppid)
            {
                ret++;
                oid = pinfo[i].kp_proc.p_pid;
                kill(oid, SIGKILL);
                fprintf(stdout, "%d: Destroyed orphan %d\n", cpid, oid);
            }
        }

        free(pinfo);
    }
#elif defined(__linux__)
    DIR *dir = NULL;
    FILE *file = NULL;
    struct dirent *entry = NULL;
    char *line = NULL, fname[128], pid[16];
    size_t len = 0;
    int oid, ppid;

    sprintf(pid, "%d", cpid);
    if ((dir = opendir("/proc")) == NULL)
    {
        perror(pid);
    }
    else if ((entry = readdir(dir)) == NULL)
    {
        perror(pid);
        closedir(dir);
    }
    else
    {
        ret = 0;

        while (entry != NULL)
        {
            if ((entry->d_type == DT_DIR) &&
                (sscanf(entry->d_name, "%d", &oid) == 1))
            {
                sprintf(fname, "%s/%s/status", "/proc", entry->d_name);
                if ((file = fopen(fname, "r")) != NULL)
                {
                    while (getline(&line, &len, file) != -1)
                    {
                        ppid = -1;
                        if (sscanf(line, "PPid: %d", &ppid) == 1)
                        {
                            break;
                        }
                    }

                    fclose(file);

                    if (ppid == cpid)
                    {
                        ret++;
                        kill(oid, SIGKILL);
                        fprintf(stdout, "%d: Destroyed orphan %d\n", cpid, oid);
                    }
                }
            }

            entry = readdir(dir);
        }

        closedir(dir);
    }
#endif
    return ret;
}

int main(int argc, char **argv)
{
    int ret = argc > 2 ? EXIT_SUCCESS : EXIT_FAILURE;
    enum term_resp resp = TERM_RESPONSE_NULL;
    int i, pid, pidc = 0, pidv[131072];

    signal(SIGCHLD, signal_handler);

    do
    {
        if ((resp == TERM_RESPONSE_NULL) && (argc > 1))
        {
            if (strncasecmp("stop-on-term",
                            argv[1],
                            strlen(argv[1])) == 0)
            {
                resp = TERM_RESPONSE_STOP;
            }
            else if (strncasecmp("restart-on-term",
                                 argv[1],
                                 strlen(argv[1])) == 0)
            {
                resp = TERM_RESPONSE_RESTART;
            }
            else
            {
                fprintf(stderr,
                        "\nError: Invalid signal termination response\n");
                resp = TERM_RESPONSE_INVALID;
            }
        }

        for (i = 2; (resp != TERM_RESPONSE_INVALID) && (i < argc); i++)
        {
            pid = fork();

            if (pid == -1)
            {
                perror("Error");
            }
            else if (pid == 0)
            {
                exec_arg(argv[i]);
            }
            else
            {
                fprintf(stdout,
                        "%d: Created child process (%s)\n",
                        pid,
                        argv[i]);
                pidv[pidc] = pid;
                pidc++;
            }
        }

        if (pidc > 0)
        {
            pause();
            fprintf(stderr,
                    "%d: Destroying remaining child processes\n",
                    getpid());
        }
        else
        {
            fprintf(stderr,
                    "\n%d: No child processes were created\n",
                    getpid());
            fprintf(stderr,
                    "\nusage: %s [restart-on-term | stop-on-term] "
                    "[program 1] [program 2] ...\n\n",
                    argv[0]);
        }

        while (pidc > 0)
        {
            killchildren(pidv[pidc-1]);
            if (pidv[pidc-1] != tpid)
            {
                kill(pidv[pidc-1], SIGKILL);
                waitpid(pidv[pidc-1], NULL, 0);
            }
            pidc--;
        }

        if (resp == TERM_RESPONSE_RESTART)
        {
            usleep(1000*1000);
        }
    } while (resp == TERM_RESPONSE_RESTART);

    return ret;
}
