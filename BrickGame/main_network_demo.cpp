#include "raylib.h"
#include "GameApp.h"
#include "NetworkGameMode.h"
#include "NetworkConfig.h"
#include <cstdio>
#include <string>

// 这是一个演示网络功能的简化游戏实现
// 支持单机双实例通过本地网络通信，实现双人合作模式

int main(int argc, char* argv[]) {
    // 检查命令行参数
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
        // 运行原始离线游戏
        printf("[Main] Starting in OFFLINE mode\n");
        GameApp app;
        return app.Run();
    }
    
    // 网络模式
    InitWindow(800, 600, "Brick Breaker - Network Mode");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    
    NetworkGameMode networkGame;
    
    // 连接网络
    bool connected = false;
    if (mode == NetworkManager::Mode::HOST) {
        connected = networkGame.StartAsHost(port);
    } else if (mode == NetworkManager::Mode::CLIENT) {
        connected = networkGame.ConnectAsClient(hostIP.c_str(), port);
    }
    
    if (!connected) {
        printf("[Main] Failed to start network connection\n");
        CloseWindow();
        return -1;
    }
    
    // 游戏循环
    bool running = true;
    float connectionCheckTimer = 0;
    const float CONNECTION_CHECK_INTERVAL = 0.5f;
    
    printf("[Main] Network connection established\n");
    printf("[Main] Network mode: %s\n", mode == NetworkManager::Mode::HOST ? "HOST" : "CLIENT");
    printf("[Main] Press ESC to exit\n");
    
    while (running && !WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        
        // 更新网络状态
        networkGame.Update(deltaTime);
        
        // 检查连接状态
        connectionCheckTimer += deltaTime;
        if (connectionCheckTimer >= CONNECTION_CHECK_INTERVAL) {
            connectionCheckTimer = 0;
            
            if (!networkGame.IsConnected()) {
                printf("[Main] Connection lost or timed out\n");
                running = false;
            }
        }
        
        // 绘制
        BeginDrawing();
        ClearBackground(DARKGRAY);
        
        // 绘制连接信息
        DrawText("Network Demonstration", 300, 50, 30, WHITE);
        
        if (networkGame.IsConnected()) {
            DrawText("CONNECTED", 350, 120, 20, GREEN);
            
            // 绘示示例信息
            char modeText[64];
            snprintf(modeText, sizeof(modeText), "Mode: %s", 
                    mode == NetworkManager::Mode::HOST ? "HOST" : "CLIENT");
            DrawText(modeText, 300, 160, 18, {40, 240, 255, 255});
            
            // 演示远程球位置
            const Vector2& ballPos = networkGame.GetRemoteBallPosition();
            DrawCircle((int)ballPos.x, (int)ballPos.y, 8, RED);
            
            // 演示远程板位置
            const Vector2& paddlePos = networkGame.GetRemotePaddlePosition();
            DrawRectangle((int)paddlePos.x, (int)paddlePos.y, 100, 20, BLUE);
            
            // 显示同步信息
            DrawText("Ball Position Synced from Remote", 250, 250, 16, YELLOW);
            char ballPosText[128];
            snprintf(ballPosText, sizeof(ballPosText), 
                    "Remote Ball: X=%.1f Y=%.1f", ballPos.x, ballPos.y);
            DrawText(ballPosText, 250, 280, 14, WHITE);
            
            char paddlePosText[128];
            snprintf(paddlePosText, sizeof(paddlePosText), 
                    "Remote Paddle: X=%.1f Y=%.1f", paddlePos.x, paddlePos.y);
            DrawText(paddlePosText, 250, 310, 14, WHITE);
            
            DrawText("Network synchronization working!", 250, 380, 16, GREEN);
            
        } else {
            DrawText("CONNECTING...", 340, 120, 20, YELLOW);
        }
        
        DrawText("Press ESC to exit", 300, 550, 16, LIGHTGRAY);
        
        EndDrawing();
        
        // 检查退出
        if (IsKeyPressed(KEY_ESCAPE)) {
            running = false;
        }
    }
    
    networkGame.Disconnect();
    CloseWindow();
    
    printf("[Main] Network game ended\n");
    return 0;
}
