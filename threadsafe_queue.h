#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#pragma once

class Command{
public:
    char* message; //NOT zero terminated
    int size;
    int sock;
};

class Threadsafe_Command_Queue{
public:
    pthread_mutex_t mutex;
    Command* array;
    int last_free_space;
    int first_ocupied_space;
    int total_size;

    Threadsafe_Command_Queue(int size){
        array = (Command*)malloc(size*sizeof(Command));
        pthread_mutex_init(&mutex, NULL);
        last_free_space = 0;
        first_ocupied_space = 0;
        total_size = size;
    }

    void enqueue(Command cmd){
        pthread_mutex_lock(&mutex);
        array[last_free_space] = cmd;
        last_free_space = (last_free_space + 1)%total_size;
        pthread_mutex_unlock(&mutex);
    }

    Command dequeue(){
        Command cmd;
        while(true){
            pthread_mutex_lock(&mutex);
            if(last_free_space != first_ocupied_space){
                cmd = array[first_ocupied_space];
                first_ocupied_space = (first_ocupied_space + 1)%total_size;
                pthread_mutex_unlock(&mutex);
                break;
            }else{
                usleep(100);
            }
            pthread_mutex_unlock(&mutex);
        }
        return cmd;
    }

};