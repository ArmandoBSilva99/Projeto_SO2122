#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MESSAGE_SIZE 50

int main(int argc, char* argv[])
{

    /*
    * 
    * ./sdstore proc-file input-filename output-filename
    * 
    */


    char args[MESSAGE_SIZE];
    strcpy(args, argv[1]);

    for(int i = 2; i < argc; i++)
    {
        char* arg = malloc(sizeof(char) * (strlen(argv[i]) + 1));
        sprintf(arg, " %s", argv[i]);
        strcat(args, arg);
    }

    //printf("server_client_fifo CREATED");
    //mkfifo("server_client_fifo", 0644);
    int pid_int = getpid();
    char* pid_string = malloc(sizeof(char)*10);
    sprintf(pid_string,"%d",pid_int);


    //CREATE FIFOS
    char* fifoToRead = malloc(sizeof(char)*30);
    char* fifoToWrite = malloc(sizeof(char)*30);
        
    strcpy(fifoToRead,"server_client_fifo_");
    strcat(fifoToRead, pid_string);
        
    strcpy(fifoToWrite,"client_server_fifo_");
    strcat(fifoToWrite, pid_string);

    mkfifo(fifoToWrite, 0644);
    mkfifo(fifoToRead, 0644);
    
    //CONNECT TO SERVER
    int connection_fifo = open("connection_fifo", O_WRONLY);
    write(connection_fifo, pid_string, strlen(args));
    close(connection_fifo);

    //OPEN FIFO TO WRITE
    int client_server_fifo = open(fifoToWrite, O_WRONLY);
    if (client_server_fifo == -1) 
        printf("ERRO\n");

    //WRITE COMMANDS TO FIFO
    printf("Going to write: \"%s\" to server fifo\n", args);
    write(client_server_fifo, args, strlen(args));
    close(client_server_fifo);

    //OPEN FIFO TO READ
    int server_client_fifo = open(fifoToRead, O_RDONLY);
    if (server_client_fifo == -1) 
       printf("ERRO\n");

    printf("Vai ler!\n");
        
    char buffer[2048];
    read(server_client_fifo, buffer, 2048);
    close(server_client_fifo);

    printf("%s\n",buffer);

    unlink(fifoToRead);
    unlink(fifoToWrite);
}