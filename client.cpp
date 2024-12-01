#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include "globals.h"
#include <stdio.h>
#include <arpa/inet.h>

using std::cout;
using std::endl;

int connect_to_sever_by_name(const char* name, const char* port){
    
    while(1){
        int error = 0;
        std::cout << "Conectando a servidor " << name << "..." << std::endl;

        // Create Socket
        addrinfo hints;
        addrinfo *servinfo;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
        hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
        hints.ai_flags = 0;     // fill in my IP for me
        error = getaddrinfo(name, port, &hints, &servinfo);
        if(error!=0){
            std::cout << "ERROR OBTENIENDO LA DIRECCION DEL SERVIDOR!!! Codigo de error: " <<  error << std::endl;
            continue;
        }
        
        int clientSocket = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

        // Connect
        error = connect(clientSocket, servinfo->ai_addr, servinfo->ai_addrlen);
        if(error!=0){
            std::cout << "ERROR AL INTENTAR CONECTARSE CON EL SERVIDOR!!!" << std::endl;
            continue;
        }else{
            std::cout << "Conectado!" << std::endl;
            return clientSocket;
        }
    }
}

int connect_to_sever_by_ip(const char* ip, int port){
    while(1){
        std::cout << "Conectando a servidor de direccion" << ip << "..." << std::endl;

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

int main()
{
    // Create socket
    int clientSocket = connect_to_sever_by_ip("10.0.11.2", 8080);

    // Main Loop
    const char* debug_commands[] = {"add [b.cpp] [lolazo, lolencio]", 
                                    "add [a.txt] [lolazo, pepe]", 
                                    "delete-tags [lolazo] [lolencio]",
                                    "list [lolazo]"};
    //int debug_command_count = sizeof(debug_commands)/sizeof(char*);
    int debug_command_count = 0;
    int debug_command_sent_count = 0;
    while(1){
        // Get command
        char message[MESSAGE_MAX_SIZE] = { 0 };

        if(debug_command_sent_count < debug_command_count){
            strcpy(message, debug_commands[debug_command_sent_count]);
            cout << "Debug Command: " << debug_commands[debug_command_sent_count] << endl;
            debug_command_sent_count += 1;
        }else{
            std::cout << "Introduzca un comando: ";
            std::cin.getline(message, MESSAGE_MAX_SIZE);
        }
        if(strlen(message) == 0) continue;

        // Tokenize command
        char** tokenized_message = tokenize_command(message);
        if(!is_message_correct(tokenized_message)){
            free_token_list(tokenized_message);
            std::cout << "COMANDO INCORECTO!!!" << std::endl;
            continue;
        }
        
        // Exit if requested
        if(strcmp(tokenized_message[0], "exit") == 0){
            std::cout << "Cerrando cliente..." << std::endl;
            break;
        }

        // If its an 'add' command, open the files, or inform an error opening them
        int file_count = 0;
        FILE** file_descriptors = NULL;
        if(strcmp(tokenized_message[0], "add") == 0){
            //Get file count and alloc memory for the descriptors
            int i = 2;
            while(true){
                if(strcmp(tokenized_message[i], "]") == 0){
                    break;
                }

                file_count += 1;
                i += 1;
            }
            file_descriptors = (FILE**)malloc(file_count*sizeof(FILE*));
            memset(file_descriptors, 0, file_count*sizeof(FILE*));

            //Open files
            i = 2;
            bool error = false;
            while(true){
                if(strcmp(tokenized_message[i], "]") == 0){
                    break;
                }

                FILE* file_stream = fopen(tokenized_message[i], "rb");
                if(file_stream == NULL){
                    std::cout << "ERROR ABRIENDO ARCHIVO \"" << tokenized_message[i] << "\"" << std::endl;
                    error = true;
                    break;
                }
                file_descriptors[i-2] = file_stream;
                i += 1;
            }

            if(error){
                list_fclose(file_descriptors, file_count);
                free_list_and_members((void**)file_descriptors, file_count);
                free_token_list(tokenized_message);
                continue;
            }

        }
        
        // Send command
        send_all(clientSocket, message, MESSAGE_MAX_SIZE);
        
        // Send more info if needed (for 'add' command)
        if(strcmp(tokenized_message[0], "add") == 0){
            for(int i = 0; i<file_count; i++){
                
                //First send size
                int file_size = get_file_size(file_descriptors[i]);
                int* file_size_pointer = &file_size;
                send_all(clientSocket, (char*)file_size_pointer, sizeof(int));

                //Then send the actual file
                char* file_contents = (char*) malloc(file_size*sizeof(char));
                fread(file_contents, file_size, 1, file_descriptors[i]);
                
                send_all(clientSocket, file_contents, file_size);
                free(file_contents);
            }
            

            list_fclose(file_descriptors, file_count);
            free(file_descriptors);
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
