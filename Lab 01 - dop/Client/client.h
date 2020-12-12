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
	static bool wrong_args(std::string& cmd, std::list<std::string>& t);

private:
	std::string ip, port; 
	std::list<std::string> split(std::string& s, char c);
	
public:
	class Handler {
	public:
	    static void os_version() {
	        auto* buf = new unsigned char[buf_size];
            serv_os_version(buf_size, buf);
            print_response(buf);

            delete [] buf;
	    }
        static void curtime() {
            auto* buf = new unsigned char[buf_size];
            serv_current_time(buf_size, buf);
            print_response(buf);

            delete [] buf;
        }
        static void ostime() {
            auto* buf = new unsigned char[buf_size];
            serv_os_time(buf_size, buf);
            print_response(buf);

            delete [] buf;
        }
        static void memstat() {
            auto* buf = new unsigned char[buf_size];
            serv_memory_status(buf_size, buf);
            print_response(buf);

            delete [] buf;
        }
        static void diskstype() {
            auto* buf = new unsigned char[buf_size];
            serv_disks_types(buf_size, buf);
            print_response(buf);

            delete [] buf;
        }
        static void freespace() {
            auto* buf = new unsigned char[buf_size];
            serv_free_space(buf_size, buf);
            print_response(buf);

            delete [] buf;
        }
        static void fowner(std::string& path) {
            auto* buf = new unsigned char[buf_size];
            serv_file_owner(buf_size, buf);
            print_response(buf);

            delete [] buf;
        }
        static void frights(std::string& path) {
            auto* buf = new unsigned char[buf_size];
            serv_file_access_right(buf_size, buf);
            print_response(buf);

            delete [] buf;
        }

	private:
        static const int buf_size = 256;

	    static void print_response(unsigned char* buf) {
            int buf_out_size = *((int*)(buf));
            for (int i = 0; i < buf_out_size; ++i) {
                std::cout << (buf + 4)[i];
            }
            std::cout << std::endl;
	    }
	};

};


#endif
