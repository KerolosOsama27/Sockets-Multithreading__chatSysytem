#ifndef CLIENT_LOGIC_H
#define CLIENT_LOGIC_H

#include <WinSock2.h>
#include <string>
#include <atomic>
#include <functional>

#pragma comment(lib, "ws2_32.lib")

// Callback types for GUI updates
typedef std::function<void(const std::string&)> MessageCallback;
typedef std::function<void(bool)> ConnectionCallback;

class ChatClient {
private:
    SOCKET clientSocket;
    std::atomic<bool> isConnected;
    std::string username;
    std::string serverIP;
    int serverPort;

    // Callbacks for GUI
    MessageCallback messageCallback;
    ConnectionCallback connectionCallback;

    void ReceiveMessages();

public:
    ChatClient(const std::string& server = "127.0.0.1", int port = 12345);
    ~ChatClient();

    // Core functions
    bool Initialize();
    bool Connect();
    void Disconnect();
    bool SendMessage(const std::string& message);

    // Setters
    void SetUsername(const std::string& name);
    void SetMessageCallback(MessageCallback callback);
    void SetConnectionCallback(ConnectionCallback callback);

    // Getters
    bool IsConnected() const { return isConnected; }
    std::string GetUsername() const { return username; }
};

#endif // CLIENT_LOGIC_H