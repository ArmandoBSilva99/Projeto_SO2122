#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MESSAGE_SIZE 50

int main(int argc, char* argv[]){

    /*
    * 
    * ./sdstore proc-file input-filename output-filename
    * 
    */


    char args[MESSAGE_SIZE];
    strcpy(args, argv[1]);

    for(int i = 2; i < argc; i++){
        char* arg = malloc(sizeof(char*));
        sprintf(arg, " %s", argv[i]);
        strcat(args, arg);
    }


    int client_server_fifo = open("client_server_fifo", O_WRONLY);
    if (client_server_fifo == -1) printf("ERRO\n");

    //Write commands to fifo
    printf("Going to write: \"%s\" to server fifo\n", args);
    write(client_server_fifo, args, strlen(args));

    close(client_server_fifo);
}