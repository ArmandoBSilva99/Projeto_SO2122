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

void exec_commands(char* transformation){

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

void transform(char* input_file, char* output_file, char** transformations){

    int fd_in = open(input_file, O_RDONLY, 0644);
    int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    int num_transformations;
    for(num_transformations = 0; transformations[num_transformations] != NULL; num_transformations++);

    int p[num_transformations - 1][2];

    int status[num_transformations];

    if(num_transformations > 1) {
        for(int i = 0; i < num_transformations; i++){

            //First Transformation
            if (i == 0){
                pipe(p[i]);
                if (fork()){
                    dup2(fd_in, 0);
                    close(fd_in);

                    close(p[i][0]);
                    
                    dup2(p[i][1], 1);
                    close(p[i][1]);
                    
                    exec_commands(transformations[i]);

                    _exit(0);
                }
                else {
                    close(p[i][1]);                
                }
            }

            //Last Transformation
            else if (i == num_transformations - 1){
                if (fork()){

                    dup2(fd_out, 1);
                    close(fd_out);

                    dup2(p[i-1][0],0);
                    close(p[i-1][0]);

                    exec_commands(transformations[i]);
                    
                    _exit(0);

                }
                else {
                    close(p[i-1][0]);
                }
            }

            //In between Transformations 
            else {
                pipe(p[i]);
                if (fork()){
                    close(p[i][0]);

                    dup2(p[i][1], 1);
                    close(p[i][1]);

                    dup2(p[i-1][0], 0);
                    close(p[i-1][0]);

                    exec_commands(transformations[i]);

                    _exit(0);
                }
                else {
                    close(p[i][1]);
                    close(p[i-1][0]);
                }
            }
        }

        for(int w = 0; w < num_transformations; w++)
            wait(&status[w]);
    }
    else {
        if(fork()){
            dup2(fd_in, 0);
            dup2(fd_out, 1);
            close(fd_in);
            close(fd_out);

            exec_commands(transformations[0]);

            _exit(0);
        }
        else {
            wait(&status[0]);
        }
    }        

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
            char* args = buffer + 10; // removes proc-file and space

            /*
            * PARSING ARGS 
            */
            char* token = strtok(args," ");
            char* input_files = malloc(sizeof(char*));
            char* output_files = malloc(sizeof(char*));
            strcpy(input_files, token);
            token = strtok(NULL, " ");
            strcpy(output_files, token);
            printf("INPUT FILE: %s\nOUTPUT FILE: %s\n", input_files, output_files);
            token = strtok(NULL, " ");
            char* transformations[7]; // 7 max commands??
            int i = 0;
            while(token != NULL){
                transformations[i] = strdup(token);
                printf("commands[%d] = %s\n", i, transformations[i]);
                token = strtok(NULL, " ");
                i++;  
            }
            transformations[i] = '\0';

            /*
            * APPLY TRANSFORMATIONS
            */

            transform(input_files, output_files, transformations);
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