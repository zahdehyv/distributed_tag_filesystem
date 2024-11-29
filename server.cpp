#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "globals.h"

using std::cout;
using std::endl;

void create_server_and_wait_for_client(int* serverSocket_ptr, int* clientSocket_ptr){
    // creating socket
    *serverSocket_ptr = socket(AF_INET, SOCK_STREAM, 0);

    // specifying the address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // binding socket.
    bind(*serverSocket_ptr, (struct sockaddr*)&serverAddress,
         sizeof(serverAddress));

    // listening to the assigned socket
    listen(*serverSocket_ptr, 5);

    // accepting connection request
    *clientSocket_ptr = accept(*serverSocket_ptr, nullptr, nullptr);
}

FILE* open_file_in_folder(char* file_name, const char* folder_name, const char* mode){
    int filepath_len = 2+strlen(folder_name)+1+strlen(file_name)+1;
    char* file_path = (char*)malloc(filepath_len);
    strcat(file_path, "./");
    strcat(file_path, folder_name);
    strcat(file_path, "/");
    strcat(file_path, file_name);

    FILE* file_descriptor = fopen(file_path, mode);

    free(file_path);
    return file_descriptor;
}


char* execute_command(char** command, int clientSocket){
    if(strcmp(command[0], "add") == 0){

        // Calculate file count
        int file_count = 0;
        int first_back_end = 0;
        int j = 2;
        while(true){
            if(strcmp(command[j], "]") == 0){
                first_back_end = j;
                break;
            }

            file_count += 1;
            j += 1;
        }
        
        // Receive and write files (in order)
        for(int i = 0; i<file_count; i++){
            // Receive File Size
            int file_size = 0;
            recv_to_fill(clientSocket, (char*)&file_size, sizeof(int));

            // Receive File Contents
            char* file_contents = (char*) malloc(file_size*sizeof(char));
            recv_to_fill(clientSocket, file_contents, file_size);

            // Write File Contents
            FILE* file_descriptor = open_file_in_folder(command[2+i], SERVER_FILES_FOLDER_NAME, "wb");
            fwrite(file_contents, file_size, 1, file_descriptor);
            std::cout << "Written: " << command[2+i] << std::endl;
            
            // Free stuff
            fclose(file_descriptor);
            free(file_contents);
        }

        // Calculate tag count
        int tags_total_size = 0;
        int tag_count = 0;
        j = first_back_end+2;
        while(true){
            if(strcmp(command[j], "]") == 0){
                break;
            }

            tags_total_size += strlen(command[j]);
            tag_count += 1;
            j += 1;
        }

        // Create Tag String
        char* tag_string = (char*)malloc(tags_total_size+tag_count+1);
        for(int i = 0; i<tag_count; i++){
            strcat(tag_string, command[first_back_end+2+i]);
            strcat(tag_string, "\n");
        }

        // Write tags
        for(int i = 0; i<file_count; i++){
            FILE* file_descriptor = open_file_in_folder(command[2+i], SERVER_TAGS_FOLDER_NAME, "wb");
            fwrite(tag_string, strlen(tag_string), 1, file_descriptor);
            fclose(file_descriptor);
        }

        return "Add Command Executed Succesfully!";
    }else if(strcmp(command[0], "delete") == 0){
        
    }

    return "WTF WAS THAT!!!";
}

int main(){
    
    // Connect
    int serverSocket = 0;
    int clientSocket = 0; 
    create_server_and_wait_for_client(&serverSocket, &clientSocket);

    // Main Loop
    while(1){
        
        // Get command
        cout << "Waiting for new command..." << endl;
        char raw_command[MESSAGE_MAX_SIZE] = { 0 };
        int err = recv(clientSocket, raw_command, MESSAGE_MAX_SIZE, 0);
        if (err == -1 || raw_command[0] == 0){ break; }
        cout << "Command from client: " << raw_command << endl;

        //Process command and respond
        char** command = tokenize_command(raw_command);
        char* response = execute_command(command, clientSocket);
        free_token_list(command);
        
        send(clientSocket, response, strlen(response), 0);
    }


    // Close the socket
    close(serverSocket);

    return 0;
}