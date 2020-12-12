#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <cassert>

#include <windows.h>
#include <VersionHelpers.h>
#include <sddl.h>
#include <aclapi.h>

#include "AppInterface.h"
#pragma comment(lib, "rpcrt4.lib")

class Server {
public:
	Server(const char* port);
	~Server();
	void listen();
};


#endif