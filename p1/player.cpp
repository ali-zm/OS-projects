#if !defined(_player_)
#define _player_

#include <string>
#include "player.hpp"
#include <iostream>

using namespace std;

void Player::update_matches(string result)
{
    if(result=="draw")
        this->draws++;
    else if (result=="win")
        this->wins++;
    else if (result=="loose")
        this->losses++;
    finished = true;
}

#endif
