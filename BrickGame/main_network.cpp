#include "GameApp.h"
#include "NetworkGameMode.h"
#include <string>
#include <cstdio>
#include <cstdlib>

#ifdef DrawText
#undef DrawText
#endif

int main() {
    InitWindow(800, 600, "Brick Breaker - Network Mode");
    SetExitKey(KEY_NULL);
    // 取消固定帧率限制（不调用 SetTargetFPS），让渲染不受限制
    // SetTargetFPS(60);

    enum class MenuState {
        MAIN_MENU,
        OFFLINE_GAME,
        NETWORK_MODE_SELECT,
        HOST_CONFIG,
        HOST_WAITING,
        CLIENT_CONNECT,
        NETWORK_PLAYING,
        EXIT
    };

    MenuState menuState = MenuState::MAIN_MENU;
    NetworkGameMode networkGame;
    
    // UI配置
    const int screenWidth = 800;
    const int screenHeight = 600;
    const Color neonCyan = { 40, 240, 255, 255 };
    const Color neonBlue = { 60, 140, 255, 255 };
    const Color neonPink = { 255, 70, 180, 255 };
    const Color panelDark = { 10, 18, 34, 220 };
    const Color panelEdge = { 55, 170, 255, 220 };

    std::string clientIP = "127.0.0.1";
    bool editingClientIP = false;
    
    // 主机配置
    std::string hostIP = "0.0.0.0";
    int hostPort = 5555;
    bool editingHostIP = false;
    bool editingHostPort = false;
    std::string hostPortStr = "5555";
    
    // 客户端配置
    int clientPort = 5555;
    bool editingClientPort = false;
    std::string clientPortStr = "5555";

    auto IsValidIPChar = [](int key) {
        return (key >= '0' && key <= '9') || key == '.';
    };
    
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

    // UI按钮定义
    Rectangle offlineButton = { 100, 180, 250, 60 };
    Rectangle networkButton = { 450, 180, 250, 60 };
    Rectangle hostButton = { 100, 300, 250, 60 };
    Rectangle clientButton = { 450, 300, 250, 60 };
    Rectangle backButton = { 300, 450, 200, 60 };
    Rectangle backButton2 = { 300, 500, 200, 60 };

    while (menuState != MenuState::EXIT && !WindowShouldClose()) {
        float uiTime = (float)GetTime();
        Vector2 mp = GetMousePosition();

        BeginDrawing();
        DrawSciFiBackground(uiTime);

        if (menuState == MenuState::MAIN_MENU) {
            Rectangle menuPanel = { 80, 80, 640, 440 };
            DrawPanel(menuPanel);

            const char* title = "BRICK BREAKER";
            int titleSize = 48;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth / 2 - titleWidth / 2, 120, titleSize, RAYWHITE);

            const char* subtitle = "Network-Enabled Edition";
            int subtitleSize = 20;
            int subtitleWidth = MeasureText(subtitle, subtitleSize);
            DrawText(subtitle, screenWidth / 2 - subtitleWidth / 2, 180, subtitleSize, Fade(neonCyan, 0.8f));

            bool hoverOffline = CheckCollisionPointRec(mp, offlineButton);
            bool hoverNetwork = CheckCollisionPointRec(mp, networkButton);

            DrawNeonButton(offlineButton, "PLAY OFFLINE", hoverOffline, false, { 100, 255, 100, 255 });
            DrawNeonButton(networkButton, "NETWORK MODE", hoverNetwork, false, neonCyan);

            DrawFPS(10, 10);

            EndDrawing();

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (hoverOffline) {
                    menuState = MenuState::OFFLINE_GAME;
                }
                if (hoverNetwork) {
                    menuState = MenuState::NETWORK_MODE_SELECT;
                }
            }
        }
        else if (menuState == MenuState::NETWORK_MODE_SELECT) {
            Rectangle menuPanel = { 80, 80, 640, 440 };
            DrawPanel(menuPanel);

            const char* title = "SELECT NETWORK MODE";
            int titleSize = 36;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth / 2 - titleWidth / 2, 120, titleSize, RAYWHITE);

            bool hoverHost = CheckCollisionPointRec(mp, hostButton);
            bool hoverClient = CheckCollisionPointRec(mp, clientButton);

            DrawNeonButton(hostButton, "HOST GAME", hoverHost, false, neonPink);
            DrawNeonButton(clientButton, "JOIN GAME", hoverClient, false, neonBlue);

            bool hoverBack = CheckCollisionPointRec(mp, backButton);
            DrawNeonButton(backButton, "BACK", hoverBack, false, { 150, 150, 150, 255 });

            DrawFPS(10, 10);

            EndDrawing();

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (hoverHost) {
                    menuState = MenuState::HOST_CONFIG;
                    editingHostIP = true;
                }
                if (hoverClient) {
                    menuState = MenuState::CLIENT_CONNECT;
                    editingClientIP = true;
                }
                if (hoverBack) {
                    menuState = MenuState::MAIN_MENU;
                }
            }
        }
        else if (menuState == MenuState::HOST_CONFIG) {
            Rectangle menuPanel = { 60, 60, 680, 480 };
            DrawPanel(menuPanel);

            const char* title = "CONFIGURE HOST";
            int titleSize = 36;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth / 2 - titleWidth / 2, 100, titleSize, neonPink);

            // IP地址输入框
            DrawText("Bind IP Address (0.0.0.0 = all interfaces):", 100, 180, 18, Fade(neonCyan, 0.9f));
            Rectangle ipBox = { 100, 210, 600, 44 };
            DrawRectangleRounded(ipBox, 0.2f, 6, Fade(panelDark, 0.95f));
            DrawRectangleRoundedLines(ipBox, 0.2f, 6, editingHostIP ? neonCyan : Fade(neonBlue, 0.8f));
            
            std::string displayHostIP = hostIP;
            if (editingHostIP && ((int)(GetTime() * 2.0f) % 2 == 0)) {
                displayHostIP += "_";
            }
            DrawText(displayHostIP.c_str(), 120, 220, 20, RAYWHITE);

            // 端口输入框
            DrawText("Listen Port:", 100, 290, 18, Fade(neonCyan, 0.9f));
            Rectangle portBox = { 100, 320, 200, 44 };
            DrawRectangleRounded(portBox, 0.2f, 6, Fade(panelDark, 0.95f));
            DrawRectangleRoundedLines(portBox, 0.2f, 6, editingHostPort ? neonCyan : Fade(neonBlue, 0.8f));
            
            std::string displayPort = hostPortStr;
            if (editingHostPort && ((int)(GetTime() * 2.0f) % 2 == 0)) {
                displayPort += "_";
            }
            DrawText(displayPort.c_str(), 120, 330, 20, RAYWHITE);

            DrawText("Click to edit. Press Tab to switch fields. Press Enter to start.", 100, 400, 16, Fade(neonBlue, 0.7f));

            // 按钮
            Rectangle startButton = { 100, 450, 250, 50 };
            Rectangle backButton3 = { 450, 450, 250, 50 };
            bool hoverStart = CheckCollisionPointRec(mp, startButton);
            bool hoverBack3 = CheckCollisionPointRec(mp, backButton3);

            DrawNeonButton(startButton, "START HOST", hoverStart, false, neonPink);
            DrawNeonButton(backButton3, "BACK", hoverBack3, false, { 150, 150, 150, 255 });

            DrawFPS(10, 10);

            EndDrawing();

            // 处理鼠标点击切换编辑字段
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mp, ipBox)) {
                    editingHostIP = true;
                    editingHostPort = false;
                } else if (CheckCollisionPointRec(mp, portBox)) {
                    editingHostIP = false;
                    editingHostPort = true;
                } else {
                    editingHostIP = false;
                    editingHostPort = false;
                }
            }

            // IP 地址输入
            if (editingHostIP) {
                int key = GetCharPressed();
                while (key > 0) {
                    if (IsValidIPChar(key) && hostIP.size() < 15) {
                        hostIP.push_back((char)key);
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && !hostIP.empty()) {
                    hostIP.pop_back();
                }
            }

            // 端口输入
            if (editingHostPort) {
                int key = GetCharPressed();
                while (key > 0) {
                    if (key >= '0' && key <= '9' && hostPortStr.size() < 5) {
                        hostPortStr.push_back((char)key);
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && !hostPortStr.empty()) {
                    hostPortStr.pop_back();
                }
            }

            // Tab 键切换
            if (IsKeyPressed(KEY_TAB)) {
                editingHostIP = !editingHostIP;
                editingHostPort = !editingHostPort;
            }

            // 处理按钮点击
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (hoverStart) {
                    if (hostPortStr.empty()) hostPortStr = "5555";
                    hostPort = std::atoi(hostPortStr.c_str());
                    if (hostPort <= 0 || hostPort > 65535) hostPort = 5555;
                    if (hostIP.empty()) hostIP = "0.0.0.0";
                    
                    printf("[Network] Starting HOST with IP: %s, Port: %d\n", hostIP.c_str(), hostPort);
                    menuState = MenuState::HOST_WAITING;
                    networkGame.StartAsHost(hostPort);
                } else if (hoverBack3) {
                    editingHostIP = false;
                    editingHostPort = false;
                    menuState = MenuState::NETWORK_MODE_SELECT;
                }
            }

            // Enter 键启动
            if (IsKeyPressed(KEY_ENTER)) {
                if (hostPortStr.empty()) hostPortStr = "5555";
                hostPort = std::atoi(hostPortStr.c_str());
                if (hostPort <= 0 || hostPort > 65535) hostPort = 5555;
                if (hostIP.empty()) hostIP = "0.0.0.0";
                
                printf("[Network] Starting HOST with IP: %s, Port: %d\n", hostIP.c_str(), hostPort);
                editingHostIP = false;
                editingHostPort = false;
                menuState = MenuState::HOST_WAITING;
                networkGame.StartAsHost(hostPort);
            }
        }
        else if (menuState == MenuState::HOST_WAITING) {
            Rectangle menuPanel = { 100, 120, 600, 360 };
            DrawPanel(menuPanel);

            const char* title = "WAITING FOR CLIENT";
            int titleSize = 36;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth / 2 - titleWidth / 2, 160, titleSize, neonPink);

            // 绘制连接信息
            char ipText[128];
            snprintf(ipText, sizeof(ipText), "Host IP: %s", hostIP.c_str());
            DrawText(ipText, 150, 240, 20, Fade(neonCyan, 0.9f));

            char portText[128];
            snprintf(portText, sizeof(portText), "Port: %d", hostPort);
            DrawText(portText, 150, 280, 20, Fade(neonCyan, 0.9f));

            DrawText("Waiting for client to connect...", 150, 340, 18, Fade(neonBlue, 0.7f));

            bool hoverBack = CheckCollisionPointRec(mp, backButton2);
            DrawNeonButton(backButton2, "BACK", hoverBack, false, { 150, 150, 150, 255 });

            DrawFPS(10, 10);

            EndDrawing();

            networkGame.Update(GetFrameTime());

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverBack) {
                networkGame.Disconnect();
                menuState = MenuState::NETWORK_MODE_SELECT;
            }

            // 主机模式下，收到客户端的板更新后再进入联机画面
            if (networkGame.GetLastReceivedPaddleUpdate().timestamp != 0) {
                printf("[Network] Client connected!\n");
                menuState = MenuState::NETWORK_PLAYING;
            }
        }
        else if (menuState == MenuState::CLIENT_CONNECT) {
            Rectangle menuPanel = { 60, 60, 680, 480 };
            DrawPanel(menuPanel);

            const char* title = "CONNECT TO HOST";
            int titleSize = 36;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth / 2 - titleWidth / 2, 100, titleSize, RAYWHITE);

            DrawText("Enter HOST IP Address:", 100, 170, 18, Fade(neonCyan, 0.8f));
            Rectangle ipBox = { 100, 200, 600, 44 };
            DrawRectangleRounded(ipBox, 0.2f, 6, Fade(panelDark, 0.95f));
            DrawRectangleRoundedLines(ipBox, 0.2f, 6, editingClientIP ? neonCyan : Fade(neonBlue, 0.8f));

            std::string displayIP = clientIP;
            if (editingClientIP && ((int)(GetTime() * 2.0f) % 2 == 0)) {
                displayIP += "_";
            }
            DrawText(displayIP.c_str(), 120, 210, 20, RAYWHITE);

            // 端口输入框
            DrawText("Listen Port:", 100, 280, 18, Fade(neonCyan, 0.8f));
            Rectangle portBox = { 100, 310, 200, 44 };
            DrawRectangleRounded(portBox, 0.2f, 6, Fade(panelDark, 0.95f));
            DrawRectangleRoundedLines(portBox, 0.2f, 6, editingClientPort ? neonCyan : Fade(neonBlue, 0.8f));
            
            std::string displayPort = clientPortStr;
            if (editingClientPort && ((int)(GetTime() * 2.0f) % 2 == 0)) {
                displayPort += "_";
            }
            DrawText(displayPort.c_str(), 120, 320, 20, RAYWHITE);

            DrawText("Digits and dots only for IP. Press Tab to switch. Press Enter to connect.", 100, 380, 14, Fade(neonBlue, 0.7f));

            // 按钮
            Rectangle connectButton = { 100, 420, 250, 50 };
            Rectangle backButton3 = { 450, 420, 250, 50 };
            bool hoverConnect = CheckCollisionPointRec(mp, connectButton);
            bool hoverBack3 = CheckCollisionPointRec(mp, backButton3);

            DrawNeonButton(connectButton, "CONNECT", hoverConnect, false, neonBlue);
            DrawNeonButton(backButton3, "BACK", hoverBack3, false, { 150, 150, 150, 255 });

            DrawFPS(10, 10);

            EndDrawing();

            // 处理鼠标点击切换编辑字段
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mp, ipBox)) {
                    editingClientIP = true;
                    editingClientPort = false;
                } else if (CheckCollisionPointRec(mp, portBox)) {
                    editingClientIP = false;
                    editingClientPort = true;
                } else {
                    editingClientIP = false;
                    editingClientPort = false;
                }
            }

            // IP 地址输入
            if (editingClientIP) {
                int key = GetCharPressed();
                while (key > 0) {
                    if (IsValidIPChar(key) && clientIP.size() < 15) {
                        clientIP.push_back((char)key);
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && !clientIP.empty()) {
                    clientIP.pop_back();
                }
            }

            // 端口输入
            if (editingClientPort) {
                int key = GetCharPressed();
                while (key > 0) {
                    if (key >= '0' && key <= '9' && clientPortStr.size() < 5) {
                        clientPortStr.push_back((char)key);
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && !clientPortStr.empty()) {
                    clientPortStr.pop_back();
                }
            }

            // Tab 键切换
            if (IsKeyPressed(KEY_TAB)) {
                editingClientIP = !editingClientIP;
                editingClientPort = !editingClientPort;
            }

            // 处理按钮点击
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (hoverConnect) {
                    if (clientPortStr.empty()) clientPortStr = "5555";
                    clientPort = std::atoi(clientPortStr.c_str());
                    if (clientPort <= 0 || clientPort > 65535) clientPort = 5555;
                    if (clientIP.empty()) clientIP = "127.0.0.1";

                    if (networkGame.ConnectAsClient(clientIP.c_str(), clientPort)) {
                        printf("[Network] Connecting to %s:%d\n", clientIP.c_str(), clientPort);
                        editingClientIP = false;
                        editingClientPort = false;
                        menuState = MenuState::NETWORK_PLAYING;
                    } else {
                        printf("[Network] Connection failed!\n");
                    }
                } else if (hoverBack3) {
                    editingClientIP = false;
                    editingClientPort = false;
                    menuState = MenuState::NETWORK_MODE_SELECT;
                }
            }

            // Enter 键连接
            if (IsKeyPressed(KEY_ENTER)) {
                if (clientPortStr.empty()) clientPortStr = "5555";
                clientPort = std::atoi(clientPortStr.c_str());
                if (clientPort <= 0 || clientPort > 65535) clientPort = 5555;
                if (clientIP.empty()) clientIP = "127.0.0.1";

                if (networkGame.ConnectAsClient(clientIP.c_str(), clientPort)) {
                    printf("[Network] Connecting to %s:%d\n", clientIP.c_str(), clientPort);
                    editingClientIP = false;
                    editingClientPort = false;
                    menuState = MenuState::NETWORK_PLAYING;
                } else {
                    printf("[Network] Connection failed!\n");
                }
            }
        }
        else if (menuState == MenuState::OFFLINE_GAME) {
            DrawFPS(10, 10);

            EndDrawing();
            // 运行离线游戏
            GameApp offlineApp;
            return offlineApp.Run();
        }
        else if (menuState == MenuState::NETWORK_PLAYING) {
            networkGame.Update(GetFrameTime());

            Rectangle panel = { 90, 90, 620, 420 };
            DrawPanel(panel);

            const char* title = "NETWORK SESSION";
            int titleSize = 36;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth / 2 - titleWidth / 2, 130, titleSize, RAYWHITE);

            const char* modeText = (networkGame.GetMode() == NetworkManager::Mode::HOST) ? "Role: HOST" : "Role: CLIENT";
            DrawText(modeText, 140, 210, 24, Fade(neonCyan, 0.95f));

            const Vector2& remoteBallPos = networkGame.GetRemoteBallPosition();
            const Vector2& remotePaddlePos = networkGame.GetRemotePaddlePosition();
            DrawText(TextFormat("Remote Ball: (%.1f, %.1f)", remoteBallPos.x, remoteBallPos.y), 140, 250, 20, Fade(neonBlue, 0.95f));
            DrawText(TextFormat("Remote Paddle: (%.1f, %.1f)", remotePaddlePos.x, remotePaddlePos.y), 140, 285, 20, Fade(neonBlue, 0.95f));

            DrawCircle((int)remoteBallPos.x, (int)remoteBallPos.y, 8, RED);
            DrawRectangle((int)remotePaddlePos.x, (int)remotePaddlePos.y, 100, 20, SKYBLUE);

            DrawText("Connected. Press ESC to return to menu.", 140, 340, 18, Fade(neonCyan, 0.85f));
            EndDrawing();
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            if (menuState == MenuState::HOST_WAITING || menuState == MenuState::CLIENT_CONNECT) {
                networkGame.Disconnect();
                editingClientIP = false;
                editingClientPort = false;
                menuState = MenuState::NETWORK_MODE_SELECT;
            } else if (menuState == MenuState::HOST_CONFIG) {
                editingHostIP = false;
                editingHostPort = false;
                menuState = MenuState::NETWORK_MODE_SELECT;
            } else {
                menuState = MenuState::MAIN_MENU;
            }
        }
    }

    CloseWindow();
    return 0;
}
