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
char* getPWD(){
    pid_t pid, sid, status;
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
        char *buffer = (char *)calloc(sizeof(char), 128);
        read(fds[0],buffer, 128);
        close(fds[0]);
        buffer[strlen(buffer)-1]='/';
        return buffer;
    }
}



int main(int argc, char *argv[]) {
    /* Our process ID and Session ID */
    char *taskfile = getPWD();
    char *outfile = getPWD();
    strcat(taskfile,argv[1]);
    printf("%s\n",taskfile);
    strcat(outfile,argv[2]);
    printf("%s",outfile);
    //printf("%s", PWD);
}
