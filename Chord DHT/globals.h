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

int connect_to_sever_by_ip(const char* ip, int port){
    while(1){
        std::cout << "Conectando a servidor de direccion " << ip << "..." << std::endl;

        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in address = {};
        address.sin_family = AF_INET;
        inet_pton(AF_INET, ip, &address.sin_addr);
        address.sin_port = htons(port);

        int error = connect(clientSocket, (struct sockaddr*)&address, sizeof(address));
        if(error!=0){
            std::cout << "ERROR AL INTENTAR CONECTARSE CON EL SERVIDOR!!!" << std::endl;
            continue;
        }else{
            std::cout << "Conectado!" << std::endl;
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