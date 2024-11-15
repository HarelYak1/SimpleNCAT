 #include <winsock2.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include "getopt.h"
#include <string.h>
#pragma comment (lib, "ws2_32")
#define DEFAULT_BUFLEN 512


//write data and send via socket(thread)
DWORD send_data(LPVOID threadsocket)
{
    int socket_fd = *(int*) threadsocket;
    char buffer2[DEFAULT_BUFLEN];
    while(1){
        fgets(buffer2, DEFAULT_BUFLEN, stdin);
        int send_result = send (socket_fd, buffer2, (int) strlen(buffer2),0);
        if(send_result == SOCKET_ERROR){
            printf("failed to send: %d\n",WSAGetLastError());
        }


    }
    return 0;
};

DWORD recv_data ( LPVOID threadsocket) 
{
    int socket_fd = *(int*) threadsocket;
    fd_set read_fds;

    while(1){
        FD_ZERO(&read_fds);

        FD_SET(socket_fd, &read_fds);
        
        fd_set temp_fds = read_fds;

        int socket_activity = select (0, &temp_fds, NULL, NULL, NULL) ;

        if (socket_activity == SOCKET_ERROR){
            printf("failed");
        }

        if(FD_ISSET (socket_fd, &temp_fds) ){
            char recv_msg[DEFAULT_BUFLEN];
            memset (recv_msg, 0, DEFAULT_BUFLEN);
            int bytes = recv (socket_fd,recv_msg, sizeof (recv_msg) ,0);
            
            if(bytes == SOCKET_ERROR){
                printf("ERROR : other side disconneted");
                exit(1);
            }

            printf("%s",recv_msg);

        }
    }

};

void socket_threading (SOCKET threading_socket){
    HANDLE read_thread, write_thread;

    read_thread = CreateThread (NULL, 0, recv_data, (LPVOID) &threading_socket, 0, NULL);
    write_thread = CreateThread (NULL, 0, send_data, (LPVOID) &threading_socket, 0, NULL);

    WaitForSingleObject(write_thread, INFINITE );
    WaitForSingleObject(read_thread, INFINITE );

};


void exec_function(char* run_command, SOCKET socket_fd) {
    STARTUPINFO sinfo;
    
    memset(&sinfo,0, sizeof(sinfo));
    PROCESS_INFORMATION pinfo;

    sinfo.cb = sizeof(sinfo);
    sinfo.dwFlags = STARTF_USESTDHANDLES;
    sinfo.hStdInput = (HANDLE) socket_fd;
    sinfo.hStdOutput = (HANDLE) socket_fd;
    sinfo.hStdError = (HANDLE) socket_fd;

    if (!CreateProcessA(NULL, run_command, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &sinfo, &pinfo)) {
        printf("Error creating process: %d\n", GetLastError());
        return 1;
    }

    WaitForSingleObject(pinfo.hProcess, INFINITE);
    CloseHandle(pinfo.hProcess);
    CloseHandle(pinfo.hThread);
};

int client_mode(char* HOST, int PORT, bool verbose, char* run_command, bool scommand){
    SOCKET client_socket_fd = INVALID_SOCKET;

    if(scommand)
        client_socket_fd = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL,0,0);

    else
        client_socket_fd = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL,0, WSA_FLAG_OVERLAPPED);
    
    if(client_socket_fd == INVALID_SOCKET){
        printf("socket failed with error ");
        return 1;
    }


    SOCKADDR_IN client_service;
    client_service.sin_family = AF_INET;
    client_service.sin_addr.s_addr = inet_addr (HOST) ;
    client_service.sin_port = htons (PORT) ;
    int connect_result = connect (client_socket_fd, (SOCKADDR*)&client_service, sizeof (client_service));

    if(connect_result == INVALID_SOCKET){
        closesocket (client_socket_fd);
        return 1;

    }
    if (verbose)
        printf ("Connected to server %s:%d\n", HOST, PORT);

    if(scommand)
        exec_function(run_command,client_socket_fd);
    else
        socket_threading(client_socket_fd);
    

};

int listen_mode (char* HOST, int PORT, bool verbose, char* run_command, bool scommand){
    SOCKET listen_sock_fd = INVALID_SOCKET;
    SOCKADDR_IN service;

    if(scommand)
        listen_sock_fd = WSASocketA (AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL,0,0);
    else
        listen_sock_fd = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL,0, WSA_FLAG_OVERLAPPED);

    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY;
    service.sin_port = htons (PORT) ;

    int bind_result = bind (listen_sock_fd, (SOCKADDR*) &service, sizeof (service) );

    if(bind_result == SOCKET_ERROR){
        closesocket(listen_sock_fd);
        printf("error");
        return 1;
    }

    if (listen (listen_sock_fd, SOMAXCONN) == SOCKET_ERROR) {
        printf("error");
        return 1;
    }

    if(verbose)
        printf("Listening on socket 0.0.0.0:%d\n",PORT);


    SOCKET Client_acc_socket = accept (listen_sock_fd, NULL, NULL);

    if(scommand)
        exec_function(run_command,Client_acc_socket);
    else
        socket_threading(Client_acc_socket);

    closesocket(listen_sock_fd);
    WSACleanup();

}
;
int main(int argc, char *argv[]){
    int cvalue;
    bool listen = FALSE;
    int PORT = 31337;
    char HOST [30] = "127.0.0.1";
    bool verbose = FALSE;
    bool scommand = FALSE;
    char run_command [30] = "";

    
    while ( (cvalue = getopt (argc, argv, "lp:d:h:ve:")) != -1) {
        switch (cvalue){
            case 'l':
                listen = TRUE;
                break;
            case 'p':
                PORT = atoi(optarg);
                break;
            
            case 'h':
                strcpy(HOST,optarg);
                break;
            case 'v':
                verbose = TRUE;
                break;

            case 'e':
                strcpy(run_command,optarg);
                scommand = TRUE;
                break;
                    
        }
    }

    WSADATA wsaData;
    int iresult = 0;

    iresult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iresult != NO_ERROR) {
      printf("WSAStartup failed: %d\n", iresult);
      return 1;
    }

    if(listen){
        listen_mode(HOST,PORT,verbose,run_command,scommand);
    }

    else{
        client_mode(HOST,PORT,verbose,run_command,scommand);
    }

    return 0;
};
