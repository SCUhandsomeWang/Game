#ifndef NETWORKGAMEMODE_H
#define NETWORKGAMEMODE_H

#include "NetworkManager.h"
#include "Interpolation.h"
#include "NetworkConfig.h"
#include "raylib.h"
#include <cstdio>
#include <string>
#include <cstring>

// 网络游戏模式高层包装器 - 处理插值和游戏集成
class NetworkGameMode {
private:
    NetworkManager networkManager;
    
    // 远程板位置平滑
    InterpolationSmoothing::InterpolatedObject remotePaddleInterp;
    InterpolationSmoothing::InterpolatedObject remoteBallInterp;
    
    // 游戏状态缓存
    GameStateMessage lastReceivedState;
    PaddleUpdateMessage lastReceivedPaddleUpdate;

    // 房间/玩家信息（仅支持最多2人）
    std::string localPlayerName;
    uint8_t localAvatar = 0;
    bool localReady = false;

    std::string remotePlayerName;
    uint8_t remoteAvatar = 0;
    bool remoteReady = false;
    uint8_t roomPlayerCount = 0;
    bool inRoom = false;
    bool gameStarting = false;
    int startLevel = 0;
    int hostedSelectedLevel = 0;
    bool hostSentStart = false;

public:
    NetworkGameMode() {
        // 初始化平滑对象
        remotePaddleInterp = InterpolationSmoothing::CreatePaddleInterpolation({0, 0});
        remoteBallInterp = InterpolationSmoothing::CreateBallInterpolation({0, 0});
        remoteBallInterp.interpolationSpeed = NetworkConfig::BALL_INTERPOLATION_SPEED;
        remotePaddleInterp.interpolationSpeed = NetworkConfig::PADDLE_INTERPOLATION_SPEED;
    }
    
    ~NetworkGameMode() {
        Disconnect();
    }
    
    // 委托到NetworkManager的方法
    bool StartAsHost(int port = 5555) {
        bool ok = networkManager.StartAsHost(port);
        if (ok) {
            if (localPlayerName.empty()) localPlayerName = "Host";
            localAvatar = 0;
            // 默认为主机已准备，方便由 Host 点击 Start 直接开始（Host 可在 UI 关闭/改变）
            localReady = true;
            // 立即广播 room state so clients see host ready
            SendRoomStateIfHost();
            // also send Hello
            SendLocalPlayerInfo();
            printf("[NetworkGameMode] StartAsHost: set localReady=1 and broadcast room\n");
            fflush(stdout);
        }
        return ok;
    }
    
    bool ConnectAsClient(const char* hostIP, int port = 5555) {
        bool ok = networkManager.ConnectAsClient(hostIP, port);
        if (ok) {
            // 如果本地名字为空，填充默认（不自动 ready，也不自动发送信息）
            if (localPlayerName.empty()) localPlayerName = "Client";
            localAvatar = 0;
        }
        return ok;
    }
    
    bool IsConnected() const { 
        return networkManager.IsConnected(); 
    }
    
    NetworkManager::Mode GetMode() const { 
        return networkManager.GetMode(); 
    }
    
    NetworkManager::ConnectionStatus GetStatus() const { 
        return networkManager.GetStatus(); 
    }
    
    bool SendGameState(const GameStateMessage& state) {
        return networkManager.SendGameState(state);
    }
    bool SendGameStateSnapshot(const GameStateMessage& state) {
        return networkManager.SendGameStateSnapshot(state);
    }
    
    bool SendPaddleUpdate(const PaddleUpdateMessage& update) {
        return networkManager.SendPaddleUpdate(update);
    }

    // Player info / room helpers
    void SetLocalPlayerInfo(const std::string& name, uint8_t avatar) {
        localPlayerName = name;
        localAvatar = avatar;
    }

    void SendLocalPlayerInfo() {
        HelloMessage hm;
        hm.isHost = (GetMode() == NetworkManager::Mode::HOST);
        memset(hm.playerName, 0, sizeof(hm.playerName));
        strncpy_s(hm.playerName, sizeof(hm.playerName), localPlayerName.c_str(), _TRUNCATE);
        hm.avatarIndex = localAvatar;
        hm.ready = localReady ? 1 : 0;
        printf("[NetworkGameMode] SendLocalPlayerInfo: ready=%d mode=%d name='%s'\n", hm.ready, (int)GetMode(), hm.playerName);
        fflush(stdout);
        networkManager.SendHelloMessage(hm);
    }

    void SendRoomStateIfHost() {
        if (GetMode() != NetworkManager::Mode::HOST) return;
        RoomStateMessage r;
        memset(r.hostName, 0, sizeof(r.hostName));
        strncpy_s(r.hostName, sizeof(r.hostName), localPlayerName.c_str(), _TRUNCATE);
        r.hostAvatar = localAvatar;
        r.hostReady = localReady ? 1 : 0;

        memset(r.guestName, 0, sizeof(r.guestName));
        if (!remotePlayerName.empty()) {
            strncpy_s(r.guestName, sizeof(r.guestName), remotePlayerName.c_str(), _TRUNCATE);
            r.guestAvatar = remoteAvatar;
            r.guestReady = remoteReady ? 1 : 0;
            r.playerCount = 2;
        } else {
            r.guestAvatar = 0; r.guestReady = 0; r.playerCount = 1;
        }
        r.selectedLevel = hostedSelectedLevel;
        networkManager.SendRoomState(r);
    }

    void SetHostedSelectedLevel(int lvl) { hostedSelectedLevel = lvl; }
    int GetHostedSelectedLevel() const { return hostedSelectedLevel; }

    void ToggleLocalReady() {
        localReady = !localReady;
        printf("[NetworkGameMode] ToggleLocalReady: new localReady=%d mode=%d\n", localReady ? 1 : 0, (int)GetMode());
        fflush(stdout);
        if (GetMode() == NetworkManager::Mode::CLIENT) {
            SendLocalPlayerInfo();
        } else {
            SendRoomStateIfHost();
        }
    }

    bool AllPlayersReady() const {
        if (roomPlayerCount < 2) return false;
        return localReady && remoteReady;
    }
    
    // 高层功能 - 处理插值和同步
    void Update(float deltaTime) {
        if (!IsConnected()) return;
        
        // 接收网络消息
        networkManager.ReceiveMessages();

        // 处理接收到的Hello（用于host收到client信息）
        HelloMessage hm;
        while (networkManager.GetReceivedHello(hm)) {
            // 如果我是主机，收到client信息后更新guest信息并广播room
            if (GetMode() == NetworkManager::Mode::HOST) {
                remotePlayerName = std::string(hm.playerName);
                remoteAvatar = hm.avatarIndex;
                remoteReady = hm.ready != 0;
                roomPlayerCount = 2;
                inRoom = true;
                // 广播更新的room信息给client
                SendRoomStateIfHost();
            } else {
                // 客户端收到Host的简单Hello也可能进入此处，忽略
            }
        }

        // 处理接收到的RoomState（主机广播或初次加入后主机发送）
        RoomStateMessage rsm;
        while (networkManager.GetReceivedRoomState(rsm)) {
            // 填充远程信息（如果我是客户端，host信息在hostName）
            if (GetMode() == NetworkManager::Mode::CLIENT) {
                // host info
                remotePlayerName = std::string(rsm.hostName);
                remoteAvatar = rsm.hostAvatar;
                remoteReady = rsm.hostReady != 0;
                // guest slot may be other client, but for 2-player, guest is this client
                roomPlayerCount = rsm.playerCount;
                inRoom = true;
                hostedSelectedLevel = rsm.selectedLevel;
            } else {
                // 主机收到自己广播回显（可用于本地UI同步），更新guest
                if (rsm.playerCount >= 2) {
                    remotePlayerName = std::string(rsm.guestName);
                    remoteAvatar = rsm.guestAvatar;
                    remoteReady = rsm.guestReady != 0;
                    roomPlayerCount = 2;
                    inRoom = true;
                    hostedSelectedLevel = rsm.selectedLevel;
                }
            }
        }

        // 主机端不再自动广播 GAME_START（恢复为手动由 UI 触发）

        // 处理GameStart消息
        GameStartMessage gsm;
        while (networkManager.GetReceivedGameStart(gsm)) {
            // 无论主机或客户端接收到 GAME_START，都将进入游戏开始状态
            gameStarting = true;
            startLevel = gsm.difficulty;
            printf("[NetworkGameMode] Received GAME_START: difficulty=%d mode=%d\n", gsm.difficulty, (int)GetMode());
            fflush(stdout);
            // 如果我是主机，收到外部的 GAME_START 时也应广播给客户端
            if (GetMode() == NetworkManager::Mode::HOST) {
                networkManager.SendGameStart(gsm);
                printf("[NetworkGameMode] Re-broadcasting GAME_START to clients\n");
                fflush(stdout);
            }
        }
        
        // 处理接收到的游戏状态
        GameStateMessage gameState;
        while (networkManager.GetReceivedGameState(gameState)) {
            lastReceivedState = gameState;
            
            // 更新远程球的插值目标
            remoteBallInterp.targetPos = gameState.ball.GetPosition();
            remoteBallInterp.isInterpolating = true;
            remoteBallInterp.interpolationSpeed = NetworkConfig::BALL_INTERPOLATION_SPEED;
            
            // 更新远程板的插值目标（取决于是主机还是客户端）
            if (GetMode() == NetworkManager::Mode::CLIENT) {
                remotePaddleInterp.targetPos = gameState.hostPaddle.GetPosition();
            } else {
                remotePaddleInterp.targetPos = gameState.guestPaddle.GetPosition();
            }
            remotePaddleInterp.isInterpolating = true;
            remotePaddleInterp.interpolationSpeed = NetworkConfig::PADDLE_INTERPOLATION_SPEED;
        }
        
        // 处理接收到的板更新
        PaddleUpdateMessage paddleUpdate;
        while (networkManager.GetReceivedPaddleUpdate(paddleUpdate)) {
            lastReceivedPaddleUpdate = paddleUpdate;
            remotePaddleInterp.targetPos = paddleUpdate.paddle.GetPosition();
            remotePaddleInterp.isInterpolating = true;
        }
        
        // 更新插值
        InterpolationSmoothing::UpdateInterpolation(remoteBallInterp, deltaTime);
        InterpolationSmoothing::UpdateInterpolation(remotePaddleInterp, deltaTime);
    }
    
    // 获取远程球的当前（平滑后的）位置
    const Vector2& GetRemoteBallPosition() const {
        return remoteBallInterp.currentPos;
    }
    
    // 获取远程板的当前（平滑后的）位置
    const Vector2& GetRemotePaddlePosition() const {
        return remotePaddleInterp.currentPos;
    }
    
    // 获取远程球的最后接收到的完整状态
    const GameStateMessage& GetLastReceivedGameState() const {
        return lastReceivedState;
    }
    
    // 获取远程板的最后接收到的完整状态
    const PaddleUpdateMessage& GetLastReceivedPaddleUpdate() const {
        return lastReceivedPaddleUpdate;
    }

    // 房间/玩家状态访问器
    const std::string& GetLocalPlayerName() const { return localPlayerName; }
    uint8_t GetLocalAvatar() const { return localAvatar; }
    bool IsLocalReady() const { return localReady; }

    const std::string& GetRemotePlayerName() const { return remotePlayerName; }
    uint8_t GetRemoteAvatar() const { return remoteAvatar; }
    bool IsRemoteReady() const { return remoteReady; }
    uint8_t GetRoomPlayerCount() const { return roomPlayerCount; }
    bool IsInRoom() const { return inRoom; }

    // 主机发送开始游戏消息（广播难度）
    void SendGameStart(int difficulty) {
        GameStartMessage msg;
        msg.difficulty = difficulty;
        msg.isHost = (GetMode() == NetworkManager::Mode::HOST);
        networkManager.SendGameStart(msg);
        // 主机也应当立即进入游戏开始状态（无需等回显）
        if (GetMode() == NetworkManager::Mode::HOST) {
            gameStarting = true;
            startLevel = difficulty;
        }
    }

    bool IsGameStarting() const { return gameStarting; }
    int GetStartLevel() const { return startLevel; }
    
    void Disconnect() {
        networkManager.Shutdown();
    }
};

#endif // NETWORKGAMEMODE_H
