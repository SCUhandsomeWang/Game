#ifndef NETWORKGAMEMODE_H
#define NETWORKGAMEMODE_H

#include "NetworkManager.h"
#include "Interpolation.h"
#include "raylib.h"
#include <cstdio>

class NetworkGameMode {
public:
    enum class Mode {
        OFFLINE,
        HOST,
        CLIENT
    };

    enum class ConnectionStatus {
        CONNECTING,
        CONNECTED,
        DISCONNECTED,
        ERROR
    };

private:
    Mode gameMode;
    ConnectionStatus connectionStatus;
    NetworkManager networkManager;
    
    // 远程板位置平滑
    InterpolationSmoothing::InterpolatedObject remotePaddleInterp;
    InterpolationSmoothing::InterpolatedObject remoteBallInterp;
    InterpolationSmoothing::PositionPredictor ballPredictor;
    
    // 上次发送状态的时间
    float lastStateSendTime;
    
    // 游戏状态缓存
    GameStateMessage lastReceivedState;
    PaddleUpdateMessage lastReceivedPaddleUpdate;

public:
    NetworkGameMode() 
        : gameMode(Mode::OFFLINE),
          connectionStatus(ConnectionStatus::DISCONNECTED),
          lastStateSendTime(0) {
        
        // 初始化平滑对象
        remotePaddleInterp = InterpolationSmoothing::CreatePaddleInterpolation({0, 0});
        remoteBallInterp = InterpolationSmoothing::CreateBallInterpolation({0, 0});
    }
    
    ~NetworkGameMode() {
        Disconnect();
    }
    
    // 作为主机启动网络游戏
    bool StartAsHost(int port = 5555) {
        gameMode = Mode::HOST;
        
        if (networkManager.StartAsHost(port)) {
            connectionStatus = ConnectionStatus::CONNECTED;
            printf("[Network] Started as HOST on port %d\n", port);
            return true;
        }
        
        connectionStatus = ConnectionStatus::ERROR;
        printf("[Network] Failed to start as HOST\n");
        return false;
    }
    
    // 作为客户端连接到主机
    bool ConnectAsClient(const char* hostIP, int port = 5555) {
        gameMode = Mode::CLIENT;
        
        if (networkManager.ConnectAsClient(hostIP, port)) {
            connectionStatus = ConnectionStatus::CONNECTED;
            printf("[Network] Connected to HOST at %s:%d\n", hostIP, port);
            return true;
        }
        
        connectionStatus = ConnectionStatus::ERROR;
        printf("[Network] Failed to connect to HOST\n");
        return false;
    }
    
    // 获取当前游戏模式
    Mode GetMode() const { return gameMode; }
    
    // 获取连接状态
    ConnectionStatus GetStatus() const { return connectionStatus; }
    
    // 检查是否连接
    bool IsConnected() const { 
        return connectionStatus == ConnectionStatus::CONNECTED && 
               networkManager.IsConnected(); 
    }
    
    // 更新网络通信
    void Update(float deltaTime) {
        if (!IsConnected()) return;
        
        // 接收网络消息
        networkManager.ReceiveMessages();
        
        // 处理接收到的游戏状态
        GameStateMessage gameState;
        while (networkManager.GetReceivedGameState(gameState)) {
            lastReceivedState = gameState;
            
            // 更新远程球的插值目标
            remoteBallInterp.targetPos = gameState.ball.GetPosition();
            remoteBallInterp.isInterpolating = true;
            remoteBallInterp.interpolationSpeed = 0.2f;
            
            // 更新远程板的插值目标（取决于是主机还是客户端）
            if (gameMode == Mode::CLIENT) {
                remotePaddleInterp.targetPos = gameState.hostPaddle.GetPosition();
            } else {
                remotePaddleInterp.targetPos = gameState.guestPaddle.GetPosition();
            }
            remotePaddleInterp.isInterpolating = true;
            remotePaddleInterp.interpolationSpeed = 0.25f;
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
    
    // 发送游戏状态（主机）
    bool SendGameState(const GameStateMessage& state) {
        lastStateSendTime += GetFrameTime();
        
        // 根据配置的间隔发送状态
        if (networkManager.ShouldUpdateState(GetFrameTime())) {
            return networkManager.SendGameState(state);
        }
        
        return false;
    }
    
    // 发送板位置更新（客户端）
    bool SendPaddleUpdate(const PaddleUpdateMessage& update) {
        return networkManager.SendPaddleUpdate(update);
    }
    
    // 获取远程球的当前（平滑后的）位置
    Vector2 GetRemoteBallPosition() const {
        return remoteBallInterp.currentPos;
    }
    
    // 获取远程板的当前（平滑后的）位置
    Vector2 GetRemotePaddlePosition() const {
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
    
    // 设置网络状态更新频率（Hz）
    void SetStateUpdateRate(float hz) {
        networkManager.SetStateUpdateInterval(hz);
    }
    
    // 断开连接
    void Disconnect() {
        networkManager.Shutdown();
        gameMode = Mode::OFFLINE;
        connectionStatus = ConnectionStatus::DISCONNECTED;
        printf("[Network] Disconnected\n");
    }
    
    // 获取网络管理器的引用（高级用途）
    NetworkManager& GetNetworkManager() {
        return networkManager;
    }
};

#endif // NETWORKGAMEMODE_H
