#include "functions.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>
enum PIPES { READ, WRITE };
void continuePipe(int *fds, int depth, int counter, char ***commands,
                  char *filename, int mode) {
    int status;
    pid_t pid;
    int null = open("/dev/null", O_WRONLY);
    int file = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0664);
    if ((pid = fork()) == 0) {
        close(fds[READ]);
        dup2(fds[WRITE], STDOUT_FILENO);
        close(fds[WRITE]);
        if (mode == 0)
            dup2(null, STDERR_FILENO);
        else if (mode == 1) {
            dup2(null, STDOUT_FILENO);
            dup2(file, STDERR_FILENO);
        } else if (mode == 2)
            dup2(file, STDERR_FILENO);
        pid_t pid;
        pid = fork();
        if (pid == 0) {
            execvp(commands[counter][0], commands[counter]);
        } else {
            setlogmask(LOG_UPTO(LOG_INFO));
            openlog("PIPE", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
            int status;
            waitpid(pid, &status, 0);
            char *args = (char *)calloc(100, sizeof(char));
            int i = 0;
            while (commands[counter][i + 1] != NULL) {
                strcat(args, commands[counter][i + 1]);
                i++;
            }
            syslog(LOG_INFO, "(%s %s) Kod wyjścia: %d", commands[counter][0],
                   args, status);
            closelog();
        }
    } else {
        close(fds[WRITE]);
        dup2(fds[READ], STDIN_FILENO);
        close(fds[READ]);
        if (depth > counter + 2) {
            int fds2[2];
            pipe(fds2);
            continuePipe(fds2, depth, counter + 1, commands, filename, mode);
        } else {
            int file = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0664);
            dup2(file, STDOUT_FILENO);
            pid_t pid2;
            pid2 = fork();
            if (pid2 == 0) {
                execvp(commands[counter + 1][0], commands[counter + 1]);
            } else {
                setlogmask(LOG_UPTO(LOG_INFO));
                openlog("PIPE", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                int status;
                waitpid(pid, &status, 0);
                char *args = (char *)calloc(100, sizeof(char));
                int i = 0;
                while (commands[counter + 1][i + 1] != NULL) {
                    strcat(args, commands[counter + 1][i + 1]);
                    i++;
                }
                syslog(LOG_INFO, "(%s %s) Kod wyjścia: %d",
                       commands[counter + 1][0], args, status);
                closelog();
            }
            waitpid(pid, &status, 0);
        }
    }
}
void startPipe(int depth, char ***commands, char *filename, int mode) {
    // mode = 0 - only STDOUT
    // mode = 1 - only STDERR
    // mode = 2 - both STDOUT and STDERR
    int fds[2], status, counter = 0;
    pid_t pid;
    int null = open("/dev/null", O_WRONLY);
    int file = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0664);
    pipe(fds);
    if ((pid = fork()) == 0) {
        close(fds[READ]);
        if (depth != 1) {
            dup2(fds[WRITE], STDOUT_FILENO);
            close(fds[WRITE]);
        } else {
            dup2(file, STDOUT_FILENO);
        }
        // obsluga mode'ow
        ////////////////////
        if (mode == 0)
            dup2(null, STDERR_FILENO);
        else if (mode == 1) {
            dup2(null, STDOUT_FILENO);
            dup2(file, STDERR_FILENO);
        } else if (mode == 2)
            dup2(file, STDERR_FILENO);
        ///////////////////
        pid_t pid;
        pid = fork();
        if (pid == 0) {
            execvp(commands[counter][0], commands[counter]);
        } else {
            setlogmask(LOG_UPTO(LOG_INFO));
            openlog("PIPE", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
            int status;
            waitpid(pid, &status, 0);
            char *args = (char *)calloc(100, sizeof(char));
            int i = 0;
            while (commands[counter][i + 1] != NULL) {
                strcat(args, commands[counter][i + 1]);
                i++;
            }
            syslog(LOG_INFO, "(%s %s) Kod wyjścia: %d", commands[counter][0],
                   args, status);
            closelog();
        }
    } else {
        int file = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0664);
        close(fds[WRITE]);
        dup2(fds[READ], STDIN_FILENO);
        close(fds[READ]);

        // obsluga mode'ow
        //////////////////
        if (mode == 0)
            dup2(null, STDERR_FILENO);
        else if (mode == 1) {
            dup2(null, STDOUT_FILENO);
            dup2(file, STDERR_FILENO);
        } else if (mode == 2)
            dup2(file, STDERR_FILENO);
        //////////////////

        if (depth > counter + 2) {
            int fds2[2];
            pipe(fds2);
            continuePipe(fds2, depth, counter + 1, commands, filename, mode);
        } else if (depth == 2) {
            dup2(file, STDOUT_FILENO);
            close(file);
            pid_t pid;
            pid = fork();
            if (pid == 0) {
                execvp(commands[counter + 1][0], commands[counter + 1]);
            } else {
                setlogmask(LOG_UPTO(LOG_INFO));
                openlog("PIPE", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                int status;
                waitpid(pid, &status, 0);
                char *args = (char *)calloc(100, sizeof(char));
                int i = 0;
                while (commands[counter + 1][i + 1] != NULL) {
                    strcat(args, commands[counter + 1][i + 1]);
                    i++;
                }
                syslog(LOG_INFO, "(%s %s) Kod wyjścia: %d",
                       commands[counter + 1][0], args, status);
                closelog();
            }
        } else {
            close(file);
        }
        waitpid(pid, &status, 0);
    }
}

int main(int argc, char *argv[]) {
    char *filename = argv[2], *mode = argv[3],
         **commandsSplitIntoTheirOwnStrings, ***commands,
         *command = (char *)malloc(sizeof(char) * 100);
    strcpy(command, argv[1]);
    int modeInt = mode[0] - 48, depthOfPipe, i, tmp;
    commandsSplitIntoTheirOwnStrings = split(argv[1], "|", &depthOfPipe);
    commands = (char ***)malloc(sizeof(char **) * 10);
    for (i = 0; i < depthOfPipe; i++) {
        commands[i] = split(commandsSplitIntoTheirOwnStrings[i], " ", &tmp);
    }
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog("PIPELINE", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    syslog(LOG_INFO, "Polecenie: %s", command);
    closelog();

    startPipe(depthOfPipe, commands, filename, modeInt);
    return 0;
}