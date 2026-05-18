#include "GameApp.h"
#include "NetworkGameMode.h"
#include "GameApp.h"
#include "NetworkGameMode.h"
#include "Profiling.h"
#include <string>
#include <cstdio>
#include <cstdlib>

#ifdef DrawText
#undef DrawText
#endif

int main(int argc, char* argv[]) {
    InitWindow(800, 600, "Brick Breaker - Network Mode");
    SetExitKey(KEY_NULL);
    // 取消固定帧率限制（不调用 SetTargetFPS），让渲染不受限制
    // SetTargetFPS(60);

    enum class MenuState {
        MAIN_MENU,
        OFFLINE_GAME,
        NETWORK_MODE_SELECT,
        PLAYER_ID_INPUT,
        AVATAR_SELECTION,
        ROOM_LOBBY,
        LEVEL_SELECTION,
        HOST_CONFIG,
        HOST_WAITING,
        CLIENT_CONNECT,
        NETWORK_PLAYING,
        EXIT
    };

    MenuState menuState = MenuState::MAIN_MENU;
    NetworkGameMode networkGame;
    // 支持命令行自动进入 host/client 模式：
    bool autoNetwork = false;
    NetworkManager::Mode autoMode = NetworkManager::Mode::OFFLINE;
    std::string autoHostIP = "127.0.0.1";
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--host" || arg == "-h") {
            autoNetwork = true;
            autoMode = NetworkManager::Mode::HOST;
        } else if (arg == "--client" || arg == "-c") {
            autoNetwork = true;
            autoMode = NetworkManager::Mode::CLIENT;
            if (argc > 2) autoHostIP = argv[2];
        }
    }
    
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

    // 玩家 ID / 头像（联机流程用）
    std::string playerName = "";
    int playerNameWeight = 0; // ASCII=1, 非ASCII=2, 最大权重8
    bool editingName = false;
    int selectedAvatar = 0;
    const int avatarCount = 8;
    int selectedLevel = 0;

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
        ZoneScoped;
        FrameMark;
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
                    // 进入联机流程：先输入玩家ID
                    menuState = MenuState::PLAYER_ID_INPUT;
                    editingName = true;
                    playerName.clear();
                    playerNameWeight = 0;
                }
            }
        }
        // 如果传入命令行参数要求自动进入网络模式，尝试在主循环早期触发一次
        if (autoNetwork) {
            if (autoMode == NetworkManager::Mode::HOST) {
                networkGame.StartAsHost();
                menuState = MenuState::ROOM_LOBBY;
                autoNetwork = false;
            } else if (autoMode == NetworkManager::Mode::CLIENT) {
                    networkGame.ConnectAsClient(autoHostIP.c_str(), clientPort);
                menuState = MenuState::ROOM_LOBBY;
                autoNetwork = false;
            }
        }
        else if (menuState == MenuState::PLAYER_ID_INPUT) {
            Rectangle menuPanel = { 60, 60, 680, 480 };
            DrawPanel(menuPanel);

            const char* title = "ENTER YOUR ID";
            int titleSize = 32;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth/2 - titleWidth/2, 100, titleSize, RAYWHITE);

            DrawText("最多 8 个英文 或 5 个汉字。按 Tab 切换输入框，按回格删除。", 100, 150, 14, Fade(neonBlue, 0.7f));

            Rectangle nameBox = { 100, 190, 600, 56 };
            DrawRectangleRounded(nameBox, 0.18f, 6, Fade(panelDark, 0.95f));
            DrawRectangleRoundedLines(nameBox, 0.18f, 6, editingName ? neonCyan : Fade(neonBlue, 0.8f));

            std::string displayName = playerName;
            if (editingName && ((int)(GetTime()*2.0f)%2==0)) displayName += "_";
            DrawText(displayName.c_str(), 120, 200, 22, RAYWHITE);

            // 下一步按钮
            Rectangle nextBtn = { 600, 420, 140, 44 };
            bool hoverNext = CheckCollisionPointRec(mp, nextBtn);
            DrawNeonButton(nextBtn, "下一步", hoverNext, false, neonCyan);

            DrawFPS(10, 10);

            EndDrawing();

            // 输入处理
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mp, nameBox)) editingName = true; else editingName = false;
                if (hoverNext) {
                    menuState = MenuState::AVATAR_SELECTION;
                }
            }

            if (editingName) {
                int key = GetCharPressed();
                while (key > 0) {
                    // 计算权重：ASCII 1，非ASCII 2
                    int w = (key <= 127) ? 1 : 2;
                    if (playerNameWeight + w <= 8) {
                        // 将 codepoint 转为 UTF-8 并追加
                        if (key <= 0x7F) {
                            playerName.push_back((char)key);
                        } else if (key <= 0x7FF) {
                            playerName.push_back((char)(0xC0 | ((key>>6)&0x1F)));
                            playerName.push_back((char)(0x80 | (key & 0x3F)));
                        } else if (key <= 0xFFFF) {
                            playerName.push_back((char)(0xE0 | ((key>>12)&0x0F)));
                            playerName.push_back((char)(0x80 | ((key>>6)&0x3F)));
                            playerName.push_back((char)(0x80 | (key & 0x3F)));
                        } else {
                            playerName.push_back((char)(0xF0 | ((key>>18)&0x07)));
                            playerName.push_back((char)(0x80 | ((key>>12)&0x3F)));
                            playerName.push_back((char)(0x80 | ((key>>6)&0x3F)));
                            playerName.push_back((char)(0x80 | (key & 0x3F)));
                        }
                        playerNameWeight += w;
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && !playerName.empty()) {
                    // 简单处理：删除最后一个字节并调整权重保守估计
                    playerName.pop_back();
                    // 重新计算权重
                    int wsum = 0; for (size_t i=0;i<playerName.size();){ unsigned char c=playerName[i]; if ((c&0x80)==0){ wsum+=1; i+=1; } else if ((c&0xE0)==0xC0){ wsum+=2; i+=2; } else if ((c&0xF0)==0xE0){ wsum+=2; i+=3; } else { wsum+=2; i+=4; } } playerNameWeight = wsum;
                }
            }
        }

        else if (menuState == MenuState::AVATAR_SELECTION) {
            Rectangle menuPanel = { 60, 60, 680, 480 };
            DrawPanel(menuPanel);

            const char* title = "SELECT AVATAR";
            int titleSize = 30;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth/2 - titleWidth/2, 100, titleSize, RAYWHITE);

            // 绘制8个头像格子
            int cols = 4; int rows = 2; int pad = 24;
            int tileW = 120; int tileH = 120;
            int startX = 100; int startY = 160;
            for (int i=0;i<avatarCount;i++){
                int cx = startX + (i%cols)*(tileW+pad);
                int cy = startY + (i/cols)*(tileH+pad);
                Rectangle aRect = {(float)cx, (float)cy, (float)tileW, (float)tileH};
                bool hover = CheckCollisionPointRec(mp, aRect);
                bool sel = (selectedAvatar==i);
                Color accent = Color{ (unsigned char)(50+ i*20), (unsigned char)(120 + (i*10)%120), (unsigned char)(200 - i*10), 255 };
                DrawRectangleRounded(aRect, 0.15f, 6, Fade(panelDark, 0.9f));
                DrawRectangleRoundedLines(aRect, 0.15f, 6, sel ? accent : Fade(neonBlue, 0.6f));
                // 简易头像：绘制不同颜色的圆环和字母
                DrawCircle(cx + tileW/2, cy + tileH/2 - 8, 36, accent);
                DrawText(TextFormat("A%d", i+1), cx + tileW/2 - 12, cy + tileH/2 - 16, 20, RAYWHITE);
                if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) selectedAvatar = i;
            }

            Rectangle nextBtn = { 600, 420, 140, 44 };
            bool hoverNext = CheckCollisionPointRec(mp, nextBtn);
            DrawNeonButton(nextBtn, "下一步", hoverNext, false, neonCyan);

            Rectangle backBtn = { 420, 420, 140, 44 };
            bool hoverBack = CheckCollisionPointRec(mp, backBtn);
            DrawNeonButton(backBtn, "上一步", hoverBack, false, {150,150,150,255});

            DrawFPS(10, 10);

            EndDrawing();

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (hoverNext) {
                    menuState = MenuState::NETWORK_MODE_SELECT;
                } else if (hoverBack) {
                    menuState = MenuState::PLAYER_ID_INPUT;
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
                // debug: 输出鼠标与按钮命中状态
                printf("[Debug] HOST_CONFIG click at (%.1f, %.1f), hoverStart=%d, hoverBack3=%d\n", mp.x, mp.y, hoverStart?1:0, hoverBack3?1:0);
                if (hoverStart) {
                    if (hostPortStr.empty()) hostPortStr = "5555";
                    hostPort = std::atoi(hostPortStr.c_str());
                    if (hostPort <= 0 || hostPort > 65535) hostPort = 5555;
                    if (hostIP.empty()) hostIP = "0.0.0.0";
                    
                    printf("[Network] Starting HOST with IP: %s, Port: %d\n", hostIP.c_str(), hostPort);
                    // 启动主机并进入房间大厅
                    printf("[Debug] Attempting StartAsHost on port %d\n", hostPort);
                    fflush(stdout);
                    bool hostStarted = networkGame.StartAsHost(hostPort);
                    printf("[Debug] StartAsHost returned: %d\n", hostStarted ? 1 : 0);
                    fflush(stdout);
                    if (hostStarted) {
                        printf("[Network] Host started successfully on port %d\n", hostPort);
                        networkGame.SetLocalPlayerInfo(playerName.empty() ? "Host" : playerName, (uint8_t)selectedAvatar);
                        networkGame.SendRoomStateIfHost();
                        menuState = MenuState::ROOM_LOBBY;
                        printf("[Debug] menuState -> ROOM_LOBBY\n");
                    } else {
                        // 启动失败，保留在 Host 配置页并输出日志，便于用户排查
                        printf("[Network] Failed to start host on port %d, stay on HOST_CONFIG.\n", hostPort);
                        menuState = MenuState::HOST_CONFIG;
                        printf("[Debug] menuState -> HOST_CONFIG\n");
                    }
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
                if (networkGame.StartAsHost(hostPort)) {
                    networkGame.SetLocalPlayerInfo(playerName.empty() ? "Host" : playerName, (uint8_t)selectedAvatar);
                    // 恢复正常行为：不自动切为 READY，等待用户手动点击 READY
                    menuState = MenuState::ROOM_LOBBY;
                } else {
                    printf("[Network] Failed to start host on port %d (Enter).\n", hostPort);
                    menuState = MenuState::HOST_CONFIG;
                }
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
        else if (menuState == MenuState::ROOM_LOBBY) {
            networkGame.Update(GetFrameTime());

            Rectangle menuPanel = { 60, 80, 680, 440 };
            DrawPanel(menuPanel);

            const char* title = "ROOM LOBBY";
            int titleSize = 36;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth / 2 - titleWidth / 2, 100, titleSize, RAYWHITE);

            const char* roleText = (networkGame.GetMode() == NetworkManager::Mode::HOST) ? "Role: HOST" : "Role: CLIENT";
            DrawText(roleText, 140, 140, 20, Fade(neonCyan, 0.95f));

            // 左侧：本地玩家卡片
            Rectangle localCard = { 100, 180, 260, 120 };
            DrawRectangleRounded(localCard, 0.12f, 6, Fade(panelDark, 0.95f));
            DrawRectangleRoundedLines(localCard, 0.12f, 6, Fade(neonCyan, 0.6f));
            // 头像
            int ax = (int)localCard.x + 56; int ay = (int)localCard.y + 60;
            Color accentL = Fade(neonBlue, 0.9f);
            DrawCircle(ax, ay, 36, accentL);
            DrawText(networkGame.GetLocalPlayerName().empty() ? "You" : networkGame.GetLocalPlayerName().c_str(), (int)localCard.x + 120, (int)localCard.y + 36, 18, RAYWHITE);
            DrawText(TextFormat("Avatar %d", networkGame.GetLocalAvatar()), (int)localCard.x + 120, (int)localCard.y + 60, 16, Fade(neonBlue, 0.9f));
            // 本地 ready 状态
            bool localReady = networkGame.IsLocalReady();
            DrawText(localReady ? "READY" : "NOT READY", (int)localCard.x + 120, (int)localCard.y + 88, 16, localReady ? GREEN : Fade(neonBlue, 0.6f));

            // 右侧：远端玩家卡片
            Rectangle remoteCard = { 420, 180, 260, 120 };
            DrawRectangleRounded(remoteCard, 0.12f, 6, Fade(panelDark, 0.95f));
            DrawRectangleRoundedLines(remoteCard, 0.12f, 6, Fade(neonCyan, 0.6f));
            if (networkGame.IsInRoom()) {
                int bx = (int)remoteCard.x + 56; int by = (int)remoteCard.y + 60;
                Color accentR = Fade(neonPink, 0.85f);
                DrawCircle(bx, by, 36, accentR);
                DrawText(networkGame.GetRemotePlayerName().empty() ? "Other" : networkGame.GetRemotePlayerName().c_str(), (int)remoteCard.x + 120, (int)remoteCard.y + 36, 18, RAYWHITE);
                DrawText(TextFormat("Avatar %d", networkGame.GetRemoteAvatar()), (int)remoteCard.x + 120, (int)remoteCard.y + 60, 16, Fade(neonBlue, 0.9f));
                DrawText(networkGame.IsRemoteReady() ? "READY" : "NOT READY", (int)remoteCard.x + 120, (int)remoteCard.y + 88, 16, networkGame.IsRemoteReady() ? GREEN : Fade(neonBlue, 0.6f));
            } else {
                DrawText("Waiting for player...", (int)remoteCard.x + 80, (int)remoteCard.y + 56, 18, Fade(neonBlue, 0.85f));
            }

            // 中间：关卡选择（仅 Host 可控）
            Rectangle levelBox = { 300, 220, 200, 80 };
            DrawRectangleRounded(levelBox, 0.12f, 6, Fade(panelDark, 0.98f));
            DrawRectangleRoundedLines(levelBox, 0.12f, 6, Fade(neonCyan, 0.6f));
            int curLevel = networkGame.GetHostedSelectedLevel();
            DrawText("Level", levelBox.x + 72, levelBox.y + 8, 14, Fade(neonCyan, 0.9f));
            DrawText(TextFormat("%d", curLevel), levelBox.x + 92, levelBox.y + 34, 28, RAYWHITE);
            // level arrows
            Rectangle lvlLeft = { levelBox.x - 36, levelBox.y + 18, 28, 28 };
            Rectangle lvlRight = { levelBox.x + levelBox.width + 8, levelBox.y + 18, 28, 28 };
            bool hoverLvlLeft = CheckCollisionPointRec(mp, lvlLeft);
            bool hoverLvlRight = CheckCollisionPointRec(mp, lvlRight);
            DrawNeonButton(lvlLeft, "<", hoverLvlLeft, false, Fade(neonCyan,0.9f));
            DrawNeonButton(lvlRight, ">", hoverLvlRight, false, Fade(neonCyan,0.9f));

            // 底部按钮
            Rectangle readyBtn = { 140, 320, 160, 48 };
            Rectangle startBtn = { 320, 320, 160, 48 };
            Rectangle leaveBtn = { 500, 320, 120, 48 };
            bool hoverReady = CheckCollisionPointRec(mp, readyBtn);
            bool hoverStart = CheckCollisionPointRec(mp, startBtn);
            bool hoverLeave = CheckCollisionPointRec(mp, leaveBtn);

            if (networkGame.GetMode() == NetworkManager::Mode::CLIENT) {
                DrawNeonButton(readyBtn, networkGame.IsLocalReady() ? "UNREADY" : "READY", hoverReady, false, neonBlue);
                    DrawNeonButton(leaveBtn, "LEAVE", hoverLeave, false, (Color){150,150,150,255});
            } else {
                // Host: start disabled until all ready
                bool canStart = networkGame.AllPlayersReady();
                DrawNeonButton(startBtn, "START GAME", hoverStart, false, canStart ? neonPink : Fade(neonPink, 0.45f));
                DrawNeonButton(leaveBtn, "CLOSE", hoverLeave, false, (Color){150,150,150,255});
            }

            DrawText("Tip: Host controls level. Both players must be READY to start.", 140, 380, 14, Fade(neonCyan, 0.7f));

            DrawFPS(10, 10);

            EndDrawing();

            // 点击处理（在 EndDrawing 后处理输入）
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (networkGame.GetMode() == NetworkManager::Mode::CLIENT) {
                    if (hoverReady) {
                        networkGame.ToggleLocalReady();
                        networkGame.SendLocalPlayerInfo();
                    } else if (hoverLeave) {
                        networkGame.Disconnect();
                        menuState = MenuState::NETWORK_MODE_SELECT;
                    }
                } else {
                    // Host clicks
                    if (hoverLvlLeft) {
                        int lvl = networkGame.GetHostedSelectedLevel();
                        if (lvl > 0) --lvl;
                        networkGame.SetHostedSelectedLevel(lvl);
                        networkGame.SendRoomStateIfHost();
                    } else if (hoverLvlRight) {
                        int lvl = networkGame.GetHostedSelectedLevel();
                        ++lvl;
                        networkGame.SetHostedSelectedLevel(lvl);
                        networkGame.SendRoomStateIfHost();
                    } else if (hoverStart) {
                        if (networkGame.AllPlayersReady()) {
                            printf("[Network] Host starting game, level=%d\n", networkGame.GetHostedSelectedLevel());
                            fflush(stdout);
                            networkGame.SendGameStart(networkGame.GetHostedSelectedLevel());
                            menuState = MenuState::NETWORK_PLAYING;
                        } else {
                            printf("[Network] Cannot start, waiting for players.\n");
                            fflush(stdout);
                        }
                    } else if (hoverLeave) {
                        networkGame.Disconnect();
                        menuState = MenuState::NETWORK_MODE_SELECT;
                    }
                }
            }

            // 客户端收到开始信号则进入游戏
            if (networkGame.IsGameStarting()) {
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
                        // 发送本地玩家信息并进入房间大厅
                        networkGame.SetLocalPlayerInfo(playerName.empty() ? "Guest" : playerName, (uint8_t)selectedAvatar);
                        networkGame.SendLocalPlayerInfo();
                        menuState = MenuState::ROOM_LOBBY;
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
                    networkGame.SetLocalPlayerInfo(playerName.empty() ? "Guest" : playerName, (uint8_t)selectedAvatar);
                    networkGame.SendLocalPlayerInfo();
                    menuState = MenuState::ROOM_LOBBY;
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
            // 简化多人游戏：主机模拟球与板物理并广播 GameState，客户端发送 PaddleUpdate 并接收 GameState
            static bool gameInitialized = false;
            static Vector2 ballPos = {400.0f, 300.0f};
            static Vector2 ballVel = {190.0f, -220.0f};
            static Rectangle hostPaddle = {300, 540, 100, 20};
            static Rectangle guestPaddle = {420, 540, 100, 20};
            static int hostScore = 0;
            static int hostLives = 3;

            float dt = GetFrameTime();
            if (!gameInitialized) {
                gameInitialized = true;
                // reset positions
                ballPos = { screenWidth*0.5f, screenHeight*0.5f };
                ballVel = { 190.0f, -220.0f };
                hostPaddle = { screenWidth*0.30f - 50.0f, 540, 100, 20 };
                guestPaddle = { screenWidth*0.70f - 50.0f, 540, 100, 20 };
                hostScore = 0; hostLives = 3;
            }

            networkGame.Update(dt);

            // local input moves paddle depending on role
            float move = 0.0f;
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) move -= 1.0f;
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) move += 1.0f;
            const float paddleSpeed = 500.0f;

            if (networkGame.GetMode() == NetworkManager::Mode::HOST) {
                // host controls hostPaddle
                {
                    float nx = (float)hostPaddle.x + move * paddleSpeed * dt;
                    if (nx < 0.0f) nx = 0.0f;
                    if (nx > screenWidth - hostPaddle.width) nx = screenWidth - hostPaddle.width;
                    hostPaddle.x = nx;
                }
                // apply received remote paddle pos to guestPaddle
                const Vector2& remoteP = networkGame.GetRemotePaddlePosition();
                if (remoteP.x > 0.0f) guestPaddle.x = remoteP.x;

                // simulate ball physics
                ballPos.x += ballVel.x * dt;
                ballPos.y += ballVel.y * dt;
                if (ballPos.x <= 5.0f) { ballPos.x = 5.0f; ballVel.x = fabs(ballVel.x); }
                if (ballPos.x >= screenWidth - 5.0f) { ballPos.x = screenWidth - 5.0f; ballVel.x = -fabs(ballVel.x); }
                if (ballPos.y <= 5.0f) { ballPos.y = 5.0f; ballVel.y = fabs(ballVel.y); }
                if (ballPos.y >= screenHeight - 5.0f) { ballPos.y = screenHeight - 5.0f; hostLives -= 1; if (hostLives>0) { ballPos = {screenWidth*0.5f, screenHeight*0.5f}; ballVel.y = -220.0f; } }

                // collision with paddles
                if (CheckCollisionCircleRec(ballPos, 8.0f, hostPaddle)) {
                    ballVel.y = -fabs(ballVel.y);
                    float hit = (ballPos.x - (hostPaddle.x + hostPaddle.width*0.5f)) / (hostPaddle.width*0.5f);
                    ballVel.x = hit * 320.0f;
                    ballPos.y = hostPaddle.y - 9.0f;
                }
                if (CheckCollisionCircleRec(ballPos, 8.0f, guestPaddle)) {
                    ballVel.y = -fabs(ballVel.y);
                    float hit = (ballPos.x - (guestPaddle.x + guestPaddle.width*0.5f)) / (guestPaddle.width*0.5f);
                    ballVel.x = hit * 320.0f;
                    ballPos.y = guestPaddle.y - 9.0f;
                }

                // send GameState (throttled to 20Hz, include seq)
                static uint32_t gameStateSeq = 0;
                static float lastSendStateTime = 0.0f;
                GameStateMessage state;
                state.timestamp = (uint32_t)(GetTime()*1000.0);
                float nowt = (float)GetTime();
                const float sendInterval = 1.0f / 20.0f; // 20 Hz
                if (nowt - lastSendStateTime >= sendInterval) {
                    lastSendStateTime = nowt;
                    state.seq = ++gameStateSeq;
                } else {
                    state.seq = gameStateSeq; // keep last seq when not sending (will be skipped)
                }
                state.ball.FromVector2(ballPos, ballVel, 8.0f);
                state.hostPaddle.FromGameObject({hostPaddle.x, hostPaddle.y}, hostPaddle.width, hostPaddle.height);
                state.guestPaddle.FromGameObject({guestPaddle.x, guestPaddle.y}, guestPaddle.width, guestPaddle.height);
                state.hostScore = hostScore; state.guestScore = 0;
                state.hostLives = hostLives; state.guestLives = 0;
                state.brickCount = 0; state.powerUpCount = 0; state.widePaddleActive = 0; state.frenzyActive = 0;
                // Only actually send when we've advanced the seq (throttled)
                if (state.seq == gameStateSeq && (nowt - lastSendStateTime) < 0.0001f) {
                    // just-sent in this frame already
                }
                if ((float)state.seq == (float)gameStateSeq && nowt - lastSendStateTime < 0.0001f) {
                    // no-op
                }
                // Send only when we updated seq this frame (throttle enforcement)
                if ((uint32_t)state.seq == gameStateSeq && nowt - lastSendStateTime < sendInterval + 0.0001f) {
                    // Send compact delta most of the time; occasionally send a full snapshot
                    const uint32_t snapshotInterval = 50; // send full snapshot every 50 updates (~2.5s at 20Hz)
                    if ((state.seq % snapshotInterval) == 0) {
                        // send full snapshot
                        networkGame.SendGameStateSnapshot(state);
                    } else {
                        networkGame.SendGameState(state);
                    }
                }

            } else {
                // client controls guestPaddle and sends updates (local prediction)
                {
                    float nx = (float)guestPaddle.x + move * paddleSpeed * dt;
                    if (nx < 0.0f) nx = 0.0f;
                    if (nx > screenWidth - guestPaddle.width) nx = screenWidth - guestPaddle.width;
                    guestPaddle.x = nx; // immediate local prediction
                }
                PaddleUpdateMessage up;
                up.timestamp = (uint32_t)(GetTime()*1000.0);
                up.paddle.FromGameObject({guestPaddle.x, guestPaddle.y}, guestPaddle.width, guestPaddle.height);
                networkGame.SendPaddleUpdate(up);

                // interpolation: keep two most recent snapshots with receive times
                static GameStateMessage prevState; static float prevRecv = 0.0f;
                static GameStateMessage currState; static float currRecv = 0.0f;

                const GameStateMessage& ls = networkGame.GetLastReceivedGameState();
                float nowt = (float)GetTime();
                if (ls.timestamp != 0 && (uint32_t)ls.timestamp != currState.timestamp) {
                    // shift snapshots
                    prevState = currState; prevRecv = currRecv;
                    currState = ls; currRecv = nowt;
                }

                // interpolation delay to smooth network jitter
                const float renderDelay = 0.06f; // 60ms
                Vector2 interpBall = ballPos;
                Vector2 interpHostPaddle = { hostPaddle.x, hostPaddle.y };

                if (prevRecv > 0.0f && currRecv > prevRecv) {
                    float renderTime = nowt - renderDelay;
                    float span = currRecv - prevRecv;
                    float t = (span > 0.0001f) ? (renderTime - prevRecv) / span : 1.0f;
                    if (t < 0.0f) t = 0.0f; if (t > 1.0f) t = 1.0f;

                    Vector2 b0 = prevState.ball.GetPosition();
                    Vector2 b1 = currState.ball.GetPosition();
                    interpBall.x = b0.x + (b1.x - b0.x) * t;
                    interpBall.y = b0.y + (b1.y - b0.y) * t;

                    Vector2 p0 = prevState.hostPaddle.GetPosition();
                    Vector2 p1 = currState.hostPaddle.GetPosition();
                    interpHostPaddle.x = p0.x + (p1.x - p0.x) * t;
                    interpHostPaddle.y = p0.y + (p1.y - p0.y) * t;
                } else if (currRecv > 0.0f) {
                    interpBall = currState.ball.GetPosition();
                    interpHostPaddle = currState.hostPaddle.GetPosition();
                }

                // apply interpolated remote values
                ballPos = interpBall;
                hostPaddle.x = interpHostPaddle.x;
            }

            // Render
            BeginDrawing();
            ClearBackground({16,18,28,255});
            DrawText(networkGame.GetMode() == NetworkManager::Mode::HOST ? "Role: HOST" : "Role: CLIENT", 20, 16, 20, GREEN);
            DrawCircleV(ballPos, 8, RED);
            DrawRectangleRec(hostPaddle, networkGame.GetMode() == NetworkManager::Mode::HOST ? ORANGE : SKYBLUE);
            DrawRectangleRec(guestPaddle, networkGame.GetMode() == NetworkManager::Mode::CLIENT ? ORANGE : SKYBLUE);
            DrawText("Press ESC to return to menu", 20, 560, 16, Fade(neonCyan,0.8f));
            DrawFPS(10, 10);
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