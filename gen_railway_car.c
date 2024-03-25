#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int generateRandomPart() {
    return rand() % 25 + 1; 
}

int main(){
    FILE *file = fopen("railway_car.txt", "w");
    if(!file){
        perror("Failed to open file");
        exit;
    }

    srand(time(NULL));
    for(int i = 0; i < 500; i++){
        int part = generateRandomPart();
        fprintf(file,"%d\n", part);
    }
    
    fclose(file);
    return 0; 
}