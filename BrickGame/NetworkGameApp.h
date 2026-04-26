#ifndef NETWORKGAMEAPP_H
#define NETWORKGAMEAPP_H

#include "GameApp.h"
#include "NetworkGameMode.h"
#include <vector>
#include <cstdio>

// 简化的网络游戏应用 - 演示双人合作模式
class NetworkGameApp {
private:
    NetworkGameMode::Mode gameMode;
    NetworkGameMode networkGame;
    
    // 游戏状态
    int hostScore = 0;
    int clientScore = 0;
    int hostLives = 3;
    int clientLives = 3;
    
    // 远程对手板的位置
    Vector2 remotePaddlePos = {400, 550};
    
    bool gameStarted = false;
    float connectionTimeout = 0;

public:
    NetworkGameApp() : gameMode(NetworkGameMode::Mode::OFFLINE) {}
    
    bool StartAsHost(int port = 5555) {
        gameMode = NetworkGameMode::Mode::HOST;
        
        if (networkGame.StartAsHost(port)) {
            printf("[Game] Started as HOST\n");
            return true;
        }
        
        return false;
    }
    
    bool StartAsClient(const char* hostIP, int port = 5555) {
        gameMode = NetworkGameMode::Mode::CLIENT;
        
        if (networkGame.ConnectAsClient(hostIP, port)) {
            printf("[Game] Connected as CLIENT\n");
            return true;
        }
        
        return false;
    }
    
    bool IsConnected() const {
        return networkGame.IsConnected();
    }
    
    // 向远程发送球和本地板的状态（主机）
    void SendGameState(Vector2 ballPos, Vector2 ballVel, float ballRadius,
                      Vector2 hostPaddlePos, float paddleWidth, float paddleHeight) {
        if (gameMode != NetworkGameMode::Mode::HOST) return;
        
        GameStateMessage state;
        state.timestamp = (uint32_t)GetTime();
        
        // 设置球状态
        state.ball.FromVector2(ballPos, ballVel, ballRadius);
        
        // 设置主机板状态
        state.hostPaddle.FromGameObject(hostPaddlePos, paddleWidth, paddleHeight);
        
        // 设置客户端板状态（从接收到的消息）
        const PaddleUpdateMessage& clientUpdate = networkGame.GetLastReceivedPaddleUpdate();
        state.guestPaddle = clientUpdate.paddle;
        
        // 设置分数和生命值
        state.hostScore = hostScore;
        state.guestScore = clientScore;
        state.hostLives = hostLives;
        state.guestLives = clientLives;
        
        networkGame.SendGameState(state);
    }
    
    // 向远程发送本地板的位置（客户端）
    void SendPaddleUpdate(Vector2 paddlePos, float paddleWidth, float paddleHeight) {
        if (gameMode != NetworkGameMode::Mode::CLIENT) return;
        
        PaddleUpdateMessage update;
        update.timestamp = (uint32_t)GetTime();
        update.paddle.FromGameObject(paddlePos, paddleWidth, paddleHeight);
        
        networkGame.SendPaddleUpdate(update);
    }
    
    // 更新网络通信
    void Update(float deltaTime) {
        networkGame.Update(deltaTime);
        
        if (networkGame.IsConnected()) {
            connectionTimeout = 0;  // 重置超时计数
        } else {
            connectionTimeout += deltaTime;
        }
    }
    
    // 获取远程球的平滑位置
    const Vector2& GetRemoteBallPosition() const {
        return networkGame.GetRemoteBallPosition();
    }
    
    // 获取远程板的平滑位置
    const Vector2& GetRemotePaddlePosition() const {
        return networkGame.GetRemotePaddlePosition();
    }
    
    // 获取远程板信息
    Rectangle GetRemotePaddleRect() const {
        Vector2 pos = GetRemotePaddlePosition();
        const GameStateMessage& state = networkGame.GetLastReceivedGameState();
        return { pos.x, pos.y, state.guestPaddle.width, state.guestPaddle.height };
    }
    
    // 获取远程球的完整状态
    const BallStateData& GetRemoteBallState() const {
        static BallStateData empty;
        if (networkGame.GetMode() == NetworkGameMode::Mode::CLIENT) {
            return networkGame.GetLastReceivedGameState().ball;
        }
        return empty;
    }
    
    // 获取游戏模式
    NetworkGameMode::Mode GetMode() const {
        return gameMode;
    }
    
    void SetLocalScores(int myScore, int myLives, int remoteScore, int remoteLives) {
        if (gameMode == NetworkGameMode::Mode::HOST) {
            hostScore = myScore;
            hostLives = myLives;
            clientScore = remoteScore;
            clientLives = remoteLives;
        } else {
            clientScore = myScore;
            clientLives = myLives;
            hostScore = remoteScore;
            hostLives = remoteLives;
        }
    }
    
    int GetRemoteScore() const {
        return (gameMode == NetworkGameMode::Mode::HOST) ? clientScore : hostScore;
    }
    
    int GetRemoteLives() const {
        return (gameMode == NetworkGameMode::Mode::HOST) ? clientLives : hostLives;
    }
    
    void Disconnect() {
        networkGame.Disconnect();
    }
    
    bool IsConnectionTimedOut(float timeout = 10.0f) const {
        return connectionTimeout > timeout;
    }
};

#endif // NETWORKGAMEAPP_H
