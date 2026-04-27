#ifndef GAMEAPP_H
#define GAMEAPP_H

#if defined(_WIN32)
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
#endif

#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include "PowerUp.h"
#include <enet/enet.h>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <cmath>

class ScoreCalculator {
public:
    int CalculateScore(int brickType) {
        return CalculateScore(brickType, 0);
    }

    int CalculateScore(int brickType, int combo) {
        int baseScore = 0;
        if (brickType == 1) {
            baseScore = 10;
        }
        else if (brickType == 2) {
            baseScore = 20;
        }
        else if (brickType == 3) {
            baseScore = -5;
        }
        return baseScore + combo * 2;
    }
};

enum class Difficulty {
    Easy,
    Normal,
    Hard
};

struct DifficultyConfig {
    Vector2 ballSpeed;
    float paddleWidth;
    int lives;
    int rows;
};

static DifficultyConfig GetDifficultyConfig(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return { { 1.6f, 1.6f }, 120.0f, 5, 5 };
    case Difficulty::Hard:
        return { { 2.8f, 2.8f }, 80.0f, 2, 7 };
    case Difficulty::Normal:
    default:
        return { { 2.0f, 2.0f }, 100.0f, 3, 6 };
    }
}

static const char* GetDifficultyLabel(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return "EASY";
    case Difficulty::Hard:
        return "HARD";
    case Difficulty::Normal:
    default:
        return "NORMAL";
    }
}

enum class NetMode {
    Offline,
    Host,
    Client
};

enum class PacketType : uint8_t {
    HostStart = 1,
    ClientInput = 2,
    Snapshot = 3
};

static const int kNetMaxBalls = 8;
static const int kNetMaxPowerUps = 32;
static const int kNetMaxBricks = 128;

#pragma pack(push, 1)
struct PacketHostStart {
    uint8_t type;
    uint8_t difficulty;
    uint32_t seed;
};

struct PacketClientInput {
    uint8_t type;
    float centerX;
    uint8_t launchPressed;
};

struct NetBallState {
    float x;
    float y;
    float vx;
    float vy;
    uint8_t launched;
};

struct NetPowerUpState {
    float x;
    float y;
    uint8_t type;
};

struct PacketSnapshot {
    uint8_t type;
    uint8_t gameState;
    int32_t score;
    int32_t lives;
    uint8_t widePaddleActive;
    uint8_t frenzyActive;
    float hostPaddleCenterX;
    float clientPaddleCenterX;
    float hostPaddleWidth;
    float clientPaddleWidth;
    uint8_t ballCount;
    uint8_t powerUpCount;
    uint8_t brickCount;
    NetBallState balls[kNetMaxBalls];
    NetPowerUpState powerUps[kNetMaxPowerUps];
    uint8_t brickActive[kNetMaxBricks];
};
#pragma pack(pop)

class GameApp {
public:
int Run() {
    srand((unsigned int)time(nullptr));

    ScoreCalculator scoreCalculator;
    printf("普通砖块得分: %d\n", scoreCalculator.CalculateScore(1));
    printf("金色砖块+3连击得分: %d\n", scoreCalculator.CalculateScore(2, 3));

    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Brick Breaker Game");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    bool enetReady = (enet_initialize() == 0);

    Difficulty selectedDifficulty = Difficulty::Normal;
    DifficultyConfig config = GetDifficultyConfig(selectedDifficulty);
    NetMode netMode = NetMode::Offline;

    struct NetContext {
        ENetHost* host = nullptr;
        ENetPeer* peer = nullptr;
        bool connected = false;
        float remoteCenterX = 600.0f;
        bool remoteLaunchPressed = false;
        uint32_t sessionSeed = 0;
        bool autoReconnect = true;
        float nextReconnectTime = 0.0f;
        char statusText[160] = "Network: idle";
    } net;

    std::vector<GameObject*> objects;
    std::vector<Brick*> bricks;
    std::vector<Ball*> balls;
    std::vector<PowerUp*> powerUps;
    Paddle* hostPaddle = nullptr;
    Paddle* clientPaddle = nullptr;
    Paddle* localPaddle = nullptr;

    float brickWidth = 60.0f;
    float brickHeight = 18.0f;
    const int wallThickness = 5;
    const int gapX = 8;
    const int gapY = 6;
    int rowsCount = config.rows;

    int score = 0;
    int lives = config.lives;
    int scoreMultiplier = 1;
    bool widePaddleActive = false;
    bool frenzyActive = false;
    float widePaddleEndTime = 0.0f;
    float frenzyEndTime = 0.0f;
    float basePaddleWidth = config.paddleWidth;

    enum class State { Start, Playing, Settings, Victory, GameOver };
    State state = State::Start;

    auto ClearObjects = [&]() {
        for (GameObject* object : objects) {
            delete object;
        }
        objects.clear();
        bricks.clear();
        balls.clear();
        powerUps.clear();
        hostPaddle = nullptr;
        clientPaddle = nullptr;
        localPaddle = nullptr;
    };

    auto RemoveObject = [&](GameObject* target) {
        for (size_t i = 0; i < objects.size(); ++i) {
            if (objects[i] == target) {
                objects.erase(objects.begin() + (int)i);
                return;
            }
        }
    };

    auto SpawnBall = [&](Vector2 pos, Vector2 velocity, int scoreValue, bool launched = false) {
        Ball* newBall = new Ball(pos, velocity, 10, RED, true, scoreValue);
        if (frenzyActive) {
            newBall->SetSpeedFactor(2.0f);
        }
        if (launched) {
            newBall->Launch();
        }
        balls.push_back(newBall);
        objects.push_back(newBall);
    };

    auto CreateBricks = [&]() {
        int usableWidth = screenWidth - 2 * wallThickness;
        int cols = (usableWidth + gapX) / ((int)brickWidth + gapX);
        if (cols < 1) {
            cols = 1;
        }

        int totalBricksWidth = cols * (int)brickWidth + (cols - 1) * gapX;
        float startX = wallThickness + (usableWidth - totalBricksWidth) / 2.0f;
        float startY = 80.0f;

        for (int r = 0; r < rowsCount; ++r) {
            for (int c = 0; c < cols; ++c) {
                float x = startX + c * (brickWidth + gapX);
                float y = startY + r * (brickHeight + gapY);
                Brick* brick = new Brick(x, y, brickWidth, brickHeight);
                bricks.push_back(brick);
                objects.push_back(brick);
            }
        }
    };

    auto CreateGameObjects = [&]() {
        ClearObjects();
        config = GetDifficultyConfig(selectedDifficulty);
        rowsCount = config.rows;
        basePaddleWidth = config.paddleWidth;
        SpawnBall({ (float)screenWidth / 2, (float)screenHeight / 2 }, config.ballSpeed, 10, false);

        if (netMode == NetMode::Offline) {
            hostPaddle = new Paddle(350, 550, basePaddleWidth, 20, BLUE);
            hostPaddle->SetBounds(5.0f, (float)screenWidth - 5.0f);
            hostPaddle->SetControlEnabled(true);
            objects.push_back(hostPaddle);
            localPaddle = hostPaddle;
        }
        else {
            float paddleWidthNet = basePaddleWidth * 0.9f;
            hostPaddle = new Paddle(120, 550, paddleWidthNet, 20, { 70, 210, 255, 255 });
            clientPaddle = new Paddle(560, 550, paddleWidthNet, 20, { 255, 100, 190, 255 });

            hostPaddle->SetBounds(5.0f, screenWidth * 0.5f - 8.0f);
            clientPaddle->SetBounds(screenWidth * 0.5f + 8.0f, (float)screenWidth - 5.0f);

            hostPaddle->SetControlEnabled(netMode == NetMode::Host);
            clientPaddle->SetControlEnabled(netMode == NetMode::Client);

            if (netMode == NetMode::Host) {
                localPaddle = hostPaddle;
            }
            else {
                localPaddle = clientPaddle;
            }

            objects.push_back(hostPaddle);
            objects.push_back(clientPaddle);
        }

        CreateBricks();
        score = 0;
        lives = config.lives;
        scoreMultiplier = 1;
        widePaddleActive = false;
        frenzyActive = false;
        widePaddleEndTime = 0.0f;
        frenzyEndTime = 0.0f;
    };

    auto ResetBall = [&]() {
        if (hostPaddle == nullptr) {
            return;
        }

        for (Ball* ball : balls) {
            RemoveObject(ball);
            delete ball;
        }
        balls.clear();

        if (netMode == NetMode::Offline) {
            Rectangle paddleRect = hostPaddle->GetRect();
            SpawnBall({ paddleRect.x + paddleRect.width / 2, paddleRect.y - 20 }, config.ballSpeed, 10, false);
        }
        else {
            SpawnBall({ (float)screenWidth / 2, 530.0f }, config.ballSpeed, 10, false);
        }
    };

    auto ShutdownNetwork = [&]() {
        if (net.peer != nullptr) {
            enet_peer_disconnect_now(net.peer, 0);
            net.peer = nullptr;
        }
        if (net.host != nullptr) {
            enet_host_destroy(net.host);
            net.host = nullptr;
        }
        net.connected = false;
        net.remoteLaunchPressed = false;
    };

    auto ParsePort = [&](const char* text) {
        int port = atoi(text);
        if (port < 1 || port > 65535) {
            return 7777;
        }
        return port;
    };

    auto StartAsHost = [&](int hostPort) {
        ShutdownNetwork();
        if (!enetReady) {
            snprintf(net.statusText, sizeof(net.statusText), "Network unavailable: ENet init failed");
            return false;
        }
        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = (enet_uint16)hostPort;
        net.host = enet_host_create(&address, 1, 2, 0, 0);
        net.connected = false;
        if (net.host == nullptr) {
            snprintf(net.statusText, sizeof(net.statusText), "Host listen failed on port %d", hostPort);
            return false;
        }
        snprintf(net.statusText, sizeof(net.statusText), "Host listening on port %d", hostPort);
        return net.host != nullptr;
    };

    auto StartAsClient = [&](const char* hostIp, int hostPort) {
        ShutdownNetwork();
        if (!enetReady) {
            snprintf(net.statusText, sizeof(net.statusText), "Network unavailable: ENet init failed");
            return false;
        }
        net.host = enet_host_create(nullptr, 1, 2, 0, 0);
        if (net.host == nullptr) {
            snprintf(net.statusText, sizeof(net.statusText), "Client init failed");
            return false;
        }

        ENetAddress address;
        if (enet_address_set_host(&address, hostIp) != 0) {
            ShutdownNetwork();
            snprintf(net.statusText, sizeof(net.statusText), "Invalid server ip: %s", hostIp);
            return false;
        }
        address.port = (enet_uint16)hostPort;
        net.peer = enet_host_connect(net.host, &address, 2, 0);
        if (net.peer == nullptr) {
            ShutdownNetwork();
            snprintf(net.statusText, sizeof(net.statusText), "Connect create failed: %s:%d", hostIp, hostPort);
            return false;
        }
        net.connected = false;
        snprintf(net.statusText, sizeof(net.statusText), "Connecting to %s:%d", hostIp, hostPort);
        return true;
    };

    char hostPortText[16] = "7777";
    char clientIpText[64] = "127.0.0.1";
    char clientPortText[16] = "7777";
    enum class InputField { None, HostPort, ClientIp, ClientPort };
    InputField activeInput = InputField::None;

    auto PushTextChar = [&](char* dst, int cap, int* len, int codepoint, bool onlyDigits, bool allowDot) {
        if (codepoint < 32 || codepoint > 126) {
            return;
        }
        char c = (char)codepoint;
        if (onlyDigits && (c < '0' || c > '9')) {
            return;
        }
        if (!onlyDigits && allowDot) {
            bool ok = (c >= '0' && c <= '9') || c == '.';
            if (!ok) {
                return;
            }
        }
        if (*len < cap - 1) {
            dst[*len] = c;
            (*len)++;
            dst[*len] = '\0';
        }
    };

    auto ReconnectOnline = [&]() {
        if (netMode == NetMode::Host) {
            return StartAsHost(ParsePort(hostPortText));
        }
        if (netMode == NetMode::Client) {
            return StartAsClient(clientIpText, ParsePort(clientPortText));
        }
        return true;
    };

    auto SendHostStart = [&]() {
        if (net.peer == nullptr || !net.connected) {
            return;
        }
        PacketHostStart packetData;
        packetData.type = (uint8_t)PacketType::HostStart;
        packetData.difficulty = (uint8_t)selectedDifficulty;
        packetData.seed = net.sessionSeed;
        ENetPacket* packet = enet_packet_create(&packetData, sizeof(packetData), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(net.peer, 0, packet);
        enet_host_flush(net.host);
    };

    auto SendClientInput = [&](float centerX, bool launchPressed) {
        if (net.peer == nullptr || !net.connected) {
            return;
        }
        PacketClientInput inputData;
        inputData.type = (uint8_t)PacketType::ClientInput;
        inputData.centerX = centerX;
        inputData.launchPressed = launchPressed ? 1 : 0;
        ENetPacket* packet = enet_packet_create(&inputData, sizeof(inputData), 0);
        enet_peer_send(net.peer, 0, packet);
    };

    auto SendSnapshot = [&]() {
        if (netMode != NetMode::Host || net.peer == nullptr || !net.connected) {
            return;
        }

        PacketSnapshot ss;
        memset(&ss, 0, sizeof(ss));
        ss.type = (uint8_t)PacketType::Snapshot;
        ss.gameState = (state == State::Victory) ? 1 : ((state == State::GameOver) ? 2 : 0);
        ss.score = score;
        ss.lives = lives;
        ss.widePaddleActive = widePaddleActive ? 1 : 0;
        ss.frenzyActive = frenzyActive ? 1 : 0;
        ss.hostPaddleCenterX = hostPaddle ? hostPaddle->GetCenterX() : 200.0f;
        ss.clientPaddleCenterX = clientPaddle ? clientPaddle->GetCenterX() : 600.0f;
        ss.hostPaddleWidth = hostPaddle ? hostPaddle->GetWidth() : basePaddleWidth;
        ss.clientPaddleWidth = clientPaddle ? clientPaddle->GetWidth() : basePaddleWidth;

        int bCount = (int)std::min((size_t)kNetMaxBalls, balls.size());
        ss.ballCount = (uint8_t)bCount;
        for (int i = 0; i < bCount; ++i) {
            Vector2 p = balls[i]->GetPosition();
            Vector2 v = balls[i]->GetVelocity();
            ss.balls[i].x = p.x;
            ss.balls[i].y = p.y;
            ss.balls[i].vx = v.x;
            ss.balls[i].vy = v.y;
            ss.balls[i].launched = balls[i]->IsLaunched() ? 1 : 0;
        }

        int pCount = (int)std::min((size_t)kNetMaxPowerUps, powerUps.size());
        ss.powerUpCount = (uint8_t)pCount;
        for (int i = 0; i < pCount; ++i) {
            Vector2 p = powerUps[i]->GetPosition();
            ss.powerUps[i].x = p.x;
            ss.powerUps[i].y = p.y;
            ss.powerUps[i].type = (uint8_t)powerUps[i]->GetType();
        }

        int brickCount = (int)std::min((size_t)kNetMaxBricks, bricks.size());
        ss.brickCount = (uint8_t)brickCount;
        for (int i = 0; i < brickCount; ++i) {
            ss.brickActive[i] = bricks[i]->IsActive() ? 1 : 0;
        }

        ENetPacket* packet = enet_packet_create(&ss, sizeof(ss), 0);
        enet_peer_send(net.peer, 0, packet);
    };

    auto ApplySnapshot = [&](const PacketSnapshot& ss) {
        score = ss.score;
        lives = ss.lives;
        widePaddleActive = ss.widePaddleActive != 0;
        frenzyActive = ss.frenzyActive != 0;

        if (hostPaddle != nullptr) {
            hostPaddle->SetWidth(ss.hostPaddleWidth);
            hostPaddle->SetCenterX(ss.hostPaddleCenterX);
        }
        if (clientPaddle != nullptr) {
            clientPaddle->SetWidth(ss.clientPaddleWidth);
            clientPaddle->SetCenterX(ss.clientPaddleCenterX);
        }

        int targetBallCount = (int)ss.ballCount;
        while ((int)balls.size() > targetBallCount) {
            Ball* b = balls.back();
            RemoveObject(b);
            delete b;
            balls.pop_back();
        }
        while ((int)balls.size() < targetBallCount) {
            SpawnBall({ 400, 300 }, config.ballSpeed, 10, false);
        }

        for (int i = 0; i < targetBallCount; ++i) {
            balls[i]->SetPosition({ ss.balls[i].x, ss.balls[i].y });
            balls[i]->SetVelocity({ ss.balls[i].vx, ss.balls[i].vy });
            if (ss.balls[i].launched) {
                balls[i]->Launch();
            }
            else {
                balls[i]->ResetLaunchState();
            }
            if (frenzyActive) {
                balls[i]->SetSpeedFactor(2.0f);
            }
            else {
                balls[i]->SetSpeedFactor(1.0f);
                balls[i]->SetColor(RED);
            }
        }

        int targetPowerCount = (int)ss.powerUpCount;
        while ((int)powerUps.size() > targetPowerCount) {
            PowerUp* p = powerUps.back();
            RemoveObject(p);
            delete p;
            powerUps.pop_back();
        }
        while ((int)powerUps.size() < targetPowerCount) {
            PowerUp* p = new PowerUp({ 0, 0 }, PowerUpType::SplitBalls);
            powerUps.push_back(p);
            objects.push_back(p);
        }

        for (int i = 0; i < targetPowerCount; ++i) {
            PowerUpType type = (PowerUpType)ss.powerUps[i].type;
            if (powerUps[i]->GetType() != type) {
                RemoveObject(powerUps[i]);
                delete powerUps[i];
                PowerUp* p = new PowerUp({ ss.powerUps[i].x, ss.powerUps[i].y }, type);
                powerUps[i] = p;
                objects.push_back(p);
            }
            else {
                powerUps[i]->SetPosition({ ss.powerUps[i].x, ss.powerUps[i].y });
            }
        }

        int bc = std::min((int)ss.brickCount, (int)bricks.size());
        for (int i = 0; i < bc; ++i) {
            bricks[i]->SetActive(ss.brickActive[i] != 0);
        }

        if (ss.gameState == 1) {
            state = State::Victory;
        }
        else if (ss.gameState == 2) {
            state = State::GameOver;
        }
        else {
            state = State::Playing;
        }
    };

    auto ProcessNetworkEvents = [&]() {
        if (net.host == nullptr) {
            return;
        }

        ENetEvent ev;
        while (enet_host_service(net.host, &ev, 0) > 0) {
            if (ev.type == ENET_EVENT_TYPE_CONNECT) {
                net.peer = ev.peer;
                net.connected = true;
                net.autoReconnect = false;
                snprintf(net.statusText, sizeof(net.statusText), "Network: connected");
            }
            else if (ev.type == ENET_EVENT_TYPE_DISCONNECT) {
                net.connected = false;
                net.peer = nullptr;
                net.remoteLaunchPressed = false;
                if (netMode == NetMode::Client) {
                    net.autoReconnect = true;
                    net.nextReconnectTime = (float)GetTime() + 1.0f;
                    snprintf(net.statusText, sizeof(net.statusText), "Disconnected, retrying...");
                }
                else if (netMode == NetMode::Host) {
                    snprintf(net.statusText, sizeof(net.statusText), "Client disconnected");
                }
                state = State::Start;
            }
            else if (ev.type == ENET_EVENT_TYPE_RECEIVE) {
                if (ev.packet->dataLength >= 1) {
                    uint8_t type = ev.packet->data[0];

                    if (netMode == NetMode::Host && type == (uint8_t)PacketType::ClientInput && ev.packet->dataLength == sizeof(PacketClientInput)) {
                        PacketClientInput inData;
                        memcpy(&inData, ev.packet->data, sizeof(inData));
                        net.remoteCenterX = inData.centerX;
                        if (inData.launchPressed != 0) {
                            net.remoteLaunchPressed = true;
                        }
                    }
                    else if (netMode == NetMode::Client && type == (uint8_t)PacketType::HostStart && ev.packet->dataLength == sizeof(PacketHostStart)) {
                        PacketHostStart startPacket;
                        memcpy(&startPacket, ev.packet->data, sizeof(startPacket));
                        selectedDifficulty = (Difficulty)startPacket.difficulty;
                        net.sessionSeed = startPacket.seed;
                        srand(net.sessionSeed);
                        CreateGameObjects();
                        state = State::Playing;
                    }
                    else if (netMode == NetMode::Client && type == (uint8_t)PacketType::Snapshot && ev.packet->dataLength == sizeof(PacketSnapshot)) {
                        PacketSnapshot ss;
                        memcpy(&ss, ev.packet->data, sizeof(ss));
                        ApplySnapshot(ss);
                    }
                }
                enet_packet_destroy(ev.packet);
            }
        }
    };

    CreateGameObjects();

    ClearObjects();

    Rectangle startButton = { (float)(screenWidth / 2 - 100), (float)(screenHeight / 2 - 30), 200.0f, 60.0f };
    Rectangle offlineButton = { (float)(screenWidth / 2 - 220), (float)(screenHeight / 2 - 95), 140.0f, 44.0f };
    Rectangle hostButton = { (float)(screenWidth / 2 - 70), (float)(screenHeight / 2 - 95), 140.0f, 44.0f };
    Rectangle clientButton = { (float)(screenWidth / 2 + 80), (float)(screenHeight / 2 - 95), 140.0f, 44.0f };
    Rectangle reconnectButton = { (float)(screenWidth / 2 - 110), (float)(screenHeight / 2 + 156), 220.0f, 38.0f };
    Rectangle hostPortInput = { 220, 184, 130, 34 };
    Rectangle clientIpInput = { 220, 184, 260, 34 };
    Rectangle clientPortInput = { 500, 184, 90, 34 };

    Rectangle easyButton = { (float)(screenWidth / 2 - 220), (float)(screenHeight / 2 + 50), 120.0f, 50.0f };
    Rectangle normalButton = { (float)(screenWidth / 2 - 60), (float)(screenHeight / 2 + 50), 120.0f, 50.0f };
    Rectangle hardButton = { (float)(screenWidth / 2 + 100), (float)(screenHeight / 2 + 50), 120.0f, 50.0f };
    Rectangle menuButton = { (float)(screenWidth / 2 - 100), (float)(screenHeight / 2 - 90), 200.0f, 50.0f };
    Rectangle restartButton = { (float)(screenWidth / 2 - 100), (float)(screenHeight / 2 - 30), 200.0f, 60.0f };
    Rectangle quitButton = { (float)(screenWidth / 2 - 100), (float)(screenHeight / 2 + 40), 200.0f, 50.0f };

    const Color neonCyan = { 40, 240, 255, 255 };
    const Color neonBlue = { 60, 140, 255, 255 };
    const Color neonPink = { 255, 70, 180, 255 };
    const Color panelDark = { 10, 18, 34, 220 };
    const Color panelEdge = { 55, 170, 255, 220 };

    auto DrawSciFiBackground = [&](float timeNow) {
        DrawRectangleGradientV(0, 0, screenWidth, screenHeight, { 5, 8, 22, 255 }, { 2, 2, 10, 255 });

        float xPulse = fmodf(timeNow * 55.0f, 42.0f);
        float yPulse = fmodf(timeNow * 35.0f, 32.0f);
        for (int x = -42; x < screenWidth + 42; x += 42) {
            Color c = (x / 42) % 4 == 0 ? Fade(neonBlue, 0.25f) : Fade(neonCyan, 0.12f);
            DrawLine(x + (int)xPulse, 0, x + (int)xPulse, screenHeight, c);
        }
        for (int y = -32; y < screenHeight + 32; y += 32) {
            Color c = (y / 32) % 3 == 0 ? Fade(neonBlue, 0.18f) : Fade(neonCyan, 0.08f);
            DrawLine(0, y + (int)yPulse, screenWidth, y + (int)yPulse, c);
        }

        for (int i = 0; i < 5; ++i) {
            int r = 120 + i * 24;
            DrawRing({ (float)screenWidth / 2, (float)screenHeight / 2 }, (float)r, (float)r + 2, 0, 360, 64, Fade(neonPink, 0.03f));
        }
    };

    auto DrawPanel = [&](Rectangle rect) {
        DrawRectangleRounded(rect, 0.15f, 8, panelDark);
        DrawRectangleRoundedLines(rect, 0.15f, 8, panelEdge);
        Rectangle inner = { rect.x + 6, rect.y + 6, rect.width - 12, rect.height - 12 };
        DrawRectangleRoundedLines(inner, 0.15f, 8, Fade(neonCyan, 0.45f));
    };

    auto DrawNeonButton = [&](Rectangle rect, const char* text, bool hover, bool selected, Color accent) {
        Color edge = selected ? accent : (hover ? Fade(accent, 0.95f) : Fade(accent, 0.65f));
        Color fill = selected ? Fade(accent, 0.25f) : Fade(accent, hover ? 0.18f : 0.1f);

        Rectangle glow = { rect.x - 3, rect.y - 3, rect.width + 6, rect.height + 6 };
        DrawRectangleRounded(glow, 0.20f, 6, Fade(edge, hover || selected ? 0.18f : 0.08f));
        DrawRectangleRounded(rect, 0.20f, 6, fill);
        DrawRectangleRoundedLines(rect, 0.20f, 6, edge);

        int fontSize = (int)(rect.height * 0.42f);
        int labelWidth = MeasureText(text, fontSize);
        DrawText(text, (int)(rect.x + rect.width / 2 - labelWidth / 2), (int)(rect.y + rect.height / 2 - fontSize / 2), fontSize, RAYWHITE);
    };

    auto DrawHudCard = [&](Rectangle rect, const char* text, Color accent) {
        DrawRectangleRounded(rect, 0.2f, 6, Fade(panelDark, 0.9f));
        DrawRectangleRoundedLines(rect, 0.2f, 6, Fade(accent, 0.85f));
        DrawText(text, (int)rect.x + 12, (int)rect.y + 8, 20, RAYWHITE);
    };

    while (!WindowShouldClose()) {
        Vector2 mp = GetMousePosition();
        float uiTime = (float)GetTime();

        ProcessNetworkEvents();

        if (netMode == NetMode::Client && state == State::Start && !net.connected && net.autoReconnect && (float)GetTime() >= net.nextReconnectTime) {
            ReconnectOnline();
            net.nextReconnectTime = (float)GetTime() + 2.0f;
        }

        if (state == State::Start) {
            BeginDrawing();
            DrawSciFiBackground(uiTime);

            Rectangle menuPanel = { 120, 80, 560, 440 };
            DrawPanel(menuPanel);

            const char* title = "NEON BREAKOUT";
            int titleSize = 46;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth / 2 - titleWidth / 2 + 2, 112, titleSize, Fade(neonPink, 0.45f));
            DrawText(title, screenWidth / 2 - titleWidth / 2, 110, titleSize, RAYWHITE);

            bool hoverModeOffline = CheckCollisionPointRec(mp, offlineButton);
            bool hoverModeHost = CheckCollisionPointRec(mp, hostButton);
            bool hoverModeClient = CheckCollisionPointRec(mp, clientButton);
            DrawNeonButton(offlineButton, "OFFLINE", hoverModeOffline, netMode == NetMode::Offline, neonBlue);
            DrawNeonButton(hostButton, "HOST", hoverModeHost, netMode == NetMode::Host, { 80, 255, 170, 255 });
            DrawNeonButton(clientButton, "CLIENT", hoverModeClient, netMode == NetMode::Client, neonPink);

            if (netMode == NetMode::Host) {
                DrawText("Port", (int)hostPortInput.x, (int)hostPortInput.y - 20, 18, Fade(neonCyan, 0.9f));
                DrawRectangleRounded(hostPortInput, 0.2f, 6, Fade(panelDark, 0.95f));
                DrawRectangleRoundedLines(hostPortInput, 0.2f, 6, activeInput == InputField::HostPort ? neonCyan : Fade(neonBlue, 0.75f));
                DrawText(hostPortText, (int)hostPortInput.x + 10, (int)hostPortInput.y + 8, 20, RAYWHITE);
            }
            else if (netMode == NetMode::Client) {
                DrawText("Server IP", (int)clientIpInput.x, (int)clientIpInput.y - 20, 18, Fade(neonCyan, 0.9f));
                DrawText("Port", (int)clientPortInput.x, (int)clientPortInput.y - 20, 18, Fade(neonCyan, 0.9f));

                DrawRectangleRounded(clientIpInput, 0.2f, 6, Fade(panelDark, 0.95f));
                DrawRectangleRoundedLines(clientIpInput, 0.2f, 6, activeInput == InputField::ClientIp ? neonCyan : Fade(neonBlue, 0.75f));
                DrawText(clientIpText, (int)clientIpInput.x + 10, (int)clientIpInput.y + 8, 20, RAYWHITE);

                DrawRectangleRounded(clientPortInput, 0.2f, 6, Fade(panelDark, 0.95f));
                DrawRectangleRoundedLines(clientPortInput, 0.2f, 6, activeInput == InputField::ClientPort ? neonCyan : Fade(neonBlue, 0.75f));
                DrawText(clientPortText, (int)clientPortInput.x + 10, (int)clientPortInput.y + 8, 20, RAYWHITE);
            }

            bool hover = CheckCollisionPointRec(mp, startButton);
            const char* startLabel = "START";
            if (netMode == NetMode::Client) {
                startLabel = "WAIT HOST";
            }
            DrawNeonButton(startButton, startLabel, hover, false, neonCyan);

            if (netMode != NetMode::Offline && !net.connected) {
                bool hoverReconnect = CheckCollisionPointRec(mp, reconnectButton);
                DrawNeonButton(reconnectButton, "RECONNECT", hoverReconnect, false, neonPink);
            }

            const char* difficultyTitle = "SELECT DIFFICULTY";
            int difficultyTitleSize = 22;
            int difficultyTitleWidth = MeasureText(difficultyTitle, difficultyTitleSize);
            DrawText(difficultyTitle, screenWidth / 2 - difficultyTitleWidth / 2, (int)easyButton.y - 35, difficultyTitleSize, Fade(neonCyan, 0.9f));

            bool hoverEasy = CheckCollisionPointRec(mp, easyButton);
            bool hoverNormal = CheckCollisionPointRec(mp, normalButton);
            bool hoverHard = CheckCollisionPointRec(mp, hardButton);

            DrawNeonButton(easyButton, "EASY", hoverEasy, selectedDifficulty == Difficulty::Easy, { 80, 255, 170, 255 });
            DrawNeonButton(normalButton, "NORMAL", hoverNormal, selectedDifficulty == Difficulty::Normal, neonCyan);
            DrawNeonButton(hardButton, "HARD", hoverHard, selectedDifficulty == Difficulty::Hard, neonPink);

            if (netMode == NetMode::Client) {
                DrawText("Difficulty from host", screenWidth / 2 - 86, (int)hardButton.y + 104, 16, Fade(RAYWHITE, 0.8f));
            }

            const char* currentLabel = GetDifficultyLabel(selectedDifficulty);
            char selectedText[64];
            snprintf(selectedText, sizeof(selectedText), "Current: %s", currentLabel);
            int selectedTextSize = 18;
            int selectedTextWidth = MeasureText(selectedText, selectedTextSize);
            DrawText(selectedText, screenWidth / 2 - selectedTextWidth / 2, (int)hardButton.y + 70, selectedTextSize, Fade(neonCyan, 0.95f));

            if (netMode != NetMode::Offline) {
                DrawText(net.statusText, 160, 160, 18, net.connected ? Fade(neonCyan, 0.9f) : Fade(neonPink, 0.9f));
            }

            EndDrawing();

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverModeOffline) {
                netMode = NetMode::Offline;
                ShutdownNetwork();
                activeInput = InputField::None;
                snprintf(net.statusText, sizeof(net.statusText), "Offline mode");
            }
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverModeHost) {
                netMode = NetMode::Host;
                ReconnectOnline();
            }
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverModeClient) {
                netMode = NetMode::Client;
                net.autoReconnect = true;
                net.nextReconnectTime = (float)GetTime() + 0.2f;
                ReconnectOnline();
            }

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (netMode == NetMode::Host && CheckCollisionPointRec(mp, hostPortInput)) {
                    activeInput = InputField::HostPort;
                }
                else if (netMode == NetMode::Client && CheckCollisionPointRec(mp, clientIpInput)) {
                    activeInput = InputField::ClientIp;
                }
                else if (netMode == NetMode::Client && CheckCollisionPointRec(mp, clientPortInput)) {
                    activeInput = InputField::ClientPort;
                }
                else if (!(CheckCollisionPointRec(mp, offlineButton) || CheckCollisionPointRec(mp, hostButton) || CheckCollisionPointRec(mp, clientButton))) {
                    activeInput = InputField::None;
                }
            }

            int key = GetCharPressed();
            while (key > 0) {
                if (activeInput == InputField::HostPort) {
                    int len = (int)strlen(hostPortText);
                    PushTextChar(hostPortText, (int)sizeof(hostPortText), &len, key, true, false);
                }
                else if (activeInput == InputField::ClientIp) {
                    int len = (int)strlen(clientIpText);
                    PushTextChar(clientIpText, (int)sizeof(clientIpText), &len, key, false, true);
                }
                else if (activeInput == InputField::ClientPort) {
                    int len = (int)strlen(clientPortText);
                    PushTextChar(clientPortText, (int)sizeof(clientPortText), &len, key, true, false);
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE)) {
                if (activeInput == InputField::HostPort) {
                    int len = (int)strlen(hostPortText);
                    if (len > 0) {
                        hostPortText[len - 1] = '\0';
                    }
                }
                else if (activeInput == InputField::ClientIp) {
                    int len = (int)strlen(clientIpText);
                    if (len > 0) {
                        clientIpText[len - 1] = '\0';
                    }
                }
                else if (activeInput == InputField::ClientPort) {
                    int len = (int)strlen(clientPortText);
                    if (len > 0) {
                        clientPortText[len - 1] = '\0';
                    }
                }
            }

            bool hoverReconnect = CheckCollisionPointRec(mp, reconnectButton);
            if (netMode != NetMode::Offline && !net.connected && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverReconnect) {
                net.autoReconnect = (netMode == NetMode::Client);
                net.nextReconnectTime = (float)GetTime() + 2.0f;
                ReconnectOnline();
            }

            bool canEditDifficulty = (netMode != NetMode::Client);
            if (canEditDifficulty && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverEasy) {
                selectedDifficulty = Difficulty::Easy;
            }
            if (canEditDifficulty && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverNormal) {
                selectedDifficulty = Difficulty::Normal;
            }
            if (canEditDifficulty && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverHard) {
                selectedDifficulty = Difficulty::Hard;
            }
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hover) {
                if (netMode == NetMode::Offline) {
                    CreateGameObjects();
                    state = State::Playing;
                }
                else if (netMode == NetMode::Host && net.connected) {
                    net.sessionSeed = (uint32_t)rand();
                    srand(net.sessionSeed);
                    CreateGameObjects();
                    SendHostStart();
                    state = State::Playing;
                }
            }

            continue;
        }

        if (state == State::Settings) {
            BeginDrawing();
            DrawSciFiBackground(uiTime);

            Rectangle panel = { 180, 130, 440, 300 };
            DrawPanel(panel);

            const char* title = "CONTROL HUB";
            int titleSize = 34;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth / 2 - titleWidth / 2, screenHeight / 2 - 150, titleSize, RAYWHITE);

            bool hoverMenu = CheckCollisionPointRec(mp, menuButton);
            bool hoverRestart = CheckCollisionPointRec(mp, restartButton);
            bool hoverQuit = CheckCollisionPointRec(mp, quitButton);
            DrawNeonButton(menuButton, "MAIN MENU", hoverMenu, false, neonBlue);
            DrawNeonButton(restartButton, "RESTART", hoverRestart, false, neonCyan);
            DrawNeonButton(quitButton, "QUIT", hoverQuit, false, neonPink);

            EndDrawing();

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverMenu) {
                state = State::Start;
            }
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverRestart) {
                if (netMode == NetMode::Client) {
                    state = State::Start;
                }
                else {
                    net.sessionSeed = (uint32_t)rand();
                    srand(net.sessionSeed);
                    CreateGameObjects();
                    if (netMode == NetMode::Host) {
                        SendHostStart();
                    }
                    state = State::Playing;
                }
            }
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverQuit) {
                break;
            }

            continue;
        }

        if (state == State::Victory) {
            BeginDrawing();
            DrawSciFiBackground(uiTime);

            Rectangle panel = { 160, 140, 480, 280 };
            DrawPanel(panel);

            const char* title = "YOU WIN!";
            int titleSize = 40;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth / 2 - titleWidth / 2 + 2, screenHeight / 2 - 138, titleSize, Fade(neonCyan, 0.4f));
            DrawText(title, screenWidth / 2 - titleWidth / 2, screenHeight / 2 - 140, titleSize, RAYWHITE);

            char scoreText[64];
            snprintf(scoreText, sizeof(scoreText), "Final Score: %d", score);
            int scoreSize = 24;
            int scoreWidth = MeasureText(scoreText, scoreSize);
            DrawText(scoreText, screenWidth / 2 - scoreWidth / 2, screenHeight / 2 - 60, scoreSize, Fade(neonCyan, 0.95f));

            bool hover = CheckCollisionPointRec(mp, restartButton);
            DrawNeonButton(restartButton, "RESTART", hover, false, neonCyan);

            EndDrawing();

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hover) {
                if (netMode == NetMode::Client) {
                    state = State::Start;
                }
                else {
                    net.sessionSeed = (uint32_t)rand();
                    srand(net.sessionSeed);
                    CreateGameObjects();
                    if (netMode == NetMode::Host) {
                        SendHostStart();
                    }
                    state = State::Playing;
                }
            }

            continue;
        }

        if (state == State::GameOver) {
            BeginDrawing();
            DrawSciFiBackground(uiTime);

            Rectangle panel = { 160, 140, 480, 280 };
            DrawPanel(panel);

            const char* title = "GAME OVER!";
            int titleSize = 40;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth / 2 - titleWidth / 2 + 2, screenHeight / 2 - 138, titleSize, Fade(neonPink, 0.45f));
            DrawText(title, screenWidth / 2 - titleWidth / 2, screenHeight / 2 - 140, titleSize, { 255, 120, 140, 255 });

            char scoreText[64];
            snprintf(scoreText, sizeof(scoreText), "Final Score: %d", score);
            int scoreSize = 24;
            int scoreWidth = MeasureText(scoreText, scoreSize);
            DrawText(scoreText, screenWidth / 2 - scoreWidth / 2, screenHeight / 2 - 60, scoreSize, RAYWHITE);

            bool hover = CheckCollisionPointRec(mp, restartButton);
            DrawNeonButton(restartButton, "RESTART", hover, false, neonPink);

            EndDrawing();

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hover) {
                if (netMode == NetMode::Client) {
                    state = State::Start;
                }
                else {
                    net.sessionSeed = (uint32_t)rand();
                    srand(net.sessionSeed);
                    CreateGameObjects();
                    if (netMode == NetMode::Host) {
                        SendHostStart();
                    }
                    state = State::Playing;
                }
            }

            continue;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            state = State::Settings;
            continue;
        }

        bool authoritativeSim = (netMode != NetMode::Client);
        bool launchPressedLocal = IsKeyPressed(KEY_SPACE);

        if (authoritativeSim) {
            if (clientPaddle != nullptr && netMode == NetMode::Host) {
                clientPaddle->SetCenterX(net.remoteCenterX);
            }

            for (GameObject* object : objects) {
                object->Update();
            }

            bool launchPressed = launchPressedLocal || (netMode == NetMode::Host && net.remoteLaunchPressed);
            net.remoteLaunchPressed = false;
            if (launchPressed) {
                for (Ball* ball : balls) {
                    if (!ball->IsLaunched()) {
                        ball->Launch();
                    }
                }
            }

            if (!balls.empty()) {
                Ball* anchorBall = balls[0];
                if (!anchorBall->IsLaunched()) {
                    if (netMode == NetMode::Offline && hostPaddle != nullptr) {
                        Rectangle paddleRect = hostPaddle->GetRect();
                        anchorBall->SetPosition({ paddleRect.x + paddleRect.width / 2, paddleRect.y - anchorBall->GetRadius() - 1.0f });
                    }
                    else {
                        anchorBall->SetPosition({ (float)screenWidth / 2, 530.0f });
                    }
                }
            }
        }
        else {
            if (localPaddle != nullptr) {
                localPaddle->Update();
                SendClientInput(localPaddle->GetCenterX(), launchPressedLocal);
            }
        }

        float now = (float)GetTime();
        if (widePaddleActive && now >= widePaddleEndTime) {
            widePaddleActive = false;
            if (hostPaddle != nullptr) {
                hostPaddle->SetWidth(basePaddleWidth);
            }
            if (clientPaddle != nullptr) {
                clientPaddle->SetWidth(basePaddleWidth * 0.9f);
            }
        }
        if (frenzyActive && now >= frenzyEndTime) {
            frenzyActive = false;
            scoreMultiplier = 1;
            for (Ball* ball : balls) {
                ball->SetSpeedFactor(1.0f);
                ball->SetColor(RED);
            }
        }

        if (frenzyActive) {
            float hueBase = now * 180.0f;
            for (size_t i = 0; i < balls.size(); ++i) {
                float hue = fmodf(hueBase + (float)i * 60.0f, 360.0f);
                balls[i]->SetColor(ColorFromHSV(hue, 0.9f, 1.0f));
            }
        }

        if (authoritativeSim && hostPaddle != nullptr) {
            Rectangle paddleHostRect = hostPaddle->GetRect();
            Rectangle paddleClientRect = clientPaddle ? clientPaddle->GetRect() : Rectangle{ 0, 0, 0, 0 };

            for (Ball* ball : balls) {
                if (!ball->IsLaunched()) {
                    continue;
                }
                bool bounced = ball->BounceRect(paddleHostRect);
                if (!bounced && clientPaddle != nullptr) {
                    ball->BounceRect(paddleClientRect);
                }
            }

            for (Ball* ball : balls) {
                if (!ball->IsLaunched()) {
                    continue;
                }
                bool hitBrick = false;
                for (Brick* brick : bricks) {
                    if (!brick->IsActive()) {
                        continue;
                    }

                    if (ball->BounceRect(brick->GetRect())) {
                        brick->SetActive(false);
                        int brickType = brick->IsGolden() ? 2 : 1;
                        score += scoreCalculator.CalculateScore(brickType, 0) * scoreMultiplier;

                        if ((rand() % 5) == 0) {
                            PowerUpType type = PowerUpType::SplitBalls;
                            int roll = rand() % 3;
                            if (roll == 1) {
                                type = PowerUpType::WidePaddle;
                            }
                            else if (roll == 2) {
                                type = PowerUpType::Frenzy;
                            }
                            Vector2 pos = brick->GetPosition();
                            PowerUp* pu = new PowerUp(pos, type);
                            powerUps.push_back(pu);
                            objects.push_back(pu);
                        }

                        hitBrick = true;
                        break;
                    }
                }
                if (hitBrick) {
                    continue;
                }
            }

            for (size_t i = 0; i < powerUps.size();) {
                PowerUp* pu = powerUps[i];
                bool removePowerUp = false;

                bool pickedByHost = CheckCollisionRecs(pu->GetRect(), paddleHostRect);
                bool pickedByClient = clientPaddle != nullptr && CheckCollisionRecs(pu->GetRect(), paddleClientRect);
                if (pickedByHost || pickedByClient) {
                    PowerUpType type = pu->GetType();
                    if (type == PowerUpType::SplitBalls) {
                        if (!balls.empty()) {
                            Ball* source = balls[0];
                            Vector2 sourcePos = source->GetPosition();
                            Vector2 sourceVel = source->GetVelocity();
                            float vx = sourceVel.x;
                            float vy = sourceVel.y;
                            Vector2 v1 = { vx * 0.6f - vy * 0.8f, vx * 0.8f + vy * 0.6f };
                            Vector2 v2 = { vx * 0.6f + vy * 0.8f, -vx * 0.8f + vy * 0.6f };
                            SpawnBall(sourcePos, v1, source->GetScoreValue(), true);
                            SpawnBall(sourcePos, v2, source->GetScoreValue(), true);
                        }
                    }
                    else if (type == PowerUpType::WidePaddle) {
                        widePaddleActive = true;
                        widePaddleEndTime = now + 10.0f;
                        if (hostPaddle != nullptr) {
                            hostPaddle->SetWidth(basePaddleWidth * 1.6f);
                        }
                        if (clientPaddle != nullptr) {
                            clientPaddle->SetWidth(basePaddleWidth * 1.45f);
                        }
                    }
                    else if (type == PowerUpType::Frenzy) {
                        frenzyActive = true;
                        frenzyEndTime = now + 10.0f;
                        scoreMultiplier = 4;
                        for (Ball* ball : balls) {
                            ball->SetSpeedFactor(2.0f);
                        }
                    }

                    removePowerUp = true;
                }
                else if (pu->IsOutOfScreen(screenHeight)) {
                    removePowerUp = true;
                }

                if (removePowerUp) {
                    RemoveObject(pu);
                    delete pu;
                    powerUps.erase(powerUps.begin() + (int)i);
                }
                else {
                    ++i;
                }
            }

            for (size_t i = 0; i < balls.size();) {
                Ball* ball = balls[i];
                if (ball->IsLaunched() && ball->IsOutOfBounds(screenHeight)) {
                    RemoveObject(ball);
                    delete ball;
                    balls.erase(balls.begin() + (int)i);
                }
                else {
                    ++i;
                }
            }

            if (balls.empty()) {
                lives--;
                if (lives <= 0) {
                    state = State::GameOver;
                    continue;
                }
                ResetBall();
            }
        }

        bool anyBrickActive = false;
        for (Brick* brick : bricks) {
            if (brick->IsActive()) {
                anyBrickActive = true;
                break;
            }
        }
        if (!anyBrickActive) {
            state = State::Victory;
        }

        if (netMode == NetMode::Host) {
            SendSnapshot();
        }

        BeginDrawing();
        DrawSciFiBackground(uiTime);

        DrawRectangle(0, 0, (int)wallThickness, screenHeight, Fade(neonBlue, 0.9f));
        DrawRectangle(screenWidth - (int)wallThickness, 0, (int)wallThickness, screenHeight, Fade(neonBlue, 0.9f));
        DrawRectangle(0, 0, screenWidth, (int)wallThickness, Fade(neonBlue, 0.9f));
        DrawRectangle(0, screenHeight - (int)wallThickness, screenWidth, (int)wallThickness, Fade(neonBlue, 0.9f));

        for (GameObject* object : objects) {
            object->Draw();
        }

        if (netMode != NetMode::Offline) {
            DrawLine(screenWidth / 2, 120, screenWidth / 2, screenHeight - 10, Fade(neonBlue, 0.4f));
        }

        char scoreText[48];
        snprintf(scoreText, sizeof(scoreText), "SCORE  %d", score);
        DrawHudCard({ 560, 12, 220, 40 }, scoreText, neonCyan);

        if (netMode == NetMode::Host) {
            DrawHudCard({ 18, 104, 200, 40 }, "ONLINE HOST", { 80, 255, 170, 255 });
        }
        else if (netMode == NetMode::Client) {
            DrawHudCard({ 18, 104, 210, 40 }, "ONLINE CLIENT", neonPink);
        }

        if (widePaddleActive) {
            DrawHudCard({ 18, 12, 148, 40 }, "WIDE x2", { 120, 255, 160, 255 });
        }
        if (frenzyActive) {
            DrawHudCard({ 18, 58, 170, 40 }, "FRENZY x4", neonPink);
        }

        bool waitingForLaunch = false;
        for (Ball* ball : balls) {
            if (!ball->IsLaunched()) {
                waitingForLaunch = true;
                break;
            }
        }
        if (waitingForLaunch) {
            if (netMode == NetMode::Client) {
                DrawHudCard({ 260, 58, 280, 40 }, "PRESS SPACE TO REQUEST LAUNCH", neonBlue);
            }
            else {
                DrawHudCard({ 290, 58, 220, 40 }, "PRESS SPACE TO LAUNCH", neonBlue);
            }
        }

        DrawHudCard({ 560, 58, 220, 40 }, "LIVES", neonBlue);

        int livesY = 70;
        for (int i = 0; i < lives; ++i) {
            DrawRectangleRounded({ (float)(618 + i * 30), (float)livesY, 16, 12 }, 0.35f, 4, Fade(neonPink, 0.9f));
        }

        EndDrawing();
    }

    ShutdownNetwork();

    ClearObjects();
    CloseWindow();
    if (enetReady) {
        enet_deinitialize();
    }
    return 0;
}
};

#endif