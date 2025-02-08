#include <stdio.h>
#include "threadsafe_queue.h"
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <iostream>
#include <arpa/inet.h>

#pragma once

#define CLIENT_DHT_DISCOVER_PORT 8080
#define DHT_COMMAND_PORT 8080

int connect_to_sever_by_ip(in_addr ip, int port){
    while(1){
        struct sockaddr_in address = {};
        address.sin_family = AF_INET;
        address.sin_addr = ip;
        address.sin_port = htons(port);

        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        int error = connect(clientSocket, (struct sockaddr*)&address, sizeof(address));

        if(error!=0){
            std::cout << "ERROR AL INTENTAR CONECTARSE CON EL SERVIDOR!!!" << std::endl;
            continue;
        }else{
            return clientSocket;
        }
    }
}

int recv_to_fill(int sock, char* data, int dLen){
    int last_received = 0;
    int received = 0;
    while(received<dLen){
        last_received = recv(sock, data+received, dLen-received, 0);
        if (last_received!=-1){
            received += last_received;
        }else{
            return -1;
        }
    }
    return 0;
}

int send_all(int sock, char* data, int dLen){
    int last_sended = 0;
    int sended = 0;
    while(sended<dLen){
        last_sended = send(sock, data+sended, dLen-sended, 0);
        if (last_sended!=-1){
            sended += last_sended;
        }else{
            return -1;
        }
    }
    return 0;
}

int send_length_then_message(int sock, int size, char* message){
    send_all(sock, (char*)&size, sizeof(int));
    send_all(sock, message, size*sizeof(char));
    return 0;
}

int recv_length_then_message(int sock, int* size, char** message){
    //First receive size
    recv_to_fill(sock, (char*)size, sizeof(int));
    
    //Then send the actual message
    *message = (char*) malloc(*size*sizeof(char));
    recv_to_fill(sock, *message, *size);
    return 0;
}