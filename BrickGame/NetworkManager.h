#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include "NetworkProtocol.h"
#include <vector>
#include <queue>
#include <string>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define closesocket close
    typedef int SOCKET;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

class NetworkManager {
public:
    enum class Mode {
        OFFLINE,
        HOST,
        CLIENT
    };

    enum class ConnectionStatus {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        ERROR
    };

private:
    Mode currentMode;
    ConnectionStatus connectionStatus;
    SOCKET udpSocket;
    SOCKET serverSocket;
    sockaddr_in remoteAddress;
    sockaddr_in serverAddress;
    
    // 消息队列
    std::queue<GameStateMessage> receivedGameStates;
    std::queue<PaddleUpdateMessage> receivedPaddleUpdates;
    
    // 时间戳用于消息同步
    float lastStateUpdateTime;
    float stateUpdateInterval;  // 每秒发送状态更新的次数（Hz）

public:
    NetworkManager() 
        : currentMode(Mode::OFFLINE),
          connectionStatus(ConnectionStatus::DISCONNECTED),
          udpSocket(INVALID_SOCKET),
          serverSocket(INVALID_SOCKET),
          lastStateUpdateTime(0),
          stateUpdateInterval(30.0f) {
        InitializeWinsock();
    }
    
    ~NetworkManager() {
        Shutdown();
        CleanupWinsock();
    }
    
    // 初始化Winsock
    bool InitializeWinsock() {
#ifdef _WIN32
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        return iResult == 0;
#endif
        return true;
    }
    
    // 清理Winsock
    void CleanupWinsock() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
    
    // 作为主机启动服务器
    bool StartAsHost(int port = 5555) {
        currentMode = Mode::HOST;
        
        // 创建UDP套接字
        udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (udpSocket == INVALID_SOCKET) {
            connectionStatus = ConnectionStatus::ERROR;
            return false;
        }
        
        // 绑定套接字
        sockaddr_in bindAddr;
        bindAddr.sin_family = AF_INET;
        bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        bindAddr.sin_port = htons(port);
        
        if (bind(udpSocket, (sockaddr*)&bindAddr, sizeof(bindAddr)) == SOCKET_ERROR) {
            closesocket(udpSocket);
            udpSocket = INVALID_SOCKET;
            connectionStatus = ConnectionStatus::ERROR;
            return false;
        }
        
        // 设置非阻塞模式
        SetNonblocking();
        
        connectionStatus = ConnectionStatus::CONNECTED;
        return true;
    }
    
    // 作为客户端连接到服务器
    bool ConnectAsClient(const std::string& serverIP, int port = 5555) {
        currentMode = Mode::CLIENT;
        
        // 创建UDP套接字
        udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (udpSocket == INVALID_SOCKET) {
            connectionStatus = ConnectionStatus::ERROR;
            return false;
        }
        
        // 设置远程地址
        remoteAddress.sin_family = AF_INET;
        remoteAddress.sin_port = htons(port);
        inet_pton(AF_INET, serverIP.c_str(), &remoteAddress.sin_addr);
        
        // 设置非阻塞模式
        SetNonblocking();
        
        // 发送握手消息
        HelloMessage hello;
        hello.isHost = false;
        strcpy_s(hello.playerName, sizeof(hello.playerName), "Client");
        SendMessage((uint8_t*)&hello, hello.GetSize());
        
        connectionStatus = ConnectionStatus::CONNECTED;
        return true;
    }
    
    // 获取当前连接模式
    Mode GetMode() const { return currentMode; }
    
    // 获取连接状态
    ConnectionStatus GetStatus() const { return connectionStatus; }
    
    // 检查是否连接
    bool IsConnected() const { return connectionStatus == ConnectionStatus::CONNECTED; }
    
    // 发送原始消息
    bool SendMessage(const uint8_t* data, size_t length) {
        if (!IsConnected() || udpSocket == INVALID_SOCKET) {
            return false;
        }
        
        int result = sendto(udpSocket, (char*)data, (int)length, 0, 
                           (sockaddr*)&remoteAddress, sizeof(remoteAddress));
        return result != SOCKET_ERROR;
    }
    
    // 发送游戏状态
    bool SendGameState(const GameStateMessage& state) {
        if (currentMode != Mode::HOST) return false;
        return SendMessage((uint8_t*)&state, state.GetSize());
    }
    
    // 发送板位置更新
    bool SendPaddleUpdate(const PaddleUpdateMessage& update) {
        if (currentMode != Mode::CLIENT) return false;
        return SendMessage((uint8_t*)&update, update.GetSize());
    }
    
    // 接收消息
    void ReceiveMessages() {
        if (!IsConnected() || udpSocket == INVALID_SOCKET) {
            return;
        }
        
        uint8_t buffer[1024];
        sockaddr_in from;
        int fromLen = sizeof(from);
        
        int result = recvfrom(udpSocket, (char*)buffer, sizeof(buffer), 0, 
                             (sockaddr*)&from, &fromLen);
        
        if (result > 0) {
            // 记录远程地址（用于主机模式）
            if (currentMode == Mode::HOST) {
                remoteAddress = from;
            }
            
            // 解析消息类型
            MessageType msgType = *(MessageType*)buffer;
            
            switch (msgType) {
                case MessageType::GAME_STATE:
                    if (result >= (int)sizeof(GameStateMessage)) {
                        GameStateMessage* state = (GameStateMessage*)buffer;
                        receivedGameStates.push(*state);
                    }
                    break;
                    
                case MessageType::PADDLE_UPDATE:
                    if (result >= (int)sizeof(PaddleUpdateMessage)) {
                        PaddleUpdateMessage* update = (PaddleUpdateMessage*)buffer;
                        receivedPaddleUpdates.push(*update);
                    }
                    break;
                    
                case MessageType::HELLO:
                    if (currentMode == Mode::HOST) {
                        // 主机收到客户端握手，发送欢迎响应
                        HelloMessage response;
                        response.isHost = true;
                        strcpy_s(response.playerName, sizeof(response.playerName), "Host");
                        SendMessage((uint8_t*)&response, response.GetSize());
                    }
                    break;
                    
                default:
                    break;
            }
        }
    }
    
    // 获取接收到的游戏状态
    bool GetReceivedGameState(GameStateMessage& state) {
        if (!receivedGameStates.empty()) {
            state = receivedGameStates.front();
            receivedGameStates.pop();
            return true;
        }
        return false;
    }
    
    // 获取接收到的板更新
    bool GetReceivedPaddleUpdate(PaddleUpdateMessage& update) {
        if (!receivedPaddleUpdates.empty()) {
            update = receivedPaddleUpdates.front();
            receivedPaddleUpdates.pop();
            return true;
        }
        return false;
    }
    
    // 关闭连接
    void Shutdown() {
        if (udpSocket != INVALID_SOCKET) {
            closesocket(udpSocket);
            udpSocket = INVALID_SOCKET;
        }
        if (serverSocket != INVALID_SOCKET) {
            closesocket(serverSocket);
            serverSocket = INVALID_SOCKET;
        }
        connectionStatus = ConnectionStatus::DISCONNECTED;
    }
    
    // 设置状态更新间隔（Hz）
    void SetStateUpdateInterval(float hz) {
        if (hz > 0) stateUpdateInterval = hz;
    }
    
    // 检查是否应该发送状态更新
    bool ShouldUpdateState(float deltaTime) {
        lastStateUpdateTime += deltaTime;
        float interval = 1.0f / stateUpdateInterval;
        if (lastStateUpdateTime >= interval) {
            lastStateUpdateTime = 0;
            return true;
        }
        return false;
    }
    
    // 获取网络管理器信息用于游戏集成
    const sockaddr_in& GetRemoteAddress() const {
        return remoteAddress;
    }

private:
    // 设置非阻塞模式
    void SetNonblocking() {
#ifdef _WIN32
        unsigned long mode = 1;
        ioctlsocket(udpSocket, FIONBIO, &mode);
#else
        int flags = fcntl(udpSocket, F_GETFL, 0);
        fcntl(udpSocket, F_SETFL, flags | O_NONBLOCK);
#endif
    }
};

#endif // NETWORKMANAGER_H
