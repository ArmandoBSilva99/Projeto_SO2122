#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX_BUF 1024
#define MAX_TRANSF 7

struct transformation
{
    char* name;
    int running;
    int max;
}* Transformation;

struct transformation transf[7];
char filters_folder[30];

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
    sprintf(nop,"%snop",filters_folder);

    char bcompress[100]; 
    sprintf(bcompress,"%sbcompress",filters_folder);

    
    char bdecompress[100]; 
    sprintf(bdecompress,"%sbdecompress",filters_folder);

    char gcompress[100]; 
    sprintf(gcompress,"%sgcompress",filters_folder);
    
    char gdecompress[100]; 
    sprintf(gdecompress,"%sgdecompress",filters_folder);

    char encrypt[100]; 
    sprintf(encrypt,"%sencrypt",filters_folder);

    char decrypt[100]; 
    sprintf(decrypt,"%sdecrypt",filters_folder);

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

//TODO Esta funcao está uma bosta, temos de a mudar
void transform(char* input_file, char* output_file, char** transformations)
{
    int fd_in = open(input_file, O_RDONLY, 0644);
    int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    
    int num_transformations;
    for(num_transformations = 0; transformations[num_transformations]; num_transformations++);

    printf("Num_T: %d\n", num_transformations);

    int p[num_transformations - 1][2];

    int status[num_transformations];

    if(num_transformations > 1) 
    {
        for(int i = 0; i < num_transformations; i++)
        {
            //First Transformation
            if (i == 0)
            {
                pipe(p[i]);
                if (fork())
                {
                    dup2(fd_in, 0);
                    close(fd_in);

                    close(p[i][0]);
                    
                    dup2(p[i][1], 1);
                    close(p[i][1]);
                    
                    exec_commands(transformations[i]);

                    _exit(0);
                }
                else 
                {
                    close(p[i][1]);                
                }
            }

            //Last Transformation
            else if (i == num_transformations - 1)
            {               
                if (fork())
                {
                    dup2(fd_out, 1);
                    close(fd_out);

                    dup2(p[i-1][0],0);
                    close(p[i-1][0]);

                    exec_commands(transformations[i]);
                    
                    _exit(0);

                }
                else 
                {
                    close(p[i-1][0]);
                }
            }

            //In between Transformations 
            else 
            {
                pipe(p[i]);
                if (fork())
                {
                    close(p[i][0]);

                    dup2(p[i][1], 1);
                    close(p[i][1]);

                    dup2(p[i-1][0], 0);
                    close(p[i-1][0]);

                    exec_commands(transformations[i]);

                    _exit(0);
                }
                else 
                {
                    close(p[i][1]);
                    close(p[i-1][0]);
                }
            }
        }

        for(int w = 0; w < num_transformations; w++)
            wait(&status[w]);
    }
    else 
    {
        if(fork() == 0)
        {
            dup2(fd_in, 0);
            dup2(fd_out, 1);
            close(fd_in);
            close(fd_out);
            exec_commands(transformations[0]);

            _exit(0);
        }
        else 
        {
            wait(&status[0]);
        }
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
    char* transformations[7]; // 7 max commands??
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

    transform(input_files, output_files, transformations);
    write(server_client_fifo,"concluded\n", 10);
}

void handler(char* buffer)
{
    int child = fork();

    if(child == 0)
    {
        char* fifoToRead = malloc(sizeof(char)*30);
        char* fifoToWrite = malloc(sizeof(char)*30);
        
        strcpy(fifoToRead,"client_server_fifo_");
        strcat(fifoToRead, buffer);
        
        strcpy(fifoToWrite,"server_client_fifo_");
        strcat(fifoToWrite, buffer);

        printf("Proc Filho!\n");
        printf("FILE TO READ: %s\n",fifoToRead);
        int client_server_fifo = open(fifoToRead, O_RDONLY);

        char buffer[2048];
        read(client_server_fifo, buffer, 2048);
        close(client_server_fifo);

        int server_client_fifo = open(fifoToWrite, O_WRONLY);
        
        if(strncmp(buffer, "proc-file", 9) == 0)
        {
            procfile(buffer,server_client_fifo);
        }

        if(strncmp(buffer, "status", 6) == 0)
        {
            getStatus(server_client_fifo);
        }

        if(strncmp(buffer, "proc-file priority",18) == 0)
        {
            // DO ADVANCED THINGS
        }


        close(server_client_fifo);

        _exit(0);
    }
}

int main(int argc, char* argv[])
{

    /*
    * 
    * ./sdstored ../etc/sdstored.conf SDStore-transf/
    * 
    */

    if (argc != 3)
    {
        perror("Número de argumentos inválidos! Tente:\n./server config-filename filters-folder\n");
        return -1;
    }

    parseTransf(argv[1]);
    strcpy(filters_folder,argv[2]);

    mkfifo("connection_fifo", 0644);

    printf("FIFO connection_fifo CREATED\n\n");

    while(1)
    {
        printf("In Connection while\n");

        char* buffer = calloc(2048, sizeof(char));
        int connection_fifo = open("connection_fifo", O_RDONLY);

        if (connection_fifo == -1) 
            printf("ERRO\n");

        read(connection_fifo, buffer, 2048);
        close(connection_fifo);

        printf("Client %s Connected!\n",buffer);

        handler(buffer);
        close(connection_fifo);
    }
}