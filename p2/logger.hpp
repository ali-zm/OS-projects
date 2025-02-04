#ifndef LOGGER
#define LOGGER

#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <sys/wait.h>
#include <vector>

class Logger{
	public:
		Logger();
		void log(std::string message , std::string color);
	private:


};

#endif