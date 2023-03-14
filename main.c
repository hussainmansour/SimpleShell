#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>

#define GREEN "\033[1m\033[32m"
#define BLUE "\033[1m\033[34m"
#define WHITE "\033[0m"

#define  new(type,size)       (type) malloc( size * sizeof(type) )
// Function to print Current Directory.
void printDir(char* username)
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf(GREEN"\nUSER is: @%s:", username);
    printf(BLUE"~%s", cwd);
    printf(WHITE"$");
}

// get the input from the user
char * takeInput(size_t n){
    char* buffer= NULL;
    ssize_t chars = getline(&buffer, &n, stdin);
    if (buffer[chars - 1] == '\n')
    {
        buffer[chars - 1] = '\0';
        --chars;
    }
    return buffer;
}

//parsing the input from the buffer to argument vector
void processString(char *buf, char *token, char** argv){
    token = strtok(buf, " ");
    int i = 0;
    argv[i++] = token;
    while (token) {
        token = strtok(NULL, " ");
        argv[i++] = token;
    }
}

// Function where the system command is executed
void execArgs(char** parsed)
{
    //to handel ls $x  and x = -a -l -h  or -l
    int status;
    int background = 0;
    char* name = new(char* , 100);
    if (strcmp(parsed[0], "ls") == 0 && parsed[1] != NULL) {
        if (parsed[1][0] == '$') {
            int j = 0;
            for (int i = 1; i < strlen(parsed[1]); ++i) {
                name[j++] = parsed[1][i];
            }
            name[j] = '\0';
            char* ptr = getenv(name);
            char* token;
            token = strtok(ptr, " ");
            int i = 1;
            parsed[i++] = token;
            while (token) {
                token = strtok(NULL, " ");
                parsed[i++] = token;
            }
        }
    }

    free(name);
    if(parsed[1] != NULL){
        if(strcmp(parsed[1],"&")==0){
            background=1;
            parsed[1] = NULL;
        }
    }
    // Forking a child
    pid_t pid = fork();
    if (pid == -1) {
        printf("\nFailed forking child..");
        return;
    } else if (pid == 0) {
        if(background){
            if (execvp(parsed[0], parsed) < 0) {
                printf("\nCould not execute command..");
            }
            exit(0);
        }
        if (execvp(parsed[0], parsed) < 0) {
            printf("\nCould not execute command..");
        }
        exit(0);
    } else {
        // waiting for child to terminate
        if(!background)
            waitpid(pid, &status, 0);

        return;
    }
}

// Function to execute builtin commands // cd export echo exit
int cdHandler(char** parsed, char* buf, char* home)
{
    if (parsed[1] == NULL || strcmp(parsed[1], "~") == 0){
        return chdir( home);
    }

    else if(parsed[1]!=NULL) {
        return chdir(parsed[1]);
    }
    return 1;
}

int echoHandler(char** parsed, char* buf)
{
    char *ptr = new(char *, 100);
    int j=1,k=0;
    // remove quotes
    while (parsed[j]!=NULL){
        for (int i = 0; i < strlen(parsed[j]); ++i) {
            if(parsed[j][i]!='"')
                ptr[k++] = parsed[j][i];
        }
        ptr[k++] = ' ';
        j++;
    }
    ptr[k] = '\0';
    //handel dollar sign

    int i=0;
    j=0;
    char *ptr2 = new(char *, 100);
    char* name =new(char *, 100);

    while (ptr[i]!='\0'){
        if(ptr[i] != '$'){
            ptr2[j++] = ptr[i];
        }
        else{
            i++;
            int t=0;
            while(ptr[i] != ' '){
                name[t++] = ptr[i];
                i++;
            }
            name[t]='\0';
            char* value = getenv(name);
            t=0;
            while (value[t]!='\0'){
                ptr2[j++] = value[t++];
            }
        }
        i++;
    }

    printf("%s",ptr2);
    free(ptr);
    free(ptr2);
    free(name);
    return 0;
}



int exportHandler(char** parsed, char* buf)
{
    char* name = new(char *, 100);
    char* value = new(char *, 100);

    int j=1,k1=0,k2=0,f=1;
    // remove quotes
    while (parsed[j]!=NULL){
        for (int i = 0; i < strlen(parsed[j]); ++i) {
            if(parsed[j][i]=='=') {
                f = 0;
                continue;
            }
            if(f == 1)
                name[k1++] = parsed[j][i];
            else
                value[k2++] = parsed[j][i];
        }
        j++;
        if(parsed[j]!=NULL){
            value[k2++] = ' ';
        }
    }
    name[k1++] = '\0';
    value[k2++] = '\0';

    k1 = 0,k2=0;
    char * NewValue = new(char *, 100);
    while (value[k1]!='\0'){
        if(value[k1] != '"')
            NewValue[k2++] = value[k1];
        k1++;
    }
    NewValue[k2] = '\0';
    free(value);
    return setenv(name,NewValue,1);
}

void writeToLog() {
    FILE *logging;
    logging = fopen("/home/hussein/CLionProjects/log.txt", "a");

    if (logging == NULL) {
        printf("failed to open file\n");
        return;
    }

    fprintf(logging, "Child process was terminated\n");
    fclose(logging);
}

void on_child_exit(int signal) {
    int status;
    pid_t pid;
    pid = waitpid((pid_t) -1, &status, WNOHANG);
    writeToLog();
}




int chooseOperationType(char* argv[],char* buf,size_t n,char* home){

    if(strcmp(argv[0],"cd")==0){
        return cdHandler(argv,buf,home);
    }
    else if(strcmp(argv[0],"echo")==0)
    {
        return echoHandler(argv,buf);

    }
    else if(strcmp(argv[0],"export")==0)
    {
        return exportHandler(argv,buf);
    }
    else if(strcmp(argv[0],"exit")==0)
    {
        exit(0);
    }
    else if(strcmp(argv[0],"clear")==0)
    {
        system("clear");
    }
    else{
        execArgs(argv);
    }
    return 0;
}

void set_up_environment(char* home){
    chdir(home);
}

int main(int argc, char* argv[]) {
    signal(SIGCHLD,on_child_exit);
    size_t n = 10;
    char *buf = NULL;
    char *token = NULL;
    char* username = getenv("USER");
    char* home = getenv("HOME");
    set_up_environment(home);

    printf("\t\t*************start********\n");
    while (1) {

        char **argvv = malloc(10 * sizeof(char *)); // Allocate row pointers
        for(int i = 0; i < 10; i++)
            argvv[i] = malloc(10 * sizeof(char));

        printDir(username);

        buf = takeInput(n);

        processString(buf, token, argvv);

        if(chooseOperationType(argvv,buf,n,home) != 0)
            printf("Can't Execute the command\n");

        free(argvv);
    }
    return 0;

}
