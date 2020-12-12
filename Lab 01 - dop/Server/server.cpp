#include "server.h"

#include <iostream>
#include <string> 
#include <fstream>

Server::Server(const char* port) {
	RPC_STATUS status = RpcServerUseProtseqEpA(
		(RPC_CSTR)"ncacn_ip_tcp",
		RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
		(RPC_CSTR)port,
		NULL
	);
	assert(status == RPC_S_OK);
	
	status = RpcServerRegisterIf2(
		Interface_v1_0_s_ifspec, // Interface to register.
		NULL, // Use the MIDL generated entry-point vector.
		NULL, // Use the MIDL generated entry-point vector.
		RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH, // Forces use of security callback.
		RPC_C_LISTEN_MAX_CALLS_DEFAULT, // Use default number of concurrent calls.
		(unsigned)-1, // Infinite max size of incoming data blocks.
		NULL); // Naive security callback (always allow).
	assert(status == RPC_S_OK);

	status = RpcServerRegisterAuthInfoA(
		NULL,
		RPC_C_AUTHN_WINNT,
		NULL,
		NULL
	); // check client username/password
	assert(status == RPC_S_OK);
}

Server::~Server() {

}

void Server::listen()
{
	std::cout << "Listening" << std::endl;

	RPC_STATUS status = RpcServerListen(
		1, // Recommended minimum number of threads.
		RPC_C_LISTEN_MAX_CALLS_DEFAULT, // Recommended maximum number of threads.
		0); // Start listening now.

	assert(status == RPC_S_OK);
}

// Хранимые процедуры 
handle_t InterfaceHandle = 0;

void serv_os_version(
        int buffer_size,
        /* [size_is][out] */ unsigned char *buffer) {

    std::cout << "Os version request\n";

    strcpy((char*)(buffer + 4), "hello");
    *((int*)buffer) = 5;
}

void serv_current_time(
        int buffer_size,
        /* [size_is][out] */ unsigned char *buffer) {

    std::cout << "Current time request\n";
}

void serv_os_time(
        int buffer_size,
        /* [size_is][out] */ unsigned char *buffer) {

    std::cout << "Os time request\n";
}

void serv_memory_status(
        int buffer_size,
        /* [size_is][out] */ unsigned char *buffer) {

    std::cout << "Memory status request\n";
}

void serv_disks_types(
        int buffer_size,
        /* [size_is][out] */ unsigned char *buffer) {

    std::cout << "Disk types request\n";
}

void serv_free_space(
        int buffer_size,
        /* [size_is][out] */ unsigned char *buffer) {


    std::cout << "Free space request\n";
}

void serv_file_owner(
        int buffer_size,
        /* [size_is][out] */ unsigned char *buffer) {

    std::cout << "File owner request\n";
}

void serv_file_access_right(
        int buffer_size,
        /* [size_is][out] */ unsigned char *buffer) {

    std::cout << "File access right request\n";
}


int serv_login(
        /* [string][in] */ const unsigned char* login,
        /* [string][in] */ const unsigned char* password)
{
    bool status = false;

    wchar_t login_[10];
    wchar_t password_[10];

    int i;
    for (i = 0; i < strlen((char*)login); ++i) {
        login_[i] = login[i];
    }
    login_[i] = 0;

    i = 0;
    for (i = 0; i < strlen((char*)password); ++i) {
        password_[i] = password[i];
    }
    password_[i] = 0;

    status = LogonUser(
            (char*)login,
            NULL,
            (char*)password,
            LOGON32_LOGON_INTERACTIVE,
            LOGON32_PROVIDER_DEFAULT,
            &InterfaceHandle
    ) && ImpersonateLoggedOnUser(InterfaceHandle);

    if (status) {
        std::cout << "New connection: " << login << std::endl;

        return 0;
    }
    else return GetLastError();
}
