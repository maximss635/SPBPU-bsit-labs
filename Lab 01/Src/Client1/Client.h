#ifndef CLIENT1_CLIENT_H
#define CLIENT1_CLIENT_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wincrypt.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdint>



#pragma comment(lib, "ws2_32.lib")

class Client {
public:
    explicit Client(char* address_port);
    ~Client();

    bool try_connect();
    void request_formation();

    int send_request() const;
    int recv_request();

    void set_key();
    int crypt_test(HCRYPTKEY key);

    const char* get_ip() const { return static_cast<const char *>(ip); }
    int get_port() const { return port; }
    int get_key() const { return session_key; }

    void print_dialog();
    int ask_request_type();

    void decrypt_msg();

    void clear_buf();

private:
    short reconnect_tries;

    struct sockaddr_in addr{};
    char ip[16]{};       // xx.xx.xx.xx
    int port;

    SOCKET socket_;

    WSABUF send_buf, recv_buf;  // Буфер на прием и передачу

    // data for crypting:
    HCRYPTPROV csp;                         // Контекст ключей
    HCRYPTKEY session_key;                  // Ключ для переписки
    HCRYPTKEY key_pair;                     // Пара для получения нижних двух ключей
    HCRYPTKEY private_key, public_key;      // приватный и публичный ключ которые нужны для получения SessionKey

    uint8_t request_type;
    char request_arg[128];

    static unsigned int get_host_ipn(const char* name);

};


#endif //CLIENT1_CLIENT_H
