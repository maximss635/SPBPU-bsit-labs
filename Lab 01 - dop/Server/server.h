#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <cassert>

#include "AppInterface.h"
#pragma comment(lib, "rpcrt4.lib")

class Server {
public:
	Server(const char* port);
	~Server();
	void listen();
};


#endif