#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BLUE "BLUE_TRUCK.txt"
#define RED "RED_TRUCK.txt"
#define RAILWAY_CAR "railway_car.txt"

int main(){
    //open railway car data file.
    FILE *railway_file = fopen(RAILWAY_CAR, "r");
    if(!railway_file){
        perror("Failed to open railway car data file");
        return 1;
    }

    //create truck output files.
    FILE *blue_file = fopen(BLUE, "w");
    FILE *red_file = fopen(RED, "w");
    if(!blue_file || !red_file){
        perror("Failed to create truck output files");
        fclose(railway_file);
        return 1;
    }

    //get file descriptors from file streams
    int fd_railwaycar = fileno(railway_file);
    int fd_blue = fileno(blue_file);
    int fd_red = fileno(red_file);

    //create child process.
    pid_t pid = fork();
    if(pid == 0){
        //file descriptors
        char fd_railwaycar_str[500], fd_blue_str[500], fd_red_str[500];
        //add contents of txt files to file descriptors
        sprintf(fd_railwaycar_str, "%d", fd_railwaycar);
        sprintf(fd_blue_str, "%d", fd_blue);
        sprintf(fd_red_str, "%d", fd_red);
        execl("./assembly_manager", "assembly_manager", fd_railwaycar_str, fd_blue_str, fd_red_str, NULL);
        
    }else if(pid < 0){
        perror("Failed to fork child process");
    }else{
        wait(NULL);  //wait for child
    }

    fclose(railway_file);
    fclose(blue_file);
    fclose(red_file);

    return 0;
}
