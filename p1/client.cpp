#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <string>
#include <iostream>
#include <signal.h>

#include "room.hpp"
#include "manual.hpp"



using namespace std;

const char* IP;
int PORT = 0;

int connectServer(int port, const char ip[], int sock_type) {
    int fd;
    struct sockaddr_in server_address;
    
    fd = socket(AF_INET, sock_type, 0);
    
    server_address.sin_family = AF_INET; 
    server_address.sin_port = htons(port); 
    server_address.sin_addr.s_addr = inet_addr(ip);

    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) { 
        printf("Error in connecting to server\n");
    }
    printf("Successfully connected!\n");
    return fd;
}

int create_boradcast_socket(struct sockaddr_in& bc_address)
{
    char* ip = (char*) BROADCAST_IP;
    int port = BROADCAST_PORT;
    int bc_fd, broadcast = 1, opt = 1;
    char buffer[BUFF_SIZE] = {0};

    bc_fd = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(bc_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(bc_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET; 
    bc_address.sin_port = htons(port); 
    bc_address.sin_addr.s_addr = inet_addr(ip);

    if (bind(bc_fd, (struct sockaddr *)&bc_address, sizeof(bc_address)) == -1)
        perror("FAILED: Couldn't bind broadcast port for player!\n");
    
    return bc_fd;
}

void alarm_handler(int sig)
{
    return;
}

void go_to_room(int room_port, string username)
{
    int client_fd_with_room = connectServer(room_port, IP, SOCK_STREAM);
    char buffer[BUFF_SIZE] = {0};

    
    
    recv(client_fd_with_room, buffer, BUFF_SIZE, 0);//receive please choose message
    write(1, buffer, strlen(buffer));
    write(1, "\n", 1);
    memset(buffer, 0, BUFF_SIZE);

    signal(SIGALRM, alarm_handler);
    siginterrupt(SIGALRM, 1);
    
    alarm(10);
    read(0, buffer, BUFF_SIZE);//get choice from stdin
    alarm(0);

    username = DELIMITER + username;
    strcat(buffer, username.c_str());
    send(client_fd_with_room, buffer, strlen(buffer), 0);
    memset(buffer, 0, BUFF_SIZE);

    recv(client_fd_with_room, buffer, BUFF_SIZE, 0);//get result of match
    write(1, buffer, strlen(buffer));
    write(1, "\n", 1);
    memset(buffer, 0, BUFF_SIZE);
}

void recieve_free_rooms(int fd)
{
    char buffer[BUFF_SIZE] = {0};
    recv(fd, buffer, BUFF_SIZE, 0);
    write(1, "free rooms are as follows: ", 27);
    write(1, buffer, strlen(buffer));
    write(1, "\n", 1);
    memset(buffer, 0, BUFF_SIZE);
}

void send_ack(int fd)
{
    char buff[BUFF_SIZE] = {0};
    memset(buff, 0, BUFF_SIZE);
    sprintf(buff, "ack");
    send(fd, buff, strlen(buff), 0);
}

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        exit(EXIT_FAILURE);
    }

    IP = argv[1];
    PORT = atoi(argv[2]);

    int server_fd, new_socket, max_sd;
    fd_set master_set, working_set;
    FD_ZERO(&master_set);
    
    int fd, bd_fd;
    char buff[BUFF_SIZE] = {0};
    string username;
    
    fd = connectServer(PORT, IP, SOCK_STREAM);
    struct sockaddr_in bc_address;
    bd_fd = create_boradcast_socket(bc_address);

    max_sd = bd_fd;
    FD_SET(bd_fd, &master_set);

    strcpy(buff, "player");
    send(fd, buff, strlen(buff), 0);//introduce player or room
    memset(buff, 0, BUFF_SIZE);
    
    recv(fd, buff, BUFF_SIZE, 0);
    write(1, buff, strlen(buff));//receive enter ur room
    write(1, "\n", 1);
    memset(buff, 0, BUFF_SIZE);

    read(0, buff, BUFF_SIZE);
    username = buff;
    send(fd, buff, strlen(buff), 0);//send name
    memset(buff, 0, BUFF_SIZE);
        

    while (1) 
    {   
        recieve_free_rooms(fd);

        memset(buff, 0, BUFF_SIZE);
        read(0, buff, BUFF_SIZE);
        buff[strlen(buff) - 1] = '\0';
        send(fd, buff, strlen(buff), 0);//send selected room
        memset(buff, 0, BUFF_SIZE);

        recv(fd, buff, BUFF_SIZE, 0);//receive result to your room select
        string buff_str(buff);
        if(buff_str == "full")
        {
            send_ack(fd);            
            write(1, "You can not enter this room! Select again: ", 43);
            write(1, "\n", 1);
            continue;
        }
        else
        {
            go_to_room(stoi(buff_str), username);
            send_ack(fd);
        }
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);
        for (int i = 0; i <= max_sd; i++) 
        {
            if (FD_ISSET(i, &working_set)) 
            {
                if (i == bd_fd) 
                {
                    recv(bd_fd, buff, BUFF_SIZE, 0);
                    write(1, buff, strlen(buff));
                    write(1, "\n", 1);
                    memset(buff, 0, BUFF_SIZE);
                }
                
            }
        }

    }
}