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


    if (os==10) strncpy((char*)buffer, (char*)"win10", 5);
    else if (IsWindows8OrGreater()) strncpy((char*)buffer, (char*)"win8", 4);
    else if (IsWindows7SP1OrGreater()) strncpy((char*)buffer, (char*)"win7SP1", 7);
    else if (IsWindows7OrGreater()) strncpy((char*)buffer, (char*)"win7", 4);
    else if (IsWindowsVistaSP2OrGreater()) strncpy((char*)buffer, (char*)"vistaSP2", 8);
    else if (IsWindowsVistaSP1OrGreater()) strncpy((char*)buffer, (char*)"vistaSP1", 8);
    else if (IsWindowsVistaOrGreater()) strncpy((char*)buffer, (char*)"vista", 5);
    else if (IsWindowsXPSP3OrGreater()) strncpy((char*)buffer, (char*)"XPSP3", 5);
    else if (IsWindowsXPSP2OrGreater()) strncpy((char*)buffer, (char*)"XPSP2", 5);
    else if (IsWindowsXPSP1OrGreater()) strncpy((char*)buffer, (char*)"XPSP1", 5);
    else if (IsWindowsXPOrGreater()) strncpy((char*)buffer, (char*)"XP", 2);


    int buf_size = strlen((char*)buffer);
    for (int i = buf_size + 3; i > 3; --i) {
        buffer[i] = buffer[i - 4];
    }
    *((int*)(buffer)) = buf_size;

}

void serv_current_time(
        int buffer_size,
        /* [size_is][out] */ unsigned char *buffer) {

    std::cout << "Current time request\n";

    SYSTEMTIME sm;
    GetSystemTime(&sm);

    sprintf((char*)buffer, "%d%d.%d%d.%d %d%d:%d%d:%d%d", sm.wDay/10, sm.wDay%10, \
        sm.wMonth/10, sm.wMonth%10, sm.wYear, \
        sm.wHour/10, sm.wHour%10, sm.wMinute/10, sm.wMinute%10, sm.wSecond/10, sm.wSecond%10);


    int buf_size = strlen((char*)buffer);
    for (int i = buf_size + 3; i > 3; --i) {
        buffer[i] = buffer[i - 4];
    }
    *((int*)(buffer)) = buf_size;

}

void serv_os_time(
        int buffer_size,
        /* [size_is][out] */ unsigned char *buffer) {

    std::cout << "Os time request\n";

    int time_ticks = GetTickCount();
    int hours = time_ticks / (1000 * 60 * 60);
    int minutes = time_ticks / (1000 * 60) - hours * 60;
    int seconds = (time_ticks / 1000) - (hours * 60 * 60) - minutes * 60;

    sprintf((char*)buffer, "%d hours, %d minutes, %d seconds", hours, minutes, seconds);

    int buf_size = strlen((char*)buffer);
    for (int i = buf_size + 3; i > 3; --i) {
        buffer[i] = buffer[i - 4];
    }
    *((int*)(buffer)) = buf_size;
}

void serv_memory_status(
        int buffer_size,
        /* [size_is][out] */ unsigned char *buffer) {

    std::cout << "Memory status request\n";

    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);

    const float convert = (const float)1073741824;

    sprintf((char*)buffer, \
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

    int buf_size = strlen((char*)buffer);
    for (int i = buf_size + 3; i > 3; --i) {
        buffer[i] = buffer[i - 4];
    }
    *((int*)(buffer)) = buf_size;

}

void serv_disks_types(
        int buffer_size,
        /* [size_is][out] */ unsigned char *buffer) {

    std::cout << "Disk types request\n";

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
            ((char*)buffer)[k] = disks[j][0];
            k++;
            ((char*)buffer)[k] = disks[j][1];
            k++;

            k += strlen (
                    strcpy(((char*)buffer) + k, "\n\t type: ")
            );

            strcpy(((char*)buffer) + k, disks_types[type]);
            k += strlen(disks_types[type]);

            k += strlen (
                    strcpy(((char*)buffer) + k, "\n\t file system: ")
            );

            len = strlen(name_file_system);
            memcpy(((char*)buffer) + k, name_file_system, len);
            k += len;

            k += strlen (
                    strcpy(((char*)buffer) + k, "\n")
            );

            j++;
        }
    }


    int buf_size = strlen((char*)buffer);
    for (int i = buf_size + 3; i > 3; --i) {
        buffer[i] = buffer[i - 4];
    }
    *((int*)(buffer)) = buf_size;

}

void serv_free_space(
        int buffer_size,
        /* [size_is][out] */ unsigned char *buffer) {


    std::cout << "Free space request\n";



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
                ((char*)buffer)[count * 13 + 2] = ' ';

                GetDiskFreeSpaceA(disks[count], (LPDWORD)& s, (LPDWORD)& b, (LPDWORD)& f, (LPDWORD)& c);
                freeSpace = (double)f * (double)s * (double)b / 1024.0 / 1024.0 / 1024.0;
                ((char*)buffer)[count * 13] = disks[count][0];
                ((char*)buffer)[count * 13 + 1] = disks[count][1];

                char buf[16];
                memcpy(((char*)buffer) + count * 13 + 3, gcvt(freeSpace, 10, buf), 8);
                memcpy(((char*)buffer) + count * 13 + 9, " Gb\n", 4);
                count++;
            }
        }
    }

    memcpy(((char*)buffer), ((char*)buffer), count * 10);



    int buf_size = strlen((char*)buffer);
    for (int i = buf_size + 3; i > 3; --i) {
        buffer[i] = buffer[i - 4];
    }
    *((int*)(buffer)) = buf_size;
}

void serv_file_owner(
        int buffer_size,
        /* [size_is][out] */ unsigned char *buffer,
        /* [string][in] */ const unsigned char *path) {

    std::cout << "File owner request\n";
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
        strcat(((char*)buffer), "Owner: ");
        strcat(((char*)buffer), user);
    }




    int buf_size = strlen((char*)buffer);
    for (int i = buf_size + 3; i > 3; --i) {
        buffer[i] = buffer[i - 4];
    }
    *((int*)(buffer)) = buf_size;
}


void serv_file_access_right(
        int buffer_size,
        /* [size_is][out] */ unsigned char *buffer,
        /* [string][in] */ const unsigned char *path) {

    std::cout << "File access right request\n";

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

            strcat(((char*)buffer), "User: ");
            strcat(((char*)buffer), user);
            strcat(((char*)buffer), "\nSID: ");
            strcat(((char*)buffer), sid_str);
            strcat(((char*)buffer), "\nMask: ");

            char mask_str[16];
            sprintf(mask_str, "0x%lx", mask);

            strcat(((char*)buffer), mask_str);
        }

        break;
    }





    int buf_size = strlen((char*)buffer);
    for (int i = buf_size + 3; i > 3; --i) {
        buffer[i] = buffer[i - 4];
    }
    *((int*)(buffer)) = buf_size;

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
