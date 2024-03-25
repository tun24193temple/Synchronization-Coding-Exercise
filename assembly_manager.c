#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BLUE_MAX 15
#define RED_MAX 10

//make global shared vars
int sequence = 0;
pthread_mutex_t sequence_lock;
int fd_railway_car;
int fd_blue_truck;
int fd_red_truck;


//part makeup
typedef struct{
    int sequence_num;
    int part_num;
}Part;

//conveyorbelt makeup
typedef struct{
    Part *buffer;
    int size;
    int count;
    int in;
    int out;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
}ConveyorBelt;

ConveyorBelt blue_cb, red_cb;

void _init_conveyorbelt(ConveyorBelt *cb, int size, const char *name){
    cb->buffer = malloc(size*sizeof(Part));
    cb->size = size;
    cb->count = 0;
    cb->in = 0;
    cb->out = 0;
    pthread_mutex_init(&cb->mutex, NULL);
    pthread_cond_init(&cb->not_empty, NULL);
    pthread_cond_init(&cb->not_full, NULL); 
}

void shutdownConveyorBelt(ConveyorBelt *belt){
    free(belt->buffer);
    pthread_mutex_destroy(&belt->mutex);
    pthread_cond_destroy(&belt->not_empty);
    pthread_cond_destroy(&belt->not_full);
    pthread_mutex_destroy(&sequence_lock);
}

void put(ConveyorBelt *cb, Part part){
    //lock the conveyor belt to prevent race conditions
    pthread_mutex_lock(&cb->mutex);

    //wait for space on cb to free up
    while (cb->count == cb->size) {
        pthread_cond_wait(&cb->not_full, &cb->mutex);
    }

    //add the part to cb
    cb->buffer[cb->in] = part;
    cb->in = (cb->in + 1) % cb->size;
    cb->count++;

    //sginal cb has contents
    pthread_cond_signal(&cb->not_empty);

    //unlock
    pthread_mutex_unlock(&cb->mutex);
}

Part get(ConveyorBelt *cb){
    pthread_mutex_lock(&cb->mutex);

    while (cb->count == 0) {
        //wait until cb has parts
        pthread_cond_wait(&cb->not_empty, &cb->mutex);
    }

    //get the next part from the conveyor belt(fifo queue) 
    Part part = cb->buffer[cb->out];
    cb->out = (cb->out + 1) % cb->size; //update queue indecies
    cb->count--;

    //signal cb is not full
    pthread_cond_signal(&cb->not_full);

    pthread_mutex_unlock(&cb->mutex);

    return part;
}

void print_cb(ConveyorBelt *cb, const char *belt_name){
    pthread_mutex_lock(&cb->mutex);

    printf("Conveyor Belt [%s]:\n", belt_name);
    printf("Size: %d, Count: %d, In: %d, Out: %d\n", cb->size, cb->count, cb->in, cb->out);
    printf("Contents:\n");

    for(int i = 0; i < cb->count; i++){
        int index = (cb->out + i) % cb->size;
        printf("{Seq: %d, Part: %d} ", cb->buffer[index].sequence_num, cb->buffer[index].part_num);
    }

    printf("\n\n");

    pthread_mutex_unlock(&cb->mutex);
}


void *threadL(void *arg){
    char buffer[10]; //buffer for numbers 1 to 25 and newline
    int str_len = 0;//current length of str in the buffer and used as index for placing ch in buffer
    char ch; //char read from file
    int part_number; //store int val of ch

    while(1){
        int parts_count = 0; //counter for amount of part processed
        while(parts_count <= 25){
            //read one char from fd
            ssize_t read_bytes = read(fd_railway_car, &ch, 1);
            if(read_bytes <= 0) {
                //if read error or EOF
                if(read_bytes == 0){
                    Part eof_marker = {0, 0};
                    put(&blue_cb, eof_marker);
                    put(&red_cb, eof_marker);
                    return NULL;
                }else{
                    perror("Read error in threadL");
                    return NULL;
                }
            }
            //if ch is not a new line add ch to buffer
            if(ch != '\n'){
                buffer[str_len++] = ch;
                continue;
            }

            buffer[str_len] = '\0'; //null-terminate the string
            part_number = atoi(buffer);//get part_number
            str_len = 0; //reset str index

            pthread_mutex_lock(&sequence_lock);
            Part part = {sequence++, part_number};//sequence num is the previous sequence num + 1
            pthread_mutex_unlock(&sequence_lock);

            //if in group A
            if(part_number <= 12){
                put(&blue_cb, part);
                print_cb(&blue_cb, "BLUE"); 
            //if in group B
            }else if(part_number >= 13){
                put(&red_cb, part);
                print_cb(&blue_cb, "BLUE"); 
            }
            parts_count++;
        }
        usleep(250000); //0.25 seconds
    }
}



void *threadR(void *arg){
    char buffer[10]; //buffer for numbers 1 to 25 and newline
    int str_len = 0;//current length of str in the buffer and used as index for placing ch in buffer
    char ch; //char read from file
    int part_number; //store int val of ch

    while(1){
        int parts_count = 0;
        while(parts_count <= 15){
            //read one char from fd
            ssize_t read_bytes = read(fd_railway_car, &ch, 1);
            if(read_bytes <= 0){
                //if read error or EOF
                if(read_bytes == 0){
                    Part eof_marker = {0, 0};
                    put(&blue_cb, eof_marker);
                    put(&red_cb, eof_marker);
                    return NULL;
                }else{
                    perror("Read error in threadL");
                    return NULL;
                }
            }
            //if ch is not a new line add ch to buffer and restart loop
            if(ch != '\n'){
                buffer[str_len++] = ch;
                continue;
            }

            buffer[str_len] = '\0'; //null-terminate the string
            part_number = atoi(buffer);//get part_number
            str_len = 0; //reset str index

            pthread_mutex_lock(&sequence_lock);
            Part part = {sequence++, part_number};//sequence num is the previous sequence num + 1
            pthread_mutex_unlock(&sequence_lock);

            //if in group A
            if(part_number <= 12){
                put(&blue_cb, part);
                print_cb(&blue_cb, "BLUE"); 

            //if in group B
            }else if(part_number >= 13){
                put(&red_cb, part);
                print_cb(&blue_cb, "BLUE"); 
            }
            parts_count++;
        }
        usleep(500000); //0.5 seconds
    }
}


void *threadX(void *arg){
    char buffer[30];
    while (1) {
        Part part = get(&blue_cb); //get part from red cb
        print_cb(&blue_cb, "BLUE");

        if (part.part_num == 0) { //check for EOF marker
            break;
        }

        int size = snprintf(buffer, sizeof(buffer), "{%d, %d}\n", part.sequence_num, part.part_num);
        write(fd_blue_truck, buffer, size);
    }
    return NULL;
}

void *threadY(void *arg){
    char buffer[30];
    while (1) {
        Part part = get(&red_cb); //get part from red cb
        print_cb(&red_cb, "RED"); 

        if (part.part_num == 0) { //check for EOF marker
            break;
        }

        int size = snprintf(buffer, sizeof(buffer), "{%d, %d}\n", part.sequence_num, part.part_num);
        write(fd_red_truck, buffer, size);
    }   
    return NULL;
}

int main(int argc, char *argv[]){
    fd_railway_car = atoi(argv[1]);
    fd_blue_truck = atoi(argv[2]);
    fd_red_truck = atoi(argv[3]);

    //initialize queues and locks
    _init_conveyorbelt(&blue_cb, BLUE_MAX, "blue_cb");
    _init_conveyorbelt(&red_cb, RED_MAX,"red_cb");
    pthread_mutex_init(&sequence_lock, NULL);

    //create threads
    pthread_t threadl, threadr, threadx, thready;
    pthread_create(&threadl, NULL, threadL, NULL);
    pthread_create(&threadr, NULL, threadR, NULL);
    pthread_create(&threadx, NULL, threadX, NULL);
    pthread_create(&thready, NULL, threadY, NULL);

    //join threads
    pthread_join(threadl, NULL);
    pthread_join(threadr, NULL);
    pthread_join(threadx, NULL);
    pthread_join(thready, NULL);

    //cleanup
    shutdownConveyorBelt(&blue_cb);
    shutdownConveyorBelt(&red_cb);
    pthread_mutex_destroy(&sequence_lock);
    close(fd_railway_car);
    close(fd_blue_truck);
    close(fd_red_truck);

    return 0;
}