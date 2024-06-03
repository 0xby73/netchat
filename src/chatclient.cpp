#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")

#define PORT "8080"
#define SERVER_ADDRESS "localhost" // comment this out if you want to use an ip address instead of a hostname
// #define SERVER_ADDRESS 127.0.0.1  
#define DEFAULT_BUFLEN 512

std::atomic<bool> shouldExit(false);
std::string username;


void sendMessages(SOCKET ConnectSocket) {
    while (!shouldExit) {   
        std::string sendbuf;
        std::cout << ">>> ";
        std::getline(std::cin, sendbuf);
        sendbuf = username + ": " + sendbuf;

        int iResult = send(ConnectSocket, sendbuf.c_str(), (int)sendbuf.length(), 0);
        if (iResult == SOCKET_ERROR) {
            std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
            shouldExit = true;
        }
    }
}

void receiveMessages(SOCKET ConnectSocket) {
    while (!shouldExit) {

        char buffer[1024] = { 0 };
        int iResult = recv(ConnectSocket, buffer, sizeof(buffer), 0);
        if (iResult > 0) {
            std::string receivedData(buffer, iResult);
            std::cout << receivedData << std::endl;
        }
        else if (iResult == 0) {
            std::cout << "Connection closed by the server." << std::endl;
            shouldExit = true;
        }
        else {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
            shouldExit = true;
        }
    }
}

int main(void) {
    WSADATA wsaData;
    int iResult;
    struct addrinfo* result = NULL, * ptr = NULL, hints;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed";
        std::this_thread::sleep_for(std::chrono::seconds(3));
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // AF_INET << change it to AF_INET if you are using an ip
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    iResult = getaddrinfo(SERVER_ADDRESS, PORT, &hints, &result);
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));
        WSACleanup();
        return 1;
    }

    SOCKET ConnectSocket = INVALID_SOCKET;
    ptr = result;

    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET) {
        std::cerr << "ur socket took a shit\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
    }
    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        std::cerr << "Unable to connect to socket\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));

        WSACleanup();
        return 1;
    }

    std::string username;
    std::cout << "Enter a username: \n";
    std::cout << "It can only be one word or it won't work\n";
    std::cout << ">>> ";
    std::cin >> username;
    std::cin.ignore();
    system("cls");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Connected to server!";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    system("cls");

    std::thread receiveThread(receiveMessages, ConnectSocket);
    std::thread sendThread(sendMessages, ConnectSocket);

    receiveThread.join();
    sendThread.join();

    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}