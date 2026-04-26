#ifndef NETWORKPROTOCOL_H
#define NETWORKPROTOCOL_H

#include "raylib.h"
#include <cstring>
#include <cstdint>

// 网络消息类型
enum class MessageType : uint8_t {
    HELLO = 1,              // 握手消息
    GAME_STATE = 2,         // 游戏状态（球、板位置）
    PADDLE_UPDATE = 3,      // 板位置更新
    GAME_START = 4,         // 游戏开始
    GAME_OVER = 5,          // 游戏结束
    SCORE_UPDATE = 6        // 分数更新
};

// 网络消息基类
struct NetworkMessage {
    MessageType type;
    uint32_t timestamp;
    
    NetworkMessage(MessageType t = MessageType::HELLO) 
        : type(t), timestamp(0) {}
    
    virtual size_t GetSize() const {
        return sizeof(MessageType) + sizeof(uint32_t);
    }
};

// 握手消息
struct HelloMessage : public NetworkMessage {
    bool isHost;
    char playerName[32];
    
    HelloMessage() : NetworkMessage(MessageType::HELLO), isHost(false) {
        memset(playerName, 0, sizeof(playerName));
    }
    
    size_t GetSize() const override {
        return sizeof(MessageType) + sizeof(uint32_t) + sizeof(bool) + sizeof(playerName);
    }
};

// 球状态信息
struct BallStateData {
    float posX;
    float posY;
    float velX;
    float velY;
    float radius;
    bool visible;
    
    BallStateData() : posX(0), posY(0), velX(0), velY(0), radius(10), visible(true) {}
    
    void FromVector2(Vector2 pos, Vector2 vel, float r) {
        posX = pos.x;
        posY = pos.y;
        velX = vel.x;
        velY = vel.y;
        radius = r;
    }
    
    Vector2 GetPosition() const {
        return { posX, posY };
    }
    
    Vector2 GetVelocity() const {
        return { velX, velY };
    }
};

// 板状态信息
struct PaddleStateData {
    float posX;
    float posY;
    float width;
    float height;
    
    PaddleStateData() : posX(0), posY(0), width(100), height(20) {}
    
    void FromGameObject(Vector2 pos, float w, float h) {
        posX = pos.x;
        posY = pos.y;
        width = w;
        height = h;
    }
    
    Vector2 GetPosition() const {
        return { posX, posY };
    }
};

// 游戏状态消息（主机发送）
struct GameStateMessage : public NetworkMessage {
    BallStateData ball;
    PaddleStateData hostPaddle;
    PaddleStateData guestPaddle;
    int hostScore;
    int guestScore;
    int hostLives;
    int guestLives;
    
    GameStateMessage() 
        : NetworkMessage(MessageType::GAME_STATE),
          hostScore(0), guestScore(0), hostLives(3), guestLives(3) {}
    
    size_t GetSize() const override {
        return sizeof(MessageType) + sizeof(uint32_t) + 
               sizeof(BallStateData) + sizeof(PaddleStateData) * 2 +
               sizeof(int) * 4;
    }
};

// 板位置更新消息（客户端发送）
struct PaddleUpdateMessage : public NetworkMessage {
    PaddleStateData paddle;
    
    PaddleUpdateMessage() : NetworkMessage(MessageType::PADDLE_UPDATE) {}
    
    size_t GetSize() const override {
        return sizeof(MessageType) + sizeof(uint32_t) + sizeof(PaddleStateData);
    }
};

// 游戏开始消息
struct GameStartMessage : public NetworkMessage {
    int difficulty;  // 0=Easy, 1=Normal, 2=Hard
    bool isHost;
    
    GameStartMessage() 
        : NetworkMessage(MessageType::GAME_START), difficulty(1), isHost(true) {}
    
    size_t GetSize() const override {
        return sizeof(MessageType) + sizeof(uint32_t) + sizeof(int) + sizeof(bool);
    }
};

// 分数更新消息
struct ScoreUpdateMessage : public NetworkMessage {
    int hostScore;
    int guestScore;
    int hostLives;
    int guestLives;
    
    ScoreUpdateMessage() 
        : NetworkMessage(MessageType::SCORE_UPDATE),
          hostScore(0), guestScore(0), hostLives(3), guestLives(3) {}
    
    size_t GetSize() const override {
        return sizeof(MessageType) + sizeof(uint32_t) + sizeof(int) * 4;
    }
};

#endif // NETWORKPROTOCOL_H
