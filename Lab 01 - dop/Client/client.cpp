#include "client.h"


Client::Client(std::string& ip_port) {
	ip = ip_port.substr(0, ip_port.find(':'));
	port = ip_port.substr(ip_port.find(':') + 1, ip_port.length());

	RPC_STATUS status = RpcStringBindingComposeA(
		(RPC_CSTR)"0f3fc7a7-cb51-437d-8a37-edecd921fdd3", // UUID to bind to.
		(RPC_CSTR)("ncacn_ip_tcp"),				// Use TCP/IP protocol.
		(RPC_CSTR)(ip.c_str()),					// TCP/IP network address to use.
		(RPC_CSTR)(port.c_str()),				// TCP/IP port to use.
		NULL,									// Protocol dependent network options to use.
		&szStringBinding						// String binding output.
	); 
	assert(!status);

	status = RpcBindingFromStringBindingA(szStringBinding, &InterfaceHandle);
	assert(!status);
}

Client::~Client() {
	if (szStringBinding) {
		RpcStringFreeA(&szStringBinding);
	}

	RpcBindingFree(&InterfaceHandle);
}

bool Client::authentification(const char* login, const  char* password)
{
	// Filling up identity struct
	SEC_WINNT_AUTH_IDENTITY_A sec;
	sec.Domain = (unsigned char*)"";
	sec.DomainLength = 0;
	sec.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
	sec.Password = (unsigned char*)password;
	sec.PasswordLength = strlen(password);
	sec.User = (unsigned char*)login;
	sec.UserLength = strlen(login);

	RpcBindingSetAuthInfo(
		InterfaceHandle,
		nullptr,
		RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
		RPC_C_AUTHN_WINNT,
		(RPC_AUTH_IDENTITY_HANDLE)&sec,
		0
	);

	int check = serv_login((unsigned char*)login, (unsigned char*)password);
	if (check) {
		std::cerr << check;
	}

	return check == 0;
}

bool Client::wrong_args(std::string& cmd, std::list<std::string>& t) {
	// ������ �� ���������� ����������

	if (cmd == "osversion" ||
            cmd == "curtime" ||
            cmd == "ostime" ||
            cmd == "memstat" ||
            cmd == "diskstype" ||
            cmd == "freespace") {
		return t.size() != 1;
	}


    if (cmd == "fowner" ||
        cmd == "frights") {
        return t.size() != 2;
    }


	return false;
}

void Client::cmd_loop()
{
	std::string line;
	std::list<std::string> tokens;

	while (true) {
		RPC_STATUS status = RpcMgmtIsServerListening(InterfaceHandle);
		if (status == RPC_S_NOT_LISTENING) {
			std::cerr << "RPC server is not listening" << std::endl;
			break;
		}


		std::cout << "Serv# ";
	
	metka_kostil:
		getline(std::cin, line);

		static int kostil = 0; if (kostil++ == 0) goto metka_kostil;

		if (line.empty()) {
			continue;
		}

		tokens = split(line, ' ');

		std::string& cmd = *tokens.begin();
		
		std::string first_arg;
		std::string second_arg;

		if (wrong_args(cmd, tokens)) {
			std::cerr << "Incorect arguments for " << cmd << " see \"help\"\n";
			continue;
		}

		if (tokens.size() > 1)
			first_arg = *(++tokens.begin());
		if (tokens.size() > 2)
			second_arg = *(++(++tokens.begin()));

		int ret = 0;

		if (cmd == "osversion") {
			Handler::os_version();
		}
		else if (cmd == "curtime") {
            Handler::curtime();
		}
		else if (cmd == "ostime") {
            Handler::ostime();
		}
        else if (cmd == "memstat") {
            Handler::memstat();
        }
        else if (cmd == "diskstype") {
            Handler::diskstype();
        }
        else if (cmd == "freespace") {
            Handler::freespace();
        }
        else if (cmd == "fowner") {
            Handler::fowner(first_arg);
        }
        else if (cmd == "frights") {
            Handler::frights(first_arg);
        }
        else if (cmd == "exit") {
            return;
        }


		else if (cmd == "help") {
			std::cout << "\tosversion" << std::endl;
			std::cout << "\tcurtime" << std::endl;
			std::cout << "\tostime" << std::endl;
		    std::cout << "\tmemstat" << std::endl;
		    std::cout << "\tdiskstype" << std::endl;
		    std::cout << "\tfreespace" << std::endl;
		    std::cout << "\tfowner [path]" << std::endl;
		    std::cout << "\tfrights [path]" << std::endl;

			std::cout << "\texit\n";
		}
		else {
			std::cerr << "Incorrect command\n";
		}

		if (ret != 0) {
			std::cerr << "Error\n";
		}
	}
}


std::list<std::string> Client::split(std::string& s, char c)
{
	std::list<std::string> res;

	size_t i = 0, j = 0;

	while (j != -1) {
		i = j;
		while (i < s.length() && s[i] == c)	i++;
		if (i == s.length()) return res;
		j = s.find(c, i);

		if (j != -1) {
			res.push_back(s.substr(i, j - i));
		}
		else {
			res.push_back(s.substr(i, s.length() - i));
		}
	}

	return res;
}
