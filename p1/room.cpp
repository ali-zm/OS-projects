#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <vector>

#include "room.hpp"
#include "manual.hpp"

using namespace std;


int setupRoomServerBrodcast(int port, sockaddr_in &address) 
{
    int server_fd;
    int broadcast = 1;
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(server_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
   
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("192.168.1.255");
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    
    return server_fd;
}

int setupRoomServerTCP(int port) {
    struct sockaddr_in address;
    int server_fd, broadcast = 1, opt = 1;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    
    listen(server_fd, 2);

    return server_fd;
}

int connectServer(int port, const char ip [], int sock_type) 
{
    int fd;
    struct sockaddr_in server_address;
    
    fd = socket(AF_INET, sock_type, 0);
    
    server_address.sin_family = AF_INET; 
    server_address.sin_port = htons(port); 
    server_address.sin_addr.s_addr = inet_addr(ip);//127.0.0.1

    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) { // checking for errors
        printf("Error in connecting to server\n");
    }
    printf("Succesfully connected!\n");
    return fd;
}

int acceptRoomClient(int server_fd) 
{
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t*) &address_len);

    return client_fd;
}



vector <Room> create_rooms(int num_of_rooms, fd_set &master_set, int &max_sd) 
{
    vector <Room> room_num_and_fd;
    for(int i=0 ; i<num_of_rooms ; i++)
    {
        int room_server_fd;        
        int port=1024+i;        
        room_server_fd = setupRoomServerTCP(port);
        room_num_and_fd.push_back({i, room_server_fd, 0, port,{NOT_DEFINED,NOT_DEFINED}, NOT_DEFINED, NULL});
        FD_SET(room_server_fd, &master_set);
        if(room_server_fd>max_sd)
            max_sd=room_server_fd;
    }
    return room_num_and_fd;
}

