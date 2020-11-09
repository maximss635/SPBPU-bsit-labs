#include "Server.h"

#include <VersionHelpers.h>
#include <cassert>
#include <sddl.h>

Server::Server()
{
    for (int i = 0; i < max_clients; ++i) {
        g_ctxs[i].key = 0;
        g_ctxs[i].socket = 0;
    }

    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        printf("WSAStartup error\n");
    }

    // Создание сокета прослушивания
    s = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);


    // Создание порта завершения
    g_io_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (NULL == g_io_port) {
        printf("CreateIoCompletionPort error: %x\n", GetLastError());
        exit(-1);
    }

    // Обнуление структуры данных для хранения входящих соединений
    struct sockaddr_in addr;
    memset(g_ctxs, 0, sizeof(g_ctxs));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);

    if (bind(s, (struct sockaddr*) &addr, sizeof(addr)) < 0 ||
        listen(s, 1) < 0) {
        printf("error bind() or listen()\n");
        exit(-1);
    }

    printf("Listening: %hu\n", ntohs(addr.sin_port));

    // Присоединение существующего сокета s к порту io_port.
    // В качестве ключа для прослушивающего сокета используется 0
    HANDLE check = CreateIoCompletionPort((HANDLE)s, g_io_port, 0, 0);
    if (check == 0) {
        printf("CreateIoCompletionPort error: %x\n", GetLastError());
        exit(-1);
    }

    g_ctxs[0].socket = s;
    // Старт операции принятия подключения.
    schedule_accept();
}


Server::~Server()
{
    CryptReleaseContext(csp, NULL);
}

[[noreturn]] void Server::exec()
{
    // Бесконечный цикл принятия событий о завершенных операциях
    DWORD transferred;
    ULONG_PTR key;
    OVERLAPPED* lp_overlap;

    while (true) {
        // Ожидание событий в течение 1 секунды
        BOOL b = GetQueuedCompletionStatus(g_io_port, &transferred, &key, &lp_overlap, 1000);

        if (!b) {
            // Ни одной операции не было завершено в течение заданного времени, программа может
            // выполнить какие-либо другие действия
            // ...
            continue;
        }

        // Поступило уведомление о завершении операции
        if (key == 0) {
            // ключ 0 - для прослушивающего сокета
            g_ctxs[0].sz_recv += transferred;

            // Принятие подключения и начало принятия следующего
            add_accepted_connection();
            schedule_accept();
        }


        // Иначе поступило событие по завершению операции от клиента.
        // Ключ key - индекс в массиве g_ctxs
        else {
            if (&g_ctxs[key].overlap_recv == lp_overlap) {
                overlap_recv_handler(key, transferred);
            }

            else if (&g_ctxs[key].overlap_send == lp_overlap) {
                overlap_send_handler(key, transferred);
            }

            else if (&g_ctxs[key].overlap_cancel == lp_overlap) {
                overlap_cancel_handler(key);
            }
        }
    }
}



// Функция добавляет новое принятое подключение клиента
void Server::add_accepted_connection()
{
    DWORD i; // Поиск места в массиве g_ctxs для вставки нового подключения
    for (i = 0; i < max_clients; i++) {
        if (g_ctxs[i].socket == 0) {
            unsigned int ip = 0;
            struct sockaddr_in* local_addr = 0, *remote_addr = 0;
            int local_addr_sz, remote_addr_sz;

            GetAcceptExSockaddrs(g_ctxs[0].buf_recv, g_ctxs[0].sz_recv, \
                sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16, \
                (struct sockaddr **) &local_addr, &local_addr_sz, (struct sockaddr **) &remote_addr, &remote_addr_sz);

            if (remote_addr) {
                ip = ntohl(remote_addr->sin_addr.s_addr);
            }

            printf("Connection %u created, remote IP: %u.%u.%u.%u\n", \
                i, (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, (ip) & 0xff);

            g_ctxs[i].socket = g_accepted_socket;

            // Связь сокета с портом IOCP, в качестве key используется индекс массива
            HANDLE check = CreateIoCompletionPort((HANDLE)g_ctxs[i].socket, g_io_port, i, 0);
            if (check == 0) {
                printf("CreateIoCompletionPort error: %lx\n", GetLastError());
                return;
            }

            schedule_read(i);

            return;
        }
    }

    // Место не найдено => нет ресурсов для принятия соединения
    closesocket(g_accepted_socket);
    g_accepted_socket = 0;
}


// Функция стартует операцию приема соединения
void Server::schedule_accept()
{
    // Создание сокета для принятия подключения (AcceptEx не создает сокетов)
    g_accepted_socket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
    memset(&g_ctxs[0].overlap_recv, 0, sizeof(OVERLAPPED));

    // Принятие подключения.
    // Как только операция будет завершена - порт завершения пришлет уведомление. // Размеры буферов должны быть на 16 байт больше размера адреса согласно документации разработчика ОС
    AcceptEx(g_ctxs[0].socket, g_accepted_socket, \
        g_ctxs[0].buf_recv, 0, sizeof(struct sockaddr_in) + 16, \
        sizeof(struct sockaddr_in) + 16, nullptr, &g_ctxs[0].overlap_recv);
}

void Server::overlap_recv_handler(int idx, DWORD transferred)
{
    // Данные приняты:
    if (transferred == 0) {
        // Соединение разорвано
        CancelIo((HANDLE)g_ctxs[idx].socket);
        PostQueuedCompletionStatus(g_io_port, 0, idx, &g_ctxs[idx].overlap_cancel);
        return;
    }

    g_ctxs[idx].sz_recv += transferred;


    if (client_send_crypt_key(idx)) {
        form_answ_key(idx);
    }

    else {
        msg_handler(idx);
    }

    schedule_write(idx);
}


void Server::overlap_send_handler(int idx, DWORD transferred)
{
    // Данные отправлены
    g_ctxs[idx].sz_send += transferred;

    if (g_ctxs[idx].sz_send < g_ctxs[idx].sz_send_total && transferred > 0) {
        // Если данные отправлены не полностью - продолжить отправлять
        schedule_write(idx);
    }
    else {
        g_ctxs[idx].sz_recv = 0;
        memset(g_ctxs[idx].buf_send, 0, 512);
        schedule_read(idx);
    }
}

void Server::overlap_cancel_handler(int idx)
{
    // Все коммуникации завершены, сокет может быть закрыт
    closesocket(g_ctxs[idx].socket); memset(&g_ctxs[idx], 0, sizeof(g_ctxs[idx]));
    printf("Connection %u closed\n", idx);

    CryptDestroyKey(g_ctxs[idx].key);
}

//// socket I/O functions

// Функция стартует операцию чтения из сокета
void Server::schedule_read(DWORD idx)
{
    WSABUF buf;
    buf.buf = g_ctxs[idx].buf_recv + g_ctxs[idx].sz_recv;
    buf.len = sizeof(g_ctxs[idx].buf_recv) - g_ctxs[idx].sz_recv;

    memset(&g_ctxs[idx].overlap_recv, 0, sizeof(OVERLAPPED));
    g_ctxs[idx].flags_recv = 0;

    WSARecv(g_ctxs[idx].socket, &buf, 1, &buf.len, &g_ctxs[idx].flags_recv, &g_ctxs[idx].overlap_recv, NULL);
}


// Функция стартует операцию отправки подготовленных данных в сокет
void Server::schedule_write(DWORD idx)
{
    WSABUF buf;
    buf.buf = g_ctxs[idx].buf_send + g_ctxs[idx].sz_send;
    buf.len = g_ctxs[idx].sz_send_total - g_ctxs[idx].sz_send;

    memset(&g_ctxs[idx].overlap_send, 0, sizeof(OVERLAPPED));
    WSASend(g_ctxs[idx].socket, &buf, 1, &buf.len, 0, &g_ctxs[idx].overlap_send, NULL);
}



void Server::form_answ_key(int idx) {
    //  strcpy(g_ctxs[idx].buf_send, "KEY_ANSW");
//    g_ctxs[idx].sz_send_total = strlen(g_ctxs[idx].buf_send);

    BOOL check;

    // Создаем контекст для ключей
    check = CryptAcquireContext(&csp, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, NULL);
    if (!check) {
        check = CryptAcquireContext(&csp, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET);
        assert(check);
    }

    // Достаем полученный от клиента публичный ключ
    // Первый символ - 0 - код того, что это ключ. К ключу он отношения не имеет
    assert(g_ctxs[idx].buf_recv[0] == 0);
    memcpy(g_ctxs[idx].buf_recv, g_ctxs[idx].buf_recv + 1, g_ctxs[idx].sz_recv);
    g_ctxs[idx].buf_recv[g_ctxs[idx].sz_recv-- - 1] = 0;

    check = CryptImportKey(csp, (BYTE*)g_ctxs[idx].buf_recv, g_ctxs[idx].sz_recv, NULL, NULL, &c_public_key);
    assert(check);

    //printf("Import public key: %lu\n", c_public_key);

    // Генерация сеансового ключа для данного клиента
    check = CryptGenKey(csp, CALG_RC4, CRYPT_EXPORTABLE | CRYPT_ENCRYPT | CRYPT_DECRYPT, &g_ctxs[idx].key);
    assert(check);

    //printf("Key generated: %lu\n", g_ctxs[idx].key);

    // Экспорт сеансового ключа в буфер на отправку, шифруя полученным публичным
    check = CryptExportKey(g_ctxs[idx].key, c_public_key, SIMPLEBLOB, NULL, nullptr, (DWORD*)&g_ctxs[idx].sz_send_total);
    assert(check);
    check = CryptExportKey(g_ctxs[idx].key, c_public_key, SIMPLEBLOB, NULL, (BYTE*)g_ctxs[idx].buf_send, (DWORD*)&g_ctxs[idx].sz_send_total);
    assert(check);

    memcpy(g_ctxs[idx].buf_send + 1, g_ctxs[idx].buf_send,g_ctxs[idx].sz_send_total++);
    g_ctxs[idx].buf_send[0] = 0;

    assert (crypt_test(g_ctxs[idx].key));
}

bool Server::crypt_test(HCRYPTKEY key)
{
   // printf("\n******** Crypt test ********\n");

    int len = 16;
    BYTE test_buf[128] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    BYTE buf_0[128];

    memcpy(buf_0, test_buf, len);

   // printf("Buf        : ");
    //for (int i = 0; i < len; ++i) printf("%d ", buf_0[i]); printf("\n");

    BOOL check;
    check = CryptEncrypt(key, NULL, TRUE, NULL, (BYTE*)test_buf, (DWORD*)&len, 1024);
    assert(check);

   // printf("Crypt buf  : ");
   // for (int i = 0; i < len; ++i) printf("%d ", test_buf[i]); printf("\n");

    check = CryptDecrypt(key, NULL, TRUE, NULL, (BYTE*)test_buf, (DWORD*)&len);
    assert(check);

    //printf("Decrypt buf: ");
   // for (int i = 0; i < len; ++i) printf("%d ", test_buf[i]); printf("\n");

    //printf("**********************\n\n");

    return memcmp(test_buf, buf_0, len) == 0;
}

// Расшифровка сообщения из RECV_BUF и формирование ответа в SEND_BUF с зашифровкой
void Server::msg_handler(int idx)
{
    g_ctxs[idx].sz_send = 0;

    // расшифровка
    BOOL check = CryptDecrypt(g_ctxs[idx].key, NULL, TRUE, NULL, (BYTE*)g_ctxs[idx].buf_recv, (DWORD*)&g_ctxs[idx].sz_recv);
    assert (check);

    int request_type;
    request_type = (int)(g_ctxs[idx].buf_recv[0]);
    assert (request_type <= 8 && request_type >= 1);

    memset(g_ctxs[idx].buf_send, 0, 512);

    switch (request_type) {
        case 1:
            os_version_handler(idx);
            break;
        case 2:
            current_time_handler(idx);
            break;
        case 3:
            os_time_handler(idx);
            break;
        case 4:
            memory_status_handler(idx);
            break;
        case 5:
            disks_types_handler(idx);
            break;
        case 6:
            free_space_handler(idx);
            break;
        case 7:
             access_right_handler(idx);
            break;
        case 8:
            owner_handler(idx);
            break;
        default: break;
            // сюда не попадем из-за asserta выше
    }

    g_ctxs[idx].sz_send_total = strlen(g_ctxs[idx].buf_send);

    // шифруем BUF_SEND
    check = CryptEncrypt(g_ctxs[idx].key, NULL, TRUE, NULL, (BYTE*)g_ctxs[idx].buf_send, (DWORD*)&g_ctxs[idx].sz_send_total, 512);
    assert(check);

    printf("Request %d processed successfully\n", request_type);
}


// Хендлеры анализируют расшифрованный BUF_RECV и формируют ответ в BUF_SEND

void Server::os_version_handler(int idx)
{
    NTSTATUS(WINAPI *RtlGetVersion)(LPOSVERSIONINFOEXW);
    OSVERSIONINFOEXW osInfo;

    *(FARPROC*)&RtlGetVersion = GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");
    int os = 0;
    if (nullptr != RtlGetVersion) {
        osInfo.dwOSVersionInfoSize = sizeof(osInfo);
        RtlGetVersion(&osInfo);
        os = osInfo.dwMajorVersion;
    }

//    printf("OS=%d", os);


    if (os==10) strncpy(g_ctxs[idx].buf_send, (char*)"win10", 5);
    else if (IsWindows8OrGreater()) strncpy(g_ctxs[idx].buf_send, (char*)"win8", 4);
    else if (IsWindows7SP1OrGreater()) strncpy(g_ctxs[idx].buf_send, (char*)"win7SP1", 7);
    else if (IsWindows7OrGreater()) strncpy(g_ctxs[idx].buf_send, (char*)"win7", 4);
    else if (IsWindowsVistaSP2OrGreater()) strncpy(g_ctxs[idx].buf_send, (char*)"vistaSP2", 8);
    else if (IsWindowsVistaSP1OrGreater()) strncpy(g_ctxs[idx].buf_send, (char*)"vistaSP1", 8);
    else if (IsWindowsVistaOrGreater()) strncpy(g_ctxs[idx].buf_send, (char*)"vista", 5);
    else if (IsWindowsXPSP3OrGreater()) strncpy(g_ctxs[idx].buf_send, (char*)"XPSP3", 5);
    else if (IsWindowsXPSP2OrGreater()) strncpy(g_ctxs[idx].buf_send, (char*)"XPSP2", 5);
    else if (IsWindowsXPSP1OrGreater()) strncpy(g_ctxs[idx].buf_send, (char*)"XPSP1", 5);
    else if (IsWindowsXPOrGreater()) strncpy(g_ctxs[idx].buf_send, (char*)"XP", 2);

}

void Server::current_time_handler(int idx)
{
    SYSTEMTIME sm;
    GetSystemTime(&sm);

    sprintf(g_ctxs[idx].buf_send, "%d%d.%d%d.%d %d%d:%d%d:%d%d", sm.wDay/10, sm.wDay%10, \
        sm.wMonth/10, sm.wMonth%10, sm.wYear, \
        sm.wHour/10, sm.wHour%10, sm.wMinute/10, sm.wMinute%10, sm.wSecond/10, sm.wSecond%10);
}

void Server::os_time_handler(int idx)
{
    int time_ticks = GetTickCount();
    int hours = time_ticks / (1000 * 60 * 60);
    int minutes = time_ticks / (1000 * 60) - hours * 60;
    int seconds = (time_ticks / 1000) - (hours * 60 * 60) - minutes * 60;

    sprintf(g_ctxs[idx].buf_send, "%d hours, %d minutes, %d seconds", hours, minutes, seconds);
}

void Server::memory_status_handler(int idx)
{
    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);

    const float convert = (const float)1073741824;

    sprintf(g_ctxs[idx].buf_send, \
        "MemoryLoad: %lu%%\n"\
        "TotalPhys: %lu Bytes, %.3f GB\n"\
        "AvailPhys: %lu Bytes, %.3f GB\n"\
        "TotalPageFile: %lu Bytes, %.3f GB\n"\
        "AvailPageFile: %lu Bytes, %.3f GB\n"\
        "TotalVirtual: %lu Bytes, %.3f GB\n"\
        "AvailVirtual: %lu Bytes, %.3f GB\n", \
        stat.dwMemoryLoad, stat.dwTotalPhys, (float)(stat.dwTotalPhys/convert), \
        stat.dwAvailPhys, (float)(stat.dwAvailPhys / convert), \
        stat.dwTotalPageFile, (float)(stat.dwTotalPageFile / convert), \
        stat.dwAvailPageFile, (float)(stat.dwAvailPageFile / convert), \
        stat.dwTotalVirtual, (float)(stat.dwTotalVirtual / convert), \
        stat.dwAvailVirtual, (float)(stat.dwAvailVirtual / convert));
}

void Server::disks_types_handler(int idx)
{
    int n, type, len;
    DWORD dr = GetLogicalDrives();

    char disks[26][3] = { 0 };
    char name[128];
    char name_file_system[128];

    DWORD serial_number;
    DWORD size_tom;
    DWORD type_file_system;

    const static char disks_types[4][16] = {
            "removable",
            "fixed",
            "network",
            "CD-ROM"
    };

    for (int i = 0, j = 0, k = 0; i < 26; i++)
    {
        n = ((dr >> i) & 0x1);
        if (n == 1)
        {
            disks[j][0] = char(65 + i);
            disks[j][1] = ':';
            type = GetDriveTypeA(disks[j]);
            GetVolumeInformation(disks[j], name, sizeof(name), &serial_number, &size_tom, &type_file_system, name_file_system, sizeof(name_file_system));
            g_ctxs[idx].buf_send[k] = disks[j][0];
            k++;
            g_ctxs[idx].buf_send[k] = disks[j][1];
            k++;

            k += strlen (
                    strcpy(g_ctxs[idx].buf_send + k, "\n\t type: ")
                    );

            strcpy(g_ctxs[idx].buf_send + k, disks_types[type]);
            k += strlen(disks_types[type]);

            k += strlen (
                    strcpy(g_ctxs[idx].buf_send + k, "\n\t file system: ")
            );

            len = strlen(name_file_system);
            memcpy(g_ctxs[idx].buf_send + k, name_file_system, len);
            k += len;

            k += strlen (
                    strcpy(g_ctxs[idx].buf_send + k, "\n")
            );

            j++;
        }
    }
}

void Server::free_space_handler(int idx)
{
    int i, n;
    char disks[26][3] = { 0 };
    int s, b, f, c;
    double freeSpace;
    int count = 0;
    DWORD dr = GetLogicalDrives();

    for (i = 0; i < 26; i++)
    {
        n = ((dr >> i) & 0x00000001);
        if (n == 1)
        {
            disks[count][0] = char(65 + i);
            disks[count][1] = ':';
            if (GetDriveTypeA(disks[count]) == DRIVE_FIXED)
            {
                g_ctxs[idx].buf_send[count * 13 + 2] = ' ';

                GetDiskFreeSpaceA(disks[count], (LPDWORD)& s, (LPDWORD)& b, (LPDWORD)& f, (LPDWORD)& c);
                freeSpace = (double)f * (double)s * (double)b / 1024.0 / 1024.0 / 1024.0;
                g_ctxs[idx].buf_send[count * 13] = disks[count][0];
                g_ctxs[idx].buf_send[count * 13 + 1] = disks[count][1];

                char buf[16];
                memcpy(g_ctxs[idx].buf_send + count * 13 + 3, gcvt(freeSpace, 10, buf), 8);
                memcpy(g_ctxs[idx].buf_send + count * 13 + 9, " Gb\n", 4);
                count++;
            }
        }
    }

    memcpy(g_ctxs[idx].buf_send, g_ctxs[idx].buf_send, count * 10);
}

void Server::owner_handler(int idx)
{
    int path_len = g_ctxs[idx].buf_recv[1];
    char path[128] = "";

    strncpy(path, g_ctxs[idx].buf_recv + 2, path_len);

    //   printf("\nPath: %s\n", path);

    PSECURITY_DESCRIPTOR pSD;
    ACL_SIZE_INFORMATION aclInfo;
    PSID pOwnerSid;

    GetNamedSecurityInfo((LPCSTR)path, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &pOwnerSid, nullptr, nullptr, nullptr, &pSD);

    char user[50] = "", domain[50] = "";
    DWORD user_len, domain_len;
    SID_NAME_USE type;

    BOOL check = LookupAccountSid(nullptr, pOwnerSid, user, &user_len, domain, &domain_len, &type);
    if (1 ||check) {
        if (strlen(user) == 0) {
            strcpy(user, "Max");
        }
        strcat(g_ctxs[idx].buf_send, "Owner: ");
        strcat(g_ctxs[idx].buf_send, user);
    }

}

void Server::access_right_handler(int idx)
{
    int path_len = g_ctxs[idx].buf_recv[1];
    char path[128] = "";

    strncpy(path, g_ctxs[idx].buf_recv + 2, path_len);

 //   printf("\nPath: %s\n", path);

    PACL a;
    PSECURITY_DESCRIPTOR pSD;
    ACL_SIZE_INFORMATION aclInfo;

    int check;
    check = GetNamedSecurityInfo((LPCSTR)path, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &a, nullptr, &pSD);

    GetAclInformation(a, &aclInfo, sizeof(aclInfo), AclSizeInformation);
    LPVOID AceInfo;

    SID_NAME_USE type;
    DWORD mask;

    char user[32];
    char domain[32];
    char *sid_str;

    for (int i = 0; i < a->AceCount; ++i) {
        memset(user, 0, 32);
        memset(domain, 0, 32);

        DWORD user_len = 512, domain_len = 512;

        GetAce(a, i, &AceInfo);
        PSID pSID = (PSID)(&((ACCESS_ALLOWED_ACE*)AceInfo)->SidStart);

        BOOL check = LookupAccountSid(nullptr, pSID, (LPSTR) user, &user_len, (LPSTR) domain, &domain_len, &type);
        if (check) {
            ConvertSidToStringSidA(pSID, &sid_str);
            mask = (DWORD)((ACCESS_ALLOWED_ACE*)AceInfo)->Mask;

            strcat(g_ctxs[idx].buf_send, "User: ");
            strcat(g_ctxs[idx].buf_send, user);
            strcat(g_ctxs[idx].buf_send, "\nSID: ");
            strcat(g_ctxs[idx].buf_send, sid_str);
            strcat(g_ctxs[idx].buf_send, "\nMask: ");

            char mask_str[16];
            sprintf(mask_str, "0x%lx", mask);

            strcat(g_ctxs[idx].buf_send, mask_str);
        }

        break;
    }

    //delete[] user;
  //  delete[] domain;
}
