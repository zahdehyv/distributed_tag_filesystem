// C++ program to illustrate the client application in the
// socket programming
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

int main()
{
    
    // creating socket
    addrinfo hints;
    addrinfo *servinfo;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = 0;     // fill in my IP for me
    printf("%d\n", getaddrinfo("juajo", "8080", &hints, &servinfo));
    
    int clientSocket = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    // connect!
    
    connect(clientSocket, servinfo->ai_addr, servinfo->ai_addrlen);

    // sending data
    const char* message = "Hello, server!";
    send(clientSocket, message, strlen(message), 0);

    // closing socket
    close(clientSocket);

    return 0;
}
