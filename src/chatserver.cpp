	#include <iostream>
	#include <WS2tcpip.h>
	#include <WinSock2.h>
	#include <vector>
	#include <thread>
	#include <chrono>
	#include <atomic>
	#include <mutex>
	#include <algorithm>

	#pragma comment(lib, "ws2_32.lib")

	#define PORT "8080"
	#define DEFAULT_BUFLEN 512
	#define NAME "root:"

	std::atomic<bool> shouldExit(false);
	std::vector<SOCKET> clients;
	std::mutex clientsMutex;


	void displayclientamount()
	{
		std::lock_guard<std::mutex> lock(clientsMutex);
		std::cout << "Connected Clients: " << clients.size() << std::endl;
	}

	void broadcastMessage(const std::string& message, SOCKET sender)
	{
		std::lock_guard<std::mutex> lock(clientsMutex);
		for (auto client : clients) {
			if (client != sender) {
				int iResult = send(client, message.c_str(), static_cast<int>(message.length()), 0);
				if (iResult == SOCKET_ERROR) {
					std::cerr << "Client did not receive last message";
				}
			}
		}
	}

	void receiveMessages(SOCKET clientSocket) {
		char buffer[1024] = { 0 };
		while (!shouldExit) {
			int iResult = recv(clientSocket, buffer, sizeof(buffer), 0);
			if (iResult > 0) {
				std::string receivedData(buffer, iResult);
				std::cout << receivedData << std::endl;
				broadcastMessage(receivedData, clientSocket);
			}
			else if (iResult == 0) {
				std::cout << "Client Disconnected\n";
				break;
			}
			else {
				std::cerr << "recv failed\n" << WSAGetLastError() << std::endl;
				break;
			}
		}

		{
			std::lock_guard<std::mutex> lock(clientsMutex);
			clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
			displayclientamount();
		}
		closesocket(clientSocket);
	}


	void sendMessages(SOCKET clientSocket) {
		std::string sendbuf;
		while (!shouldExit) {
			{
				static std::mutex cinMutex;
				std::lock_guard<std::mutex> cinLock(cinMutex);
				std::cout << NAME << " >>> ";
				std::getline(std::cin, sendbuf);
			}

			sendbuf = NAME + sendbuf;
			std::lock_guard<std::mutex> lock(clientsMutex);
			int iResult = send(clientSocket, sendbuf.c_str(), (int)sendbuf.length(), 0);
			if (iResult == SOCKET_ERROR) {
				std::cerr << "Message failed to send, error: " << WSAGetLastError() << std::endl;
				shouldExit = true;
			}
		}
	}

	void handleClient(SOCKET clientSocket) {
		{
			{
				std::lock_guard<std::mutex> lock(clientsMutex);
				clients.push_back(clientSocket);
			}
			displayclientamount();
		}

		std::thread receiveThread(receiveMessages, clientSocket);
		std::thread sendThread(sendMessages, clientSocket);

		receiveThread.join();
		sendThread.join();

		{
			std::lock_guard<std::mutex> lock(clientsMutex);
			auto position = std::find(clients.begin(), clients.end(), clientSocket);
			if (position != clients.end()) {
				clients.erase(position);
				displayclientamount();
			}
		}
		closesocket(clientSocket);
	}




	int main()
	{

		WSADATA wsaData;
		struct addrinfo* result = NULL, * ptr = NULL, hints;
		int iResult;
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			std::cerr << "Wsastartup error\n";
			return 1;
		}


		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		iResult = getaddrinfo(NULL, PORT, &hints, &result);
		if (iResult != 0)
		{
			std::cerr << "getaddrinfo failed\n";
			WSACleanup();
			return 1;
		}

		SOCKET ListenSocket = INVALID_SOCKET;

		ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (ListenSocket == INVALID_SOCKET)
		{
			std::cerr << "Invalid Socket";
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}

		iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			std::cerr << "bind error\n";
			freeaddrinfo(result);
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		freeaddrinfo(result);

		if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
			std::cerr << "Listen not working\n";
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		std::cout << "Server is listening on port: " << PORT << std::endl;

		while (true) {
			SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
			if (ClientSocket == INVALID_SOCKET) {
				std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
				continue;
			}

			std::cout << "Client connected!" << std::endl;
			std::thread clientThread(handleClient, ClientSocket);
			clientThread.detach();
		}


	}