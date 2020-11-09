#include "Client.h"

Client::Client(char *address_port) : reconnect_tries(10), request_type(0)  {
    strncpy(ip, strtok_s(address_port, ":", &address_port), 16);
    port = atoi(address_port);

    WSADATA wsa_data;
    int check = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    assert(check == 0);

    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    assert (socket_ != 0);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = get_host_ipn(ip);

    send_buf.buf = (CHAR*)malloc(1024);
    recv_buf.buf = (CHAR*)malloc(1024);

}

Client::~Client() {
    free(send_buf.buf);
    free(recv_buf.buf);

    closesocket(socket_);
}

unsigned int Client::get_host_ipn(const char* name)
{
    struct addrinfo* addr = nullptr;
    unsigned int ip4addr = 0;

    int check = getaddrinfo(name, nullptr, nullptr, &addr);
    if (check == 0) {
        struct addrinfo* cur = addr;
        while (cur) {
            if (cur->ai_family == AF_INET) {
                ip4addr = ((struct sockaddr_in*) cur->ai_addr)->sin_addr.s_addr;
                break;
            }
            cur = cur->ai_next;
        }
        freeaddrinfo(addr);
    }

    return ip4addr;
}

bool Client::try_connect() {
    int check;
    for (int i = 0; i < reconnect_tries; ++i) {
        check = connect(socket_, (struct sockaddr*)&addr, sizeof(addr));
        if (check != 0) {
            // connection fail
            printf("Trying to reconnect... (%d)\n", i + 1);
            Sleep(100);
        }
        else {
            break;
        }
    }

    return check == 0;
}

void Client::request_formation()
{
    if (request_type == 0) {
        // Формируем пакет в виде ключа

        // контекст для ключей
        BOOL check = CryptAcquireContext(&csp, nullptr, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET);
        if (!check) {
            check = CryptAcquireContext(&csp, nullptr, MS_ENHANCED_PROV, PROV_RSA_FULL, (DWORD) NULL);
            assert(check);
        }

        // генерируем пару ключей
        check = CryptGenKey(csp, AT_KEYEXCHANGE, 1024<<16, &key_pair);
        assert(check);
        check = CryptGetUserKey(csp, AT_KEYEXCHANGE, &public_key);
        assert(check);
        check = CryptGetUserKey(csp, AT_KEYEXCHANGE, &private_key);
        assert(check);

        //printf("Generated public key: %lu\n", public_key);
        //printf("Generated private key: %lu\n", private_key);

        // экспортируем публичный ключ в буфер
        send_buf.len = 1024;

        check = CryptExportKey(public_key, (HCRYPTKEY) NULL, PUBLICKEYBLOB, 0, nullptr, &send_buf.len);
        assert(check);

        check = CryptExportKey(public_key, (HCRYPTKEY) NULL, PUBLICKEYBLOB, 0, (BYTE*)send_buf.buf, &send_buf.len);
        assert(check);

        // Добавляем в начало код 0 - т.к. это ключ
        memcpy(send_buf.buf + 1, send_buf.buf, send_buf.len++);
        send_buf.buf[0] = 0;

    }
    else {
        // Формируем пакет в виде сообщения
        // шифруем REQUEST_TYPE на ключе SESSION_KEY и кладем в буфер на отправку SEND_BUF

        memset(send_buf.buf, 0, 1024);
        send_buf.buf[0] = request_type;
        send_buf.len = 1;

        if (request_type == 7 || request_type == 8) {
            memcpy(send_buf.buf + 2, request_arg, strlen(request_arg));
            send_buf.len += strlen(request_arg);
            send_buf.buf[1] = strlen(request_arg);
            send_buf.len++;
        }

        BOOL check = CryptEncrypt(session_key, NULL, TRUE, NULL, (BYTE*)send_buf.buf, (DWORD*)&send_buf.len, 1024);
        assert(check);
    }
}

// Отправляет запрос из SEND_BUF через сокет SOCKET
int Client::send_request() const
{
    int send_bytes = 0;
    int res;

    while (send_bytes < send_buf.len) {
        res = send(socket_, send_buf.buf + send_bytes, send_buf.len - send_bytes, 0);

        if (res < 0) {
            return -1;
        }

        send_bytes += res;
    }

    return 0;
}

// Принимает запрос в RECV_BUF через сокет SOCKET
int Client::recv_request()
{
    recv_buf.len = recv(socket_, recv_buf.buf, 1024, 0);

    return recv_buf.len;
}

// Устанавливает параметры для ключей из RECV_BUF
void Client::set_key()
{
    // Первый символ - код 0, тк это ответ на ключ. Игнорируем его

    BOOL check = CryptImportKey(csp, (const BYTE*)(recv_buf.buf + 1), recv_buf.len - 1, private_key, 0, &session_key);
    assert(check);

//    printf("Session key: %lu\n", session_key);
}


int Client::crypt_test(HCRYPTKEY key)
{
   // printf("\n******** Crypt test ********\n");

    int len = 16;
    BYTE test_buf[128] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    BYTE buf_0[128];

    memcpy(buf_0, test_buf, len);

    //printf("Buf        : ");
   // for (int i = 0; i < len; ++i) printf("%d ", buf_0[i]); printf("\n");

    BOOL check;
    check = CryptEncrypt(key, NULL, TRUE, NULL, (BYTE*)test_buf, (DWORD*)&len, 1024);
    assert(check);

  //  printf("Crypt buf  : ");
  //  for (int i = 0; i < len; ++i) printf("%d ", test_buf[i]); printf("\n");

    check = CryptDecrypt(key, NULL, TRUE, NULL, (BYTE*)test_buf, (DWORD*)&len);
    assert(check);

   // printf("Decrypt buf: ");
   // for (int i = 0; i < len; ++i) printf("%d ", test_buf[i]); printf("\n");

   // printf("**********************\n\n");

    return memcmp(test_buf, buf_0, len) == 0;
}

void Client::print_dialog()
{
    printf("1 - OS type and version\n");
    printf("2 - Current time\n");
    printf("3 - Time elapsed since OS start\n");
    printf("4 - Information about used memory\n");
    printf("5 - Types of connected drives (local / network / removable, file system)\n");
    printf("6 - Free space on local drives\n");
    printf("7 - Access rights in text form to the specified file / folder / registry key\n");
    printf("8 - Owner of file / folder / registry key\n");
    printf("9 - Exit\n");
}

int Client::ask_request_type()
{
    char e;
    do {
        scanf("%d%c", &request_type, &e);
    } while (request_type > 9 || request_type < 1);

    printf("Path: ");

    if (request_type == 7 || request_type == 8) {
        int i = 0;
        do {
            scanf("%c", &request_arg[i]);
            if (request_arg[i] == '\n') {
                request_arg[i] = '\0';
                break;
            }
            ++i;
        } while (true);
    }

    if (request_type == 9) return -1;
    else return 0;
}

// decrypt RECV_BUF and print to stdout
void Client::decrypt_msg()
{
    BOOL check = CryptDecrypt(session_key, NULL, TRUE, NULL, (BYTE*)recv_buf.buf, (DWORD*)&recv_buf.len);
    assert (check);

    for (int i = 0; i < recv_buf.len; ++i) {
        printf("%c", recv_buf.buf[i]);
    }
    printf("\n");
}

void Client::clear_buf()
{
    memset(recv_buf.buf, 0, 512);
    memset(send_buf.buf, 0, 512);
    recv_buf.len = 0;
    send_buf.len = 0;
}
