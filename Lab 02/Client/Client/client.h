#ifndef CLIENT_H
#define CLIENT_H

#include <cassert>
#include <iostream>
#include <list>
#include <string>
#include <fstream>

#include "AppInterface.h"
#pragma comment(lib, "rpcrt4.lib")

class Client {
public:
	Client(std::string& ip_port);
	~Client();

	bool authentification(const char* login, const  char* password);
	unsigned char* szStringBinding;
	void cmd_loop();
	bool wrong_args(std::string& cmd, std::list<std::string>& t);

private:
	std::string ip, port; 
	std::list<std::string> split(std::string& s, char c);
	
public:
	class Handler {
	public:
		static int remove(std::string& path_file) {
			return serv_remove((const unsigned char*)path_file.c_str());
		}

		static int download(std::string& path_from, std::string& path_to) {
			int file_size = serv_get_file_size((const unsigned char*)path_from.c_str());
			if (file_size == -1) {
				std::cerr << "No such file, ";
				return -1;
			}
			
			unsigned char* buffer = new unsigned char[file_size];

			int ret = serv_download((const unsigned char*)path_from.c_str(), buffer, file_size);
			if (ret) {
				std::cerr << "No such file, ";
				return -1;
			}

			std::ofstream file_to(path_to, std::ios::binary);
			file_to.write((char*)buffer, file_size);
			file_to.close();

			delete[] buffer;

			return ret;
		}

		static int upload(std::string& path_from, std::string& path_to) {
			int file_size = get_file_size(path_from); 
			if (file_size == -1) {
				std::cerr << "No such file, ";
				return -1;
			}

			std::ifstream file_from(path_from, std::ios::binary);
			unsigned char* buffer = new unsigned char[file_size];

			file_from.read((char*)buffer, file_size);
			int ret = serv_upload((unsigned char*)path_to.c_str(), buffer, file_size);

			delete[] buffer;

			return ret;
		}

		static int get_file_size(std::string& path) {
			std::fstream file(path);
			if (!file.is_open()) {
				return -1;
			}

			int size = 0;
			
			file.seekg(0, std::ios::end);
			size = static_cast<int>(file.tellg());
			file.close();
			
			return size;
		}
	};

};


#endif