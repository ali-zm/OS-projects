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
#include "logger.hpp"
#include "manual.hpp"


using namespace std;

Logger logger;

vector <string> find_stores_dir(string path)
{
    vector <string> files;
    vector <string> org_files;
    DIR *dr;
    struct dirent *en;

    dr = opendir(path.c_str());

    if (dr) {
        while ((en = readdir(dr)) != NULL) {
            files.push_back(en->d_name);
        }
        closedir(dr);
    }

    for (size_t i = 0; i< files.size(); i++)
    {
        if ( files[i] != "." && files[i] != ".." && files[i] != "Parts.csv" && files[i]!="named-pipes")
        {
            org_files.push_back(files[i]);
        }
    }

    return org_files;
}

vector<string> read_products(const string &filePath) {
    vector<string> columns;
    ifstream file(filePath);
    string line;

    if (file.is_open()) {
        // Read the first line (first row)
        if (getline(file, line)) {
            stringstream ss(line);
            string column;
            
            // Split the line by commas and store the columns in the vector
            while (getline(ss, column, ',')) {
                columns.push_back(column);
            }
        }
        file.close();
    } else {
        cerr << "Unable to open file: " << filePath << endl;
    }

    return columns;
}

void show_stores(string path){

    vector <string> stores = find_stores_dir(path);
    cout << "The number of stores is: " << stores.size() << endl;
    cout<<"Store names are:"<<endl;
    
    for (size_t i = 0; i < stores.size(); i++)
    {
        cout << stores[i].substr(0, stores[i].length() - CSV_FORMAT_SUFFIX_SIZE);
        if(i != (stores.size()-1))
            cout<<" - ";
        else
            cout << endl;
    }

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

void show_products(vector<string> products)
{
    cout << "Enter products:" << endl;

    for (size_t i = 0; i < products.size(); i++)
    {
        cout << i << " : " <<products[i] << endl; 
    }
}

vector <string> get_chosen_products(vector<string> products)
{
    string temp;
    vector <string> selected_products;
    getline(cin, temp);
    vector <string> chosen_resources = tokenize_m(temp,' ');
    for(int i=0; i<chosen_resources.size(); i++)
        selected_products.push_back(products[stoi(chosen_resources[i])]);

    return selected_products;
}


int create_process(int& write_pipe, int& read_pipe, string executable)
{
    int pipe_fd[2]; // 
    int pipe_fd2[2]; //

    

    if (pipe(pipe_fd) == -1) {
        perror("pipe");
    }
    if (pipe(pipe_fd2) == -1) {
        perror("pipe");
    }

    int pid = fork();

    if (pid == 0) {
        // Child process
        close(pipe_fd[WRITE]);
        close(pipe_fd2[READ]);
        char read_fd[20];
        char write_fd[20];
        sprintf(read_fd , "%d" , pipe_fd[READ]);
        sprintf(write_fd , "%d" , pipe_fd2[WRITE]);
        execl(executable.c_str(), read_fd , write_fd, NULL);
        perror("execl");
    } else if (pid > 0) {
        // Parent process
        close(pipe_fd[READ]);
        close(pipe_fd2[WRITE]);
        read_pipe = pipe_fd2[READ] ;
        write_pipe = pipe_fd[WRITE];
        logger.log("MAIN: Store Forked and PID: " + to_string(pid) + "\n", "green");
    }else{
        perror("fork");
    }
    logger.log("MAIN: Pipe created between store and main!\n" , "green");
    return pid;
}

string encode_data(vector <string> resources, char delimiter)
{
	string result = resources[0];
	for (size_t i=1; i< resources.size(); i++)
    {
		result = result + delimiter + resources[i];
    }
    return result;
}

vector<int> create_stores_process(vector<string> stores_csv_path, vector<string> chosen_parts, vector <int>& child_pids, string path, vector<string> products)
{
    vector<int> read_pipes;
    string coded_chosen_prods = encode_data(chosen_parts, ',');
    string coded_all_prods = encode_data(products, ',');
    for (size_t i = 0; i < stores_csv_path.size(); i++)
    {
        int write_pipe;
        int read_pipe;
        int pid = create_process(write_pipe,read_pipe, STORE_EXE);
        if(pid != 0)
        {
            string data = encode_data({path,stores_csv_path[i], coded_chosen_prods, coded_all_prods}, ';');
            write(write_pipe, data.c_str(), 100);
            logger.log("MAIN: Main threw '" + data + "'through Unnamed pipe into store process\n"  , "green");
            child_pids.push_back(pid);
            close(write_pipe);
            read_pipes.push_back(read_pipe);
        }
    }
    return read_pipes;

}

void removeFolderContents(const std::string& folderPath) 
{
        DIR* dir = opendir(folderPath.c_str());
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string path = folderPath + "/" + entry->d_name;
            if (entry->d_type == DT_DIR) {
                if (std::string(entry->d_name) != "." && std::string(entry->d_name) != "..") {
                    removeFolderContents(path);
                    rmdir(path.c_str());
                }
            } else {
                std::remove(path.c_str());
            }
        }
        closedir(dir);
}

void create_empty_folder(string folderPath)
{
    struct stat info;
    if (stat(folderPath.c_str(), &info) == 0 && (info.st_mode & S_IFDIR))
        removeFolderContents(folderPath);
    else
        mkdir(folderPath.c_str(), 0777);
}


void create_named_pipe(string pipePath)
{
    if (mkfifo(pipePath.c_str(), 0666) == -1) 
    {
        perror("mkfifo");
        exit(1);
    }
}

vector<int> create_products_process(vector<string> product_names, vector <int>& child_pids, string path)
{
    vector<int> read_pipes;
    create_empty_folder(path + NAMED_PIPES_PATH);
    for (size_t i = 0; i < product_names.size(); i++)
    {
        int write_pipe, read_pipe;
        int pid = create_process(write_pipe,read_pipe, PRODUCT_EXE);
        if(pid != 0)
        {
            create_named_pipe(path + NAMED_PIPES_PATH + "/" + product_names[i]);
            logger.log("MAIN: Created named pipe at "+ path + NAMED_PIPES_PATH + "/" + product_names[i] + "\n", "green");

            int num_of_stores = find_stores_dir(path).size();
            string data = path + "," + product_names[i] + "," + to_string(num_of_stores);
            write(write_pipe, data.c_str(), 100);
            close(write_pipe);
            
            child_pids.push_back(pid);
            read_pipes.push_back(read_pipe);
            
            logger.log("MAIN: Main threw '" + data + "'MAIN: through Unnamed pipe into product process"  , "green");
        }
    }
    return read_pipes;

}

bool check_str_in_list(string prod, vector<string> ls)
{
    for(int i=0; i<ls.size(); i++)
        if(ls[i]==prod)
            return true;

    return false;
}

void handle_data_received(vector<string>data_received, int &total_profit, vector <string> chosen_products)
{
    string sender_type = data_received[0];
    if(sender_type==STORE)
    {
        logger.log("MAIN: Profit gained from "+ data_received[1]+": " + data_received[2], "yellow");
        total_profit+=stoi(data_received[2]);   
    }
    else if(sender_type==PRODUCT)
    {
        string product = data_received[1];
        if(check_str_in_list(product, chosen_products))
        {
            logger.log("MAIN: Product "+ product +" amount: "+ data_received[2], "yellow");
            logger.log("MAIN: Product "+ product +" value: "+ data_received[3], "yellow");
        }
    }
    
}

int main(int argc, char const *argv[])
{
    string path = argv[1]; 
    show_stores(path);
    vector<string>stores_csv_path = find_stores_dir(path);
    vector <string> products = read_products(path+ "/Parts.csv");
    show_products(products);
    vector <string> chosen_products = get_chosen_products(products);

    vector <int> child_pids;
    vector<int> read_from_store_fds = create_stores_process(stores_csv_path, chosen_products, child_pids, path, products);
    vector<int> read_from_product_fds = create_products_process(products, child_pids, path);

    vector<int> read_fds;
    read_fds.insert( read_fds.end(), read_from_store_fds.begin(), read_from_store_fds.end() );
    read_fds.insert( read_fds.end(), read_from_product_fds.begin(), read_from_product_fds.end() );

    int max_sd = *max_element(read_fds.begin(), read_fds.end());
    fd_set master_set, working_set;
    FD_ZERO(&master_set);




    for(int i= 0; i<read_from_store_fds.size(); i++)
        FD_SET(read_from_store_fds[i], &master_set);
    for(int i= 0; i<read_from_product_fds.size(); i++)
        FD_SET(read_from_product_fds[i], &master_set);

    int total_profit = 0;
    int count_data_received = 0;
    while (count_data_received < read_fds.size()) 
    {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);
        for (int i = 0; i <= max_sd; i++) 
        {
            if (FD_ISSET(i, &working_set)) 
            {
                char buf [100] ={0}; 
                read(i , buf , 100);
                close(i);
                if (buf[0]=='\0')
                    continue;
                
                vector<string> data_received = tokenize_m(string(buf), ',');
                handle_data_received(data_received, total_profit, chosen_products);
                count_data_received++;
            }
        }

    }
    logger.log("MAIN: Total profit: "+ to_string(total_profit), "yellow");
    return 0;
}
