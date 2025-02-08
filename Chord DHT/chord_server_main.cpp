#include <stdio.h>
#include "globals.h"
#include <stdlib.h>
#include <unistd.h>

int succesor_key;
in_addr succesor_ip;
Threadsafe_Command_Queue command_queue(10000);

int send_length_then_message(int socket, int size, char* message){
    //First send size
    int* size_pointer = &size;
    send_all(socket, (char*)size_pointer, sizeof(int));

    //Then send the actual file
    char* file_contents = (char*) malloc(size*sizeof(char));
    send_all(socket, message, size);
    return 0;
}

/*void* command_accept_thread(void* args){
    //Create Server
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(DHT_COMMAND_PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    //Wait for commands
    while(1){
        listen(serverSocket, 1);
        
        sockaddr ip;
        socklen_t a;
        int clientSocket = accept(serverSocket, &ip, &a);
        sockaddr_in* ipv4Addr = (sockaddr_in*) &ip;
        printf("%s\n", inet_ntoa(ipv4Addr->sin_addr));

        pthread_mutex_lock(&dht_dirs_mutex);
        dht_dirs.push_back(inet_ntoa(ipv4Addr->sin_addr));
        pthread_mutex_unlock(&dht_dirs_mutex);
    }
}*/



struct ip_and_port{
    char* ip;
    int port;
};
void join_dht(char* ip, int port){

    // Create UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in in_addr;
    in_addr.sin_family = AF_INET;
    in_addr.sin_addr.s_addr = INADDR_ANY;
    in_addr.sin_port = htons(port);

    bind(sock, (sockaddr*) &in_addr, sizeof(in_addr));

    // Subscribe to the multicast 
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));

    // Listen for signals of dht nodes
    int dummy;
    sockaddr recv_saddr;
    socklen_t recv_socklen = sizeof(recv_saddr);
    recvfrom(sock, &dummy, sizeof(int), 0, (struct sockaddr *)&recv_saddr, &recv_socklen);
    
    sockaddr_in* ipv4Addr = (sockaddr_in*) &recv_saddr;
    printf("lol, recv %i from %s\n", dummy, inet_ntoa(ipv4Addr->sin_addr));

    //Find predecesor
    struct sockaddr_in address = {};
    address.sin_family = AF_INET;
    inet_pton(AF_INET, inet_ntoa(ipv4Addr->sin_addr), &address.sin_addr);
    address.sin_port = htons(DHT_COMMAND_PORT);

    //close(clientSocket);
    
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
        sleep(3);
    }

}

// First argument is the name of the folder in which the files are going to be saved
// Second And Third arguments are broadcast IP and Port of discover port
// Fourth And Fifth arguments are IP and Port of a known chord node command port
// Sixth argument says if this is the first node in the DHT (Y or N)
int main(int argc, char** argv){
    
    printf("%s\n", argv[6]);

    // Enter the network, find Predecesor and Succesor
    if(strcmp(argv[6], "N") == 0){
        int port = strtol(argv[3], NULL, 10);
        join_dht(argv[2], port);
    }
    
    // Create multicast send thread
    ip_and_port* args = (ip_and_port*) malloc(sizeof(ip_and_port));
    args->ip = argv[2];
    args->port = strtol(argv[3], NULL, 10);
    pthread_t listen_find_thread_id;
    pthread_create(&listen_find_thread_id, NULL, ping_dht_nodes, args);

    
    // Find succesor, predecesor, and send them commands to enter the network

    // Create the thread that accepts commands
    //pthread_t thread_id;
    //pthread_create(&thread_id, NULL, add_dht_node, NULL);

    //Command Execution Loop
    while(true){
        sleep(1);
    }
}