#pragma once

#include <stdio.h>
#include "globals.h"
#include <stdlib.h>
#include <unistd.h>

// Command Codes
unsigned char CMD_TEST = 0;
unsigned char CMD_FIND_PREDECESSOR = 1;
unsigned char CMD_CHANGE_PREDECESSOR = 2;
unsigned char CMD_CHANGE_SUCCESOR = 3;

char* rpc_hello_world(in_addr ip){
    int sock = connect_to_sever_by_ip(ip, DHT_COMMAND_PORT);
    
    send_length_then_message(sock, 1, (char*)&CMD_TEST);

    char* response = 0;
    int size = 0;
    recv_length_then_message(sock, &size, &response);

    close(sock);
    return response;
}

struct Node_Reference{
    in_addr ip;
    int key;
    in_addr successor_ip;
    int successor_key;
};
Node_Reference* rpc_find_predecessor(in_addr ip, int key){
    int sock = connect_to_sever_by_ip(ip, DHT_COMMAND_PORT);

    // Send request
    int request_size = (1+4)*sizeof(char);
    char* request = (char*)malloc(request_size);
    request[0] = CMD_FIND_PREDECESSOR;
    memcpy(request+1, &key, 4);

    send_length_then_message(sock, request_size, request);

    // Receive Response
    Node_Reference* response = 0;
    int response_size = 0;
    recv_length_then_message(sock, &response_size, (char**)&response);

    printf("Predecessor key is: %i\n", response->key);

    close(sock);
    return response;
}

//Note: only ip and key will be correct in the return of this function
Node_Reference* rpc_find_succesor(in_addr ip, int key){
    Node_Reference* predecessor = rpc_find_predecessor(ip, key);
    Node_Reference* successor = (Node_Reference*)malloc(sizeof(Node_Reference));
    successor->ip = predecessor->successor_ip;
    successor->key = predecessor->successor_key;
    free(predecessor);
    return successor;
}

//Note: change successor and predeccessor kind of assumes the network is quiet
void _rpc_change_pred_or_succ(in_addr ip, in_addr new_succ, int key, char succ_or_pred){
    int sock = connect_to_sever_by_ip(ip, DHT_COMMAND_PORT);

    // Send request
    int request_length = 1+sizeof(in_addr)+sizeof(int);
    char* request = (char*)malloc(request_length);
    request[0] = succ_or_pred;
    memcpy(request+1, &new_succ, sizeof(in_addr));
    memcpy(request+1+sizeof(in_addr), &key, sizeof(int));
    
    send_length_then_message(sock, request_length, request);

    // Receive response
    char* response = 0;
    int response_size = 0;
    recv_length_then_message(sock, &response_size, (char**)&response);
    printf("response from change: %i\n", *response);
}

void rpc_change_succesor(in_addr ip, in_addr new_succ, int key){
    _rpc_change_pred_or_succ(ip, new_succ, key, CMD_CHANGE_SUCCESOR);
}

void rpc_change_predecessor(in_addr ip, in_addr new_succ, int key){
    _rpc_change_pred_or_succ(ip, new_succ, key, CMD_CHANGE_PREDECESSOR);
}