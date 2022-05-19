#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX_BUF 1024

struct transformation{
    char* name;
    int max;
}* Transformation;

struct transformation transf[7];
char filters_folder[30];

ssize_t myreadln(int fildes, void* buf, size_t nbyte){
    ssize_t size = 0;
    char c;
    char* buff = (char*)buf;
    while (size < nbyte && read(fildes, &c, 1) > 0) {
        if (c == '\0')
            return size;
        buff[size++] = c;
        if (c == '\n')
            return size;
    }
    return size;
}

void exec_filtros(char* filtro){

    char nop[100];
    sprintf(nop,"%s/nop",filters_folder);

    char bcompress[100]; 
    sprintf(bcompress,"%s/bcompress",filters_folder);

    
    char bdecompress[100]; 
    sprintf(bdecompress,"%s/bdecompress",filters_folder);

    char gcompress[100]; 
    sprintf(gcompress,"%s/gcompress",filters_folder);
    
    char gdecompress[100]; 
    sprintf(gdecompress,"%s/gdecompress",filters_folder);

    char encrypt[100]; 
    sprintf(encrypt,"%s/encrypt",filters_folder);

    char decrypt[100]; 
    sprintf(decrypt,"%s/decrypt",filters_folder);

    if (strcmp(filtro,"nop") == 0)        
        execl(nop,"nop", NULL);
    
    else if (strcmp(filtro,"bcompress") == 0)
        execl(bcompress,"bcompress", NULL);
    
    else if (strcmp(filtro,"bdecompress") == 0)
        execl(bdecompress,"bdecompress", NULL);
    
    else if (strcmp(filtro,"gcompress") == 0)
        execl(gcompress,"gcompress", NULL);
    
    else if (strcmp(filtro,"gdecompress") == 0)
        execl(gdecompress,"gdecompress", NULL);
    
    else if (strcmp(filtro,"encrypt") == 0)
        execl(encrypt,"encrypt", NULL);
    
    else if (strcmp(filtro,"decrypt") == 0)
        execl(decrypt,"decrypt", NULL);    
}


void fillTransf(char * line, int i){
    transf[i].name = strdup(strtok(line, " "));
    transf[i].max = atoi(strtok(NULL, "\n"));
}

void parseTransf(char* filepath){

    int fd;
    if((fd = open(filepath, O_RDONLY, 0644)) == -1){
        perror("File doesn't exist!\n");
        exit(-1);
    }
    printf("heyo\n");
    char* line = malloc(sizeof(char) * MAX_BUF);
    int i=0;

    while((myreadln(fd, line, 30) > 0) && i < 7){
        fillTransf(line, i); 
        i++;
    }

    close(fd);
}


int main(int argc, char* argv[]){

    /*
    * 
    * ./sdstored ../etc/sdstored.conf SDStore-transf/
    * 
    */


    if (argc != 3){
        perror("Número de argumentos inválidos! Tente:\n./server config-filename filters-folder\n");
        return -1;
    }

    parseTransf(argv[1]);
    strcpy(filters_folder,argv[2]);

    mkfifo("client_server_fifo", 0644);

    while(1){

        printf("In while\n");

        char buffer[2048];
        int client_server_fifo = open("client_server_fifo", O_RDONLY);
        if (client_server_fifo == -1) printf("ERRO\n");
        read(client_server_fifo, buffer, 2048);
        
        printf("INSIDE FIFO: %s\n", buffer);

        if(strncmp(buffer, "proc-file", 9) == 0){
            // DO THINGS
        }
        if(strncmp(buffer, "status", 6) == 0){
            // DO THINGS
        }
        if(strncmp(buffer, "proc-file priority",18) == 0){
            // DO ADVANCED THINGS
        }

        close(client_server_fifo);
    }
}