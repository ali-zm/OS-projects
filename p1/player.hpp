#if !defined(_playerh_)
#define _playerh_

#include <string>
#include <vector>

using namespace std;

class Player
{
public:
    Player(int _fd): last_fd(_fd) {}

    int get_id(){return id;}
    int get_last_fd(){return last_fd;}
    bool get_valid(){return valid;}
    bool get_finished(){return finished;}
    void set_finished(bool b){finished=b;}
    int get_wins(){return wins;}
    
    string get_name(){return name;}
    void set_valid(bool v){valid=v;}
    void set_id(int _id){id=_id;}
    void set_name(string _name){name=_name;}


    void update_matches(string result);


private:
    int last_fd;
    int id;
    string name;
    bool valid=false;
    int wins=0;
    int draws=0;
    int losses=0;
    bool finished = false;
    
};


#endif