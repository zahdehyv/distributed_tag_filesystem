#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include "globals.h"
#include <stdio.h>
#include <arpa/inet.h>

using std::cout;
using std::endl;

int try_connect_to_sever_by_ip(const char* ip, int port){
    std::cout << "Conectando a servidor de direccion " << ip << "..." << std::endl;

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in address = {};
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);
    
    int val = 1;
    setsockopt(clientSocket, IPPROTO_TCP, TCP_SYNCNT, &val, sizeof(int));

    int error = connect(clientSocket, (struct sockaddr*)&address, sizeof(address));
    if(error!=0){
        std::cout << "ERROR AL INTENTAR CONECTARSE CON EL SERVIDOR!!!" << std::endl;
        return -1;
    }else{
        std::cout << "Conectado!" << std::endl;
        return clientSocket;
    }
}



std::vector<const char*> server_ips = {"10.0.11.2", "10.0.11.231"};
int main(){
    printf("I am a client\n\n");
    
    // Main Loop
    const char* debug_commands[] = {"list [lolazo]",
                                    "add [b.cpp] [lolazo, lolencio]", 
                                    "add [a.txt] [lolazo, pepe]",
                                    "list [lolazo]",
                                    "get [lolazo]",
                                    "add-tags [lolencio] [lolencio, jaujau]",
                                    "list [lolazo]"};
    int debug_command_count = sizeof(debug_commands)/sizeof(char*);
    //int debug_command_count = 0;
    int debug_command_sent_count = 0;

    sleep(3);

    while(1){
        // Connect to a server
        int clientSocket;
        int server_ip_index = 0; //Can be selected randomly for simple load balancing
        while (true){
            clientSocket = try_connect_to_sever_by_ip(server_ips[server_ip_index], MAIN_SERVER_PORT);
            if(clientSocket != -1) break;

            server_ip_index = (server_ip_index+1)%server_ips.size();
        }

        int val = 1;
        setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&val, sizeof(int));
        setsockopt(clientSocket, IPPROTO_TCP, TCP_KEEPCNT, (char*)&val, sizeof(int));
        setsockopt(clientSocket, IPPROTO_TCP, TCP_KEEPIDLE, (char*)&val, sizeof(int));
        setsockopt(clientSocket, IPPROTO_TCP, TCP_KEEPINTVL, (char*)&val, sizeof(int));
        
        int err = 0;
        
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
            std::cout << "COMANDO INCORRECTO!!!" << std::endl;
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
                free(file_descriptors);
                free_token_list(tokenized_message);
                continue;
            }

        }
        
        // Send command
        err = send_all(clientSocket, message, MESSAGE_MAX_SIZE);
        if (err == -1) {print_connection_error(); continue;}
        
        // Send more info if needed (for 'add' command)
        if(strcmp(tokenized_message[0], "add") == 0){
            for(int i = 0; i<file_count; i++){
                //First send size
                int file_size = get_file_size(file_descriptors[i]);
                int* file_size_pointer = &file_size;
                err = send_all(clientSocket, (char*)file_size_pointer, sizeof(int));
                if (err == -1) {print_connection_error(); break;}

                //Then send the actual file
                char* file_contents = (char*) malloc(file_size*sizeof(char));
                fread(file_contents, file_size, 1, file_descriptors[i]);
                
                err = send_all(clientSocket, file_contents, file_size);
                free(file_contents);
                if (err == -1) {print_connection_error(); break;}
            }

            
            list_fclose(file_descriptors, file_count);
            if (err == -1) {print_connection_error(); continue;}

            free(file_descriptors);
        }

        // Receive response
        char response_message[MESSAGE_MAX_SIZE] = { 0 };
        err = recv_to_fill(clientSocket, response_message, MESSAGE_MAX_SIZE);
        if (err == -1) {print_connection_error(); continue;}

        std::cout << response_message << std::endl;

        // If the command was a get, receive files
        if(strcmp(tokenized_message[0], "get") == 0){
            int count;
            err = recv_to_fill(clientSocket, (char*)&count, 4);
            if (err == -1) {print_connection_error(); continue;}

            for(int i = 0; i<count; i++){
                char* file_name = 0;
                int name_l = 0;
                err = recv_length_then_message(clientSocket, &name_l, &file_name);
                if (err == -1) {print_connection_error(); break;}

                char* file_content = 0;
                int file_size = 0;
                err = recv_length_then_message(clientSocket, &file_size, &file_content);
                if (err == -1) {print_connection_error(); break;}

                FILE* file_descriptor = open_file_in_folder(file_name, "dload", "wb");
                fwrite(file_content, file_size, 1, file_descriptor);
                fclose(file_descriptor);
            }

            if (err == -1) {print_connection_error(); continue;}
        }

        // Free stuff and close the socket
        free_token_list(tokenized_message);
        close(clientSocket);
    }

    
    

    return 0;
}
