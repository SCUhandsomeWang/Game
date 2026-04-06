#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cstdio>

int main() {
    srand((unsigned int)time(nullptr));

    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Brick Breaker Game");
    SetExitKey(KEY_NULL);

    // Create game objects
    Ball ball({ 400, 300 }, { 2, 2 }, 10);
    Paddle paddle(350, 550, 100, 20);

    // Create bricks
    std::vector<Brick> bricks;
    float brickWidth = 60.0f;
    float brickHeight = 18.0f;
    const int wallThickness = 5;
    const int gapX = 8;
    const int gapY = 6;
    const int rowsCount = 6;

    // Game variables
    int score = 0;
    int lives = 3;

    auto CreateBricks = [&]() {
        bricks.clear();
        int usableWidth = screenWidth - 2 * wallThickness;
        int cols = (usableWidth + gapX) / ((int)brickWidth + gapX);
        if (cols < 1) cols = 1;

        int totalBricksWidth = cols * (int)brickWidth + (cols - 1) * gapX;
        float startX = wallThickness + (usableWidth - totalBricksWidth) / 2.0f;
        float startY = 80.0f;

        for (int r = 0; r < rowsCount; ++r) {
            for (int c = 0; c < cols; ++c) {
                float x = startX + c * (brickWidth + gapX);
                float y = startY + r * (brickHeight + gapY);
                bricks.emplace_back(x, y, brickWidth, brickHeight);
            }
        }
    };

    CreateBricks();

    auto ResetGame = [&]() {
        ball = Ball({ (float)screenWidth / 2, (float)screenHeight / 2 }, { 2, 2 }, 10);
        ball.ResetSpeed();
        paddle = Paddle(350, 550, 100, 20);
        CreateBricks();
        score = 0;
        lives = 3;
    };

    auto ResetBall = [&]() {
        Rectangle paddleRect = paddle.GetRect();
        ball = Ball({ paddleRect.x + paddleRect.width / 2, paddleRect.y - 20 }, { 2, 2 }, 10);
    };

    SetTargetFPS(60);

    enum class State { Start, Playing, Settings, Victory, GameOver };
    State state = State::Start;

    Rectangle startButton = { (float)(screenWidth / 2 - 100), (float)(screenHeight / 2 - 30), 200.0f, 60.0f };
    Rectangle restartButton = { (float)(screenWidth / 2 - 100), (float)(screenHeight / 2 - 30), 200.0f, 60.0f };
    Rectangle quitButton = { (float)(screenWidth / 2 - 100), (float)(screenHeight / 2 + 40), 200.0f, 50.0f };

    while (!WindowShouldClose()) {
        Vector2 mp = GetMousePosition();

        if (state == State::Start) {
            BeginDrawing();
            ClearBackground(RAYWHITE);
            const char* title = "BREAKOUT";
            int titleSize = 40;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth / 2 - titleWidth / 2, screenHeight / 2 - 140, titleSize, DARKGRAY);

            bool hover = CheckCollisionPointRec(mp, startButton);
            Color btnColor = hover ? LIGHTGRAY : GRAY;
            DrawRectangleRec(startButton, btnColor);
            int labelSize = 24;
            const char* label = "START";
            int labelWidth = MeasureText(label, labelSize);
            DrawText(label, (int)(startButton.x + startButton.width / 2 - labelWidth / 2), (int)(startButton.y + startButton.height / 2 - labelSize / 2), labelSize, BLACK);

            EndDrawing();

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hover) {
                ResetGame();
                state = State::Playing;
            }

            continue;
        }

        if (state == State::Settings) {
            BeginDrawing();
            ClearBackground(RAYWHITE);
            const char* title = "SETTINGS";
            int titleSize = 32;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth / 2 - titleWidth / 2, screenHeight / 2 - 160, titleSize, DARKGRAY);

            bool hoverRestart = CheckCollisionPointRec(mp, restartButton);
            bool hoverQuit = CheckCollisionPointRec(mp, quitButton);
            DrawRectangleRec(restartButton, hoverRestart ? LIGHTGRAY : GRAY);
            DrawRectangleRec(quitButton, hoverQuit ? LIGHTGRAY : GRAY);
            int labelSize = 22;
            const char* rlabel = "RESTART";
            const char* qlabel = "QUIT";
            int rwidth = MeasureText(rlabel, labelSize);
            int qwidth = MeasureText(qlabel, labelSize);
            DrawText(rlabel, (int)(restartButton.x + restartButton.width / 2 - rwidth / 2), (int)(restartButton.y + restartButton.height / 2 - labelSize / 2), labelSize, BLACK);
            DrawText(qlabel, (int)(quitButton.x + quitButton.width / 2 - qwidth / 2), (int)(quitButton.y + quitButton.height / 2 - labelSize / 2), labelSize, BLACK);
            EndDrawing();

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverRestart) {
                ResetGame();
                state = State::Playing;
            }
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverQuit) {
                break;
            }

            continue;
        }

        if (state == State::Victory) {
            BeginDrawing();
            ClearBackground(RAYWHITE);
            const char* title = "YOU WIN!";
            int titleSize = 40;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth / 2 - titleWidth / 2, screenHeight / 2 - 140, titleSize, DARKGRAY);

            // Display final score
            char scoreText[64];
            snprintf(scoreText, sizeof(scoreText), "Final Score: %d", score);
            int scoreSize = 24;
            int scoreWidth = MeasureText(scoreText, scoreSize);
            DrawText(scoreText, screenWidth / 2 - scoreWidth / 2, screenHeight / 2 - 60, scoreSize, BLACK);

            bool hover = CheckCollisionPointRec(mp, restartButton);
            DrawRectangleRec(restartButton, hover ? LIGHTGRAY : GRAY);
            int labelSize = 24;
            const char* label = "RESTART";
            int labelWidth = MeasureText(label, labelSize);
            DrawText(label, (int)(restartButton.x + restartButton.width / 2 - labelWidth / 2), (int)(restartButton.y + restartButton.height / 2 - labelSize / 2), labelSize, BLACK);

            EndDrawing();

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hover) {
                ResetGame();
                state = State::Playing;
            }

            continue;
        }

        if (state == State::GameOver) {
            BeginDrawing();
            ClearBackground(RAYWHITE);
            const char* title = "GAME OVER!";
            int titleSize = 40;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screenWidth / 2 - titleWidth / 2, screenHeight / 2 - 140, titleSize, RED);

            // Display final score
            char scoreText[64];
            snprintf(scoreText, sizeof(scoreText), "Final Score: %d", score);
            int scoreSize = 24;
            int scoreWidth = MeasureText(scoreText, scoreSize);
            DrawText(scoreText, screenWidth / 2 - scoreWidth / 2, screenHeight / 2 - 60, scoreSize, BLACK);

            bool hover = CheckCollisionPointRec(mp, restartButton);
            DrawRectangleRec(restartButton, hover ? LIGHTGRAY : GRAY);
            int labelSize = 24;
            const char* label = "RESTART";
            int labelWidth = MeasureText(label, labelSize);
            DrawText(label, (int)(restartButton.x + restartButton.width / 2 - labelWidth / 2), (int)(restartButton.y + restartButton.height / 2 - labelSize / 2), labelSize, BLACK);

            EndDrawing();

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hover) {
                ResetGame();
                state = State::Playing;
            }

            continue;
        }

        // Playing state
        if (IsKeyPressed(KEY_ESCAPE)) {
            state = State::Settings;
            continue;
        }

        // Game update
        ball.Move();
        ball.BounceEdge(screenWidth, screenHeight);

        paddle.SetCenterX((float)GetMouseX(), screenWidth);

        if (ball.BounceRect(paddle.GetRect())) {
            // Collision with paddle
        }

        // Collision detection: ball vs bricks
        for (auto& brick : bricks) {
            if (!brick.IsActive()) continue;
            if (ball.BounceRect(brick.GetRect())) {
                brick.SetActive(false);
                if (brick.IsGolden()) {
                    score += 100;
                }
                else {
                    score += 10;
                }
                break;
            }
        }

        // Check if ball is out of bounds
        if (ball.IsOutOfBounds(screenHeight)) {
            lives--;
            if (lives <= 0) {
                state = State::GameOver;
            }
            else {
                ResetBall();
            }
        }

        // Check victory
        bool anyBrickActive = false;
        for (auto& brick : bricks) {
            if (brick.IsActive()) {
                anyBrickActive = true;
                break;
            }
        }
        if (!anyBrickActive) {
            state = State::Victory;
        }

        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawRectangle(0, 0, (int)wallThickness, screenHeight, GRAY);
        DrawRectangle(screenWidth - (int)wallThickness, 0, (int)wallThickness, screenHeight, GRAY);
        DrawRectangle(0, 0, screenWidth, (int)wallThickness, GRAY);
        DrawRectangle(0, screenHeight - (int)wallThickness, screenWidth, (int)wallThickness, GRAY);

        ball.Draw();
        paddle.Draw();
        for (auto& brick : bricks) brick.Draw();

        // Draw score (top right corner)
        char scoreText[32];
        snprintf(scoreText, sizeof(scoreText), "Score: %d", score);
        DrawText(scoreText, screenWidth - 200, 20, 20, BLACK);

        // Draw lives
        int livesY = 50;
        for (int i = 0; i < lives; ++i) {
            DrawRectangle(screenWidth - 200 + i * 30, livesY, 10, 10, RED);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}