#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <sys/wait.h> 
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <stdbool.h> /* false */


#define CET 2
struct tm* getTime(){
    time_t seconds;
    time (&seconds);
    struct tm *ptm = gmtime(&seconds);
    return ptm;
}
//----------------------------------------------------------------zmienna globalna xd
int row = 0;

int getTaskSeconds(char* buffer){
    int hour = (buffer[0]%48)*10 + (buffer[1]%48);
    int minute = (buffer[3]%48)*10 + (buffer[4]%48);
     return 60*minute + hour*60*60; 
}


void doTask(char* buffline, char* outfile){
    if(buffline == NULL) return;
    pid_t pid, status;
    pid = fork();
    if(pid == (pid_t) 0){
        FILE *file = fopen(outfile, "a");
        char *command = (char*)calloc((strlen(buffline)-8),sizeof(char));
        memmove(command, buffline+6, (strlen(buffline)-9));
        command[strlen(command)]='\0';
        fprintf(file, ">>%s\n", command);
        fflush(file);
        fclose(file);
        char mode = buffline[strlen(buffline)-2];
        char *arg[]={"pipe",command, outfile, &mode, NULL};
        //printf("mincron: %s %s %c\n",command, outfile, mode);
        execvp("./pipe", arg);       
    }
    else {
        waitpid(pid, &status, 0);
        exit(0);
    }
} 

char* getNextTask(char *taskfile){
    /*
&buffer is the address of the first character position where the input string will be stored. It’s not the base address of the buffer, but of the first character in the buffer. This pointer type (a pointer-pointer or the ** thing) causes massive confusion.

&buf_size is the address of the variable that holds the size of the input buffer, another pointer.

stream is the input file handle. So you could use getline() to read a line of text from a file, but when stdin is specified, standard input is read.
    */
    //printf("%p\n",(void *)stream);
    FILE *stream = fopen(taskfile, "r");
    if(stream == NULL)
        perror("Blad otwarcia pliku");

    char* buffer = (char*)calloc(sizeof(char),1024);
    size_t buf_size = 128;
    int j = 0;
    while(!feof (stream) && !ferror (stream) && getline(&buffer,&buf_size,stream)!=EOF){
        if(j++ >= row){
            row = j;
            fclose(stream);
            //printf("%s",buffer);
            return buffer;
        }
    }
    return NULL;
}

void writeFromFile (FILE** stream, char* filename) {
    FILE *file = fopen(filename, "r");
    if(file == NULL){
        perror("Blad otwarcia pliku");
        return;
    }
    //printf("writeFromFile--> %s\n", filename);
    char c = getc(file); 
    while (c != EOF){
    //  printf("%c", c);
        fprintf(*stream, "%c", c);
        fflush(*stream);
        c = getc(file);
    }
    fclose(file);
}

void sortTaskFile(char* taskfile){
    pid_t pid, status;
    int fds[2]; 
    pipe(fds);
    pid = fork();
    if (pid == (pid_t) 0) { //child
        dup2(fds[0], STDIN_FILENO);
        int fd = open(taskfile, O_WRONLY);
        dup2(fd, STDOUT_FILENO);
        close(fds[0]);
        close(fds[1]);
        execlp("sort", "sort", (char* )0);
    } 
    else { //parent
        FILE* stream;
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
    int act_seconds = ptm->tm_sec + ptm->tm_min*60 + ((ptm->tm_hour+CET)%24)*60*60;
    char* buffer = (char*)calloc(sizeof(char),1024);
    size_t buf_size = 128;
    while(!feof (main_file) && !ferror (main_file) && getline(&buffer,&buf_size,main_file)!=EOF){
        int task_seconds = getTaskSeconds(buffer);
        if(task_seconds > act_seconds){
            fprintf(extra_file,"%s",buffer);
            fflush(extra_file);   
        }
    }
    fclose(extra_file);
    fclose(main_file);
    main_file = fopen(taskfile, "r");
    extra_file = fopen("extra.txt", "a");
    while(!feof (main_file) && !ferror (main_file) && getline(&buffer,&buf_size,main_file)!=EOF){
        int task_seconds = getTaskSeconds(buffer);
        if(task_seconds < act_seconds){
            fprintf(extra_file,"%s",buffer);
            fflush(extra_file);   
        }
    }    
    fclose(main_file);
    remove(taskfile);
    rename("extra.txt", taskfile);
    fclose(extra_file);
}

void sleepIfNeeded(char* buffer){
    if(buffer == NULL) return;
    int task_seconds = getTaskSeconds(buffer);
    struct tm *ptm = getTime();
    int act_seconds = ptm->tm_sec + ptm->tm_min*60 + ((ptm->tm_hour+CET)%24)*60*60;
    if(act_seconds != task_seconds){
        //printf("godz:%d:%d:%d, odliczanie->%d\n",(ptm->tm_hour+CET)%24, ptm->tm_min, ptm->tm_sec, (task_seconds - act_seconds));
        if(task_seconds - act_seconds < 0){
            //sleep(24*60*60 - act_seconds + task_seconds);
        }
        //sleep(task_seconds - act_seconds);
        sleep(10);
    }
}

int main(int argc, char* argv[]) {   
    /* Our process ID and Session ID */
    pid_t pid, sid, status;
    char* taskfile = argv[1];
    char* outfile = argv[2];

    sortTaskFile(taskfile);
//-------------inicjalizacja DEMONA-------------------------------

    /* Fork off the parent process */
    pid = fork();
    if(pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if(pid > 0) {
        exit(EXIT_SUCCESS);
    }
    /* Change the file mode mask */
    umask(0);            
    /* Open any logs here */        
           
    /* Create a new SID for the child process */
    sid = setsid();
    if(sid < 0) {
        /* Log the failure */
        exit(EXIT_FAILURE);
    }        
    /* Change the current working directory */
/*if ((chdir("/")) < 0) {
    //Log the failure 
    exit(EXIT_FAILURE);
}    
    /* Close out the standard file descriptors */   
    //close(STDIN_FILENO);
    //close(STDOUT_FILENO);
    //close(STDERR_FILENO);
/*-------------------koniec inicjalizacji !!----------------------------------*/

    /* Daemon-specific initialization goes here */

    printf("AAAAAAA\n");
    sortTaskFile(taskfile);
    printf("BBBBBBB\n");

    char* buffer = NULL;
    /* The Big Loop */
    pid = fork();
    if (pid == -1){
        perror("fork");
        assert(false);
    }
    else {
        if(pid == 0){
            exit(EXIT_SUCCESS);
        }
        waitpid(pid, NULL, 0);
        while (1) {           
            buffer = getNextTask(taskfile);
            if(buffer == NULL){
                printf("Koniec listy zadan\n");
                break;
            }
            //sleepIfNeeded(buffer);
            pid = fork();
            if(pid==(pid_t)0){
                printf("%s",buffer);
                doTask(buffer, outfile);
                exit(0);
            }
            else {
                waitpid(pid, NULL, 0);
            }
        }
    }
    exit(EXIT_SUCCESS);
}
















