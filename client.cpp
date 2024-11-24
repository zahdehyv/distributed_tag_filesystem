// C++ program to illustrate the client application in the
// socket programming
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include "globals.h"

int connect_to_sever_by_name(const char* name, const char* port){
    int error = 0;
    std::cout << "Conectando a servidor " << name << "..." << std::endl;

    // creating socket
    addrinfo hints;
    addrinfo *servinfo;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = 0;     // fill in my IP for me
    error = getaddrinfo(name, port, &hints, &servinfo);
    if(error!=0){
        std::cout << "ERROR OBTENIENDO LA DIRECCION DEL SERVIDOR!!! Codigo de error: " <<  error << std::endl;
        exit(-1);
    }
    
    int clientSocket = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    // connect!
    
    error = connect(clientSocket, servinfo->ai_addr, servinfo->ai_addrlen);
    if(error!=0){
        std::cout << "ERROR AL INTENTAR CONECTARSE CON EL SERVIDOR!!!" << std::endl;
        exit(-1);
    }

    std::cout << "Conectado!" << std::endl;
    return clientSocket;
}

int main()
{
    // Create socket
    int clientSocket = connect_to_sever_by_name("filesystem_server", "8080");

    // Main Loop
    while(1){
        // Get command
        std::cout << "Introduzca un comando: ";
        char message[MESSAGE_MAX_SIZE] = { 0 };
        std::cin.getline(message, MESSAGE_MAX_SIZE);
        if(strlen(message) == 0) continue;

        // Tokenize command
        char** tokenized_message = tokenize_command(message);
        if(!is_message_correct(tokenized_message)){
            free_token_list(tokenized_message);
            std::cout << "COMANDO INCORECTO!!!" << std::endl;
            continue;
        }
        
        // Exit if requested
        if(tokenized_message[0] != NULL && strcmp(tokenized_message[0], "exit") == 0){
            std::cout << "Cerrando cliente..." << std::endl;
            break;
        }
        
        // Send command
        send(clientSocket, message, strlen(message), 0);

        // Send more info if needed
        if(tokenized_message[0] != NULL && strcmp(tokenized_message[0], "add") == 0){
            //TODO
        }
        free_token_list(tokenized_message);

        // Receive response
        char response_message[MESSAGE_MAX_SIZE] = { 0 };
        recv(clientSocket, response_message, sizeof(response_message), 0);
        std::cout << response_message << std::endl; 
    }

    // Close the socket
    close(clientSocket);

    return 0;
}
