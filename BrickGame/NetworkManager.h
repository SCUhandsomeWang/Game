#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include "NetworkProtocol.h"
#include <vector>
#include <queue>
#include <string>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <thread>
#include <atomic>
#include <chrono>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #ifndef NOGDI
    #define NOGDI
    #endif
    #ifndef NOUSER
    #define NOUSER
    #endif
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

#ifdef _WIN32
// Some MinGW environments may not define SIO_UDP_CONNRESET; provide a fallback
#ifndef SIO_UDP_CONNRESET
#include <windef.h>
#ifndef IOC_VENDOR
#define IOC_VENDOR 0x18000000
#endif
#ifndef _WSAIOW
#define _WSAIOW(x,y) (IOC_OUT | ((x) << 16) | (y))
#endif
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)
#endif
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
        FAILED
    };

private:
    Mode currentMode;
    ConnectionStatus connectionStatus;
    SOCKET udpSocket;
    SOCKET serverSocket;
    sockaddr_in remoteAddress;
    sockaddr_in serverAddress;
    // receive thread
    std::thread recvThread;
    std::atomic<bool> recvThreadRunning{false};
    
    // 消息队列
    std::queue<GameStateMessage> receivedGameStates;
    std::queue<PaddleUpdateMessage> receivedPaddleUpdates;
    std::queue<HelloMessage> receivedHellos;
    std::queue<RoomStateMessage> receivedRoomStates;
    std::queue<GameStartMessage> receivedGameStarts;
    
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
                // ensure sockaddr_in structs start zeroed to avoid garbage addresses
                memset(&remoteAddress, 0, sizeof(remoteAddress));
                memset(&serverAddress, 0, sizeof(serverAddress));
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
        printf("[Network] StartAsHost: entering, port=%d\n", port);
        
        // 创建UDP套接字
        udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        // On Windows, disable UDP connection-reset errors (WSAECONNRESET) caused by ICMP Port Unreachable
    #ifdef _WIN32
        {
            DWORD bytes = 0;
            BOOL bNewBehavior = FALSE; // FALSE to disable RST on UDP
            // SIO_UDP_CONNRESET prevents WSAECONNRESET when remote sends ICMP Port Unreachable
            WSAIoctl(udpSocket, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &bytes, NULL, NULL);
        }
    #endif
        printf("[Network] StartAsHost: socket() returned %d\n", (int)udpSocket);
        fflush(stdout);
        if (udpSocket == INVALID_SOCKET) {
            connectionStatus = ConnectionStatus::FAILED;
#ifdef _WIN32
            int err = WSAGetLastError();
            printf("[Network] StartAsHost: socket() failed, WSAGetLastError=%d\n", err);
#else
            perror("[Network] StartAsHost: socket() failed");
#endif
            return false;
        }
        
        // 绑定套接字
        sockaddr_in bindAddr;
        bindAddr.sin_family = AF_INET;
        // bind to all interfaces (INADDR_ANY) for normal operation
        bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        bindAddr.sin_port = htons(port);
        
        if (bind(udpSocket, (sockaddr*)&bindAddr, sizeof(bindAddr)) == SOCKET_ERROR) {
            int bindErr = 0;
#ifdef _WIN32
            bindErr = WSAGetLastError();
#else
            bindErr = errno;
#endif
            printf("[Network] StartAsHost: bind() failed, err=%d\n", bindErr);
            closesocket(udpSocket);
            udpSocket = INVALID_SOCKET;
            connectionStatus = ConnectionStatus::FAILED;
            return false;
        }
        // print bound address/port for debugging
    #ifdef _WIN32
        {
            unsigned long addr = bindAddr.sin_addr.S_un.S_addr;
            unsigned char b1 = addr & 0xFF;
            unsigned char b2 = (addr >> 8) & 0xFF;
            unsigned char b3 = (addr >> 16) & 0xFF;
            unsigned char b4 = (addr >> 24) & 0xFF;
            printf("[Network] Bound to %u.%u.%u.%u:%d\n", b1,b2,b3,b4, ntohs(bindAddr.sin_port));
        }
    #else
        printf("[Network] Bound to %s:%d\n", inet_ntoa(bindAddr.sin_addr), ntohs(bindAddr.sin_port));
    #endif
        
        // 设置非阻塞模式
        SetNonblocking();
        printf("[Network] StartAsHost: SetNonblocking done\n");
        fflush(stdout);
        
        connectionStatus = ConnectionStatus::CONNECTED;
        // start receive thread to ensure we pick up incoming packets even if main loop timing is off
        StartReceiveThread();
        printf("[Network] StartAsHost: returning success\n");
        fflush(stdout);
        return true;
    }
    
    // 作为客户端连接到服务器
    bool ConnectAsClient(const std::string& serverIP, int port = 5555) {
        currentMode = Mode::CLIENT;
        
        // 创建UDP套接字
        udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        // Disable WSAECONNRESET on Windows for UDP sockets
    #ifdef _WIN32
        {
            DWORD bytes = 0;
            BOOL bNewBehavior = FALSE;
            WSAIoctl(udpSocket, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &bytes, NULL, NULL);
        }
    #endif
        if (udpSocket == INVALID_SOCKET) {
            connectionStatus = ConnectionStatus::FAILED;
            return false;
        }
        
        // 设置远程地址
        remoteAddress.sin_family = AF_INET;
        remoteAddress.sin_port = htons(port);
        int p = inet_pton(AF_INET, serverIP.c_str(), &remoteAddress.sin_addr);
        if (p != 1) {
            // fallback for environments without inet_pton support or invalid format
            remoteAddress.sin_addr.s_addr = inet_addr(serverIP.c_str());
        }
        // debug: print resolved remote address
    #ifdef _WIN32
        unsigned long raddr = remoteAddress.sin_addr.S_un.S_addr;
        unsigned char rb1 = raddr & 0xFF; unsigned char rb2 = (raddr>>8)&0xFF; unsigned char rb3 = (raddr>>16)&0xFF; unsigned char rb4 = (raddr>>24)&0xFF;
        printf("[Network] ConnectAsClient: remote set to %u.%u.%u.%u:%d\n", rb1,rb2,rb3,rb4, ntohs(remoteAddress.sin_port)); fflush(stdout);
    #else
        printf("[Network] ConnectAsClient: remote set to %s:%d\n", inet_ntoa(remoteAddress.sin_addr), ntohs(remoteAddress.sin_port)); fflush(stdout);
    #endif
        
        // 设置非阻塞模式
        SetNonblocking();
        
        // 标记为已连接以允许发送握手
        connectionStatus = ConnectionStatus::CONNECTED;

        // start receive thread to pick up responses
        StartReceiveThread();

        // 发送握手消息（使用手动序列化，避免发送带 vptr 的对象内存）
        HelloMessage hello;
        hello.isHost = false;
        memset(hello.playerName, 0, sizeof(hello.playerName));
        strcpy_s(hello.playerName, sizeof(hello.playerName), "Client");
        hello.avatarIndex = 0;
        hello.ready = 0;
        // serialize
        std::vector<uint8_t> buf;
        buf.reserve((size_t)hello.GetSize());
        // type
        buf.push_back((uint8_t)hello.type);
        // timestamp (uint32_t little-endian)
        uint32_t ts = hello.timestamp;
        buf.push_back((uint8_t)(ts & 0xFF));
        buf.push_back((uint8_t)((ts >> 8) & 0xFF));
        buf.push_back((uint8_t)((ts >> 16) & 0xFF));
        buf.push_back((uint8_t)((ts >> 24) & 0xFF));
        // isHost as uint8_t
        buf.push_back(hello.isHost ? 1 : 0);
        // playerName (fixed 32)
        for (size_t i = 0; i < sizeof(hello.playerName); ++i) buf.push_back((uint8_t)hello.playerName[i]);
        // avatarIndex, ready
        buf.push_back(hello.avatarIndex);
        buf.push_back(hello.ready);
        SendPacket(buf.data(), buf.size());

        return true;
    }
    
    // 获取当前连接模式
    Mode GetMode() const { return currentMode; }
    
    // 获取连接状态
    ConnectionStatus GetStatus() const { return connectionStatus; }
    
    // 检查是否连接
    bool IsConnected() const { return connectionStatus == ConnectionStatus::CONNECTED; }
    
    // 发送原始消息
    bool SendPacket(const uint8_t* data, size_t length) {
        if (!IsConnected() || udpSocket == INVALID_SOCKET) {
            return false;
        }
        // 如果目标地址未设置，避免发送
        if (remoteAddress.sin_addr.s_addr == 0 || remoteAddress.sin_port == 0) {
            printf("[Network] SendPacket: remote address not set, skipping send (len=%zu)\n", length);
            fflush(stdout);
            return false;
        }
        // debug: print destination
    #ifdef _WIN32
        unsigned long addr = remoteAddress.sin_addr.S_un.S_addr;
        unsigned char b1 = addr & 0xFF;
        unsigned char b2 = (addr >> 8) & 0xFF;
        unsigned char b3 = (addr >> 16) & 0xFF;
        unsigned char b4 = (addr >> 24) & 0xFF;
        printf("[Network] SendPacket -> %u.%u.%u.%u:%d, len=%zu\n", b1,b2,b3,b4, ntohs(remoteAddress.sin_port), length);
    #else
        printf("[Network] SendPacket -> %s:%d, len=%zu\n", inet_ntoa(remoteAddress.sin_addr), ntohs(remoteAddress.sin_port), length);
    #endif
        // dump outgoing bytes
        int odump = (int)length < 12 ? (int)length : 12;
        printf("[Network] out data: ");
        for (int i = 0; i < odump; ++i) printf("%02X ", data[i]);
        printf("\n");
        fflush(stdout);
        // debug: if sending a GameState, print seq/timestamp
        if (length > 0 && data[0] == (uint8_t)MessageType::GAME_STATE) {
            // timestamp at offset 1, seq at offset 5 (little-endian)
            if (length >= 9) {
                uint32_t ts = (uint32_t)data[1] | ((uint32_t)data[2]<<8) | ((uint32_t)data[3]<<16) | ((uint32_t)data[4]<<24);
                uint32_t sq = (uint32_t)data[5] | ((uint32_t)data[6]<<8) | ((uint32_t)data[7]<<16) | ((uint32_t)data[8]<<24);
                printf("[Network] SendGameState seq=%u ts=%u\n", sq, ts);
                fflush(stdout);
            }
        }
        
        int result = sendto(udpSocket, (char*)data, (int)length, 0, 
                   (sockaddr*)&remoteAddress, sizeof(remoteAddress));
        if (result == SOCKET_ERROR) {
    #ifdef _WIN32
            int err = WSAGetLastError();
            printf("[Network] SendPacket failed, WSA=%d\n", err);
    #else
            perror("[Network] SendPacket failed");
    #endif
            fflush(stdout);
            return false;
        }
        return true;
    }
    
    // 发送紧凑的游戏状态（增量）：不包含砖块/道具数据，适用于高频发送
    bool SendGameState(const GameStateMessage& state) {
        if (currentMode != Mode::HOST) return false;
        std::vector<uint8_t> buf;
        // compact delta: type(1) + timestamp(4) + seq(4) + ball(5*4+1) + paddles(2*4*4) + scores/lives (4*4) + brickCount(4) + flags(2)
        buf.reserve(128);
        buf.push_back((uint8_t)MessageType::GAME_STATE);
        uint32_t ts = state.timestamp;
        buf.push_back((uint8_t)(ts & 0xFF)); buf.push_back((uint8_t)((ts>>8)&0xFF)); buf.push_back((uint8_t)((ts>>16)&0xFF)); buf.push_back((uint8_t)((ts>>24)&0xFF));
        // seq
        uint32_t sq = state.seq;
        buf.push_back((uint8_t)(sq & 0xFF)); buf.push_back((uint8_t)((sq>>8)&0xFF)); buf.push_back((uint8_t)((sq>>16)&0xFF)); buf.push_back((uint8_t)((sq>>24)&0xFF));
        auto push_f = [&](float f){ uint32_t u; memcpy(&u, &f, sizeof(float)); buf.push_back(u & 0xFF); buf.push_back((u>>8)&0xFF); buf.push_back((u>>16)&0xFF); buf.push_back((u>>24)&0xFF); };
        auto push_i = [&](int v){ uint32_t u = (uint32_t)v; buf.push_back(u & 0xFF); buf.push_back((u>>8)&0xFF); buf.push_back((u>>16)&0xFF); buf.push_back((u>>24)&0xFF); };
        // ball
        push_f(state.ball.posX); push_f(state.ball.posY); push_f(state.ball.velX); push_f(state.ball.velY); push_f(state.ball.radius);
        buf.push_back(state.ball.visible ? 1 : 0);
        // paddles
        push_f(state.hostPaddle.posX); push_f(state.hostPaddle.posY); push_f(state.hostPaddle.width); push_f(state.hostPaddle.height);
        push_f(state.guestPaddle.posX); push_f(state.guestPaddle.posY); push_f(state.guestPaddle.width); push_f(state.guestPaddle.height);
        // scores/lives
        push_i(state.hostScore); push_i(state.guestScore); push_i(state.hostLives); push_i(state.guestLives);
        // minimal brick info: count only (client will wait for snapshot for per-brick states)
        push_i(state.brickCount);
        // flags
        buf.push_back(state.widePaddleActive); buf.push_back(state.frenzyActive);
        return SendPacket(buf.data(), buf.size());
    }

    // 发送完整的游戏快照（低频）：包含砖块与道具完整数据
    bool SendGameStateSnapshot(const GameStateMessage& state) {
        if (currentMode != Mode::HOST) return false;
        std::vector<uint8_t> buf;
        buf.reserve((size_t)state.GetSize());
        buf.push_back((uint8_t)MessageType::GAME_STATE_SNAPSHOT);
        uint32_t ts = state.timestamp;
        buf.push_back((uint8_t)(ts & 0xFF)); buf.push_back((uint8_t)((ts>>8)&0xFF)); buf.push_back((uint8_t)((ts>>16)&0xFF)); buf.push_back((uint8_t)((ts>>24)&0xFF));
        // seq
        uint32_t sq = state.seq;
        buf.push_back((uint8_t)(sq & 0xFF)); buf.push_back((uint8_t)((sq>>8)&0xFF)); buf.push_back((uint8_t)((sq>>16)&0xFF)); buf.push_back((uint8_t)((sq>>24)&0xFF));
        auto push_f = [&](float f){ uint32_t u; memcpy(&u, &f, sizeof(float)); buf.push_back(u & 0xFF); buf.push_back((u>>8)&0xFF); buf.push_back((u>>16)&0xFF); buf.push_back((u>>24)&0xFF); };
        auto push_i = [&](int v){ uint32_t u = (uint32_t)v; buf.push_back(u & 0xFF); buf.push_back((u>>8)&0xFF); buf.push_back((u>>16)&0xFF); buf.push_back((u>>24)&0xFF); };
        // ball
        push_f(state.ball.posX); push_f(state.ball.posY); push_f(state.ball.velX); push_f(state.ball.velY); push_f(state.ball.radius);
        buf.push_back(state.ball.visible ? 1 : 0);
        // paddles
        push_f(state.hostPaddle.posX); push_f(state.hostPaddle.posY); push_f(state.hostPaddle.width); push_f(state.hostPaddle.height);
        push_f(state.guestPaddle.posX); push_f(state.guestPaddle.posY); push_f(state.guestPaddle.width); push_f(state.guestPaddle.height);
        // scores/lives
        push_i(state.hostScore); push_i(state.guestScore); push_i(state.hostLives); push_i(state.guestLives);
        // full bricks
        push_i(state.brickCount);
        for (size_t i = 0; i < sizeof(state.brickActive); ++i) buf.push_back(state.brickActive[i]);
        // powerups
        push_i(state.powerUpCount);
        for (int i = 0; i < MAX_SYNC_POWERUPS; ++i) { push_f(state.powerUps[i].posX); push_f(state.powerUps[i].posY); buf.push_back(state.powerUps[i].type); buf.push_back(state.powerUps[i].active); }
        buf.push_back(state.widePaddleActive); buf.push_back(state.frenzyActive);
        return SendPacket(buf.data(), buf.size());
    }
    
    // 发送板位置更新
    bool SendPaddleUpdate(const PaddleUpdateMessage& update) {
        if (currentMode != Mode::CLIENT) return false;
        std::vector<uint8_t> buf;
        buf.reserve((size_t)update.GetSize());
        buf.push_back((uint8_t)update.type);
        uint32_t ts = update.timestamp;
        buf.push_back((uint8_t)(ts & 0xFF)); buf.push_back((uint8_t)((ts>>8)&0xFF)); buf.push_back((uint8_t)((ts>>16)&0xFF)); buf.push_back((uint8_t)((ts>>24)&0xFF));
        uint32_t u;
        memcpy(&u, &update.paddle.posX, sizeof(float)); buf.push_back(u & 0xFF); buf.push_back((u>>8)&0xFF); buf.push_back((u>>16)&0xFF); buf.push_back((u>>24)&0xFF);
        memcpy(&u, &update.paddle.posY, sizeof(float)); buf.push_back(u & 0xFF); buf.push_back((u>>8)&0xFF); buf.push_back((u>>16)&0xFF); buf.push_back((u>>24)&0xFF);
        memcpy(&u, &update.paddle.width, sizeof(float)); buf.push_back(u & 0xFF); buf.push_back((u>>8)&0xFF); buf.push_back((u>>16)&0xFF); buf.push_back((u>>24)&0xFF);
        memcpy(&u, &update.paddle.height, sizeof(float)); buf.push_back(u & 0xFF); buf.push_back((u>>8)&0xFF); buf.push_back((u>>16)&0xFF); buf.push_back((u>>24)&0xFF);
        return SendPacket(buf.data(), buf.size());
    }
    
    // 接收消息
    void ReceiveMessages() {
        if (!IsConnected() || udpSocket == INVALID_SOCKET) {
            return;
        }
        // (log removed) receive loop is backgrounded; avoid noisy per-iteration prints
        
        uint8_t buffer[1024];
        sockaddr_in from;
        int fromLen = sizeof(from);
        
        int result = recvfrom(udpSocket, (char*)buffer, sizeof(buffer), 0, 
                             (sockaddr*)&from, &fromLen);
        if (result == SOCKET_ERROR) {
#ifdef _WIN32
            int err = WSAGetLastError();
            printf("[Network] recvfrom returned SOCKET_ERROR, WSA=%d\n", err); fflush(stdout);
            // Print additional info for non-WOULDBLOCK errors
            if (err != WSAEWOULDBLOCK) {
                printf("[Network] recvfrom non-WOULDBLOCK error (WSA=%d)\n", err); fflush(stdout);
            }
#else
            int err = errno;
            printf("[Network] recvfrom returned SOCKET_ERROR, errno=%d\n", err); fflush(stdout);
            if (err != EWOULDBLOCK && err != EAGAIN) {
                perror("[Network] recvfrom non-EWOULDBLOCK error"); fflush(stdout);
            }
#endif
            return;
        }

        if (result > 0) {
            // debug: print from addr and size
            unsigned short port = ntohs(from.sin_port);
            const char* fromIp = inet_ntoa(from.sin_addr);
            printf("[Network] Received %d bytes from %s:%d\n", result, fromIp, port);
            // dump first bytes
            int dump = result < 12 ? result : 12;
            printf("[Network] data: ");
            for (int i = 0; i < dump; ++i) printf("%02X ", buffer[i]);
            printf("\n");
            fflush(stdout);
            // 记录远程地址（用于主机模式）
            if (currentMode == Mode::HOST) {
                remoteAddress = from;
            }
            
            // 解析消息类型
            // safe parsing helpers (little-endian)
            auto read_u8 = [&](int& off)->uint8_t { uint8_t v = buffer[off]; off += 1; return v; };
            auto read_u32 = [&](int& off)->uint32_t { uint32_t v = (uint32_t)buffer[off] | ((uint32_t)buffer[off+1]<<8) | ((uint32_t)buffer[off+2]<<16) | ((uint32_t)buffer[off+3]<<24); off += 4; return v; };
            auto read_int = [&](int& off)->int { uint32_t u = read_u32(off); return (int)u; };
            auto read_float = [&](int& off)->float { uint32_t u = read_u32(off); float f; memcpy(&f, &u, sizeof(float)); return f; };
            auto read_bytes = [&](int& off, void* dst, size_t len){ memcpy(dst, buffer + off, len); off += (int)len; };

            int off = 0;
            MessageType msgType = (MessageType)read_u8(off);
            uint32_t timestamp = read_u32(off);
            printf("[Network] MessageType=%d, timestamp=%u\n", (int)msgType, timestamp);
            fflush(stdout);

            switch (msgType) {
                case MessageType::GAME_STATE: {
                    // compact delta: seq + ball + paddles + scores/lives + brickCount + flags
                    GameStateMessage state;
                    state.timestamp = timestamp;
                    uint32_t seq = read_u32(off);
                    state.seq = seq;
                    printf("[Network] Parsed GAME_STATE(delta) seq=%u timestamp=%u\n", state.seq, state.timestamp); fflush(stdout);
                    state.ball.posX = read_float(off);
                    state.ball.posY = read_float(off);
                    state.ball.velX = read_float(off);
                    state.ball.velY = read_float(off);
                    state.ball.radius = read_float(off);
                    state.ball.visible = read_u8(off) ? true : false;
                    // paddles
                    state.hostPaddle.posX = read_float(off);
                    state.hostPaddle.posY = read_float(off);
                    state.hostPaddle.width = read_float(off);
                    state.hostPaddle.height = read_float(off);
                    state.guestPaddle.posX = read_float(off);
                    state.guestPaddle.posY = read_float(off);
                    state.guestPaddle.width = read_float(off);
                    state.guestPaddle.height = read_float(off);
                    state.hostScore = read_int(off);
                    state.guestScore = read_int(off);
                    state.hostLives = read_int(off);
                    state.guestLives = read_int(off);
                    state.brickCount = read_int(off);
                    // DO NOT parse per-brick active array here; client will rely on latest snapshot
                    state.powerUpCount = 0;
                    state.widePaddleActive = read_u8(off);
                    state.frenzyActive = read_u8(off);
                    receivedGameStates.push(state);
                } break;

                case MessageType::GAME_STATE_SNAPSHOT: {
                    // full snapshot parsing (contains brickActive + powerUps)
                    GameStateMessage state;
                    state.timestamp = timestamp;
                    uint32_t seq = read_u32(off);
                    state.seq = seq;
                    printf("[Network] Parsed GAME_STATE_SNAPSHOT seq=%u timestamp=%u\n", state.seq, state.timestamp); fflush(stdout);
                    state.ball.posX = read_float(off);
                    state.ball.posY = read_float(off);
                    state.ball.velX = read_float(off);
                    state.ball.velY = read_float(off);
                    state.ball.radius = read_float(off);
                    state.ball.visible = read_u8(off) ? true : false;
                    // paddles
                    state.hostPaddle.posX = read_float(off);
                    state.hostPaddle.posY = read_float(off);
                    state.hostPaddle.width = read_float(off);
                    state.hostPaddle.height = read_float(off);
                    state.guestPaddle.posX = read_float(off);
                    state.guestPaddle.posY = read_float(off);
                    state.guestPaddle.width = read_float(off);
                    state.guestPaddle.height = read_float(off);
                    state.hostScore = read_int(off);
                    state.guestScore = read_int(off);
                    state.hostLives = read_int(off);
                    state.guestLives = read_int(off);
                    state.brickCount = read_int(off);
                    read_bytes(off, state.brickActive, sizeof(state.brickActive));
                    state.powerUpCount = read_int(off);
                    for (int i = 0; i < MAX_SYNC_POWERUPS; ++i) {
                        state.powerUps[i].posX = read_float(off);
                        state.powerUps[i].posY = read_float(off);
                        state.powerUps[i].type = read_u8(off);
                        state.powerUps[i].active = read_u8(off);
                    }
                    state.widePaddleActive = read_u8(off);
                    state.frenzyActive = read_u8(off);
                    receivedGameStates.push(state);
                } break;

                case MessageType::PADDLE_UPDATE: {
                    PaddleUpdateMessage update;
                    update.timestamp = timestamp;
                    update.paddle.posX = read_float(off);
                    update.paddle.posY = read_float(off);
                    update.paddle.width = read_float(off);
                    update.paddle.height = read_float(off);
                    receivedPaddleUpdates.push(update);
                } break;

                case MessageType::HELLO: {
                    HelloMessage hm;
                    hm.timestamp = timestamp;
                    hm.isHost = read_u8(off) ? true : false;
                    read_bytes(off, hm.playerName, sizeof(hm.playerName));
                    hm.avatarIndex = read_u8(off);
                    hm.ready = read_u8(off);
                    receivedHellos.push(hm);
                    if (currentMode == Mode::HOST) {
                        HelloMessage response;
                        response.isHost = true;
                        memset(response.playerName, 0, sizeof(response.playerName));
                        strcpy_s(response.playerName, sizeof(response.playerName), "Host");
                        response.avatarIndex = 0;
                        response.ready = 0;
                        // serialize response Hello
                        std::vector<uint8_t> rbuf;
                        rbuf.reserve((size_t)response.GetSize());
                        rbuf.push_back((uint8_t)response.type);
                        uint32_t rts = response.timestamp;
                        rbuf.push_back((uint8_t)(rts & 0xFF));
                        rbuf.push_back((uint8_t)((rts >> 8) & 0xFF));
                        rbuf.push_back((uint8_t)((rts >> 16) & 0xFF));
                        rbuf.push_back((uint8_t)((rts >> 24) & 0xFF));
                        rbuf.push_back(response.isHost ? 1 : 0);
                        for (size_t i = 0; i < sizeof(response.playerName); ++i) rbuf.push_back((uint8_t)response.playerName[i]);
                        rbuf.push_back(response.avatarIndex);
                        rbuf.push_back(response.ready);
                        SendPacket(rbuf.data(), rbuf.size());
                    }
                } break;

                case MessageType::ROOM_STATE: {
                    RoomStateMessage rsm;
                    rsm.timestamp = timestamp;
                    read_bytes(off, rsm.hostName, sizeof(rsm.hostName));
                    rsm.hostAvatar = read_u8(off);
                    rsm.hostReady = read_u8(off);
                    read_bytes(off, rsm.guestName, sizeof(rsm.guestName));
                    rsm.guestAvatar = read_u8(off);
                    rsm.guestReady = read_u8(off);
                    rsm.selectedLevel = read_u8(off);
                    rsm.playerCount = read_u8(off);
                    receivedRoomStates.push(rsm);
                    printf("[Network] Parsed ROOM_STATE: hostReady=%d guestReady=%d playerCount=%d selectedLevel=%d hostName='%s' guestName='%s'\n",
                           rsm.hostReady, rsm.guestReady, rsm.playerCount, rsm.selectedLevel, rsm.hostName, rsm.guestName);
                    fflush(stdout);
                } break;

                case MessageType::GAME_START: {
                    GameStartMessage gsm;
                    gsm.timestamp = timestamp;
                    gsm.difficulty = read_int(off);
                    gsm.isHost = read_u8(off) ? true : false;
                    receivedGameStarts.push(gsm);
                } break;

                case MessageType::SCORE_UPDATE: {
                    ScoreUpdateMessage sum;
                    sum.timestamp = timestamp;
                    sum.hostScore = read_int(off);
                    sum.guestScore = read_int(off);
                    sum.hostLives = read_int(off);
                    sum.guestLives = read_int(off);
                    // reuse received queue if needed (not pushed currently)
                } break;

                default:
                    break;
            }
        }
    }

    // receive loop for background thread
    void ReceiveLoop() {
        recvThreadRunning.store(true);
        printf("[Network] ReceiveLoop started\n"); fflush(stdout);
        while (recvThreadRunning.load()) {
            ReceiveMessages();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        printf("[Network] ReceiveLoop exiting\n"); fflush(stdout);
    }

    void StartReceiveThread() {
        bool expected = false;
        if (recvThreadRunning.compare_exchange_strong(expected, true)) {
            // spawn thread
            recvThread = std::thread([this]() { this->ReceiveLoop(); });
        }
    }

    void StopReceiveThread() {
        if (recvThreadRunning.load()) {
            recvThreadRunning.store(false);
            if (recvThread.joinable()) recvThread.join();
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

    // 发送房间状态（主机调用以广播当前room）
    bool SendRoomState(const RoomStateMessage& room) {
        std::vector<uint8_t> buf;
        buf.reserve((size_t)room.GetSize());
        buf.push_back((uint8_t)room.type);
        uint32_t ts = room.timestamp;
        buf.push_back((uint8_t)(ts & 0xFF)); buf.push_back((uint8_t)((ts>>8)&0xFF)); buf.push_back((uint8_t)((ts>>16)&0xFF)); buf.push_back((uint8_t)((ts>>24)&0xFF));
        for (size_t i = 0; i < sizeof(room.hostName); ++i) buf.push_back((uint8_t)room.hostName[i]);
        buf.push_back(room.hostAvatar); buf.push_back(room.hostReady);
        for (size_t i = 0; i < sizeof(room.guestName); ++i) buf.push_back((uint8_t)room.guestName[i]);
        buf.push_back(room.guestAvatar); buf.push_back(room.guestReady);
        buf.push_back(room.selectedLevel); buf.push_back(room.playerCount);
        return SendPacket(buf.data(), buf.size());
    }

    // 发送带信息的Hello（客户端用于发送名字/头像/准备状态）
    bool SendHelloMessage(const HelloMessage& hello) {
        // Debug: print hello contents (name/avatar/ready)
        printf("[Network] SendHello: name='%s' avatar=%d ready=%d\n", hello.playerName, hello.avatarIndex, hello.ready);
        fflush(stdout);
        std::vector<uint8_t> buf;
        buf.reserve((size_t)hello.GetSize());
        buf.push_back((uint8_t)hello.type);
        uint32_t ts = hello.timestamp;
        buf.push_back((uint8_t)(ts & 0xFF));
        buf.push_back((uint8_t)((ts >> 8) & 0xFF));
        buf.push_back((uint8_t)((ts >> 16) & 0xFF));
        buf.push_back((uint8_t)((ts >> 24) & 0xFF));
        buf.push_back(hello.isHost ? 1 : 0);
        for (size_t i = 0; i < sizeof(hello.playerName); ++i) buf.push_back((uint8_t)hello.playerName[i]);
        buf.push_back(hello.avatarIndex);
        buf.push_back(hello.ready);
        return SendPacket(buf.data(), buf.size());
    }

    // 发送游戏开始消息
    bool SendGameStart(const GameStartMessage& msg) {
        std::vector<uint8_t> buf;
        buf.reserve((size_t)msg.GetSize());
        buf.push_back((uint8_t)msg.type);
        uint32_t ts = msg.timestamp;
        buf.push_back((uint8_t)(ts & 0xFF)); buf.push_back((uint8_t)((ts>>8)&0xFF)); buf.push_back((uint8_t)((ts>>16)&0xFF)); buf.push_back((uint8_t)((ts>>24)&0xFF));
        // difficulty (int)
        uint32_t di = (uint32_t)msg.difficulty;
        buf.push_back((uint8_t)(di & 0xFF)); buf.push_back((uint8_t)((di>>8)&0xFF)); buf.push_back((uint8_t)((di>>16)&0xFF)); buf.push_back((uint8_t)((di>>24)&0xFF));
        buf.push_back(msg.isHost ? 1 : 0);
        return SendPacket(buf.data(), buf.size());
    }

    // 发送分数更新消息
    bool SendScoreUpdate(const ScoreUpdateMessage& msg) {
        std::vector<uint8_t> buf;
        buf.reserve((size_t)msg.GetSize());
        buf.push_back((uint8_t)msg.type);
        uint32_t ts = msg.timestamp;
        buf.push_back((uint8_t)(ts & 0xFF)); buf.push_back((uint8_t)((ts>>8)&0xFF)); buf.push_back((uint8_t)((ts>>16)&0xFF)); buf.push_back((uint8_t)((ts>>24)&0xFF));
        auto push_i = [&](int v){ uint32_t u = (uint32_t)v; buf.push_back(u & 0xFF); buf.push_back((u>>8)&0xFF); buf.push_back((u>>16)&0xFF); buf.push_back((u>>24)&0xFF); };
        push_i(msg.hostScore); push_i(msg.guestScore); push_i(msg.hostLives); push_i(msg.guestLives);
        return SendPacket(buf.data(), buf.size());
    }

    // 获取接收到的Hello消息
    bool GetReceivedHello(HelloMessage& out) {
        if (!receivedHellos.empty()) {
            out = receivedHellos.front();
            receivedHellos.pop();
            return true;
        }
        return false;
    }

    // 获取接收到的房间状态
    bool GetReceivedRoomState(RoomStateMessage& out) {
        if (!receivedRoomStates.empty()) {
            out = receivedRoomStates.front();
            receivedRoomStates.pop();
            return true;
        }
        return false;
    }

    // 获取接收到的GameStart消息
    bool GetReceivedGameStart(GameStartMessage& out) {
        if (!receivedGameStarts.empty()) {
            out = receivedGameStarts.front();
            receivedGameStarts.pop();
            return true;
        }
        return false;
    }
    
    // 关闭连接
    void Shutdown() {
        // stop receive thread first
        StopReceiveThread();
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
