#if !defined(_MANUAL_)
#define _MANUAL_

#include <string>
#include <iostream>

using namespace std;

#define NOT_DEFINED -1
#define NOT_FOUND -1
#define BUFF_SIZE 1024

const char DELIMITER = '$';
const string EMPTY = "";
const string ROCK = "rock";
const string PAPER = "paper";
const string SCISSORS = "scissors";

#define WON_FIRST_PL_CHOOSING 1
#define WON_SECOND_PL_CHOOSING -1
#define DRAW 0
#define NUM_OF_ROOMS 4 

const char BROADCAST_IP []= "255.255.255.255";
const int BROADCAST_PORT = 1024;



#endif