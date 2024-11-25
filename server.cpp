// C++ program to show the example of server application in
// socket programming
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "globals.h"

using namespace std;

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

int main(){
    int serverSocket = 0;
    int clientSocket = 0; 
    create_server_and_wait_for_client(&serverSocket, &clientSocket);

    // recieving data
    while(1){
        char buffer[COMMAND_MAX_SIZE] = { 0 };
        int err = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (err == -1){ break; }
        cout << "Message from client: " << buffer << endl;
    }


    // closing the socket.
    close(serverSocket);

    return 0;
}