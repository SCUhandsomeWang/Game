#ifndef NETWORKGAMEMODE_H
#define NETWORKGAMEMODE_H

#include "NetworkManager.h"
#include "Interpolation.h"
#include "raylib.h"
#include <cstdio>

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

public:
    NetworkGameMode() {
        // 初始化平滑对象
        remotePaddleInterp = InterpolationSmoothing::CreatePaddleInterpolation({0, 0});
        remoteBallInterp = InterpolationSmoothing::CreateBallInterpolation({0, 0});
    }
    
    ~NetworkGameMode() {
        Disconnect();
    }
    
    // 委托到NetworkManager的方法
    bool StartAsHost(int port = 5555) {
        return networkManager.StartAsHost(port);
    }
    
    bool ConnectAsClient(const char* hostIP, int port = 5555) {
        return networkManager.ConnectAsClient(hostIP, port);
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
    
    bool SendPaddleUpdate(const PaddleUpdateMessage& update) {
        return networkManager.SendPaddleUpdate(update);
    }
    
    // 高层功能 - 处理插值和同步
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
            if (GetMode() == NetworkManager::Mode::CLIENT) {
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
    
    void Disconnect() {
        networkManager.Shutdown();
    }
};

#endif // NETWORKGAMEMODE_H
