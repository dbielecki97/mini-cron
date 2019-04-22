#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h> /* false */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#define CET 2

int row = 0, in, out, needsSorting = 1, needsSending = 0;
char *pipePath;

struct tm *getTime() {
    time_t seconds;
    time(&seconds);
    struct tm *ptm = gmtime(&seconds);
    return ptm;
}

int getTaskSeconds(char *buffer) {
    int hour = (buffer[0] % 48) * 10 + (buffer[1] % 48);
    int minute = (buffer[3] % 48) * 10 + (buffer[4] % 48);
    return 60 * minute + hour * 60 * 60;
}

void doTask(char *buffline, char *outfile) {
    if (buffline == NULL)
        return;
    pid_t pid, status;
    pid = fork();
    if (pid == (pid_t)0) {
        FILE *file = fopen(outfile, "a");
        char *command = (char *)calloc((strlen(buffline) - 8), sizeof(char));
        memmove(command, buffline + 6, (strlen(buffline) - 9));
        command[strlen(command)] = '\0';
        fprintf(file, ">>%s\n", command);
        fflush(file);
        fclose(file);
        char mode = buffline[strlen(buffline) - 2];
        char *arg[] = {"pipe", command, outfile, &mode, NULL};
        execvp(pipePath, arg);
    } else {
        waitpid(pid, &status, 0);
        exit(0);
    }
}

char *getNextTask(char *taskfile) {
    FILE *stream = fopen(taskfile, "r");
    if (stream == NULL)
        perror("Blad otwarcia pliku");

    char *buffer = (char *)calloc(sizeof(char), 1024);
    size_t buf_size = 128;
    int j = 0;
    while (!feof(stream) && !ferror(stream) &&
           getline(&buffer, &buf_size, stream) != EOF) {
        if (j++ >= row) {
            row = j;
            fclose(stream);
            return buffer;
        }
    }
    return NULL;
}

void writeFromFile(FILE **stream, char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Blad otwarcia pliku");
        return;
    }
    char c = getc(file);
    while (c != EOF) {
        fprintf(*stream, "%c", c);
        fflush(*stream);
        c = getc(file);
    }
    fclose(file);
}

void sortTaskFile(char *taskfile) {
    dup(in);
    dup(out);
    pid_t pid, status;
    int fds[2];
    pipe(fds);
    pid = fork();
    if (pid == (pid_t)0) {
        dup2(fds[0], STDIN_FILENO);
        int fd = open(taskfile, O_WRONLY);
        dup2(fd, STDOUT_FILENO);
        close(fds[0]);
        close(fds[1]);
        execlp("sort", "sort", (char *)0);
    } else {
        FILE *stream;
        close(fds[0]);
        stream = fdopen(fds[1], "w");
        writeFromFile(&stream, taskfile);
        close(fds[1]);
        waitpid(pid, &status, 0);
        fclose(stream);
    }
    FILE *main_file = fopen(taskfile, "r");
    FILE *extra_file = fopen("extra.txt", "w");

    struct tm *ptm = getTime();
    int act_seconds =
        ptm->tm_sec + ptm->tm_min * 60 + ((ptm->tm_hour + CET) % 24) * 60 * 60;
    char *buffer = (char *)calloc(sizeof(char), 1024);
    size_t buf_size = 1024;
    while (!feof(main_file) && !ferror(main_file) &&
           getline(&buffer, &buf_size, main_file) != EOF) {
        int task_seconds = getTaskSeconds(buffer);
        if (task_seconds > act_seconds) {
            fprintf(extra_file, "%s", buffer);
            fflush(extra_file);
        }
    }
    fclose(extra_file);
    fclose(main_file);
    main_file = fopen(taskfile, "r");
    extra_file = fopen("extra.txt", "a");
    while (!feof(main_file) && !ferror(main_file) &&
           getline(&buffer, &buf_size, main_file) != EOF) {
        int task_seconds = getTaskSeconds(buffer);
        if (task_seconds < act_seconds) {
            fprintf(extra_file, "%s", buffer);
            fflush(extra_file);
        }
    }
    fclose(main_file);
    remove(taskfile);
    rename("extra.txt", taskfile);
    fclose(extra_file);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
}

int sleepIfNeeded(char *buffer) {
    int timeLeft = 0;
    int task_seconds = getTaskSeconds(buffer);
    struct tm *ptm = getTime();
    int act_seconds =
        ptm->tm_sec + ptm->tm_min * 60 + ((ptm->tm_hour + CET) % 24) * 60 * 60;
    if (act_seconds != task_seconds) {
        if (task_seconds - act_seconds < 0) {
            // timeLeft = slee(24*60*60 - act_seconds + task_seconds);
        }
        //else timeLeft = sleep(task_seconds - act_seconds);
        timeLeft = sleep(15);
    }
    return timeLeft;
}
void handler(int sig) {
    if (sig == SIGUSR1) {
        needsSorting = 1;
    } else if (sig == SIGUSR2) {
        needsSending = 1;
    }
}
void sendRemainingTasks(char *taskfile){
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog("REMAINING TASKS", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    int row_copy = row;
    row -= 1;
    char *buffer = getNextTask(taskfile);
    while(buffer != NULL){
        syslog(LOG_INFO, "%s", buffer);
        buffer = getNextTask(taskfile);
    }
    row = row_copy-1;
    closelog();
}

char* getPWD(){
    pid_t pid;
    int fds[2];
    pipe(fds);
    pid = fork();
    if(pid == (pid_t)0){
        close(fds[0]);
        dup2(fds[1],STDOUT_FILENO);
        execlp("pwd","pwd",(char *)0);
    }
    else {
        close(fds[1]);
        waitpid(pid,NULL,0);
        char *buffer = (char *)calloc(sizeof(char), 256);
        read(fds[0],buffer, 256);
        close(fds[0]);
        buffer[strlen(buffer)-1]='/';
        return buffer;
    }
}

int main(int argc, char *argv[]) {
    pid_t pid, sid, status;

    char *taskfile = getPWD();
    char *outfile = getPWD();
    char *path = getPWD();
    strcat(path,"pipe");
    pipePath = (char *)calloc(sizeof(char), strlen(path)+1);
    pipePath[0] = '.';
    strcat(pipePath, path);
    strcat(taskfile,argv[1]);
    strcat(outfile,argv[2]);

    signal(SIGUSR1, handler);
    signal(SIGUSR2, handler);


    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    umask(0);

    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }

    in = dup(0);
    out = dup(1);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);



    char *buffer = NULL;

    pid = fork();
    if (pid == -1) {
        perror("fork");
        assert(false);
    } else {
        if (pid == 0) {
            exit(EXIT_SUCCESS);
        }
        waitpid(pid, NULL, 0);
        while (1) {
            if (needsSorting) {
                sortTaskFile(taskfile);
                row = 0;
                needsSorting = 0;
            }
            if (needsSending){
                sendRemainingTasks(taskfile);
                needsSending = 0;
            }
            buffer = getNextTask(taskfile);
            if (buffer == NULL) {
                printf("Koniec listy zadan\n");
                break;
            }
            if (sleepIfNeeded(buffer) == 0) {
                pid = fork();
                if (pid == (pid_t)0) {
                    printf("%s", buffer);
                    doTask(buffer, outfile);
                    exit(0);
                } else {
                    waitpid(pid, NULL, 0);
                }
            }
        }
    }
    exit(EXIT_SUCCESS);
}
