#include "client_logic.h"
#include <WS2tcpip.h>
#include <thread>
#include <iostream>

using namespace std;

ChatClient::ChatClient(const string& server, int port)
    : clientSocket(INVALID_SOCKET), isConnected(false),
    serverIP(server), serverPort(port), username("User") {
}

ChatClient::~ChatClient() {
    Disconnect();
}

bool ChatClient::Initialize() {
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
}

bool ChatClient::Connect() {
    if (isConnected) {
        if (messageCallback) messageCallback("Already connected!");
        return false;
    }

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        if (messageCallback) messageCallback("Socket creation failed!");
        return false;
    }

    // Enable address reuse for client too
    int opt = 1;
    setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    // Setup server address
    sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIP.c_str(), &(serveraddr.sin_addr));

    // Connect to server
    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR) {
        if (messageCallback) messageCallback("Connection failed! Is the server running?");
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        return false;
    }

    isConnected = true;
    if (messageCallback) messageCallback("Connected to server!");
    if (connectionCallback) connectionCallback(true);

    // Start receive thread
    thread receiveThread(&ChatClient::ReceiveMessages, this);
    receiveThread.detach();

    return true;
}

void ChatClient::Disconnect() {
    if (!isConnected) return;

    isConnected = false;

    if (clientSocket != INVALID_SOCKET) {
        shutdown(clientSocket, SD_BOTH);
        Sleep(50);  // Give time for graceful shutdown
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }

    if (messageCallback) messageCallback("Disconnected from server");
    if (connectionCallback) connectionCallback(false);
}

bool ChatClient::SendMessage(const string& message) {
    if (!isConnected) {
        if (messageCallback) messageCallback("Not connected to server!");
        return false;
    }

    string fullMessage = username + ": " + message;
    int bytesSent = send(clientSocket, fullMessage.c_str(), fullMessage.length(), 0);

    if (bytesSent == SOCKET_ERROR) {
        if (messageCallback) messageCallback("Failed to send message!");
        Disconnect();
        return false;
    }

    // Echo own message
    if (messageCallback) messageCallback("You: " + message);

    return true;
}

void ChatClient::ReceiveMessages() {
    char buffer[4096];

    while (isConnected) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            if (isConnected) {
                if (messageCallback) messageCallback("Connection lost!");
                Disconnect();
            }
            break;
        }

        string message(buffer, bytesReceived);
        if (messageCallback) messageCallback(message);
    }
}

void ChatClient::SetUsername(const string& name) {
    if (!name.empty()) {
        username = name;
    }
}

void ChatClient::SetMessageCallback(MessageCallback callback) {
    messageCallback = callback;
}

void ChatClient::SetConnectionCallback(ConnectionCallback callback) {
    connectionCallback = callback;
}