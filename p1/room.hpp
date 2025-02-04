#if !defined(_roomh_)
#define _roomh_

#include <vector>
#include <string>

#include "manual.hpp"

using namespace std;

struct Room
{
    int number;
    int room_fd;
    int num_of_pls_in_room;
    int port;
    pair<int,int> room_clients_fd;
    int broadcast_fd;
    struct sockaddr_in* bc_address;

    int room_main_server_fd = -1;

    pair<string,string> first_pl_choosing = {EMPTY,EMPTY};
    pair<string,string> second_pl_choosing = {EMPTY,EMPTY};
};


int setupRoomServer(int port);

int acceptRoomClient(int server_fd);

vector <Room> create_rooms(int num_of_rooms, fd_set &master_set, int &max_sd); 

int connectServer(int port, const char ip [], int sock_type) ;

#endif