#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX_BUF 1024
#define MAX_TRANSF 7

//struct processos
struct process
{
    int pid;
    //1 -> Proc-File 0 -> Status
    int message;
    //0 -> pending 1 -> executing
    int status;

    char* request;
    char** transformations;
    struct process *next;
};
struct process *start=NULL;

struct transformation
{
    char* name;
    int running;
    int max;
}* Transformation;

struct transformation transf[7];
char transf_folder[30];

void printProcess()
{
    struct process *temp;

    temp = start;

    while(temp)
    {
        printf("PID: %d\n",temp->pid);
        if(temp->message)
        {
            printf("Proc-File\n");
            for(int i = 0; temp->transformations[i];i++)
                printf("Transf: %s\n",temp->transformations[i]);
        }
        temp = temp->next;
    }
}

struct process* insert_client(int pid, char* buffer)
{
    struct process *temp,*ptr;
    char* mess = malloc(sizeof(char)*300);
    strcpy(mess,buffer);

    temp=(struct process *)malloc(sizeof(struct process));

    temp->pid = pid;
    
    if(strncmp(buffer, "status", 6) == 0)
        temp->message = 0;
    else
    {
        temp->request = mess;
        temp->message = 1;
        char* args = buffer + 10;
        char* token = strtok(args," ");
        token = strtok(NULL, " ");
        token = strtok(NULL, " \0");

        char** transformations = malloc(sizeof(char*)*MAX_TRANSF);
        int i = 0;
        while(token != NULL)
        {
            transformations[i] = malloc(sizeof(char) * strlen(token));
            strcpy(transformations[i], token);
            token = strtok(NULL, " \0");
            i++;  
        }
        transformations[i] = '\0';
        free(token);
        temp->transformations = transformations;
    }


    temp->next = NULL;
    if(start==NULL)
    {
        start=temp;
        return start;
    }
    else
    {
        ptr=start;
        while(ptr->next !=NULL)
        {
            ptr=ptr->next ;
        }
        ptr->next =temp;
        return temp;
    }
}

void addRunningTransf(char** transformations)
{
    printf("Adicionado!\n");
    for(int i = 0; transformations[i]; i++)
    {
        for(int j = 0; j < MAX_TRANSF; j++) 
        {
            if ((strcmp(transformations[i], transf[j].name) == 0))
                transf[j].running++;         
        }
    }
}

void removeRunningTransf(char** transformations)
{    
    for(int i = 0; i < 2; i++) 
    {
        for(int j = 0; j < 7; j++) 
        {
            if ((strcmp(transformations[i], transf[j].name) == 0))
                transf[j].running--;         
        }
    }
}

int checkConfig(char** transformations){
    for(int i = 0; transformations[i]; i++) {
        for(int j = 0; j < MAX_TRANSF; j++) {
            if ((strcmp(transformations[i], transf[j].name) == 0) && (transf[j].max == transf[j].running))
                return 0;         
        }
    }
    return 1;
}


void sigchild_handler(int signum)
{
    char* transf[2] = { "bcompress", "bdecompress" };
    int status;
    pid_t pid;

    //Devolve o pid do processo q morreu
    while((pid = waitpid(-1,&status,WNOHANG)) > 0)
    {
        struct process* temp = start;
        while(temp)
        {
            if(temp->pid==pid && temp->message == 1)
                removeRunningTransf(transf);
            temp = temp->next;
        }
    }
}

ssize_t myreadln(int fildes, void* buf, size_t nbyte)
{
    ssize_t size = 0;
    char c;
    char* buff = (char*)buf;

    while (size < nbyte && read(fildes, &c, 1) > 0) 
    {
        if (c == '\0')
            return size;
        
        buff[size++] = c;
        
        if (c == '\n')
            return size;
    }
    return size;
}

void getStatus(int server_client_fifo)
{
    char* buff = malloc(sizeof(char)*50*MAX_TRANSF);
    for (int i = 0;i < MAX_TRANSF; ++i)
    {
        char temp[30];
        sprintf(temp,"transf %s: %d/%d (running/max)\n",transf[i].name,transf[i].running,transf[i].max);
        strcat(buff,temp);
    }

    write(server_client_fifo,buff,strlen(buff));
}

void exec_commands(char* transformation)
{
    char nop[100];
    sprintf(nop,"%snop",transf_folder);

    char bcompress[100]; 
    sprintf(bcompress,"%sbcompress",transf_folder);

    
    char bdecompress[100]; 
    sprintf(bdecompress,"%sbdecompress",transf_folder);

    char gcompress[100]; 
    sprintf(gcompress,"%sgcompress",transf_folder);
    
    char gdecompress[100]; 
    sprintf(gdecompress,"%sgdecompress",transf_folder);

    char encrypt[100]; 
    sprintf(encrypt,"%sencrypt",transf_folder);

    char decrypt[100]; 
    sprintf(decrypt,"%sdecrypt",transf_folder);

    if (strcmp(transformation,"nop") == 0)        
        execl(nop,"nop", NULL);
    
    else if (strcmp(transformation,"bcompress") == 0)
        execl(bcompress,"bcompress", NULL);
    
    else if (strcmp(transformation,"bdecompress") == 0)
        execl(bdecompress,"bdecompress", NULL);
    
    else if (strcmp(transformation,"gcompress") == 0)
        execl(gcompress,"gcompress", NULL);
    
    else if (strcmp(transformation,"gdecompress") == 0)
        execl(gdecompress,"gdecompress", NULL);
    
    else if (strcmp(transformation,"encrypt") == 0)
        execl(encrypt,"encrypt", NULL);
    
    else if (strcmp(transformation,"decrypt") == 0)
        execl(decrypt,"decrypt", NULL);    
}

void transform(char* input_file, char* output_file, char** transformations)
{
    int fd_in = open(input_file, O_RDONLY, 0644);
    int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    
    int num_transformations;
    for(num_transformations = 0; transformations[num_transformations]; num_transformations++);

    printf("Num_T: %d\n", num_transformations);

    int p[num_transformations - 1][2];

    int status[num_transformations];

    
    if(num_transformations == 1) 
    {
        switch(fork()) {
            case -1:
                perror("fork");

            case 0:
                dup2(fd_in, 0);
                dup2(fd_out, 1);
                close(fd_in);
                close(fd_out);
                exec_commands(transformations[0]);
        }
    }
    else
    {
        for(int i = 0; i < num_transformations; i++) 
        {
            if (i == 0) 
            {
                if(pipe(p[i]) != 0) 
                {
                    perror("pipe");
                }
                switch(fork()) 
                {
                    case -1: 
                        perror("fork");
                    case 0:
                        dup2(fd_in, 0);
                        close(fd_in);

                        close(p[i][0]);
                    
                        dup2(p[i][1], 1);
                        close(p[i][1]);
                    
                        exec_commands(transformations[i]);

                        _exit(0);
                    default: 
                        close(p[i][1]);
                }
            }
            else if (i == num_transformations-1) 
            {
                switch(fork()) {
                    case -1:
                        perror("fork");
                    case 0:
                        dup2(fd_out, 1);
                        close(fd_out);

                        dup2(p[i-1][0],0);
                        close(p[i-1][0]);

                        exec_commands(transformations[i]);
                    
                        _exit(0);
                    default: 
                        close(p[i-1][0]);
                }
            }
            else {
                if(pipe(p[i]) != 0) {
                    perror("pipe");
                }
                switch(fork()) {
                    case -1:
                        perror("fork");
                    case 0:
                        close(p[i][0]);

                        dup2(p[i][1], 1);
                        close(p[i][1]);

                        dup2(p[i-1][0], 0);
                        close(p[i-1][0]);

                        exec_commands(transformations[i]);

                        _exit(0);
                    default:
                        close(p[i][1]);
                        close(p[i-1][0]);
                }
            }
        }
    }
   
    for(int w = 0; w < num_transformations; w++)
    {
        int pid = wait(&status[w]);  
        printf("Pid: %d\n",pid);
    }
}

void fillTransf(char * line, int i)
{
    transf[i].name = strdup(strtok(line, " "));
    transf[i].running = 0;
    transf[i].max = atoi(strtok(NULL, "\n"));
}

void parseTransf(char* filepath){

    int fd;
    
    if((fd = open(filepath, O_RDONLY, 0644)) == -1)
    {
        perror("File doesn't exist!\n");
        exit(-1);
    }
    

    char* line = malloc(sizeof(char) * MAX_BUF);
    int i=0;

    while((myreadln(fd, line, 30) > 0) && i < 7)
    {
        fillTransf(line, i); 
        i++;
    }

    close(fd);
}

void procfile(char* buffer, int server_client_fifo)
{
    char* args = buffer + 10; // removes proc-file and space

    printf("ARGS: %s\n", args);

    /*
    * PARSING ARGS 
    */

    char* token = strtok(args," ");

    char* input_files = malloc(sizeof(char) * 50);
    char* output_files = malloc(sizeof(char) * 50);

    printf("Going to copy\n");

    strcpy(input_files, token);
    token = strtok(NULL, " ");
    strcpy(output_files, token);
    printf("INPUT FILE: %s\nOUTPUT FILE: %s\n", input_files, output_files);
    token = strtok(NULL, " \0");
    char* transformations[MAX_TRANSF];
    int i = 0;

    while(token != NULL)
    {
        
        transformations[i] = malloc(sizeof(char) * strlen(token));
        strcpy(transformations[i], token);
        printf("commands[%d] = %s\n", i, transformations[i]);
        token = strtok(NULL, " \0");
        i++;  
    }
    transformations[i] = '\0';
    free(token);

    /*
    * APPLY TRANSFORMATIONS
    */

    for (int i = 0;i < MAX_TRANSF; ++i)
    {
        printf("transf %s: %d/%d (running/max)\n",transf[i].name,transf[i].running,transf[i].max);
        
    }

    transform(input_files, output_files, transformations); 
    
    removeRunningTransf(transformations); 

    for (int i = 0;i < MAX_TRANSF; ++i)
    {
        printf("transf %s: %d/%d (running/max)\n",transf[i].name,transf[i].running,transf[i].max);
        
    }

    for(int j = 0; j < i; j++){
        free(transformations[j]);
    }

    write(server_client_fifo,"concluded\n", 10);

}

void handler(char* pid_m, char* message)
{
    int child = fork();
 
    if(child == 0)
    {
        char* fifoToRead = malloc(sizeof(char)*30);
        char* fifoToWrite = malloc(sizeof(char)*30);
        
        strcpy(fifoToRead,"client_server_fifo_");
        strcat(fifoToRead, pid_m);
        
        strcpy(fifoToWrite,"server_client_fifo_");
        strcat(fifoToWrite, pid_m);

        printf("Proc Filho!\n");
        printf("FILE TO READ: %s\n",fifoToRead);

        int server_client_fifo = open(fifoToWrite, O_WRONLY);
        
        if(strncmp(message, "proc-file", 9) == 0)
        {
            procfile(message,server_client_fifo);
        }

        if(strncmp(message, "status", 6) == 0)
        {
            getStatus(server_client_fifo);
        }

        if(strncmp(message, "proc-file priority",18) == 0)
        {
            // DO ADVANCED THINGS
        }

        free(fifoToRead);
        free(fifoToWrite);
        close(server_client_fifo);

        _exit(0);
    }
}

void pendingTasks()
{
    int child = fork();

    if(child == 0)
    {
        struct process* temp = start; 
        while(1)
        {
            if(temp && temp->status == 1 && checkConfig(temp->transformations))
            {
                char* pid_m = malloc(sizeof(char)*10);
                sprintf(pid_m,"%d",start->pid);
                handler(pid_m, start->request);
            }
            /*if(temp->next == NULL)
                temp = start;
            else
                temp = temp->next;*/
        }

        _exit(0);
    }
}

void printProc(struct process* proc )
{
    printf("-------\n");
    printf("PID: %d\n",proc->pid);
        if(proc->message)
        {
            printf("Proc-File\n");
            for(int i = 0; proc->transformations[i];i++)
                printf("Transf: %s\n",proc->transformations[i]);
        }
    printf("-------\n");
}

int main(int argc, char* argv[])
{

    signal(SIGCHLD, sigchild_handler);
    /*
    * 
    * ./sdstored ../etc/sdstored.conf SDStore-transf/
    * 
    */

    if (argc != 3)
    {
        perror("Número de argumentos inválidos! Tente:\n./server config-filename transf-folder\n");
        return -1;
    }

    parseTransf(argv[1]);
    strcpy(transf_folder,argv[2]);

    mkfifo("connection_fifo", 0644);

    printf("FIFO connection_fifo CREATED\n\n");

    //pendingTasks();

    while(1)
    {
        printf("In Connection while\n");

        char* buffer = calloc(2048, sizeof(char));
        int connection_fifo = open("connection_fifo", O_RDONLY);

        if (connection_fifo == -1) 
            printf("ERRO\n");

        read(connection_fifo, buffer, 2048);
        close(connection_fifo);

        printf("Buffer: %s\n", buffer);

        char* message = strsep(&buffer, "_");

        char* pid_m = malloc(sizeof(char)*10);
        strcpy(pid_m,message);    
        
        int pid = atoi(pid_m);
        message = strsep(&buffer, "_");
        
        char * insertMessage = strdup(message);

        struct process* proc = insert_client(pid,insertMessage);
        printf("Client %s Connected!\n",pid_m);


        printProc(proc);
        if(proc->message == 0)
        {
            handler(pid_m,message);
        }
        else if(proc->message != 0 && checkConfig(proc->transformations) != 0)
        {
            addRunningTransf(proc->transformations); 
            handler(pid_m,message);
        }
        else
        {
            proc->status = 1;
            char * tempfifo = malloc(sizeof(char)*50);
            strcpy(tempfifo,"client_server_fifo_");
            strcat(tempfifo,pid);
            int connection_fifo = open(tempfifo, O_WRONLY);
            write(tempfifo,"Pending",7);
            close(tempfifo);
        }

        //printProcess();
        close(connection_fifo);
    }
}