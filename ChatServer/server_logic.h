#ifndef SERVER_LOGIC_H
#define SERVER_LOGIC_H

#include <WinSock2.h>
#include <vector>
#include <mutex>
#include <string>
#include <functional>

#pragma comment(lib,"ws2_32.lib")

// Callback types for GUI updates
typedef std::function<void(const std::string&)> LogCallback;
typedef std::function<void(int)> ClientCountCallback;

class ChatServer {
private:
    SOCKET listenSocket;
    std::vector<SOCKET> clients;
    std::mutex clientsMutex;
    bool isRunning;
    int port;

    // Callbacks for GUI
    LogCallback logCallback;
    ClientCountCallback clientCountCallback;

    void InteractWithClient(SOCKET clientSocket);
    void BroadcastMessage(const std::string& message, SOCKET senderSocket);

public:
    ChatServer(int serverPort = 12345);
    ~ChatServer();

    // Core functions
    bool Initialize();
    bool Start();
    void Stop();

    // Setters for callbacks
    void SetLogCallback(LogCallback callback);
    void SetClientCountCallback(ClientCountCallback callback);

    // Getters
    bool IsRunning() const { return isRunning; }
    int GetClientCount();
    int GetPort() const { return port; }
};

#endif // SERVER_LOGIC_H