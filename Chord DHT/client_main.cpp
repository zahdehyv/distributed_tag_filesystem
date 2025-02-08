#include <stdio.h>
#include "threadsafe_queue.h"
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "globals.h"
#include <vector>
#include <iostream>

using std::cout;
using std::endl;

// Discovered DHT list
std::vector<char*> dht_dirs;
pthread_mutex_t dht_dirs_mutex;

//Returns the message (+ zero at the end)
int recv_length_then_message(int socket, int* length, char** message){
    
    // Receive Size
    recv_to_fill(socket, (char*)length, sizeof(int));

    // Receive Contents
    *message = (char*) malloc((*length+1)*sizeof(char));
    recv_to_fill(socket, *message, *length);
    message[*length] = 0;
    
    // Return
    return 0;
}

void* add_dht_node(void* args){
    int client_socket = *(int*)args;


    return NULL;
}

int main(){
    //Mutex
    pthread_mutex_init(&dht_dirs_mutex, NULL);
    
    //Create Server
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(CLIENT_DHT_DISCOVER_PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    while(1){
        // Connect
        int clientSocket = 0;
        listen(serverSocket, 1);
        
        sockaddr ip;
        socklen_t l;
        clientSocket = accept(serverSocket, &ip, &l);
        sockaddr_in* ipv4Addr = (sockaddr_in*) &ip;
        printf("%s\n", inet_ntoa(ipv4Addr->sin_addr));

        pthread_mutex_lock(&dht_dirs_mutex);
        dht_dirs.push_back(inet_ntoa(ipv4Addr->sin_addr));
        pthread_mutex_unlock(&dht_dirs_mutex);

        // Start Connection Thread
        //pthread_t thread_id;
        //pthread_create(&thread_id, NULL, add_dht_node, (void*)&clientSocket);
    }
    
}