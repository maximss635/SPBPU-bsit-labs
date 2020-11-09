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

int serv_remove(
	/* [string][in] */ const unsigned char* path_file)
{
	int ret = remove((const char*)path_file);
	if (!ret) {
		std::cout << "Remove " << path_file << "\n";
	}

	return ret;
}

int serv_download(
	/* [string][in] */ const unsigned char* path_from,
	/* [size_is][string][out] */ unsigned char* buffer,
	/* [in] */ int buffer_size)
{	
	std::ifstream file_from((char*)path_from, std::ios::binary);
	if (!file_from.is_open()) {
		return -1;
	}
	file_from.read((char*)buffer, buffer_size);
	file_from.close();

	std::cout << "Download " << path_from << "\n";

	return 0;
}

int serv_upload(
	/* [string][in] */ const unsigned char* path_to,
	/* [size_is][in] */ const unsigned char* buffer,
	/* [in] */ int buffer_size)
{
	std::ofstream file_to((char*)path_to, std::ios::binary);
	file_to.write((char*)buffer, buffer_size);
	file_to.close();

	std::cout << "Upload " << path_to << "\n";

	return 0;
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
		login_,
		NULL,
		password_,
		LOGON32_LOGON_INTERACTIVE,
		LOGON32_PROVIDER_DEFAULT,
		&InterfaceHandle
	) && ImpersonateLoggedOnUser(InterfaceHandle);

	if (status) {
		std::cout << "New connection: " << login;

		return 0;
	}
	else return GetLastError();
}

int serv_get_file_size(
	/* [string][in] */ const unsigned char* path_file)
{
	std::fstream file;
	file.open((const char*)path_file);

	if (!file.is_open()) {
		return -1;
	}

	int size = 0;
	file.seekg(0, std::ios::end);
	size = static_cast<int>(file.tellg());
	file.close();

	return size;
}