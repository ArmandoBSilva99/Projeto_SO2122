#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char* argv[]){

    int client_server_fifo = open("server_client_fifo", O_WRONLY);

    //Write commands to fifo
    printf("Going to write: %s to server fifo\n", argv + 1);
    write(client_server_fifo, argv + 1, strlen(argv + 1));
}