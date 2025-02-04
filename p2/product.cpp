#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <sys/wait.h>
#include <vector>
#include <bits/stdc++.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <algorithm>
#include "logger.hpp"
#include "manual.hpp"

using namespace std;

Logger logger;

float value = 0;
int amount = 0;

int number_of_str_in_list(string str, vector<string> ls)
{
    int count = 0;
    for(int i=0; i<ls.size(); i++)
        if(ls[i]==str)
            count++;

    return count;
}

vector <string> tokenize_m(string inp_str,char delimiter)
{
    stringstream ss(inp_str); 
    string s; 
    vector <string> str;
    while (getline(ss, s, delimiter)) {    
        str.push_back(s);
    }
    return str; 
}

vector<string> get_commands(int &read_fd){
    char buf [100] ={0}; 
    read(read_fd , buf , 100);
    close(read_fd);
    vector <string> commands = tokenize_m(string(buf),',');
    return commands;
}

string get_message_through_pipe(string pipe_path)
{
    int fd = open(pipe_path.c_str(), O_RDONLY);
    if (fd == -1) {
        perror("open");
        return "\0";
    }

    // Read data from the FIFO
    char buffer[128] = {0};
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if ((bytesRead == -1)) {
        perror("read");
        close(fd);
        return "\0";
    }
    close(fd);
    return string(buffer);

}

void update_amount_value(vector<string> messages_read_from_pipe, int &amount, float &value)
{
    for(int i=0; i<messages_read_from_pipe.size(); i++)
    {
        if(messages_read_from_pipe[i] == "End" || messages_read_from_pipe[i]=="\0")
            continue;
        vector<string> update_data = tokenize_m(messages_read_from_pipe[i], ',');
        amount += stoi(update_data[0]);
        value += stof(update_data[1]);
    }
}

void update_data(string store, int quantity, double val)
{
        value += val;
        amount += quantity;
}

string join_by_delimiter(vector <string> strs, char delimiter)
{
	string result = strs[0];
	for (size_t i=1; i< strs.size(); i++)
    {
		result = result + delimiter + strs[i];
    }
    return result;
}

void read_data_named_pipe(string fifo_name, int store_count)
{
    unordered_map<string, pair<int, double>> remaining_data;
    while (access(fifo_name.c_str(), F_OK) == -1)
    {
        this_thread::sleep_for(chrono::milliseconds(500));
    }
    int fifo_fd = open(fifo_name.c_str(), O_RDONLY);
    if (fifo_fd == -1)
    {
        logger.log("PRODUCT: FILED to open Fifo filed", "red");
    }
    
    logger.log("PRODUCT: Fifo file " + fifo_name +" open", "red");

    int valid_message_recieved = 0;

    while (true)
    {
        char buf[MAX_BUF_SIZE] = {'\0'};

        while (read(fifo_fd, buf, MAX_BUF_SIZE) >= 0 )
        {
            string message = string(buf);
            size_t pos;
            while((pos = message.find('\n')) != string::npos)
            {
                string m = message.substr(0, pos);
                valid_message_recieved ++;
                logger.log("PRODUCT: "+fifo_name+ " recieved message from store" + " : " + m, "red");
                vector<string>row = tokenize_m(m, ',');
                update_data(row[2], stoi(row[0]), stod(row[1]));
                message = message.erase(0, pos+1);
                if (valid_message_recieved == store_count) break;
                
            }
            if (valid_message_recieved == store_count) break;
        }
        if (valid_message_recieved == store_count)
        {
            logger.log("PRODUCT:"+ fifo_name+" recieved all data from all sources" , "red");

            break;
        }   
    }  
}

void send_product_data_to_main(int fd, vector<string> words)
{
    string data = join_by_delimiter(words, ',');
    write(fd , data.c_str(), 100);
    close(fd);
}

int main(int argc, char const *argv[])
{
    int read_fd = atoi(argv[0]);
    int write_fd = atoi(argv[1]);
    vector <string> commands = get_commands(read_fd);
    string path = commands[0];
    string product = commands[1];
    int num_of_stores = stoi(commands[2]);
    read_data_named_pipe((path + NAMED_PIPES_PATH + "/" + product), num_of_stores);
    vector <string> data_to_main = {"product",product,to_string(amount), to_string(value)};
    send_product_data_to_main(write_fd, data_to_main);
    return 0;
}