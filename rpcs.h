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
unsigned char CMD_FORCE_ADD = 4;
unsigned char CMD_FORCE_DELETE = 5;
unsigned char CMD_FORCE_GET = 6;

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
Node_Reference* rpc_find_predecessor(in_addr ip, int key, char* kdbg = 0){
    int sock = connect_to_sever_by_ip(ip, DHT_COMMAND_PORT);
    // Send request
    int request_size = (1+4)*sizeof(char);
    char* request = (char*)malloc(request_size);
    request[0] = CMD_FIND_PREDECESSOR;
    memcpy(request+1, &key, 4);

    send_length_then_message(sock, request_size, request);
    free(request);
    
    // Receive Response
    Node_Reference* response = 0;
    int response_size = 0;
    recv_length_then_message(sock, &response_size, (char**)&response);

    close(sock);
    return response;
}

//Note: only ip and key will be correct in the return of this function
Node_Reference* rpc_find_succesor(in_addr ip, int key, char* kdbg = 0){
    
    Node_Reference* predecessor = rpc_find_predecessor(ip, key, kdbg);
    Node_Reference* successor = (Node_Reference*)malloc(sizeof(Node_Reference)+10);
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

    free(request);

    // Receive response
    char* response = 0;
    int response_size = 0;
    recv_length_then_message(sock, &response_size, (char**)&response);
}

void rpc_change_succesor(in_addr ip, in_addr new_succ, int key){
    _rpc_change_pred_or_succ(ip, new_succ, key, CMD_CHANGE_SUCCESOR);
}

void rpc_change_predecessor(in_addr ip, in_addr new_succ, int key){
    _rpc_change_pred_or_succ(ip, new_succ, key, CMD_CHANGE_PREDECESSOR);
}

void _rpc_force_add(in_addr ip, char* key, char* value, int value_size){
    int sock = connect_to_sever_by_ip(ip, DHT_COMMAND_PORT);
    
    // Send Request
    send_length_then_message(sock, 1, (char*)&CMD_FORCE_ADD);

    // Send key
    send_length_then_message(sock, strlen(key)+1, key);

    // Send value
    send_length_then_message(sock, value_size, value);

    // Receive response
    char* response = 0;
    int response_size = 0;
    recv_length_then_message(sock, &response_size, (char**)&response);
}
void rpc_add(in_addr ip, char* key, char* value, int value_size){
    int h = hash_string(key);
    Node_Reference* node_ref = rpc_find_succesor(ip, h);

    _rpc_force_add(node_ref->ip, key, value, value_size);

    free(node_ref);
}

void _rpc_force_delete(in_addr ip, char* key){
    int sock = connect_to_sever_by_ip(ip, DHT_COMMAND_PORT);
    
    // Send Request
    send_length_then_message(sock, 1, (char*)&CMD_FORCE_DELETE);

    // Send key
    send_length_then_message(sock, strlen(key)+1, key);

    // Receive response
    char* response = 0;
    int response_size = 0;
    recv_length_then_message(sock, &response_size, (char**)&response);
}
void rpc_delete(in_addr ip, char* key){
    int h = hash_string(key);
    Node_Reference* node_ref = rpc_find_succesor(ip, h);

    _rpc_force_delete(node_ref->ip, key);

    free(node_ref);
}

void _rpc_force_get(in_addr ip, char* key, char** response, int* response_size){
    int sock = connect_to_sever_by_ip(ip, DHT_COMMAND_PORT);
    
    // Send Request
    send_length_then_message(sock, 1, (char*)&CMD_FORCE_GET);

    // Send key
    send_length_then_message(sock, strlen(key)+1, key);

    // Receive response
    bool contains;
    recv_to_fill(sock, (char*)&contains, 1);

    *response = 0;
    *response_size = 0;
    if(contains){
        recv_length_then_message(sock, response_size, response);
    }
}
void rpc_get(in_addr ip, char* key, char** response, int* response_size){
    int h = hash_string(key);
    
    Node_Reference* node_ref = rpc_find_succesor(ip, h, key);
    
    _rpc_force_get(node_ref->ip, key, response, response_size);

    free(node_ref);
}

std::vector<char*> rpc_get_in_list(in_addr ip, char* key){
    char* file_raw = 0;
    int file_raw_size = 0;
    rpc_get(ip, key, &file_raw, &file_raw_size);
    std::vector<char*> list = file_lines_to_list(file_raw, file_raw_size);
    free(file_raw);
    return list;
}

void rpc_add_in_list(in_addr ip, char* key, std::vector<char*> value){
    char* file_raw = 0;
    int file_raw_size = 0;
    list_to_file_lines(value, &file_raw, &file_raw_size);
    rpc_add(ip, key, file_raw, file_raw_size);
    free(file_raw);
}


