#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "globals.h"
#include <vector>
#include <dirent.h>
#include <pthread.h>

using std::cout;
using std::endl;

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

std::vector<char*> get_file_names_from_query(char** command){
    
    // Get query tags
    std::vector<char*> query_tags;
    int it = 2;
    while(true){
        if(strcmp(command[it], "]") == 0){
            break;
        }
        query_tags.push_back(command[it]);
        it += 1;
    }

    // Ge all files
    int folder_path_len = 2+strlen(SERVER_TAGS_FOLDER_NAME)+1+1;
    char* folder_path = (char*)malloc(folder_path_len);
    folder_path[0] = 0;
    strcat(folder_path, "./");
    strcat(folder_path, SERVER_TAGS_FOLDER_NAME);
    strcat(folder_path, "/");
    folder_path[folder_path_len] = 0;

    dirent** all_file_names;
    int file_count = scandir(folder_path, &all_file_names, NULL, alphasort);

    // Filter the files
    std::vector<char*> file_names;
    for(int i = 0; i<file_count; i++){
        if(strcmp(all_file_names[i]->d_name, ".") != 0 && strcmp(all_file_names[i]->d_name, "..") != 0){
            
            // Get tag string
            FILE* file_descriptor = open_file_in_folder(all_file_names[i]->d_name, SERVER_TAGS_FOLDER_NAME, "rb");
            int file_size = get_file_size(file_descriptor);

            char* file_contents = (char*)malloc((file_size+1)*sizeof(char));
            fread(file_contents, file_size, 1, file_descriptor);
            file_contents[file_size] = 0;

            // Compare tags
            int coincidence_count = 0;
            char* tag = strtok(file_contents, "\n");
            while(tag != NULL){
                for(int j = 0; j<query_tags.size(); j++){
                    
                    if(strcmp(query_tags[j], tag) == 0){
                        coincidence_count += 1;
                        break;
                    }
                }
                tag = strtok(NULL, "\n");
            }

            if(coincidence_count == query_tags.size()){
                file_names.push_back(all_file_names[i]->d_name);
            }
            
            // Clean
            fclose(file_descriptor);
            free(file_contents);
        }
    }

    

    
    // Free and return
    for(int i = 0; i<file_count; i++){
        free(all_file_names[i]);
    }
    free(all_file_names);
    free(folder_path);

    return file_names;
}

std::vector<char*> get_tag_list(char** command){
    int i = 2;
    while(1){
        if(strcmp(command[i], "]") == 0){
            break;
        }
        i += 1;
    }
    i += 2;

    std::vector<char*> tag_list;
    while(1){
        if(strcmp(command[i], "]") == 0){
            break;
        }

        tag_list.push_back(command[i]);

        i += 1;
    }

    return tag_list;
}

char* execute_command(char** command, int clientSocket, pthread_mutex_t* mutex){
    pthread_mutex_lock(mutex);
    
    char* response = (char*)malloc(MESSAGE_MAX_SIZE);
    memset(response, 0, MESSAGE_MAX_SIZE);
    
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
        tag_string[0] = 0;
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
        
        strcpy(response, "Add Command Executed Succesfully!");
    }else if(strcmp(command[0], "list") == 0){
        std::vector<char*> target_file_names = get_file_names_from_query(command);
        
        for(int i = 0; i<target_file_names.size(); i++){
            strcat(response, "-File: ");
            strcat(response, target_file_names[i]);
            strcat(response, "\n");
            
            strcat(response, "-Tags: \n");
            FILE* file_descriptor = open_file_in_folder(target_file_names[i], SERVER_TAGS_FOLDER_NAME, "rb");
            int file_size = get_file_size(file_descriptor);
            char* tags_content = (char*)malloc((file_size+1)*sizeof(char));
            fread(tags_content, file_size, 1, file_descriptor);
            tags_content[file_size] = 0;

            strcat(response, tags_content);
            strcat(response, "\n");

            fclose(file_descriptor);
            free(tags_content);
        }
        
        strcat(response, "List Command Executed Succesfully!");
    }else if(strcmp(command[0], "delete") == 0){
        std::vector<char*> target_file_names = get_file_names_from_query(command);
        
        for(int i = 0; i<target_file_names.size(); i++){
            char* file_name = get_file_full_path(target_file_names[i], SERVER_FILES_FOLDER_NAME);
            remove(file_name);
            char* file_tags_name = get_file_full_path(target_file_names[i], SERVER_TAGS_FOLDER_NAME);
            remove(file_tags_name);
            
            free(file_name);
            free(file_tags_name);
        }
        strcat(response, "Delete Command Executed Succesfully!");
    }else if(strcmp(command[0], "add-tags") == 0){
        std::vector<char*> target_file_names = get_file_names_from_query(command);
        std::vector<char*> command_tag_list = get_tag_list(command);

        for(int i = 0; i<target_file_names.size(); i++){
            
            // Read full tags file
            FILE* tag_file_read = open_file_in_folder(target_file_names[i], SERVER_TAGS_FOLDER_NAME, "rb");
            int file_size_i = get_file_size(tag_file_read);
            char* tags_content_i = (char*)malloc((file_size_i+1)*sizeof(char));
            fread(tags_content_i, file_size_i, 1, tag_file_read);
            tags_content_i[file_size_i] = 0;
            fclose(tag_file_read);

            // Get current tags
            std::vector<char*> new_tag_list;
            char* tag = strtok(tags_content_i, "\n");
            while(tag != NULL){
                new_tag_list.push_back(tag);
                tag = strtok(NULL, "\n");
            }

            // Construct new tag list without repeated tags
            for(int tag_q_index = 0; tag_q_index<command_tag_list.size(); tag_q_index++){
                
                //See if tag is aleready in list
                bool is_repeated = false;
                for(int j = 0; j<new_tag_list.size(); j++){
                    if(strcmp(new_tag_list[j], command_tag_list[tag_q_index]) == 0){
                        is_repeated = true;
                        break;
                    }
                }

                if(!is_repeated){
                    new_tag_list.push_back(command_tag_list[tag_q_index]);
                }
            }

            // Construct and write new tag string
            int tags_total_size = 0; 
            for(int j = 0; j<new_tag_list.size(); j++){
                tags_total_size += strlen(new_tag_list[j]);
            }

            char* new_tag_file_contents = (char*)malloc(tags_total_size+new_tag_list.size()+1);
            new_tag_file_contents[0] = 0;
            for(int j = 0; j<new_tag_list.size(); j++){
                strcat(new_tag_file_contents, new_tag_list[j]);
                strcat(new_tag_file_contents, "\n");
            }

            FILE* tag_file = open_file_in_folder(target_file_names[i], SERVER_TAGS_FOLDER_NAME, "wb");
            fwrite(new_tag_file_contents, strlen(new_tag_file_contents), 1, tag_file);
            fclose(tag_file);
            
            strcat(response, "Add-Tags Command Executed Succesfully!");
        }
    }else if(strcmp(command[0], "delete-tags") == 0){
        std::vector<char*> target_file_names = get_file_names_from_query(command);
        std::vector<char*> command_tag_list = get_tag_list(command);

        for(int i = 0; i<target_file_names.size(); i++){
            
            // Read full tags file
            FILE* tag_file_read = open_file_in_folder(target_file_names[i], SERVER_TAGS_FOLDER_NAME, "rb");
            int file_size_i = get_file_size(tag_file_read);
            char* tags_content_i = (char*)malloc((file_size_i+1)*sizeof(char));
            fread(tags_content_i, file_size_i, 1, tag_file_read);
            tags_content_i[file_size_i] = 0;
            fclose(tag_file_read);

            // Get current tags without
            std::vector<char*> new_tag_list;
            char* tag = strtok(tags_content_i, "\n");
            while(tag != NULL){
                bool should_delete = false;
                for(int j = 0; j<command_tag_list.size(); j++){
                    if(strcmp(command_tag_list[j], tag) == 0){
                        should_delete = true;
                        break;
                    }
                }
                if(!should_delete){
                    new_tag_list.push_back(tag);
                }
                tag = strtok(NULL, "\n");
            }
            
            // Construct and write new tag string
            int tags_total_size = 0; 
            for(int j = 0; j<new_tag_list.size(); j++){
                tags_total_size += strlen(new_tag_list[j]);
            }

            char* new_tag_file_contents = (char*)malloc(tags_total_size+new_tag_list.size()+1);
            new_tag_file_contents[0] = 0;
            for(int j = 0; j<new_tag_list.size(); j++){
                strcat(new_tag_file_contents, new_tag_list[j]);
                strcat(new_tag_file_contents, "\n");
            }

            FILE* tag_file = open_file_in_folder(target_file_names[i], SERVER_TAGS_FOLDER_NAME, "wb");
            fwrite(new_tag_file_contents, strlen(new_tag_file_contents), 1, tag_file);
            fclose(tag_file);
            
            strcat(response, "Add-Tags Command Executed Succesfully!");
        }
    }else{
        strcpy(response, "WTF WAS THAT!!!");
    }

    pthread_mutex_unlock(mutex);

    return response;
}

struct Server_Arguments{
    int clientSocket;
    pthread_mutex_t* mutex;
};

void* server_loop(void* args){
    Server_Arguments* s_a = (Server_Arguments*)args;
    int clientSocket = s_a->clientSocket;
    pthread_mutex_t* mutex = s_a->mutex;

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
        char* response = execute_command(command, clientSocket, mutex);
        free_token_list(command);
        
        send(clientSocket, response, strlen(response), 0);
        free(response);
    }

    return NULL;
}

int main(){
    //Mutex
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    //Create Server
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    while(1){
        // Connect
        int clientSocket = 0;
        listen(serverSocket, 1);
        clientSocket = accept(serverSocket, nullptr, nullptr);
        
        Server_Arguments* s_a = (Server_Arguments*)malloc(sizeof(Server_Arguments));
        s_a->clientSocket = clientSocket;
        s_a->mutex = &mutex;

        // Connect
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, server_loop, (void*)s_a);
    }
    

    return 0;
}