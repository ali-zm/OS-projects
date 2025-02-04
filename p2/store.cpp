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
#include <sys/file.h>

using namespace std;

struct Update_data
{
    string name;
    int amount;
    float value;
};


Logger logger;
string name;

string join_by_delimiter(vector <string> strs, char delimiter)
{
	string result = strs[0];
	for (size_t i=1; i< strs.size(); i++)
    {
		result = result + delimiter + strs[i];
    }
    return result;
}

string trim(const string& str) 
{ 
    size_t first = str.find_first_not_of(" \t\n\r"); size_t last = str.find_last_not_of(" \t\n\r"); return str.substr(first, (last - first + 1)); 
}

vector <string> tokenize_m(string chosen_pos,char delimiter)
{
    stringstream ss(chosen_pos); 
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
    vector <string> commands = tokenize_m(string(buf),';');
    return commands;
}


vector<vector<string>> read_csv(string path) 
{ 
    vector<vector<string>> data;
    ifstream file(path);
    string line;
    while (getline(file, line)) 
    { 
        stringstream ss(line);
        string cell;
        vector<string> row;
        while (getline(ss, cell, ',')) 
        { 
            row.push_back(trim(cell)); 
        } 
        data.push_back(row); 
    }

    file.close(); 
    return data;
}


bool valid_to_extract_product(vector<string> line, string desired_prod_type)
{
    return (line[0]==desired_prod_type) && (line[3]=="input") && (stoi(line[2])>0);
}

Update_data* find_update_data(string product, vector<Update_data> &update_data_ls)
{
    for(int i=0; i<update_data_ls.size() ; i++)
        if(update_data_ls[i].name == product)
            return &update_data_ls[i];
    return NULL;
}

void update_prods_data(vector<vector<string>> data, vector<Update_data> &update_data_ls)
{
    for(int i=0; i<data.size(); i++)
    {
        if(data[i][3] == "input")
        {
            Update_data* cur_update_data = find_update_data(data[i][0], update_data_ls);
            int amount = stoi(data[i][2]);
            float price = stof(data[i][1]);
            float value = amount*price;
            cur_update_data->amount += amount;
            cur_update_data->value += value;
        }
    }
}

int send_message_to_named_pipe(string path, string message)
{
    // Open the FIFO for writing
    int fd = open(path.c_str(), O_WRONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }


    if (write(fd, message.c_str(), strlen(message.c_str())) == -1) {
        perror("write");
        close(fd);
        return 1;
    }
    close(fd);
    return 0;

}

bool check_str_in_list(string prod, vector<string> ls)
{
    for(int i=0; i<ls.size(); i++)
        if(ls[i]==prod)
            return true;

    return false;
}

int find_index_in_update_prod(vector<Update_data> &update_data_ls, string product)
{
    for(int i=0; i<update_data_ls.size(); i++)
        if(update_data_ls[i].name == product)
            return i;
    return -1;
}

void send_updated_data_to_prods(string path,  vector<Update_data> &update_data_ls, vector<string>prods, string file_name)
{
    for(int i=0; i<prods.size(); i++)
    {
        int temp_ind = find_index_in_update_prod(update_data_ls, prods[i]);
        if(temp_ind!=-1)
            send_message_to_named_pipe(path + NAMED_PIPES_PATH + "/" + update_data_ls[temp_ind].name, to_string(update_data_ls[temp_ind].amount)+","+to_string(int(update_data_ls[temp_ind].value))+","+file_name+"\n");
        else
            send_message_to_named_pipe(path + NAMED_PIPES_PATH + "/" + prods[i], "0,0,"+file_name+"\n");

    }
}


int compute_total_profit(string path, string file_name, vector<string> desired_products, vector<string>prods)
{
    
    float profit = 0;
    vector<vector<string>>data = read_csv(path+"/"+file_name);
    vector<Update_data> update_data_ls;
    for (int i = 0; i < data.size(); i++)
    {
        string transaction_type = data[i][3];
        string product = data[i][0];

        Update_data* cur_update_data;
        cur_update_data = find_update_data(product, update_data_ls);
        if(cur_update_data==NULL)
        {
            update_data_ls.push_back({product,0,0});
            cur_update_data = &(update_data_ls.back());
        }


        if (transaction_type == "output")
        {
            
            for(int j=0; j<i; j++)
            {
                if (!valid_to_extract_product(data[j], product))
                    continue;
                
                float cur_sell_price = stof(data[i][1]);
                int cur_sell_amount = stoi(data[i][2]);
                float cur_buy_price = stof(data[j][1]);
                int cur_buy_amount = stoi(data[j][2]);
                if(cur_sell_amount >= cur_buy_amount)
                {
                    if(check_str_in_list(product, desired_products))
                        profit += cur_buy_amount * (cur_sell_price - cur_buy_price);
                    data[i][2] = to_string(cur_sell_amount - cur_buy_amount);
                    data[j][2] = "0";
                }
                else
                {
                    if(check_str_in_list(product, desired_products))
                        profit += cur_sell_amount * (cur_sell_price - cur_buy_price);
                    data[i][2] = "0";
                    data[j][2] = to_string(stoi(data[j][2]) - cur_sell_amount);
                }
            }
        }
    }
    update_prods_data(data, update_data_ls);
    send_updated_data_to_prods(path, update_data_ls, prods, file_name);
    int round_profit = floor(profit+0.5);
    return round_profit;
}

void send_profit_to_main(int fd, vector<string> words)
{
    string data = join_by_delimiter(words, ',');
    write(fd , ("store,"+data).c_str(), 100);
    close(fd);
}

int main(int argc, char const *argv[])
{
    int read_fd = atoi(argv[0]);
    int write_fd = atoi(argv[1]);
    vector <string> commands = get_commands(read_fd);
    string path =  commands[0];
    string file_name = commands[1];
    vector<string> desired_products = tokenize_m(commands[2],',');
    vector <string> all_products = tokenize_m(commands[3],',');
    string city = file_name.substr(0, file_name.length() - CSV_FORMAT_SUFFIX_SIZE);
    int profit = compute_total_profit(path, file_name, desired_products, all_products);
    logger.log("STORE: Profit for stroe in " + city + ": " + to_string(profit), "blue");
    send_profit_to_main(write_fd, {city, to_string(profit)});
    return 0;
}
