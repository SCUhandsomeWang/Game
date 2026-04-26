#include "GameApp.h"
#include "NetworkGameMode.h"
#include <string>
#include <cstdio>

#ifdef DrawText
#undef DrawText
#endif

int main() {
    InitWindow(800, 600, "Brick Breaker - Network Mode");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    enum class MenuState {
        MAIN_MENU,
        OFFLINE_GAME,
        NETWORK_MODE_SELECT,
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
    int selectedPort = 5555;
    bool editingClientIP = false;

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

            EndDrawing();

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (hoverHost) {
                    menuState = MenuState::HOST_WAITING;
                    networkGame.StartAsHost(selectedPort);
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
        else if (menuState == MenuState::HOST_WAITING) {
            Rectangle menuPanel = { 100, 120, 600, 360 };
            DrawPanel(menuPanel);

            const char* title = "WAITING FOR CLIENT";
            int titleSize = 36;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth / 2 - titleWidth / 2, 160, titleSize, neonPink);

            // 绘制连接信息
            char ipText[128];
            snprintf(ipText, sizeof(ipText), "Host IP: 127.0.0.1 (Local)");
            DrawText(ipText, 150, 240, 20, Fade(neonCyan, 0.9f));

            char portText[128];
            snprintf(portText, sizeof(portText), "Port: %d", selectedPort);
            DrawText(portText, 150, 280, 20, Fade(neonCyan, 0.9f));

            DrawText("Waiting for client to connect...", 150, 340, 18, Fade(neonBlue, 0.7f));

            bool hoverBack = CheckCollisionPointRec(mp, backButton2);
            DrawNeonButton(backButton2, "BACK", hoverBack, false, { 150, 150, 150, 255 });

            EndDrawing();

            networkGame.Update(GetFrameTime());

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverBack) {
                networkGame.Disconnect();
                menuState = MenuState::MAIN_MENU;
            }

            // 这里可以检测客户端连接并开始游戏
            if (networkGame.IsConnected()) {
                // 在实际实现中，这里应该过渡到游戏状态
                printf("[Network] Client connected!\n");
            }
        }
        else if (menuState == MenuState::CLIENT_CONNECT) {
            Rectangle menuPanel = { 80, 80, 640, 440 };
            DrawPanel(menuPanel);

            const char* title = "CONNECT TO HOST";
            int titleSize = 36;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth / 2 - titleWidth / 2, 120, titleSize, RAYWHITE);

            DrawText("Enter HOST IP for LAN / local testing", 150, 220, 18, Fade(neonCyan, 0.8f));

            Rectangle ipBox = { 150, 250, 500, 44 };
            DrawRectangleRounded(ipBox, 0.2f, 6, Fade(panelDark, 0.95f));
            DrawRectangleRoundedLines(ipBox, 0.2f, 6, editingClientIP ? neonCyan : Fade(neonBlue, 0.8f));

            std::string displayIP = clientIP;
            if (editingClientIP && ((int)(GetTime() * 2.0f) % 2 == 0)) {
                displayIP += "_";
            }
            DrawText(displayIP.c_str(), 170, 260, 20, RAYWHITE);

            DrawText("Digits and dots only. Press Backspace to delete.", 150, 320, 16, Fade(neonBlue, 0.7f));
            DrawText("Press Enter to connect", 150, 350, 18, Fade(neonBlue, 0.9f));

            bool hoverBack = CheckCollisionPointRec(mp, backButton);
            DrawNeonButton(backButton, "BACK", hoverBack, false, { 150, 150, 150, 255 });

            EndDrawing();

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverBack) {
                editingClientIP = false;
                menuState = MenuState::MAIN_MENU;
            }

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

            if (IsKeyPressed(KEY_ENTER)) {
                editingClientIP = false;
                if (clientIP.empty()) {
                    clientIP = "127.0.0.1";
                }

                if (networkGame.ConnectAsClient(clientIP.c_str(), selectedPort)) {
                    printf("[Network] Connecting to %s:%d\n", clientIP.c_str(), selectedPort);
                    menuState = MenuState::NETWORK_PLAYING;
                } else {
                    printf("[Network] Connection failed!\n");
                }
            }
        }
        else if (menuState == MenuState::OFFLINE_GAME) {
            EndDrawing();
            // 运行离线游戏
            GameApp offlineApp;
            return offlineApp.Run();
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            if (menuState == MenuState::HOST_WAITING || menuState == MenuState::CLIENT_CONNECT) {
                networkGame.Disconnect();
            }
            menuState = MenuState::MAIN_MENU;
        }
    }

    CloseWindow();
    return 0;
}
