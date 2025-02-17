#include <stdio.h>
#include "globals.h"
#include <stdlib.h>
#include <unistd.h>
#include "rpcs.h"
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netinet/tcp.h>

int node_key = 0;
in_addr node_ip = {0};
std::vector<int> successors_key;
std::vector<in_addr> successors_ip;
std::vector<int> predecessors_key; // [0] is immediate predecessor
std::vector<in_addr> predecessors_ip;

char* main_folder_name = 0;

Threadsafe_Command_Queue command_queue(10000);

void* command_accept_thread(void* args){
    //Create Server
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(DHT_COMMAND_PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    //Wait for commands
    while(1){
        
        // Listen for connection
        listen(serverSocket, 1);
        int clientSocket = accept(serverSocket, nullptr, nullptr);

        // Receive and enqueue command
        Command new_command;
        new_command.sock = clientSocket;
        recv_length_then_message(clientSocket, &(new_command.size), &(new_command.message));
        
        command_queue.enqueue(new_command);
    }
}

void* heart_beat_accept_thread(void* args){
    //Create Server
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(DHT_HEART_BEAT_PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    //Wait for petitions
    while(1){
        // Listen for connection
        listen(serverSocket, 1);
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        close(clientSocket);
    }
    
}

void handle_succesors_down(bool succ_succ_down){
    
    in_addr zero_ip = {0};
    if(!succ_succ_down){
        // Fix my succesors
        in_addr succ_down_ip = successors_ip[0];
        int succ_down_key = successors_key[0];
        shift_left_ereasing_index(&successors_ip, 0, node_ip);
        shift_left_ereasing_index(&successors_key, 0, node_key);

        printf("New successor list: ");
        print_list(successors_key);

        if(successors_ip[1].s_addr != node_ip.s_addr){
            Node_Reference* ref = rpc_find_succesor(successors_ip[1], successors_key[1]+1);
            successors_ip[2] = ref->ip;
            successors_key[2] = ref->key;
        }
        
        // Fix my predecessors (if succesor_down is in my three predecessors, then is the last one, and the net cycles)
        for(int i = 0; i<predecessors_ip.size(); i++){
            if(predecessors_ip[i].s_addr == succ_down_ip.s_addr){
                predecessors_ip[i] = node_ip;
                predecessors_key[i] = node_key;
                break;
            }
        }

        printf("New predecessor list: ");
        print_list(predecessors_key);

        // Fix my predecessor's successors
        if(predecessors_ip[0].s_addr != node_ip.s_addr) {
            rpc_deprecate_succesors(predecessors_ip[0], succ_down_ip, zero_ip);
            if(predecessors_ip[0].s_addr != successors_ip[1].s_addr){
                rpc_insert_succesor(predecessors_ip[0], successors_ip[1], successors_key[1]);
            }
            
        }
        if(predecessors_ip[1].s_addr != node_ip.s_addr) {
            rpc_deprecate_succesors(predecessors_ip[1], succ_down_ip, zero_ip);
            if(predecessors_ip[1].s_addr != successors_ip[0].s_addr){
                rpc_insert_succesor(predecessors_ip[1], successors_ip[0], successors_key[0]);
            }
        }

        // Fix my successor's predecessors
        if(successors_ip[0].s_addr != node_ip.s_addr) {
            rpc_deprecate_predecessors(successors_ip[0], succ_down_ip, zero_ip);
            if(successors_ip[0].s_addr != predecessors_ip[1].s_addr){
                rpc_insert_predecessor(successors_ip[0], predecessors_ip[1], predecessors_key[1]);
            }
        }
        if(successors_ip[1].s_addr != node_ip.s_addr) {
            rpc_deprecate_predecessors(successors_ip[1], succ_down_ip, zero_ip);
            if(successors_ip[1].s_addr != predecessors_ip[0].s_addr){
                rpc_insert_predecessor(successors_ip[1], predecessors_ip[0], predecessors_key[0]);
            }
        }
        if(successors_ip[2].s_addr != node_ip.s_addr) {
            rpc_deprecate_predecessors(successors_ip[2], succ_down_ip, zero_ip);
            if(successors_ip[2].s_addr != node_ip.s_addr){
                rpc_insert_predecessor(successors_ip[2], node_ip, node_key);
            }
        }
        
        // Replicate the stuff from the down node
        std::vector<char*> down_file_names = rpc_get_keys_in_range(successors_ip[0], node_key, succ_down_key);
        for(int i = 0; i<down_file_names.size(); i++){
            char* file_contents;
            int file_size;
            rpc_get(successors_ip[0], down_file_names[i], &file_contents, &file_size);
            if(successors_ip[2].s_addr == node_ip.s_addr){
                FILE* file_descriptor = open_file_in_folder(down_file_names[i], main_folder_name, "wb");
                fwrite(file_contents, file_size, 1, file_descriptor);
                fclose(file_descriptor);
            }else{
                _rpc_force_add(successors_ip[2], down_file_names[i], file_contents, file_size, false);
            }
            free(file_contents);
        }

        // Replicate the stuff from myself
        if(successors_ip[1].s_addr != node_ip.s_addr){
            dirent** all_file_names;
            int file_count = scandir(main_folder_name, &all_file_names, NULL, alphasort);

            std::vector<char*> my_file_names;
            for(int i = 0; i<file_count; i++){
                if(strcmp(all_file_names[i]->d_name, ".") != 0 && strcmp(all_file_names[i]->d_name, "..") != 0){
                    
                    // If file name is in between, add it
                    int h = hash_string(all_file_names[i]->d_name);
                    if(is_in_between_modular(h, predecessors_key[0], node_key)){
                        my_file_names.push_back(all_file_names[i]->d_name);
                    }
                }
            }

            for(int i = 0; i<my_file_names.size(); i++){
                int file_size;
                char* file_contents;
                FILE* file_descriptor = open_file_in_folder(my_file_names[i], main_folder_name, "rb");
                read_entire_file(file_descriptor, &file_contents, &file_size);
                _rpc_force_add(successors_ip[1], my_file_names[i], file_contents, file_size, false);
                free(file_contents);
            }
        }

        // Replicate the stuff that my immediate predecessor replicated in the down node
        if(predecessors_ip[0].s_addr != node_ip.s_addr){
            std::vector<char*> rep_file_names = rpc_get_keys_in_range(predecessors_ip[0], predecessors_key[1], predecessors_key[0]);
            
            for(int i = 0; i<rep_file_names.size(); i++){
                char* file_contents;
                int file_size;
                rpc_get(predecessors_ip[0], rep_file_names[i], &file_contents, &file_size);
                _rpc_force_add(successors_ip[0], rep_file_names[i], file_contents, file_size, false);
                free(file_contents);
            }
        }
    }
}

void* heart_beat_send_thread(void* args){
    int max_failed_attempts = 3;
    while(1){
        int failed_attempts = 0;
        while(1){
            if(successors_ip[0].s_addr == node_ip.s_addr) break;
            
            int error = try_connect_to_sever_by_ip(successors_ip[0], DHT_HEART_BEAT_PORT);
            if(error == -1) failed_attempts += 1;
            else break;

            // The succesor is down
            if(failed_attempts >= max_failed_attempts){
                std::cout << "Succesor Down! I repeat, succesor down!" << std::endl;
                
                //sleep(4);

                // Is also second succesor down?
                bool succ_succ_is_down = false;
                /*failed_attempts = 0;
                
                while(successors_ip[1].s_addr != node_ip.s_addr){
                    int error = try_connect_to_sever_by_ip(successors_ip[1], DHT_HEART_BEAT_PORT);
                    if(error == -1) failed_attempts += 1;
                    else break;

                    if(failed_attempts >= max_failed_attempts){
                        succ_succ_is_down = true;
                        std::cout << "Succ Succ also Down!" << std::endl;
                        break;
                    }

                    usleep(100000);
                }*/

                // Handle the situation
                handle_succesors_down(succ_succ_is_down);
            }

            usleep(300000);
        }
        usleep(300000);
    }
}

void join_dht(char* ip, int port){

    in_addr dht_node_ip = get_ip_from_multicast(ip, port);

    // Find predecesor
    Node_Reference* pred_reference = rpc_find_predecessor(dht_node_ip, node_key);
    
    // Get current predecessors and successors of the new predecessor
    int l_count = DHT_FAULT_TOLERANCE_LEVEL + 1;
    
    int* pred_pred_keys = 0;
    in_addr* pred_pred_ips = 0;
    get_all_predecesors(pred_reference->ip, &pred_pred_ips, &pred_pred_keys);
    
    int* pred_succ_keys = 0;
    in_addr* pred_succ_ips = 0;
    get_all_succesors(pred_reference->ip, &pred_succ_ips, &pred_succ_keys);
    
    // Set node successors and predecessors
    for(int i = 0; i<l_count; i++){
        successors_ip[i] = pred_succ_ips[i];
        successors_key[i] = pred_succ_keys[i];

        if(pred_succ_ips[i].s_addr == pred_reference->ip.s_addr) break;
    }

    predecessors_ip[0] = pred_reference->ip;
    predecessors_key[0] = pred_reference->key;
    for(int i = 1; i<l_count; i++){
        if(pred_pred_ips[i-1].s_addr == pred_reference->ip.s_addr) break;
        
        predecessors_ip[i] = pred_pred_ips[i-1];
        predecessors_key[i] = pred_pred_keys[i-1];
    }
    
    // Debug Prints
    printf("Joined!!!\n");
    printf("Succesors: ");
    print_list(successors_key);
    printf("Predecessors: ");
    print_list(predecessors_key);

    // Get files that would have corresponded to the node, and delete last replica
    std::vector<char*> my_filenames = rpc_get_keys_in_range(successors_ip[0], predecessors_key[0], node_key);

    for(int i = 0; i<my_filenames.size(); i++){
        // Get file
        char* value;
        int value_length;
        _rpc_force_get(successors_ip[0], my_filenames[i], &value, &value_length);
        FILE* file_descriptor = open_file_in_folder(my_filenames[i], main_folder_name, "wb");
        fwrite(value, value_length, 1, file_descriptor);
        fclose(file_descriptor);
        free(value);

        // Delete last replica
        if(successors_ip[DHT_FAULT_TOLERANCE_LEVEL].s_addr != node_ip.s_addr){
            _rpc_force_delete(successors_ip[DHT_FAULT_TOLERANCE_LEVEL], my_filenames[i], false);
        }
    }

    // Get files that would have been replicated in the node, and delete extra replicas
    for(int node_i = 0; node_i<DHT_FAULT_TOLERANCE_LEVEL; node_i++){
        if(predecessors_ip[node_i].s_addr == node_ip.s_addr) break;
        
        std::vector<char*> my_replicas_names = rpc_get_keys_in_range(predecessors_ip[node_i], predecessors_key[node_i+1], predecessors_key[node_i]);
        for(int i = 0; i<my_replicas_names.size(); i++){
            // Get file
            char* value;
            int value_length;
            _rpc_force_get(predecessors_ip[node_i], my_replicas_names[i], &value, &value_length);
            FILE* file_descriptor = open_file_in_folder(my_replicas_names[i], main_folder_name, "wb");
            fwrite(value, value_length, 1, file_descriptor);
            fclose(file_descriptor);
            free(value);

            // Delete last replica
            if(successors_ip[DHT_FAULT_TOLERANCE_LEVEL].s_addr != node_ip.s_addr && successors_ip[DHT_FAULT_TOLERANCE_LEVEL-1-node_i].s_addr != predecessors_ip[node_i].s_addr){
                _rpc_force_delete(successors_ip[DHT_FAULT_TOLERANCE_LEVEL-1-node_i], my_replicas_names[i], false);
            }
        }
    }

    // Heart beat accept
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, heart_beat_accept_thread, NULL);

    // For the predecessors, change list of succesors, and vice versa
    for(int i = 0; i<l_count; i++){
        if(predecessors_ip[i].s_addr != node_ip.s_addr){
            rpc_insert_succesor(predecessors_ip[i], node_ip, node_key);
        }

        if(successors_ip[i].s_addr != node_ip.s_addr){
            rpc_insert_predecessor(successors_ip[i], node_ip, node_key);
        }
    }

    // Heart beat send
    pthread_create(&thread_id, NULL, heart_beat_send_thread, NULL);
}


struct ip_and_port{
    char* ip;
    int port;
};
void* ping_dht_nodes(void* raw_args){
    ip_and_port args = *((ip_and_port*)raw_args);
    
    // Open a UDP socket
    int sock = socket(PF_INET, SOCK_DGRAM, 0);

    sockaddr_in sock_addr;
    sock_addr.sin_family = PF_INET;
    sock_addr.sin_port = htons(0); // Use the first free port
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY); // bind socket to any interface
    bind(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr));

    // Set destination multicast address
    sockaddr_in mcast_addr;
    mcast_addr.sin_family = PF_INET;
    mcast_addr.sin_addr.s_addr = inet_addr(args.ip);
    mcast_addr.sin_port = htons(args.port);
    
    while(true){
        // send packet
        int buffer[1] = {256};
        sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&mcast_addr, sizeof(mcast_addr));
        usleep(50);
    }

}

in_addr get_self_ip(){
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    ifreq ifr;
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
    ioctl(sock, SIOCGIFADDR, &ifr);
    close(sock);

    in_addr ip = ((sockaddr_in*)&ifr.ifr_addr)->sin_addr;
    return ip;
}

bool i_am_predecessor_of_key(int key){
    if(successors_ip[0].s_addr == node_ip.s_addr) return true;
    if(node_key <= successors_key[0]){
        if(node_key < key && key <= successors_key[0]) return true;
    }else{
        if(node_key < key || key <= successors_key[0]) return true;
    }

    return false;
}



void test_main(char* ip, int port){
    in_addr dht_node_ip = get_ip_from_multicast(ip, port);

    const char* test_key = "a.txt";
    const char* test_value = "lolazo mannn";
    printf("hash: %i\n", hash_string(test_key));
    rpc_add(dht_node_ip, test_key, test_value, strlen(test_value));

    test_key = "a";
    test_value = "lolazo mannsn";
    printf("hash: %i\n", hash_string(test_key));
    rpc_add(dht_node_ip, test_key, test_value, strlen(test_value));
    //sleep(20);

    //printf("hash: %i\n", hash_string(test_key));
    //rpc_delete(dht_node_ip, test_key);
}

// First argument is the name of the folder in which the files are going to be saved
// Second And Third arguments are multicsat IP and Port of discover port
// Forth argument says if this is the first node in the DHT (Y or N)
// Fifth argument is the key for the node 
int main(int argc, char** argv){
    printf("I am DHT server %s with key %s\n\n", argv[1], argv[5]);

    printf("%s\n", argv[4]);

    // Set node key and ip, and folder name
    node_key = atoi(argv[5]);
    node_ip = get_self_ip();

    // If key is 999, this is a test node
    if(node_key == 999){
        int port = atoi(argv[3]);
        test_main(argv[2], port);
        return 0;
    }
    
    // Get folder name and make folder
    main_folder_name = argv[1];
    mkdir(main_folder_name, 777);

    // Set default predecessors and successors (the node itself)
    for(int i = 0; i<DHT_FAULT_TOLERANCE_LEVEL+1; i++){
        successors_ip.push_back(node_ip);
        successors_key.push_back(node_key);
    }

    for(int i = 0; i<DHT_FAULT_TOLERANCE_LEVEL+1; i++){
        predecessors_ip.push_back(node_ip);
        predecessors_key.push_back(node_key);
    }

    // Enter the network, and set right predecessors and successors
    if(strcmp(argv[4], "N") == 0){
        int port = atoi(argv[3]);
        join_dht(argv[2], port);
    }else{
        // Heart beat accept
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, heart_beat_accept_thread, NULL);

        // Heart beat send
        pthread_create(&thread_id, NULL, heart_beat_send_thread, NULL);
    }
    
    // Create multicast send thread
    ip_and_port* args = (ip_and_port*) malloc(sizeof(ip_and_port));
    args->ip = argv[2];
    args->port = atoi(argv[3]);
    pthread_t listen_find_thread_id;
    pthread_create(&listen_find_thread_id, NULL, ping_dht_nodes, args);

    // Create the thread that accepts commands
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, command_accept_thread, NULL);

    //Command Execution Loop
    while(true){
        Command cmd = command_queue.dequeue();
        printf("Will execute command: %i\n", cmd.message[0]);
        
        if(cmd.message[0] == CMD_TEST){
            const char* message = "lolazo";
            send_length_then_message(cmd.sock, strlen(message)+1, message);
        }
        
        else if(cmd.message[0] == CMD_FIND_PREDECESSOR){
            int key;
            memcpy(&key, cmd.message+1, 4);

            Node_Reference* node_reference;
            if(i_am_predecessor_of_key(key)){
                node_reference = (Node_Reference*)malloc(sizeof(Node_Reference));
                node_reference->ip = node_ip;
                node_reference->key = node_key;
                node_reference->successor_ip = successors_ip[0];
                node_reference->successor_key = successors_key[0];
            }else{
                node_reference = rpc_find_predecessor(successors_ip[0], key);
            }
            
            send_length_then_message(cmd.sock, sizeof(Node_Reference), (char*)node_reference);
            free(node_reference);
        }

        else if(cmd.message[0] == CMD_INSERT_SUCCESOR){
            
            // Recv ip and key
            in_addr new_ip;
            int new_key = 0;
            recv_to_fill(cmd.sock, (char*)&new_ip, sizeof(in_addr));
            recv_to_fill(cmd.sock, (char*)&new_key, sizeof(int));

            // Find place to put in the new node
            int place = 0;
            if(successors_ip[0].s_addr != node_ip.s_addr){
                for(int i = 0; i<successors_ip.size()-1; i++){
                    if(is_in_between_modular(new_key, successors_key[i], successors_key[i+1])){
                        place = i+1;
                        break;
                    }
                }
            }

            // Insert the new node, shifting the others
            int key_in_hand = new_key;
            in_addr ip_in_hand = new_ip;
            for(int i = place; i<successors_ip.size(); i++){
                int tmp_key = successors_key[i];
                in_addr tmp_ip = successors_ip[i];

                successors_key[i] = key_in_hand;
                successors_ip[i] = ip_in_hand;

                key_in_hand = tmp_key;
                ip_in_hand = tmp_ip;
            }
            
            printf("New succesor list: ");
            print_list(successors_key);
            
            // Send Response
            int resp = 1000;
            send_all(cmd.sock, (char*)&resp, sizeof(int));
        }

        else if(cmd.message[0] == CMD_INSERT_PREDECESSOR){
            
            // Recv ip and key
            in_addr new_ip;
            int new_key = 0;
            recv_to_fill(cmd.sock, (char*)&new_ip, sizeof(in_addr));
            recv_to_fill(cmd.sock, (char*)&new_key, sizeof(int));

            // Find place to put in the new node
            int place = 0;
            if(predecessors_ip[0].s_addr != node_ip.s_addr){
                for(int i = 0; i<predecessors_ip.size()-1; i++){
                    if(is_in_between_modular(new_key, predecessors_key[i+1], predecessors_key[i])){
                        place = i+1;
                        break;
                    }
                }
            }

            // Insert the new node, shifting the others
            int key_in_hand = new_key;
            in_addr ip_in_hand = new_ip;
            for(int i = place; i<predecessors_ip.size(); i++){
                int tmp_key = predecessors_key[i];
                in_addr tmp_ip = predecessors_ip[i];

                predecessors_key[i] = key_in_hand;
                predecessors_ip[i] = ip_in_hand;

                key_in_hand = tmp_key;
                ip_in_hand = tmp_ip;
            }

            printf("New predecessor list: ");
            print_list(predecessors_key);

            // Send Response
            int resp = 1000;
            send_all(cmd.sock, (char*)&resp, sizeof(int));
        }

        else if(cmd.message[0] == CMD_FORCE_ADD){
            
            // Get key, value and should_replicate
            char* key;
            int key_length;
            recv_length_then_message(cmd.sock, &key_length, &key);

            char* value;
            int value_length;
            recv_length_then_message(cmd.sock, &value_length, &value);

            bool should_replicate = 0;
            recv_to_fill(cmd.sock, (char*)&should_replicate, sizeof(bool));

            // Create (if needed) and fill the file
            FILE* file_descriptor = open_file_in_folder(key, main_folder_name, "wb");
            fwrite(value, value_length, 1, file_descriptor);
            fclose(file_descriptor);

            // Create replicas if requested
            if(should_replicate){
                for(int i = 0; i<successors_ip.size()-1; i++){
                    if(successors_ip[i].s_addr == node_ip.s_addr){
                        break;
                    }
                    _rpc_force_add(successors_ip[i], key, value, value_length, false);
                }
            }

            // Send ok response
            char ok_message = 33;
            send_length_then_message(cmd.sock, 1, &ok_message);
            free(value);
            free(key);
        }

        else if(cmd.message[0] == CMD_FORCE_DELETE){
            
            // Get key and should_delete_replicas
            char* key;
            int key_length;
            recv_length_then_message(cmd.sock, &key_length, &key);

            bool should_delete_replicas = 0;
            recv_to_fill(cmd.sock, (char*)&should_delete_replicas, sizeof(bool));

            // Delete key
            char* file_name = get_file_full_path(key, main_folder_name);
            remove(file_name);
            free(file_name);

            // Delete replicas if requested
            if(should_delete_replicas){
                for(int i = 0; i<successors_ip.size()-1; i++){
                    if(successors_ip[i].s_addr == node_ip.s_addr){
                        break;
                    }
                    _rpc_force_delete(successors_ip[i], key, false);
                }
            }

            // Send ok response
            char ok_message = 34;
            send_length_then_message(cmd.sock, 1, &ok_message);
            free(key);
        }

        else if(cmd.message[0] == CMD_FORCE_GET){
            // Get key
            char* key;
            int key_length;
            recv_length_then_message(cmd.sock, &key_length, &key);

            // Get File contents
            FILE* file_descriptor = open_file_in_folder(key, main_folder_name, "rb");
            if(file_descriptor == NULL){
                char nok = 0;
                send_all(cmd.sock, &nok, 1);
            }else{
                char ok = 1;
                send_all(cmd.sock, &ok, 1);
                
                char* contents;
                int size;
                read_entire_file(file_descriptor, &contents, &size);

                send_length_then_message(cmd.sock, size, contents);

                fclose(file_descriptor);
            }

            free(key);
        }

        else if(cmd.message[0] == CMD_GET_ALL_PREDECESSORS){
            for(int i = 0; i<predecessors_ip.size(); i++){
                send_all(cmd.sock, (char*) &(predecessors_ip[i]), sizeof(in_addr));
            }
            for(int i = 0; i<predecessors_key.size(); i++){
                send_all(cmd.sock, (char*) &(predecessors_key[i]), sizeof(int));
            }
        }

        else if(cmd.message[0] == CMD_GET_ALL_SUCCESSORS){
            for(int i = 0; i<successors_ip.size(); i++){
                send_all(cmd.sock, (char*) &(successors_ip[i]), sizeof(in_addr));
            }
            for(int i = 0; i<successors_key.size(); i++){
                send_all(cmd.sock, (char*) &(successors_key[i]), sizeof(int));
            }
        }

        else if(cmd.message[0] == CMD_GET_KEYS_IN_RANGE){
            
            // Receive a and b
            int a;
            recv_to_fill(cmd.sock, (char*)&a, sizeof(int));
            int b;
            recv_to_fill(cmd.sock, (char*)&b, sizeof(int));

            // Get files that belongto the range
            dirent** all_file_names;
            int file_count = scandir(main_folder_name, &all_file_names, NULL, alphasort);

            // Filter the files
            std::vector<char*> right_file_names;
            for(int i = 0; i<file_count; i++){
                if(strcmp(all_file_names[i]->d_name, ".") != 0 && strcmp(all_file_names[i]->d_name, "..") != 0){
                    
                    // If file name is in between, add it
                    int h = hash_string(all_file_names[i]->d_name);
                    if(is_in_between_modular(h, a, b)){
                        right_file_names.push_back(all_file_names[i]->d_name);
                    }
                }
            }

            // Send wanted file names
            int s = right_file_names.size();
            send_all(cmd.sock, (char*)(&s), sizeof(int));

            for(int i = 0; i<right_file_names.size(); i++){
                send_length_then_message(cmd.sock, strlen(right_file_names[i])+1, right_file_names[i]);
            }
        }
        else if(cmd.message[0] == CMD_DEPRECATE_PREDECESSORS){
            
            // Get
            in_addr pred_1;
            recv_to_fill(cmd.sock, (char*)&pred_1, sizeof(in_addr));
            in_addr pred_2;
            recv_to_fill(cmd.sock, (char*)&pred_2, sizeof(in_addr));

            int c = 0;
            // Deprecate pred 1
            int i_i = -1;
            for(int i = 0; i<predecessors_ip.size(); i++){
                if(predecessors_ip[i].s_addr == pred_1.s_addr){
                    i_i = i;
                    break;
                }
            }

            if(i_i != -1){
                c += 1;
                shift_left_ereasing_index(&predecessors_ip, i_i, node_ip);
                shift_left_ereasing_index(&predecessors_key, i_i, node_key);
            }

            // Send response
            send_all(cmd.sock, (char*)&c, sizeof(int));
            
            printf("-New predecessor list: ");
            print_list(predecessors_key);
        }

        else if(cmd.message[0] == CMD_DEPRECATE_SUCCESORS){
            
            // Get
            in_addr succ_1;
            recv_to_fill(cmd.sock, (char*)&succ_1, sizeof(in_addr));
            in_addr succ_2;
            recv_to_fill(cmd.sock, (char*)&succ_2, sizeof(in_addr));

            int c = 0;
            // Deprecate pred 1
            int i_i = -1;
            for(int i = 0; i<successors_ip.size(); i++){
                if(successors_ip[i].s_addr == succ_1.s_addr){
                    i_i = i;
                    break;
                }
            }

            if(i_i != -1){
                c += 1;
                shift_left_ereasing_index(&successors_ip, i_i, node_ip);
                shift_left_ereasing_index(&successors_key, i_i, node_key);
            }

            // Send response
            send_all(cmd.sock, (char*)&c, sizeof(int));

            printf("-New succesors list: ");
            print_list(successors_key);
        }

        std::cout << "Command " << (int)(cmd.message[0]) << " finished" << std::endl;
    }
}