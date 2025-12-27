#include "server_logic.h"
#include <WS2tcpip.h>
#include <thread>
#include <algorithm>
#include <iostream>

using namespace std;

ChatServer::ChatServer(int serverPort)
    : listenSocket(INVALID_SOCKET), isRunning(false), port(serverPort) {
}

ChatServer::~ChatServer() {
    Stop();
}

bool ChatServer::Initialize() {
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
}

bool ChatServer::Start() {
    if (isRunning) {
        if (logCallback) logCallback("Server is already running!");
        return false;
    }

    // Create socket
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        if (logCallback) logCallback("Socket creation failed!");
        return false;
    }

    // Allow socket reuse
    int opt = 1;
    if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        if (logCallback) logCallback("Warning: Could not set socket options");
    }

    // Setup address
    sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    // Bind
    if (::bind(listenSocket, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR) {
        if (logCallback) logCallback("Bind failed!");
        closesocket(listenSocket);
        return false;
    }

    // Listen
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        if (logCallback) logCallback("Listen failed!");
        closesocket(listenSocket);
        return false;
    }

    isRunning = true;
    if (logCallback) logCallback("Server started on port " + to_string(port));

    // Accept loop in separate thread
    thread acceptThread([this]() {
        while (isRunning) {
            SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);

            if (clientSocket == INVALID_SOCKET) {
                if (isRunning) {
                    if (logCallback) logCallback("Accept failed!");
                }
                continue;
            }

            {
                lock_guard<mutex> lock(clientsMutex);
                clients.push_back(clientSocket);
                if (logCallback) logCallback("Client connected! Total: " + to_string(clients.size()));
                if (clientCountCallback) clientCountCallback(clients.size());
            }

            thread clientThread(&ChatServer::InteractWithClient, this, clientSocket);
            clientThread.detach();
        }
        });
    acceptThread.detach();

    return true;
}

void ChatServer::Stop() {
    if (!isRunning) return;

    isRunning = false;

    // Close listen socket first to stop accepting new connections
    if (listenSocket != INVALID_SOCKET) {
        shutdown(listenSocket, SD_BOTH);
        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
    }

    // Give it a moment for threads to notice
    Sleep(100);

    // Close all client sockets
    {
        lock_guard<mutex> lock(clientsMutex);
        for (auto client : clients) {
            shutdown(client, SD_BOTH);
            closesocket(client);
        }
        clients.clear();
    }

    if (logCallback) logCallback("Server stopped");
    if (clientCountCallback) clientCountCallback(0);
}

void ChatServer::InteractWithClient(SOCKET clientSocket) {
    char buffer[4096];

    while (isRunning) {
        memset(buffer, 0, sizeof(buffer));
        int bytesreced = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesreced <= 0) {
            if (logCallback) logCallback("Client disconnected");
            break;
        }

        string message(buffer, bytesreced);
        if (logCallback) logCallback("Message: " + message);

        BroadcastMessage(message, clientSocket);
    }

    // Remove client
    {
        lock_guard<mutex> lock(clientsMutex);
        auto it = find(clients.begin(), clients.end(), clientSocket);
        if (it != clients.end()) {
            clients.erase(it);
            if (clientCountCallback) clientCountCallback(clients.size());
        }
    }

    closesocket(clientSocket);
}

void ChatServer::BroadcastMessage(const string& message, SOCKET senderSocket) {
    lock_guard<mutex> lock(clientsMutex);
    for (auto client : clients) {
        if (client != senderSocket) {
            int bytesSent = send(client, message.c_str(), message.length(), 0);
            if (bytesSent == SOCKET_ERROR) {
                if (logCallback) logCallback("Failed to send to a client");
            }
        }
    }
}

void ChatServer::SetLogCallback(LogCallback callback) {
    logCallback = callback;
}

void ChatServer::SetClientCountCallback(ClientCountCallback callback) {
    clientCountCallback = callback;
}

int ChatServer::GetClientCount() {
    lock_guard<mutex> lock(clientsMutex);
    return clients.size();
}