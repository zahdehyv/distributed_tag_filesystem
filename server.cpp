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

char* execute_command(char* command, int clientSocket){
    return "lolazo";
}

int main(){
    
    // Connect
    int serverSocket = 0;
    int clientSocket = 0; 
    create_server_and_wait_for_client(&serverSocket, &clientSocket);

    // Main Loop
    while(1){
        
        // Get command
        char command[MESSAGE_MAX_SIZE] = { 0 };
        int err = recv(clientSocket, command, sizeof(command), 0);
        if (err == -1 || command[0] == 0){ break; }
        cout << "Message from client: " << command << endl;

        //Process command and respond
        char** tokenized_message = tokenize_command(command);
        char* response = execute_command(command, clientSocket);
        free_token_list(tokenized_message);
        
        send(clientSocket, response, strlen(response), 0);
    }


    // Close the socket
    close(serverSocket);

    return 0;
}