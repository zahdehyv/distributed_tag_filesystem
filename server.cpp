#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "globals.h"
#include <vector>
#include <dirent.h>
#include <pthread.h>
#include "rpcs.h"

using std::cout;
using std::endl;

#define TAGS_TO_FILES_DHT_BROADCAST_IP "239.193.7.77"
#define TAGS_TO_FILES_DHT_BROADCAST_PORT 4096

#define FILES_DHT_BROADCAST_IP "239.193.7.78"
#define FILES_DHT_BROADCAST_PORT 4097

#define FILES_TO_TAGS_DHT_BROADCAST_IP "239.193.7.79"
#define FILES_TO_TAGS_DHT_BROADCAST_PORT 4098

std::vector<char*> get_file_names_from_query(char** command, in_addr tags_to_files_dht_ip){
    
    // Get query tags
    std::vector<char*> query_tags;
    int it = 2;
    while(true){
        if(strcmp(command[it], "]") == 0){
            break;
        }
        if(!contains(query_tags, command[it])){
            query_tags.push_back(command[it]);
        }
        it += 1;
    }

    // Get separated file lists and intersect them
    std::vector<char*> file_names;
    for(int i = 0; i<query_tags.size(); i++){
        
        // Get tag files as list
        char* tag_to_files = 0;
        int tag_to_files_size = 0;
        rpc_get(tags_to_files_dht_ip, query_tags[i], &tag_to_files, &tag_to_files_size);

        if(tag_to_files == NULL) continue;

        std::vector<char*> current_tag_files = file_lines_to_list(tag_to_files, tag_to_files_size);
        free(tag_to_files);

        // Intersect
        if(i == 0){
            file_names = current_tag_files;
        }else{
            std::vector<char*> new_file_names;
            for(int ii = 0; ii<file_names.size(); ii++){
                
                if(contains(current_tag_files, file_names[ii])){
                    char* cpy = strdup(file_names[ii]);
                    new_file_names.push_back(cpy);
                }
                
            }
            file_names = new_file_names;
        }
    }

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

void execute_command(char** command, int clientSocket, pthread_mutex_t* mutex){
    pthread_mutex_lock(mutex);
    
    char* response = (char*)malloc(MESSAGE_MAX_SIZE);
    memset(response, 0, MESSAGE_MAX_SIZE);
    in_addr tags_to_files_dht_ip = get_ip_from_multicast(TAGS_TO_FILES_DHT_BROADCAST_IP, TAGS_TO_FILES_DHT_BROADCAST_PORT);
    in_addr files_dht_ip = get_ip_from_multicast(FILES_DHT_BROADCAST_IP, FILES_DHT_BROADCAST_PORT);
    in_addr files_to_tags_dht_ip = get_ip_from_multicast(FILES_TO_TAGS_DHT_BROADCAST_IP, FILES_TO_TAGS_DHT_BROADCAST_PORT);

    int file_count_for_get = 0;
    int* sizes_for_get = 0;
    char** contents_for_get = 0;
    char** names_for_get = 0;

    if(strcmp(command[0], "add") == 0){

        // Get file names
        std::vector<char*> file_names;
        int first_back_end = 0;
        int j = 2;
        while(true){
            if(strcmp(command[j], "]") == 0){
                first_back_end = j;
                break;
            }

            file_names.push_back(command[j]);
            j += 1;
        }

        // Get tag names
        std::vector<char*> tag_names;
        j = first_back_end+2;
        while(true){
            if(strcmp(command[j], "]") == 0){
                break;
            }

            if(!contains(tag_names, command[j])){
                tag_names.push_back(command[j]);
            }
            
            j += 1;
        }

        // Get file to tags
        char* files_to_tags_content = 0;
        int files_to_tags_size = 0;
        list_to_file_lines(tag_names, &files_to_tags_content, &files_to_tags_size);

        // Receive and write files and file_to_tags (in order)
        for(int i = 0; i<file_names.size(); i++){
            // Receive File Size
            int file_size = 0;
            recv_to_fill(clientSocket, (char*)&file_size, sizeof(int));

            // Receive File Contents
            char* file_contents = (char*) malloc(file_size*sizeof(char));
            recv_to_fill(clientSocket, file_contents, file_size);

            // Write File Contents
            rpc_add(files_dht_ip, file_names[i], file_contents, file_size);
            std::cout << "Written: " << file_names[i] << std::endl;

            // Write File To Tags
            rpc_add(files_to_tags_dht_ip, file_names[i], files_to_tags_content, files_to_tags_size);
            
            // Free stuff
            free(file_contents);
        }

        // Update tags
        for(int i = 0; i<tag_names.size(); i++){
            
            // Get tag contents
            char* files_with_tag_raw = 0;
            int files_with_tag_raw_size = 0;
            rpc_get(tags_to_files_dht_ip, tag_names[i], &files_with_tag_raw, &files_with_tag_raw_size);
            
            // Tag content to list
            std::vector<char*> files_with_tag_list;
            if(files_with_tag_raw != 0){
                files_with_tag_list = file_lines_to_list(files_with_tag_raw, files_with_tag_raw_size);
            }

            // Append the file to the list if not repeated
            for(int ii = 0; ii<file_names.size(); ii++){
                if(!contains(files_with_tag_list, file_names[ii])){
                    files_with_tag_list.push_back(file_names[ii]);
                }
            }

            // Build new file contents and write it
            char* contents = 0;
            int contents_size = 0;
            list_to_file_lines(files_with_tag_list, &contents, &contents_size);

            rpc_add(tags_to_files_dht_ip, tag_names[i], contents, contents_size);
            free(contents);
        }
        
        strcpy(response, "Add Command Executed Succesfully!");
    }else if(strcmp(command[0], "list") == 0){
        std::vector<char*> target_file_names = get_file_names_from_query(command, tags_to_files_dht_ip);
        for(int i = 0; i<target_file_names.size(); i++){
            strcat(response, "-File: ");
            strcat(response, target_file_names[i]);
            strcat(response, "\n");
            
            strcat(response, "-Tags: \n");
            char* tags_of_file = 0;
            int tags_of_file_size = 0;
            rpc_get(files_to_tags_dht_ip, target_file_names[i], &tags_of_file, &tags_of_file_size);

            char* tags_of_file_str = (char*)malloc((tags_of_file_size+1)*sizeof(char));
            
            memcpy(tags_of_file_str, tags_of_file, tags_of_file_size);
            tags_of_file_str[tags_of_file_size] = 0;

            strcat(response, tags_of_file_str);
            strcat(response, "\n");

            free(tags_of_file);
            free(tags_of_file_str);
        }
        
        strcat(response, "List Command Executed Succesfully!");
    }else if(strcmp(command[0], "delete") == 0){
        std::vector<char*> target_file_names = get_file_names_from_query(command, tags_to_files_dht_ip);
        for(int i = 0; i<target_file_names.size(); i++){
            
            // Delete the hard stuff
            char* file_tags_raw = 0;
            int file_tags_raw_size = 0;
            rpc_get(files_to_tags_dht_ip, target_file_names[i], &file_tags_raw, &file_tags_raw_size);
            std::vector<char*> tag_list = file_lines_to_list(file_tags_raw, file_tags_raw_size);

            for(int ii = 0; ii<tag_list.size(); ii++){
                // Get the file list of the tag
                char* file_list_raw = 0;
                int file_list_raw_size = 0;
                rpc_get(tags_to_files_dht_ip, tag_list[ii], &file_list_raw, &file_list_raw_size);
                std::vector<char*> file_list = file_lines_to_list(file_list_raw, file_list_raw_size);

                // Remove the file
                std::vector<char*> new_file_list;
                for(int iii = 0; iii<file_list.size(); iii++){
                    printf("%s\n", file_list[iii]);
                    if(strcmp(file_list[iii], target_file_names[i]) != 0){
                        new_file_list.push_back(file_list[iii]);
                    }
                }

                if(new_file_list.size() == 0){
                    rpc_delete(tags_to_files_dht_ip, tag_list[ii]);
                }else{
                    char* new_file_list_raw = 0;
                    int new_file_list_raw_size = 0;
                    list_to_file_lines(new_file_list, &new_file_list_raw, &new_file_list_raw_size);
                    rpc_add(tags_to_files_dht_ip, tag_list[ii], new_file_list_raw, new_file_list_raw_size);
                    free(new_file_list_raw);
                }
                
                free(file_list_raw);
            }

            free(file_tags_raw);

            // Delete the easy stuff
            rpc_delete(files_dht_ip, target_file_names[i]);
            rpc_delete(files_to_tags_dht_ip, target_file_names[i]);
        }

        strcat(response, "Delete Command Executed Succesfully!");
    }else if(strcmp(command[0], "add-tags") == 0){
        std::vector<char*> target_file_names = get_file_names_from_query(command, tags_to_files_dht_ip);
        std::vector<char*> command_tag_list = get_tag_list(command);

        // Add to files_to_tags
        for(int i = 0; i<target_file_names.size(); i++){
            std::vector<char*> file_tags_list = rpc_get_in_list(files_to_tags_dht_ip, target_file_names[i]);

            for(int ii = 0; ii<command_tag_list.size(); ii++){
                if(!contains(file_tags_list, command_tag_list[ii])){
                    file_tags_list.push_back(command_tag_list[ii]);
                }
            }

            rpc_add_in_list(files_to_tags_dht_ip, target_file_names[i], file_tags_list);
        }

        // Add to tags_to_files
        for(int i = 0; i<command_tag_list.size(); i++){
            std::vector<char*> tag_files_list = rpc_get_in_list(tags_to_files_dht_ip, command_tag_list[i]);

            for(int ii = 0; ii<target_file_names.size(); ii++){
                
                if(!contains(tag_files_list, target_file_names[ii])){
                    tag_files_list.push_back(target_file_names[ii]);
                }
            }

            rpc_add_in_list(tags_to_files_dht_ip, command_tag_list[i], tag_files_list);
        }

        
        strcat(response, "Add-Tags Command Executed Succesfully!");

        /*std::vector<char*> target_file_names = get_file_names_from_query(command);
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
        }*/
    }else if(strcmp(command[0], "delete-tags") == 0){
        std::vector<char*> target_file_names = get_file_names_from_query(command, tags_to_files_dht_ip);
        std::vector<char*> command_tag_list = get_tag_list(command);

        // Delete from files_to_tags
        for(int i = 0; i<target_file_names.size(); i++){
            std::vector<char*> file_tags_list = rpc_get_in_list(files_to_tags_dht_ip, target_file_names[i]);
            
            std::vector<char*> file_new_tags_list;
            for(int ii = 0; ii<file_tags_list.size(); ii++){
                if(!contains(command_tag_list, file_tags_list[ii])){
                    file_new_tags_list.push_back(file_tags_list[ii]);
                }
            }

            rpc_add_in_list(files_to_tags_dht_ip, target_file_names[i], file_new_tags_list);
        }

        // Delete from tags_to_files
        for(int i = 0; i<command_tag_list.size(); i++){
            std::vector<char*> tag_files_list = rpc_get_in_list(tags_to_files_dht_ip, command_tag_list[i]);

            std::vector<char*> tag_new_files_list;
            for(int ii = 0; ii<tag_files_list.size(); ii++){
                if(!contains(target_file_names, tag_files_list[ii])){
                    tag_new_files_list.push_back(tag_files_list[ii]);
                }
            }

            rpc_add_in_list(tags_to_files_dht_ip, target_file_names[i], tag_new_files_list);
        }

        strcat(response, "Delete-Tags Command Executed Succesfully!");
    }else if(strcmp(command[0], "get") == 0){
        std::vector<char*> target_file_names = get_file_names_from_query(command, tags_to_files_dht_ip);

        file_count_for_get = target_file_names.size();
        sizes_for_get = (int*)malloc(file_count_for_get*sizeof(char*));
        names_for_get = (char**)malloc(file_count_for_get*sizeof(char*));
        contents_for_get = (char**)malloc(file_count_for_get*sizeof(char*));

        for(int i = 0; i<target_file_names.size(); i++){
            char* get_response = 0;
            int get_respose_size = 0;
            rpc_get(files_dht_ip, target_file_names[i], &get_response, &get_respose_size);

            sizes_for_get[i] = get_respose_size;
            names_for_get[i] = strdup(target_file_names[i]);
            contents_for_get[i] = get_response;
        }
        strcat(response, "Get Command Executed Succesfully!");
    }else{
        strcpy(response, "WTF WAS THAT!!!");
    }

    pthread_mutex_unlock(mutex);

    //Send response
    send_all(clientSocket, response, MESSAGE_MAX_SIZE);
    free(response);

    // Send files if the command was a get
    if(strcmp(command[0], "get") == 0){
        
        send_all(clientSocket, (char*)&file_count_for_get, 4);

        for(int i = 0; i<file_count_for_get; i++){
            send_length_then_message(clientSocket, strlen(names_for_get[i])+1, names_for_get[i]);
            send_length_then_message(clientSocket, sizes_for_get[i], contents_for_get[i]);
        }

    }

    
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
        int err = recv_to_fill(clientSocket, raw_command, MESSAGE_MAX_SIZE);
        if (err == -1 || raw_command[0] == 0){ break; }
        cout << "Command from client: " << raw_command << endl;

        //Process command and respond
        char** command = tokenize_command(raw_command);
        execute_command(command, clientSocket, mutex);
        free_token_list(command);
    }

    return NULL;
}

int main(){
    printf("I am a controller server\n\n");
    
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