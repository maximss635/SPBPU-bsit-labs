#include "client.h"

void* __RPC_USER midl_user_allocate(size_t size)
{
	return malloc(size);
}

void __RPC_USER midl_user_free(void* p)
{
	free(p);
}

int main(int argc, char** argv)
{
	std::string ip_port, login, password;


	std::cout << "Ip:port: ";
	std::cin >> ip_port;
	std::cout << "Login: ";
	std::cin >> login;
	std::cout << "Password: ";
	std::cin >> password;

	Client client(ip_port);

	bool check = client.authentification(login.c_str(), password.c_str());
	if (!check) {
		std::cerr << "Authentification fail";
		return -1;
	}

	client.cmd_loop();

	return 0;
}