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
#include <iostream>
#include <bits/stdc++.h>
#include <algorithm> // For std::all_of
#include <cctype>    // For std::isdigit

#include "manual.hpp"
#include "room.hpp"
#include "player.hpp"


using namespace std;


int PORT = 0;
const char* IP;
int num_of_rooms=0;

int setupServer(int port, int num_of_rooms) {
    struct sockaddr_in address;
    int server_fd, broadcast = 1, opt = 1;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // setsockopt(server_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    
    listen(server_fd, num_of_rooms*3);

    return server_fd;
}

int acceptClient(int server_fd) {
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t*) &address_len);
    return client_fd;
}

void send_message(int fd, const char msg[])
{
    send(fd, msg, strlen(msg), 0);
}

bool isNumeric(const string& str) {
    return !str.empty() && all_of(str.begin(), str.end(), ::isdigit);
}

void vectorToCharArray(const std::vector<int>& vec, char buffer[], size_t bufferSize) 
{
    size_t offset = 0;
    for (size_t i = 0; i < vec.size() && offset < bufferSize; ++i) 
    {
        int written = std::snprintf(buffer + offset, bufferSize - offset, "%d", vec[i]);
        if (written <= 0 || (offset + written) >= bufferSize) 
        {
            break;
        }
        
        offset += written;
        if (i != vec.size() - 1 && offset < bufferSize - 1) 
        {
            buffer[offset++] = ' ';
        }
    }
    buffer[offset] = '\0'; 
}

void remove_from_players(int fd, vector <Player*> pls)
{
    for(int i=0; i<pls.size(); i++)
        if(pls[i]->get_last_fd()==fd)
            pls.erase(pls.begin()+i);
        
}

void split_by_delimiter(string &str1, string &str2, string buff_str, char c)
{
    int delimiter_index = buff_str.find(c);
    str2 = buff_str.substr(0,delimiter_index);
    str1 = buff_str.substr(delimiter_index+1);
}

Player* get_from_players(int fd, vector <Player*> pls)
{
    for(int i=0; i<pls.size(); i++)
        if(pls[i]->get_last_fd()==fd)
            return pls[i];

    return NULL;        
}

vector <int> get_free_rooms(vector <Room> rooms)
{
    vector <int> result;
    for(int i=0; i<rooms.size(); i++)
        if(rooms[i].num_of_pls_in_room<2)
            result.push_back(rooms[i].number);
    return result;
}

Room* find_room(int room_number, vector <Room> &rooms)
{
    for(int i=0; i<rooms.size(); i++)
        if (rooms[i].number==room_number)
            return &rooms[i];
    return NULL;
}

void send_free_rooms(vector <Room> rooms, int fd)
{
    vector <int> free_rooms = get_free_rooms(rooms);
    char free_rooms_msg[BUFF_SIZE]={0};
    vectorToCharArray(free_rooms, free_rooms_msg, sizeof(free_rooms_msg));
    send(fd, free_rooms_msg, strlen(free_rooms_msg), 0);
    memset(free_rooms_msg, 0, BUFF_SIZE);
}

void handle_player_message(string player_msg, Player* pl, int &count_pls, int server_fd_for_this_pl, vector<Room> &rooms)
{
    if(pl->get_valid())
    {
        if(player_msg=="ack")
        {
            char selected_room_msg[BUFF_SIZE]={0};
            send_free_rooms(rooms, server_fd_for_this_pl);
        }
        else if(isNumeric(player_msg))
        {
            int desired_room = stoi(player_msg);
            vector<int> free_rooms = get_free_rooms(rooms);
            if(count(free_rooms.begin(), free_rooms.end(), desired_room) == 0)
            {
                char selected_room_msg[BUFF_SIZE]={0};
                send_message(server_fd_for_this_pl, "full");
                memset(selected_room_msg, 0, BUFF_SIZE);
            }
            else
            {
                Room* selected_room = find_room(desired_room,rooms);
                char selected_room_msg[BUFF_SIZE]={0};
                vector <int> temp_selected_room;
                temp_selected_room.push_back(selected_room->port);
                vectorToCharArray(temp_selected_room, selected_room_msg, sizeof(selected_room_msg));
                send(server_fd_for_this_pl, selected_room_msg, strlen(selected_room_msg), 0);
                memset(selected_room_msg, 0, BUFF_SIZE);
                selected_room->num_of_pls_in_room++;
                // close(server_fd_for_this_pl);
            }

        }
            
    }
    else
    {
        pl->set_id(count_pls);
        pl->set_valid(true);
        pl->set_name(player_msg);
        count_pls++;

        //send rooms
        send_free_rooms(rooms, server_fd_for_this_pl);

    }
}

Room* get_from_server_rooms(int fd, vector <Room> &rooms)
{
    for(int i=0; i<rooms.size(); i++)
        if(rooms[i].room_fd==fd)
            return &rooms[i];

    return NULL;        
}

int create_new_socket_room_to_server()
{
    int new_socket = connectServer(PORT, IP, SOCK_STREAM);
    char buff[BUFF_SIZE]={0};
    return new_socket;
}

void send_result_to_clients_and_server(int result, Room* room, int fd_second_pl_choosing)
{
    char buff[BUFF_SIZE]={0};
    int new_socket_room_to_server = create_new_socket_room_to_server();
    if(result == DRAW)
    {
        send_message(room->room_clients_fd.first, "Draw!");
        send_message(room->room_clients_fd.second, "Draw!");
    }
    else if((room->room_clients_fd.second == fd_second_pl_choosing) && (result == WON_SECOND_PL_CHOOSING) || //send second fd won
        (room->room_clients_fd.first == fd_second_pl_choosing) && (result == WON_FIRST_PL_CHOOSING))
    {
        send_message(room->room_clients_fd.first, "Lost!");
        send_message(room->room_clients_fd.second, "Won!");
    }
    else //send first won
    {
        send_message(room->room_clients_fd.first, "Won!");
        send_message(room->room_clients_fd.second, "Lost!");
    }


    if(result == DRAW)
    {
        strcpy(buff, "draw$");
        strcat(buff, room->first_pl_choosing.first.c_str());
        strcat(buff, "draw$");
        strcat(buff, room->second_pl_choosing.first.c_str());
        strcat(buff, (to_string(room->number)).c_str());
        send_message(new_socket_room_to_server, buff);
        memset(buff, 0, BUFF_SIZE);
        // sendto(bd_fd, buffer, strlen(buffer), 0,(struct sockaddr *)&bc_address, sizeof(bc_address));
    }
    else if(result == WON_FIRST_PL_CHOOSING)
    {
        strcpy(buff, "win$");
        strcat(buff, room->first_pl_choosing.first.c_str());
        strcat(buff, "loose$");
        strcat(buff, room->second_pl_choosing.first.c_str());
        strcat(buff, (to_string(room->number)).c_str());
        
        send_message(new_socket_room_to_server, buff);
        memset(buff, 0, BUFF_SIZE);
    }
    else if(result == WON_SECOND_PL_CHOOSING)
    {
        strcpy(buff, "loose$");
        strcat(buff, room->first_pl_choosing.first.c_str());
        strcat(buff, "win$");
        strcat(buff, room->second_pl_choosing.first.c_str());
        strcat(buff, (to_string(room->number)).c_str());
        send_message(new_socket_room_to_server, buff);
        memset(buff, 0, BUFF_SIZE);
    }
    

}

void process_result(Room *room, int fd_second_pl_choosing)
{
    int result;
    string first_pl_choosing_choice = room->first_pl_choosing.second;
    if(first_pl_choosing_choice!=EMPTY)
        first_pl_choosing_choice.pop_back();//remove \n
    string second_pl_choosing_choice = room->second_pl_choosing.second;
    if(second_pl_choosing_choice!=EMPTY)
        second_pl_choosing_choice.pop_back();//remove \n
    if(first_pl_choosing_choice==EMPTY && second_pl_choosing_choice==EMPTY)
        result = DRAW;
    else if (first_pl_choosing_choice==EMPTY && second_pl_choosing_choice!=EMPTY)
        result = WON_SECOND_PL_CHOOSING;
    else if (first_pl_choosing_choice!=EMPTY && second_pl_choosing_choice==EMPTY)
        result = WON_FIRST_PL_CHOOSING;
    else
    {
        if(first_pl_choosing_choice == second_pl_choosing_choice)
            result = DRAW;
        else if(((first_pl_choosing_choice == ROCK) && (second_pl_choosing_choice == SCISSORS)) ||
                ((first_pl_choosing_choice ==  SCISSORS) && (second_pl_choosing_choice == PAPER)) ||
                ((first_pl_choosing_choice == PAPER) && (second_pl_choosing_choice == ROCK)))

            result = WON_FIRST_PL_CHOOSING;
        else
            result = WON_SECOND_PL_CHOOSING;
    }
    send_result_to_clients_and_server(result, room, fd_second_pl_choosing);
    
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
        perror("FAILED: Couldn't bind broascast port for player!\n");
    
    return bc_fd;
}

void handle_message_to_room(string username, string choice, Room* room, int fd)
{
    if((room->first_pl_choosing.first==EMPTY) && (room->first_pl_choosing.second==EMPTY))
    {
        room->first_pl_choosing.first = username;
        room->first_pl_choosing.second = choice;
        return;
    }
    else
    {
        room->second_pl_choosing.first = username;
        room->second_pl_choosing.second = choice;
        process_result(room, fd);
    }
}

Room* get_room_by_client_fd(int fd, vector <Room> &rooms)
{
    for(int i=0; i<rooms.size(); i++)
        if(rooms[i].room_clients_fd.first==fd || rooms[i].room_clients_fd.second==fd)
            return &rooms[i];

    return NULL;        
}

Player* find_pl_by_name(string name, vector < Player*> players)
{
    for(int i=0; i<players.size(); i++)
    {
        if(name.back()!='\n')
            name+='\n';
        if(players[i]->get_name()==name)
            return players[i];

    }
    return NULL; 
}

void reset_room(Room* room)
{
    room->num_of_pls_in_room =0;
    room->room_clients_fd = {-1,-1};
    room->first_pl_choosing = {EMPTY,EMPTY};
    room->second_pl_choosing = {EMPTY,EMPTY};
    room->room_main_server_fd = -1;
}

void report_final_result(vector<Player*> pls, int bd_fd, struct sockaddr_in &bc_address)
{
    char buffer[BUFF_SIZE] = {0};
    for(int i=0; i<pls.size(); i++)
    {
        strcat(buffer, pls[i]->get_name().c_str());
        strcat(buffer, " ");
        strcat(buffer, to_string(pls[i]->get_wins()).c_str());
        strcat(buffer, "\n");
    }
    sendto(bd_fd, buffer, strlen(buffer), 0,(struct sockaddr *)&bc_address, sizeof(bc_address));

}

int main(int argc, char const *argv[]) 
{
    if (argc != 4) {
        exit(EXIT_FAILURE);
    }

    IP = argv[1];
    PORT = atoi(argv[2]);
    num_of_rooms = atoi(argv[3]);

    int server_fd, new_socket, max_sd;
    vector <Player*> players;
    char buffer[BUFF_SIZE] = {0};
    fd_set master_set, working_set;
    int count_players=0;
    vector <int> room_to_players_sockets;
       
    server_fd = setupServer(PORT, num_of_rooms);

    struct sockaddr_in bc_address;
    int bd_fd = create_boradcast_socket(bc_address);

    FD_ZERO(&master_set);
    max_sd = server_fd;
    FD_SET(server_fd, &master_set);
    FD_SET(STDIN_FILENO, &master_set);

    write(1, "Server is running\n", 18);

    vector <Room> rooms = create_rooms(num_of_rooms, master_set, max_sd);
    while (1) 
    {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);

        for (int i = 0; i <= max_sd; i++) 
        {
            if (FD_ISSET(i, &working_set)) 
            {
                if (i == server_fd) 
                {  
                    new_socket = acceptClient(server_fd);
                    recv(new_socket, buffer, BUFF_SIZE, 0);
                    string buff_str(buffer); 
                    memset(buffer, 0, BUFF_SIZE);
                    if(buff_str=="player")
                    {
                        players.push_back(new Player(new_socket));
                        send_message(new_socket, "Please enter your name: ");
                        memset(buffer, 0, BUFF_SIZE);
                        
                    }
                    else
                    {
                        string line1, line2, line3, line2temp;
                        string pl1_name, pl2_name;
                        string pl1_result, pl2_result;
                        split_by_delimiter(line2temp, line1, buff_str, '\n');
                        split_by_delimiter(line3, line2, line2temp, '\n');
                        split_by_delimiter(pl1_name, pl1_result, line1, DELIMITER);
                        split_by_delimiter(pl2_name, pl2_result, line2, DELIMITER);
                        Player* pl1 = find_pl_by_name(pl1_name, players);
                        Player* pl2 = find_pl_by_name(pl2_name, players);
                        if(pl1!=NULL)
                            pl1->update_matches(pl1_result);
                        if(pl2!=NULL)
                            pl2->update_matches(pl2_result);
                        string result;
                        if(pl1_result=="draw")
                            result = pl1_name + " draw " + pl2_name;
                        else if(pl1_result=="win")
                            result = pl1_name + " won " + pl2_name;
                        else
                            result =  pl1_name + " lost to " + pl2_name;
                        Room* cur_room = find_room(stoi(line3), rooms);
                        reset_room(cur_room);
                        strcpy(buffer, result.c_str());
                        sendto(bd_fd, buffer, strlen(buffer), 0,(struct sockaddr *)&bc_address, sizeof(bc_address));
                        memset(buffer, 0, BUFF_SIZE);
                    }

                    
                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                    
                }
                else if(get_from_players(i, players)!=NULL)
                { 
                    int bytes_received;
                    bytes_received = recv(i , buffer, BUFF_SIZE, 0);
                    
                    if (bytes_received == 0) 
                    { 
                        printf("client fd = %d closed\n", i);
                        close(i);
                        FD_CLR(i, &master_set);
                        remove_from_players(i, players);
                        continue;
                    }
                    else
                    {
                        string buff_str(buffer);
                        Player* cur_player = get_from_players(i, players);
                        handle_player_message(buff_str, cur_player, count_players, i, rooms);
                    }
                    memset(buffer, 0, BUFF_SIZE);
                }
                else if(get_from_server_rooms(i, rooms)!=NULL)
                {
                    Room *cur_room = get_from_server_rooms(i, rooms);
                    new_socket = acceptClient(i);
                    if(cur_room->room_clients_fd.first == NOT_FOUND)
                        cur_room->room_clients_fd.first = new_socket;
                    else
                    {
                        cur_room->room_clients_fd.second = new_socket;
                        send_message(cur_room->room_clients_fd.first, "choose: rock, paper or scissors");
                        send_message(cur_room->room_clients_fd.second, "choose: rock, paper or scissors");
                        memset(buffer, 0, BUFF_SIZE);
                        
                    }
                    room_to_players_sockets.push_back(new_socket);  

                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                }
                else if(find(room_to_players_sockets.begin(),room_to_players_sockets.end(), i) != room_to_players_sockets.end())
                {
                    recv(i , buffer, BUFF_SIZE, 0);

                    string buff_str(buffer);
                    memset(buffer, 0, BUFF_SIZE);

                    string username, choice;
                    split_by_delimiter(username, choice, buff_str, DELIMITER);

                    Room* cur_room = get_room_by_client_fd(i, rooms);

                    handle_message_to_room(username, choice, cur_room, i);
                }
                else if(i == STDIN_FILENO)
                {
                    read(STDIN_FILENO, buffer, BUFF_SIZE);
                    string buff_str(buffer);
                    if(buff_str=="end_game\n")
                        report_final_result(players, bd_fd, bc_address);
                    // handle user input
                }
            }
        }

    }

    return 0;
}