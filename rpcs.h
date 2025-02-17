#pragma once

#include <stdio.h>
#include "globals.h"
#include <stdlib.h>
#include <unistd.h>

// Command Codes
unsigned char CMD_TEST = 0;
unsigned char CMD_FIND_PREDECESSOR = 1;
unsigned char CMD_INSERT_PREDECESSOR = 2;
unsigned char CMD_INSERT_SUCCESOR = 3;
unsigned char CMD_FORCE_ADD = 4;
unsigned char CMD_FORCE_DELETE = 5;
unsigned char CMD_FORCE_GET = 6;

unsigned char CMD_GET_ALL_PREDECESSORS = 7;
unsigned char CMD_GET_ALL_SUCCESSORS = 8;

unsigned char CMD_GET_KEYS_IN_RANGE = 9;

unsigned char CMD_DEPRECATE_PREDECESSORS = 10;
unsigned char CMD_DEPRECATE_SUCCESORS = 11;

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

void _rpc_force_add(in_addr ip, const char* key, const char* value, int value_size, bool should_replicate){
    int sock = connect_to_sever_by_ip(ip, DHT_COMMAND_PORT);
    
    // Send Request
    send_length_then_message(sock, 1, (char*)&CMD_FORCE_ADD);

    // Send key
    send_length_then_message(sock, strlen(key)+1, key);

    // Send value
    send_length_then_message(sock, value_size, value);

    // Send Should Replicate
    send_all(sock, (char*)&should_replicate, sizeof(bool));

    // Receive response
    char* response = 0;
    int response_size = 0;
    recv_length_then_message(sock, &response_size, (char**)&response);
}
void rpc_add(in_addr ip, const char* key, const char* value, int value_size){
    int h = hash_string(key);
    Node_Reference* node_ref = rpc_find_succesor(ip, h);

    _rpc_force_add(node_ref->ip, key, value, value_size, true);

    free(node_ref);
}

void _rpc_force_delete(in_addr ip, const char* key, bool should_delete_replicas){
    int sock = connect_to_sever_by_ip(ip, DHT_COMMAND_PORT);
    
    // Send Request
    send_length_then_message(sock, 1, (char*)&CMD_FORCE_DELETE);

    // Send key
    send_length_then_message(sock, strlen(key)+1, key);

    // Send should_delete_replicas
    send_all(sock, (char*)&should_delete_replicas, sizeof(bool));

    // Receive response
    char* response = 0;
    int response_size = 0;
    recv_length_then_message(sock, &response_size, (char**)&response);
}
void rpc_delete(in_addr ip, const char* key){
    int h = hash_string(key);
    Node_Reference* node_ref = rpc_find_succesor(ip, h);

    _rpc_force_delete(node_ref->ip, key, true);

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

// Only self.ip and self.key are valid on this node references
void __rpc_get_all_predecesors_or_succesors(in_addr ip, in_addr** ips, int** keys, int predecessors_or_succesors){
    int sock = connect_to_sever_by_ip(ip, DHT_COMMAND_PORT);

    // Send Request
    send_length_then_message(sock, 1, (char*)&predecessors_or_succesors);
    
    // Receive the ips
    *ips = (in_addr*)malloc(DHT_FAULT_TOLERANCE_LEVEL+1);
    for(int i = 0; i<DHT_FAULT_TOLERANCE_LEVEL+1; i++){
        recv_to_fill(sock, (char*)&((*ips)[i]), sizeof(in_addr));
    }

    // Receive the keys
    *keys = (int*)malloc(DHT_FAULT_TOLERANCE_LEVEL+1);
    for(int i = 0; i<DHT_FAULT_TOLERANCE_LEVEL+1; i++){
        recv_to_fill(sock, (char*)&((*keys)[i]), sizeof(int));
    }
}

void get_all_predecesors(in_addr ip, in_addr** ips, int** keys){
    __rpc_get_all_predecesors_or_succesors(ip, ips, keys, CMD_GET_ALL_PREDECESSORS);
}

void get_all_succesors(in_addr ip, in_addr** ips, int** keys){
    __rpc_get_all_predecesors_or_succesors(ip, ips, keys, CMD_GET_ALL_SUCCESSORS);
}

//Note: change successor and predeccessor kind of assumes the network is quiet
void _rpc_insert_pred_or_succ(in_addr ip, in_addr new_ip, int key, char succ_or_pred){
    int sock = connect_to_sever_by_ip(ip, DHT_COMMAND_PORT);

    // Send request
    send_length_then_message(sock, 1, (char*)&succ_or_pred);
    
    // Send ip then key
    send_all(sock, (char*)&new_ip, sizeof(in_addr));
    send_all(sock, (char*)&key, sizeof(int));

    // Receive response
    int response = 0;
    recv_to_fill(sock, (char*)&response, sizeof(int));
}

void rpc_insert_succesor(in_addr ip, in_addr new_succ, int key){
    _rpc_insert_pred_or_succ(ip, new_succ, key, CMD_INSERT_SUCCESOR);
}

void rpc_insert_predecessor(in_addr ip, in_addr new_pred, int key){
    _rpc_insert_pred_or_succ(ip, new_pred, key, CMD_INSERT_PREDECESSOR);
}

// Gets the file names (keys) stored in ip, that correspond to the range (a,b]
std::vector<char*> rpc_get_keys_in_range(in_addr ip, int a, int b){
    int sock = connect_to_sever_by_ip(ip, DHT_COMMAND_PORT);

    // Send request
    send_length_then_message(sock, 1, (char*)&CMD_GET_KEYS_IN_RANGE);
    
    // Send a and b
    send_all(sock, (char*)&a, sizeof(int));
    send_all(sock, (char*)&b, sizeof(int));
    
    // Recv file count
    int file_count = 0;
    recv_to_fill(sock, (char*)&file_count, sizeof(int));

    // Recv file names
    std::vector<char*> names;
    for(int i = 0; i<file_count; i++){
        int name_length = 0;
        char* name = 0;
        recv_length_then_message(sock, &name_length, &name);
        names.push_back(name);
    }

    return names;
}

int rpc_deprecate_predecessors(in_addr ip, in_addr pred_1_ip, in_addr pred_2_ip){
    int sock = connect_to_sever_by_ip(ip, DHT_COMMAND_PORT);
    
    // Send request
    send_length_then_message(sock, 1, (char*)&CMD_DEPRECATE_PREDECESSORS);

    // Send directions to deprecate
    send_all(sock, (char*)&pred_1_ip, sizeof(in_addr));
    send_all(sock, (char*)&pred_2_ip, sizeof(in_addr));

    int resp = 0;
    recv_to_fill(sock, (char*)&resp, sizeof(int));
    return resp;
}

int rpc_deprecate_succesors(in_addr ip, in_addr succ_1_ip, in_addr succ_2_ip){
    int sock = connect_to_sever_by_ip(ip, DHT_COMMAND_PORT);
    
    // Send request
    send_length_then_message(sock, 1, (char*)&CMD_DEPRECATE_SUCCESORS);

    // Send directions to deprecate
    send_all(sock, (char*)&succ_1_ip, sizeof(in_addr));
    send_all(sock, (char*)&succ_2_ip, sizeof(in_addr));

    int resp = 0;
    recv_to_fill(sock, (char*)&resp, sizeof(int));
    return resp;
}


