#ifndef SERVER1_SERVER_H
#define SERVER1_SERVER_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>
#include <cstdio>
#include <cstdlib>
#include <Wincrypt.h>
#include <aclapi.h>


#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")


class Server {
public:
    Server();
    ~Server();

    [[noreturn]] void exec();
private:
    static const int max_clients = 100;

    typedef struct client_ctx {
        int socket;
        CHAR buf_recv[512];             // Буфер приема
        CHAR buf_send[512];             // Буфер отправки
        unsigned int sz_recv;           // Принято данных
        unsigned int sz_send_total;     // Данных в буфере отправки
        unsigned int sz_send;           // Данных отправлено

        // Структуры OVERLAPPED для уведомлений о завершении
        OVERLAPPED overlap_recv;
        OVERLAPPED overlap_send;
        OVERLAPPED overlap_cancel;
        DWORD flags_recv;              // Флаги для WSARecv

        HCRYPTKEY key = 0;      // Ключ по которому переписываемся с данным клиентом

    } client_ctx;

    HANDLE g_io_port;
    SOCKET s;   // Прослушивающий сокет

    // Crypting data:
    HCRYPTPROV csp{};     // контекст
    HCRYPTKEY c_public_key{};

    // Прослушивающий сокет и все сокеты подключения хранятся
    // в массиве структур (вместе с overlapped и буферами)
    client_ctx g_ctxs[max_clients + 1];
    int g_accepted_socket{};


    // Функция добавляет новое принятое подключение клиента
    void add_accepted_connection();

    // Функция стартует операцию приема соединения
    void schedule_accept();


    // Handlers for overlap
    void overlap_recv_handler(int idx, DWORD transferred);
    void overlap_send_handler(int idx, DWORD transferred);
    void overlap_cancel_handler(int idx);


    // Функция стартует операцию чтения из сокета
    void schedule_read(DWORD idx);

    // Функция стартует операцию отправки подготовленных данных в сокет
    void schedule_write(DWORD idx);


    // Crypting:
    inline bool client_send_crypt_key(int idx) { return g_ctxs[idx].buf_recv[0] == 0; }
    void form_answ_key(int idx);

    // Функция тестирования шифрования. Принимает на вход дескриптор ключа, формирует тестовый буфер, шифрует его и расшифровывает
    static bool crypt_test(HCRYPTKEY key);

    void msg_handler(int idx);



    // Requests handlers:
    void os_version_handler(int idx);
    void current_time_handler(int idx);
    void os_time_handler(int idx);
    void memory_status_handler(int idx);
    void disks_types_handler(int idx);
    void free_space_handler(int idx);
    void access_right_handler(int idx);
    void owner_handler(int idx);
};


#endif //SERVER1_SERVER_H
