
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
    char* exe;
    int max;
}* Transformation;

struct transformation transf[7];

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


void fillTransf(char * line, int i){
    transf[i].name = strdup(strtok(line, " "));
    transf[i].exe = strdup(strtok(NULL, " "));
    transf[i].max = atoi(strtok(NULL, "\n"));
}

int parseTransf(char* filepath){

    int fd;
    if((fd = open(filepath, O_RDONLY, 0644)) == -1){
        perror("File doesn't exist!\n");
        exit(-1);
    }

    char* line = malloc(MAX_BUF);
    int i=0;

    while((myreadln(fd, line, 30) > 0) && i < 5){
        fillTransf(line, i); 
        i++;
    }

    close(fd);
}


int main(int argc, char* argv[]){
    if (argc != 3){
        perror("Número de argumentos inválidos! Tente:\n./server config-filename filters-folder\n");
        return -1;
    }

    parseTransf(argv[1]);
}