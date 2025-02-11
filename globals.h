#pragma once

#include <stdio.h>
#include "threadsafe_queue.h"
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <iostream>
#include <arpa/inet.h>

#define MESSAGE_MAX_SIZE 10000
#define MAX_TOKEN_SIZE 200
#define SERVER_FILES_FOLDER_NAME "files"
#define SERVER_TAGS_FOLDER_NAME "file_tags"

#define CLIENT_DHT_DISCOVER_PORT 8080
#define DHT_COMMAND_PORT 8080

//Returns a list of strings, each one a token. The real list ends when a char* = null is found.
char** tokenize_command(char* command){
    
    //Allocate token list
    char** token_list = (char**)malloc(MESSAGE_MAX_SIZE*sizeof(char*));
    for(int i = 0; i<MESSAGE_MAX_SIZE; i++){
        token_list[i] = (char*) malloc(MAX_TOKEN_SIZE*sizeof(char));
        memset(token_list[i], 0, MAX_TOKEN_SIZE*sizeof(char));
    }
    
    //Tokenize
    int i_token = 0;
    int i_char = 0;
    int i_char_token = 0;
    while(true){
        if(command[i_char] == 0){
            if(i_char_token >= 1){
                i_token += 1;
            }
            token_list[i_token] = 0;
            break;
        }
        
        if(command[i_char] == ' ' || command[i_char] == ','){
            if(i_char_token >= 1){
                i_token += 1;
            }
           i_char_token = 0;
        }else if(command[i_char] == '[' || command[i_char] == ']'){
            if(i_char_token >= 1){
                i_token += 1;
            }
            token_list[i_token][0] = command[i_char] ;
            i_token += 1;
            i_char_token = 0;
        }else{
            token_list[i_token][i_char_token] = command[i_char];
            i_char_token += 1;
        }

        i_char += 1;
    }

    return token_list;
}

void print_token_list(char** token_list){
    int i = 0;
    while(true){
        if(token_list[i] == 0){
            break;
        }
        printf("%s\n", token_list[i]);
        i += 1;
    }
}

void free_list_and_members(void** list, int count){
    for(int i = 0; i<count; i++){
        free(list[i]);
    }
    free(list);
}

void free_token_list(char** token_list){
    free_list_and_members((void**)token_list, MESSAGE_MAX_SIZE);
}

void list_fclose(FILE** f_list, int file_count){
    for(int i = 0; i<file_count; i++){
        if(f_list[i] != NULL){
            fclose(f_list[i]);
        }
    }
}

int get_file_size(FILE* descriptor){
    fseek(descriptor, 0, SEEK_END);
    int f_size = ftell(descriptor);
    fseek(descriptor, 0, SEEK_SET);
    return f_size;
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

int send_all(int sock, const char* data, int dLen){
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

// return the index after the end of the list, or -1 if its not a valid list
int is_token_list(char** tokenized_message, int start_index){
    
    if(tokenized_message[start_index] == 0 || strcmp(tokenized_message[start_index], "[") != 0){
        return -1;
    }

    int i = start_index+1;
    while(true){
        if(tokenized_message[i] == NULL){
            return -1;
        }else if(strcmp(tokenized_message[i], "]") == 0){
            if(i == start_index+1){
                return -1; //Empty list
            }else{
                break;
            }
        }
        i += 1;
    }

    i += 1;
    return i;
}

bool is_message_correct(char** tokenized_message){
    if(tokenized_message[0] == NULL){
        return false;
    }
    
    if(strcmp(tokenized_message[0], "exit") == 0){
        if(tokenized_message[1] == NULL){
            return true;
        }else{
            return false;
        }
    }else if(strcmp(tokenized_message[0], "add") == 0){
        int next_list_start = is_token_list(tokenized_message, 1);
        if (next_list_start == -1){ return false; }
        
        int end_token_index = is_token_list(tokenized_message, next_list_start);
        if (end_token_index == -1){ return false; }

        if(tokenized_message[end_token_index] != 0){ return false; }
    }else if(strcmp(tokenized_message[0], "delete") == 0 || strcmp(tokenized_message[0], "list") == 0 || strcmp(tokenized_message[0], "get") == 0){
        int end_token_index = is_token_list(tokenized_message, 1);
        if (end_token_index == -1){ return false; }

        if(tokenized_message[end_token_index] != 0){ return false; }
    }else if(strcmp(tokenized_message[0], "add-tags") == 0 || strcmp(tokenized_message[0], "delete-tags") == 0){
        int next_list_start = is_token_list(tokenized_message, 1);
        if (next_list_start == -1){ return false; }
        
        int end_token_index = is_token_list(tokenized_message, next_list_start);
        if (end_token_index == -1){ return false; }

        if(tokenized_message[end_token_index] != 0){ return false; }
    }else{
        return false;
    }

    return true;
}

int send_length_then_message(int sock, int size, const char* message){
    send_all(sock, (char*)&size, sizeof(int));
    send_all(sock, message, size*sizeof(char));
    return 0;
}

int recv_length_then_message(int sock, int* size, char** message){
    //First receive size
    recv_to_fill(sock, (char*)size, sizeof(int));
    
    //Then send the actual message
    *message = (char*) malloc((*size)*sizeof(char));
    recv_to_fill(sock, *message, *size);
    return 0;
}

#define DHT_KEY_MODULE 256
int hash_string(char* str){
    char c = str[0];
    int i = 0;
    int h = 0;

    while(c != 0){
        h += (i+1)*c;
        h = h%DHT_KEY_MODULE;
        i++;
        c = str[i];
    }

    return h;
}

char* get_file_full_path(char* file_name, const char* folder_name){
    int filepath_len = 2+strlen(folder_name)+1+strlen(file_name)+1;
    char* file_path = (char*)malloc(filepath_len);
    file_path[0] = 0;
    strcat(file_path, "./");
    strcat(file_path, folder_name);
    strcat(file_path, "/");
    strcat(file_path, file_name);
    return file_path;
}

FILE* open_file_in_folder(char* file_name, const char* folder_name, const char* mode){
    char* file_path = get_file_full_path(file_name, folder_name);

    FILE* file_descriptor = fopen(file_path, mode);
    free(file_path);
    return file_descriptor;
}

void read_entire_file(FILE* file_descriptor, char** contents, int* size){
    *size = get_file_size(file_descriptor);
    *contents = (char*)malloc((*size)*sizeof(char));
    fread(*contents, *size, 1, file_descriptor);
}

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

in_addr get_ip_from_multicast(const char* ip, int port){
    // Create UDP socket
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in in_addr;
    in_addr.sin_family = AF_INET;
    in_addr.sin_addr.s_addr = INADDR_ANY;
    in_addr.sin_port = htons(port);

    bind(udp_sock, (sockaddr*) &in_addr, sizeof(in_addr));

    // Subscribe to the multicast 
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    setsockopt(udp_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));

    // Listen for signals of dht nodes
    int dummy;
    sockaddr recv_saddr;
    socklen_t recv_socklen = sizeof(recv_saddr);
    recvfrom(udp_sock, &dummy, sizeof(int), 0, (struct sockaddr *)&recv_saddr, &recv_socklen);
    
    sockaddr_in* ipv4Addr = (sockaddr_in*) &recv_saddr;
    //printf("lol, recv %i from %s\n", dummy, inet_ntoa(ipv4Addr->sin_addr));

    close(udp_sock);
    return ipv4Addr->sin_addr;
}

bool contains(std::vector<char*> l, char* s){
    for(int i = 0; i<l.size(); i++){
        if(strcmp(l[i], s) == 0){
            return true;
        }
    }
    return false;
}

std::vector<char*> file_lines_to_list(char* contents, int size){
    char* contents_str = (char*)malloc((size+1)*sizeof(char));
    memcpy(contents_str, contents, size);
    contents_str[size] = 0;

    std::vector<char*> l;
    char* tk = strtok(contents_str, "\n");
    while(tk != NULL){
        char* cpy = strdup(tk);
        l.push_back(cpy);
        tk = strtok(NULL, "\n");
    }

    free(contents_str);
    return l;
}

void list_to_file_lines(std::vector<char*> l, char** contents, int* size){
    
    // Get total size of the document
    *size = 0;
    for(int i = 0; i<l.size(); i++){
        *size += strlen(l[i])+1;
    }

    // Construct document
    *contents = (char*)malloc((*size)*sizeof(char));
    int current_length = 0;
    for(int i = 0; i<l.size(); i++){
        memcpy((*contents)+current_length, l[i], strlen(l[i]));
        current_length += strlen(l[i]);
        (*contents)[current_length] = '\n';
        current_length += 1;
    }
}