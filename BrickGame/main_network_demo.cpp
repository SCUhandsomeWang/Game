#include "raylib.h"
#include "GameApp.h"
#include "NetworkGameMode.h"
#include "NetworkConfig.h"
#include "Brick.h"
#include "PowerUp.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>

#ifdef DrawText
#undef DrawText
#endif

static float ClampPaddleX(float x, float width, float screenWidth) {
    if (x < 0.0f) return 0.0f;
    if (x + width > screenWidth) return screenWidth - width;
    return x;
}

static void ResetBall(Vector2& ballPos, Vector2& ballVel, float screenWidth, float screenHeight) {
    ballPos = { screenWidth * 0.5f, screenHeight * 0.5f };
    ballVel.x = (GetRandomValue(0, 1) == 0) ? -190.0f : 190.0f;
    ballVel.y = -250.0f;
}

int main(int argc, char* argv[]) {
    NetworkManager::Mode mode = NetworkManager::Mode::OFFLINE;
    int port = NetworkConfig::DEFAULT_PORT;
    std::string hostIP = NetworkConfig::LOCALHOST;

    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--host" || arg == "-h") {
            mode = NetworkManager::Mode::HOST;
            printf("[Main] Starting as HOST\n");
        } else if (arg == "--client" || arg == "-c") {
            mode = NetworkManager::Mode::CLIENT;
            if (argc > 2) {
                hostIP = argv[2];
            }
            printf("[Main] Starting as CLIENT, connecting to %s\n", hostIP.c_str());
        }
    }

    if (mode == NetworkManager::Mode::OFFLINE) {
        printf("[Main] Starting in OFFLINE mode\n");
        GameApp app;
        return app.Run();
    }

    InitWindow(800, 600, "Brick Breaker - Network Co-op");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    NetworkGameMode networkGame;
    bool connected = false;
    if (mode == NetworkManager::Mode::HOST) {
        connected = networkGame.StartAsHost(port);
    } else {
        connected = networkGame.ConnectAsClient(hostIP.c_str(), port);
    }

    if (!connected) {
        printf("[Main] Failed to start network connection\n");
        CloseWindow();
        return -1;
    }

    const float screenWidth = 800.0f;
    const float screenHeight = 600.0f;
    const float paddleWidthBase = 110.0f;
    const float paddleHeight = 18.0f;
    const float paddleY = screenHeight - 44.0f;
    const float paddleSpeed = 500.0f;
    const float ballRadius = 10.0f;

    Rectangle hostPaddle = { screenWidth * 0.30f - paddleWidthBase * 0.5f, paddleY, paddleWidthBase, paddleHeight };
    Rectangle guestPaddle = { screenWidth * 0.70f - paddleWidthBase * 0.5f, paddleY, paddleWidthBase, paddleHeight };

    Vector2 ballPos = { screenWidth * 0.5f, screenHeight * 0.5f };
    Vector2 ballVel = { 190.0f, -250.0f };

    int teamScore = 0;
    int teamLives = 3;
    bool gameOver = false;

    bool widePaddleActive = false;
    bool frenzyActive = false;
    float widePaddleEndTime = 0.0f;
    float frenzyEndTime = 0.0f;
    int scoreMultiplier = 1;

    std::vector<Brick*> bricks;
    std::vector<PowerUp*> powerUps;

    auto ClearWorld = [&]() {
        for (Brick* brick : bricks) {
            delete brick;
        }
        bricks.clear();

        for (PowerUp* pu : powerUps) {
            delete pu;
        }
        powerUps.clear();
    };

    auto CreateBricks = [&]() {
        for (Brick* brick : bricks) {
            delete brick;
        }
        bricks.clear();

        const int rows = 6;
        const int gapX = 8;
        const int gapY = 6;
        const float brickWidth = 60.0f;
        const float brickHeight = 18.0f;
        const int wallThickness = 5;

        int usableWidth = (int)screenWidth - 2 * wallThickness;
        int cols = (usableWidth + gapX) / ((int)brickWidth + gapX);
        if (cols < 1) cols = 1;

        int totalBricksWidth = cols * (int)brickWidth + (cols - 1) * gapX;
        float startX = wallThickness + (usableWidth - totalBricksWidth) / 2.0f;
        float startY = 80.0f;

        // 固定种子保证两端金砖布局一致
        SetRandomSeed(20260427);
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                float x = startX + c * (brickWidth + gapX);
                float y = startY + r * (brickHeight + gapY);
                bricks.push_back(new Brick(x, y, brickWidth, brickHeight));
            }
        }
    };

    auto ResetMatch = [&]() {
        hostPaddle = { screenWidth * 0.30f - paddleWidthBase * 0.5f, paddleY, paddleWidthBase, paddleHeight };
        guestPaddle = { screenWidth * 0.70f - paddleWidthBase * 0.5f, paddleY, paddleWidthBase, paddleHeight };
        ResetBall(ballPos, ballVel, screenWidth, screenHeight);

        teamScore = 0;
        teamLives = 3;
        gameOver = false;

        widePaddleActive = false;
        frenzyActive = false;
        widePaddleEndTime = 0.0f;
        frenzyEndTime = 0.0f;
        scoreMultiplier = 1;

        for (PowerUp* pu : powerUps) {
            delete pu;
        }
        powerUps.clear();
        CreateBricks();
    };

    CreateBricks();

    printf("[Main] Network connection established\n");
    printf("[Main] Network mode: %s\n", mode == NetworkManager::Mode::HOST ? "HOST" : "CLIENT");
    printf("[Main] Press ESC to exit\n");

    bool running = true;
    float connectionCheckTimer = 0.0f;
    const float CONNECTION_CHECK_INTERVAL = 0.5f;

    while (running && !WindowShouldClose()) {
        float dt = GetFrameTime();
        float now = (float)GetTime();

        networkGame.Update(dt);

        float move = 0.0f;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) move -= 1.0f;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) move += 1.0f;

        if (!gameOver && mode == NetworkManager::Mode::HOST) {
            hostPaddle.x = ClampPaddleX(hostPaddle.x + move * paddleSpeed * dt, hostPaddle.width, screenWidth);
        } else if (!gameOver) {
            guestPaddle.x = ClampPaddleX(guestPaddle.x + move * paddleSpeed * dt, guestPaddle.width, screenWidth);
        }

        if (widePaddleActive && now >= widePaddleEndTime) {
            widePaddleActive = false;
            hostPaddle.width = paddleWidthBase;
            guestPaddle.width = paddleWidthBase;
            hostPaddle.x = ClampPaddleX(hostPaddle.x, hostPaddle.width, screenWidth);
            guestPaddle.x = ClampPaddleX(guestPaddle.x, guestPaddle.width, screenWidth);
        }

        if (frenzyActive && now >= frenzyEndTime) {
            frenzyActive = false;
            scoreMultiplier = 1;
        }

        if (mode == NetworkManager::Mode::HOST) {
            const Vector2& remoteGuest = networkGame.GetRemotePaddlePosition();
            if (remoteGuest.x > 0.0f || remoteGuest.y > 0.0f) {
                guestPaddle.x = ClampPaddleX(remoteGuest.x, guestPaddle.width, screenWidth);
            }
            hostPaddle.y = paddleY;
            guestPaddle.y = paddleY;

            if (!gameOver) {
                float speedMul = frenzyActive ? 1.8f : 1.0f;
                ballPos.x += ballVel.x * dt * speedMul;
                ballPos.y += ballVel.y * dt * speedMul;

                if (ballPos.x - ballRadius <= 5.0f) {
                    ballPos.x = 5.0f + ballRadius;
                    ballVel.x = std::fabs(ballVel.x);
                }
                if (ballPos.x + ballRadius >= screenWidth - 5.0f) {
                    ballPos.x = screenWidth - 5.0f - ballRadius;
                    ballVel.x = -std::fabs(ballVel.x);
                }
                if (ballPos.y - ballRadius <= 5.0f) {
                    ballPos.y = 5.0f + ballRadius;
                    ballVel.y = std::fabs(ballVel.y);
                }

                bool hitHost = CheckCollisionCircleRec(ballPos, ballRadius, hostPaddle);
                bool hitGuest = CheckCollisionCircleRec(ballPos, ballRadius, guestPaddle);
                if (ballVel.y > 0.0f && (hitHost || hitGuest)) {
                    const Rectangle& hitPaddle = hitHost ? hostPaddle : guestPaddle;
                    float hit = (ballPos.x - (hitPaddle.x + hitPaddle.width * 0.5f)) / (hitPaddle.width * 0.5f);
                    ballVel.y = -std::fabs(ballVel.y);
                    ballVel.x = hit * 320.0f;
                    ballPos.y = paddleY - ballRadius - 1.0f;
                }

                for (size_t i = 0; i < bricks.size(); ++i) {
                    Brick* brick = bricks[i];
                    if (!brick->IsActive()) continue;

                    if (CheckCollisionCircleRec(ballPos, ballRadius, brick->GetRect())) {
                        brick->SetActive(false);
                        ballVel.y = -ballVel.y;
                        teamScore += (brick->IsGolden() ? 20 : 10) * scoreMultiplier;

                        if ((i % 7) == 0) {
                            powerUps.push_back(new PowerUp(brick->GetPosition(), PowerUpType::WidePaddle));
                        } else if ((i % 11) == 0) {
                            powerUps.push_back(new PowerUp(brick->GetPosition(), PowerUpType::Frenzy));
                        }
                        break;
                    }
                }

                for (size_t i = 0; i < powerUps.size();) {
                    PowerUp* pu = powerUps[i];
                    pu->Update();

                    bool removePowerUp = false;
                    if (CheckCollisionRecs(pu->GetRect(), hostPaddle) || CheckCollisionRecs(pu->GetRect(), guestPaddle)) {
                        if (pu->GetType() == PowerUpType::WidePaddle) {
                            widePaddleActive = true;
                            widePaddleEndTime = now + 10.0f;
                            hostPaddle.width = paddleWidthBase * 1.8f;
                            guestPaddle.width = paddleWidthBase * 1.8f;
                            hostPaddle.x = ClampPaddleX(hostPaddle.x, hostPaddle.width, screenWidth);
                            guestPaddle.x = ClampPaddleX(guestPaddle.x, guestPaddle.width, screenWidth);
                        } else if (pu->GetType() == PowerUpType::Frenzy) {
                            frenzyActive = true;
                            frenzyEndTime = now + 10.0f;
                            scoreMultiplier = 3;
                        }
                        removePowerUp = true;
                    } else if (pu->IsOutOfScreen((int)screenHeight)) {
                        removePowerUp = true;
                    }

                    if (removePowerUp) {
                        delete pu;
                        powerUps.erase(powerUps.begin() + (int)i);
                    } else {
                        ++i;
                    }
                }

                if (ballPos.y + ballRadius >= screenHeight - 5.0f) {
                    teamLives -= 1;
                    if (teamLives <= 0) {
                        teamLives = 0;
                        gameOver = true;
                    } else {
                        ResetBall(ballPos, ballVel, screenWidth, screenHeight);
                    }
                }
            }

            GameStateMessage state;
            state.timestamp = (uint32_t)(GetTime() * 1000.0);
            state.ball.FromVector2(ballPos, ballVel, ballRadius);
            state.hostPaddle.FromGameObject({ hostPaddle.x, hostPaddle.y }, hostPaddle.width, hostPaddle.height);
            state.guestPaddle.FromGameObject({ guestPaddle.x, guestPaddle.y }, guestPaddle.width, guestPaddle.height);
            state.hostScore = teamScore;
            state.guestScore = 0;
            state.hostLives = teamLives;
            state.guestLives = 0;

            state.brickCount = (int)std::min<size_t>(bricks.size(), MAX_SYNC_BRICKS);
            for (int i = 0; i < state.brickCount; ++i) {
                state.brickActive[i] = bricks[i]->IsActive() ? 1 : 0;
            }
            for (int i = state.brickCount; i < MAX_SYNC_BRICKS; ++i) {
                state.brickActive[i] = 0;
            }

            state.powerUpCount = (int)std::min<size_t>(powerUps.size(), MAX_SYNC_POWERUPS);
            for (int i = 0; i < state.powerUpCount; ++i) {
                Vector2 puPos = powerUps[i]->GetPosition();
                state.powerUps[i].posX = puPos.x;
                state.powerUps[i].posY = puPos.y;
                state.powerUps[i].type = (uint8_t)powerUps[i]->GetType();
                state.powerUps[i].active = powerUps[i]->IsActive() ? 1 : 0;
            }
            for (int i = state.powerUpCount; i < MAX_SYNC_POWERUPS; ++i) {
                state.powerUps[i].posX = 0.0f;
                state.powerUps[i].posY = 0.0f;
                state.powerUps[i].type = 0;
                state.powerUps[i].active = 0;
            }

            state.widePaddleActive = widePaddleActive ? 1 : 0;
            state.frenzyActive = frenzyActive ? 1 : 0;
            networkGame.SendGameState(state);
        } else {
            PaddleUpdateMessage update;
            update.timestamp = (uint32_t)(GetTime() * 1000.0);
            update.paddle.FromGameObject({ guestPaddle.x, guestPaddle.y }, guestPaddle.width, guestPaddle.height);
            networkGame.SendPaddleUpdate(update);

            const GameStateMessage& state = networkGame.GetLastReceivedGameState();
            if (state.timestamp != 0) {
                const Vector2& remoteBall = networkGame.GetRemoteBallPosition();
                if (remoteBall.x > 0.0f || remoteBall.y > 0.0f) {
                    ballPos = remoteBall;
                    ballVel = state.ball.GetVelocity();
                }

                const Vector2& remoteHost = networkGame.GetRemotePaddlePosition();
                if (remoteHost.x > 0.0f || remoteHost.y > 0.0f) {
                    hostPaddle.x = ClampPaddleX(remoteHost.x, hostPaddle.width, screenWidth);
                }

                hostPaddle.y = paddleY;
                guestPaddle.y = paddleY;

                teamScore = state.hostScore;
                teamLives = state.hostLives;
                gameOver = (teamLives <= 0);

                bool remoteWide = state.widePaddleActive != 0;
                if (remoteWide && !widePaddleActive) {
                    hostPaddle.width = paddleWidthBase * 1.8f;
                    guestPaddle.width = paddleWidthBase * 1.8f;
                }
                if (!remoteWide && widePaddleActive) {
                    hostPaddle.width = paddleWidthBase;
                    guestPaddle.width = paddleWidthBase;
                }
                widePaddleActive = remoteWide;
                frenzyActive = (state.frenzyActive != 0);

                int syncBrickCount = std::min(state.brickCount, (int)bricks.size());
                for (int i = 0; i < syncBrickCount; ++i) {
                    bricks[i]->SetActive(state.brickActive[i] != 0);
                }

                for (PowerUp* pu : powerUps) {
                    delete pu;
                }
                powerUps.clear();

                int syncPowerUpCount = std::min(state.powerUpCount, MAX_SYNC_POWERUPS);
                for (int i = 0; i < syncPowerUpCount; ++i) {
                    const PowerUpStateData& netPu = state.powerUps[i];
                    PowerUpType type = PowerUpType::SplitBalls;
                    if (netPu.type == (uint8_t)PowerUpType::WidePaddle) {
                        type = PowerUpType::WidePaddle;
                    } else if (netPu.type == (uint8_t)PowerUpType::Frenzy) {
                        type = PowerUpType::Frenzy;
                    }

                    PowerUp* pu = new PowerUp({ netPu.posX, netPu.posY }, type, 0.0f);
                    pu->SetActive(netPu.active != 0);
                    powerUps.push_back(pu);
                }
            }
        }

        bool allBricksCleared = true;
        for (Brick* brick : bricks) {
            if (brick->IsActive()) {
                allBricksCleared = false;
                break;
            }
        }

        if (mode == NetworkManager::Mode::HOST && allBricksCleared) {
            gameOver = true;
        }

        connectionCheckTimer += dt;
        if (connectionCheckTimer >= CONNECTION_CHECK_INTERVAL) {
            connectionCheckTimer = 0.0f;
            if (!networkGame.IsConnected()) {
                printf("[Main] Connection lost or timed out\n");
                running = false;
            }
        }

        BeginDrawing();
        ClearBackground({ 16, 18, 28, 255 });

        DrawRectangle(0, 0, 5, (int)screenHeight, Fade(BLUE, 0.8f));
        DrawRectangle((int)screenWidth - 5, 0, 5, (int)screenHeight, Fade(BLUE, 0.8f));
        DrawRectangle(0, 0, (int)screenWidth, 5, Fade(BLUE, 0.8f));
        DrawRectangle(0, (int)screenHeight - 5, (int)screenWidth, 5, Fade(BLUE, 0.8f));

        for (Brick* brick : bricks) {
            brick->Draw();
        }

        for (PowerUp* pu : powerUps) {
            pu->Draw();
        }

        DrawRectangleRec(hostPaddle, mode == NetworkManager::Mode::HOST ? ORANGE : SKYBLUE);
        DrawRectangleRec(guestPaddle, mode == NetworkManager::Mode::CLIENT ? ORANGE : SKYBLUE);
        DrawCircleV(ballPos, ballRadius, frenzyActive ? MAGENTA : RED);

        DrawText("Network Co-op Brick Breaker", 220, 16, 30, RAYWHITE);
        DrawText(mode == NetworkManager::Mode::HOST ? "Mode: HOST" : "Mode: CLIENT", 20, 20, 20, GREEN);
        DrawText(TextFormat("SCORE %d", teamScore), 620, 20, 24, GOLD);
        DrawText(TextFormat("LIVES %d", teamLives), 620, 52, 24, SKYBLUE);
        DrawText("Two paddles are both at bottom", 20, 545, 18, LIGHTGRAY);
        DrawText("A/D or Left/Right to move your paddle", 20, 568, 18, LIGHTGRAY);

        if (widePaddleActive) {
            DrawText("WIDE PADDLE", 20, 50, 20, LIME);
        }
        if (frenzyActive) {
            DrawText("FRENZY x3", 20, 78, 20, MAGENTA);
        }

        if (gameOver) {
            DrawRectangle(170, 230, 460, 130, Fade(BLACK, 0.55f));
            DrawRectangleLines(170, 230, 460, 130, RED);
            if (allBricksCleared) {
                DrawText("TEAM WIN! ALL BRICKS CLEARED", 200, 270, 30, GREEN);
            } else {
                DrawText("TEAM GAME OVER", 280, 270, 42, RED);
            }
            DrawText("Press R to restart (Host)", 300, 315, 22, RAYWHITE);
        }

        DrawText("Press ESC to exit", 20, 580, 16, LIGHTGRAY);
        EndDrawing();

        if (IsKeyPressed(KEY_R) && mode == NetworkManager::Mode::HOST) {
            ResetMatch();
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            running = false;
        }
    }

    ClearWorld();
    networkGame.Disconnect();
    CloseWindow();

    printf("[Main] Network game ended\n");
    return 0;
}
