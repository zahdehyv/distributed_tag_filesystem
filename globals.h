#define MESSAGE_MAX_SIZE 10000
#define MAX_TOKEN_SIZE 200
#define SERVER_FILES_FOLDER_NAME "files"
#define SERVER_TAGS_FOLDER_NAME "file_tags"

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
    }else if(strcmp(tokenized_message[0], "delete") == 0 || strcmp(tokenized_message[0], "list") == 0){
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