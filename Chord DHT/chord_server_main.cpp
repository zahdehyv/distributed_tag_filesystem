#include <stdio.h>
#include "globals.h"
#include <stdlib.h>
#include <unistd.h>
#include "rpcs.h"
#include <net/if.h>
#include <sys/ioctl.h>

int node_key = 0;
in_addr node_ip = {0};
int successor_key = 0;
in_addr successor_ip = {0};
int predecessor_key = 0;
in_addr predecessor_ip = {0};

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



struct ip_and_port{
    char* ip;
    int port;
};
void join_dht(char* ip, int port){

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
    printf("lol, recv %i from %s\n", dummy, inet_ntoa(ipv4Addr->sin_addr));

    //Find predecesor and insert itself into the network
    Node_Reference* pred_reference = rpc_find_predecessor(ipv4Addr->sin_addr, node_key);
    predecessor_ip = pred_reference->ip;
    predecessor_key = pred_reference->key;
    successor_ip = pred_reference->successor_ip;
    successor_key = pred_reference->successor_key;

    rpc_change_succesor(predecessor_ip, node_ip, node_key);
    rpc_change_predecessor(successor_ip, node_ip, node_key);

    close(udp_sock);
}


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
        usleep(300);
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
    if(successor_ip.s_addr == node_ip.s_addr) return true;
    if(node_key <= predecessor_key){
        if(node_key < key && predecessor_key > key) return true;
    }else{
        if(node_key < key || predecessor_key > key) return true;
    }

    return false;
}

// First argument is the name of the folder in which the files are going to be saved
// Second And Third arguments are broadcast IP and Port of discover port
// Fourth And Fifth arguments are IP and Port of a known chord node command port
// Sixth argument says if this is the first node in the DHT (Y or N)
// Seventh argument is the key for the node 
int main(int argc, char** argv){
    
    printf("%s\n", argv[6]);

    // Set node key and ip
    node_key = strtol(argv[7], NULL, 10);
    node_ip = get_self_ip();

    // Enter the network, set predecessor and successor
    if(strcmp(argv[6], "N") == 0){
        int port = strtol(argv[3], NULL, 10);
        join_dht(argv[2], port);
    }else{
        predecessor_ip = node_ip;
        predecessor_key = node_key;
        successor_ip = node_ip;
        successor_key = node_key;
    }
    
    // Create multicast send thread
    ip_and_port* args = (ip_and_port*) malloc(sizeof(ip_and_port));
    args->ip = argv[2];
    args->port = strtol(argv[3], NULL, 10);
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
            char* message = "lolazo";
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
                node_reference->successor_ip = successor_ip;
                node_reference->successor_key = successor_key;
            }else{
                node_reference = rpc_find_predecessor(successor_ip, key);
            }
            
            send_length_then_message(cmd.sock, sizeof(Node_Reference), (char*)node_reference);
            free(node_reference);
        }

        else if(cmd.message[0] == CMD_CHANGE_SUCCESOR){
            memcpy(&successor_ip, cmd.message+1, sizeof(in_addr));
            memcpy(&successor_key, cmd.message+1+sizeof(in_addr), sizeof(int));
            char ok_message = 14;
            send_length_then_message(cmd.sock, 1, &ok_message);
            printf("New Succesor is of key %i\n", successor_key);
        }

        else if(cmd.message[0] == CMD_CHANGE_PREDECESSOR){
            memcpy(&predecessor_ip, cmd.message+1, sizeof(in_addr));
            memcpy(&predecessor_key, cmd.message+1+sizeof(in_addr), sizeof(int));
            char ok_message = 13;
            send_length_then_message(cmd.sock, 1, &ok_message);
            printf("New Predecessor is of key %i\n", predecessor_key);
        }

    }
}