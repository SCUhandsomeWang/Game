#include "GameManager.h"
#include <cstdlib>
#include <ctime>
#include <cstdio>

GameManager::GameManager(int width, int height)
    : screenWidth(width), screenHeight(height), state(GameState::Start),
      ball({ (float)width / 2, (float)height / 2 }, { 2, 2 }, 10),
      paddle(350, 550, 100, 20), score(0), lives(3), difficulty(Difficulty::Normal) {
    startButton = { (float)(width / 2 - 100), (float)(height / 2 - 30), 200.0f, 60.0f };
    restartButton = { (float)(width / 2 - 100), (float)(height / 2 - 30), 200.0f, 60.0f };
    quitButton = { (float)(width / 2 - 100), (float)(height / 2 + 40), 200.0f, 50.0f };
    InitializeDifficultyButtons();
    CreateBricks();
}

void GameManager::InitializeDifficultyButtons() {
    easyButton = { (float)(screenWidth / 2 - 200), (float)(screenHeight / 2 - 80), 150.0f, 60.0f };
    normalButton = { (float)(screenWidth / 2 - 25), (float)(screenHeight / 2 - 80), 150.0f, 60.0f };
    hardButton = { (float)(screenWidth / 2 + 150), (float)(screenHeight / 2 - 80), 150.0f, 60.0f };
}

void GameManager::CreateBricks() {
    bricks.clear();
    float brickWidth = 60.0f, brickHeight = 18.0f;
    const int wallThickness = 5, gapX = 8, gapY = 6, rowsCount = 6;
    int usableWidth = screenWidth - 2 * wallThickness;
    int cols = (usableWidth + gapX) / ((int)brickWidth + gapX);
    if (cols < 1) cols = 1;
    int totalBricksWidth = cols * (int)brickWidth + (cols - 1) * gapX;
    float startX = wallThickness + (usableWidth - totalBricksWidth) / 2.0f;
    float startY = 80.0f;
    for (int r = 0; r < rowsCount; ++r)
        for (int c = 0; c < cols; ++c)
            bricks.emplace_back(startX + c * (brickWidth + gapX), startY + r * (brickHeight + gapY), brickWidth, brickHeight);
}

void GameManager::ApplyDifficultySettings() {
    float ballSpeedX = 2.0f, ballSpeedY = 2.0f;
    float paddleWidth = 100.0f;
    float paddleX = 350.0f;

    switch (difficulty) {
        case Difficulty::Easy:
            ballSpeedX = 1.5f;
            ballSpeedY = 1.5f;
            paddleWidth = 140.0f;
            paddleX = screenWidth / 2.0f - paddleWidth / 2.0f;
            lives = 5;
            break;
        case Difficulty::Normal:
            ballSpeedX = 2.0f;
            ballSpeedY = 2.0f;
            paddleWidth = 100.0f;
            paddleX = screenWidth / 2.0f - paddleWidth / 2.0f;
            lives = 3;
            break;
        case Difficulty::Hard:
            ballSpeedX = 3.0f;
            ballSpeedY = 3.0f;
            paddleWidth = 70.0f;
            paddleX = screenWidth / 2.0f - paddleWidth / 2.0f;
            lives = 2;
            break;
    }

    paddle = Paddle(paddleX, 550, paddleWidth, 20);
    ball = Ball({ paddleX + paddleWidth / 2, 530 }, { ballSpeedX, ballSpeedY }, 10);
}

void GameManager::ResetGame() {
    CreateBricks();
    score = 0;
    ApplyDifficultySettings();
}

void GameManager::ResetBall() {
    Rectangle paddleRect = paddle.GetRect();
    ball = Ball({ paddleRect.x + paddleRect.width / 2, paddleRect.y - 20 }, { 2, 2 }, 10);
}

void GameManager::HandleInput() {
    Vector2 mp = GetMousePosition();

    if (state == GameState::Start) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mp, startButton)) {
            state = GameState::DifficultySelect;
        }
    } else if (state == GameState::DifficultySelect) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mp, easyButton)) {
                difficulty = Difficulty::Easy;
                ResetGame();
                state = GameState::Playing;
            }
            else if (CheckCollisionPointRec(mp, normalButton)) {
                difficulty = Difficulty::Normal;
                ResetGame();
                state = GameState::Playing;
            }
            else if (CheckCollisionPointRec(mp, hardButton)) {
                difficulty = Difficulty::Hard;
                ResetGame();
                state = GameState::Playing;
            }
        }
    } else if (state == GameState::Settings) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mp, restartButton)) {
            state = GameState::DifficultySelect;
        }
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mp, quitButton)) {
            CloseWindow();
        }
    } else if (state == GameState::Victory || state == GameState::GameOver) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mp, restartButton)) {
            state = GameState::DifficultySelect;
        }
    } else if (state == GameState::Playing) {
        if (IsKeyPressed(KEY_ESCAPE)) state = GameState::Settings;
        paddle.SetCenterX((float)GetMouseX());
    }
}

void GameManager::UpdateGame() {
    if (state != GameState::Playing) return;

    ball.Move();

    if (ball.IsOutOfBounds(screenHeight)) {
        if (--lives <= 0) state = GameState::GameOver;
        else ResetBall();
        return;
    }

    ball.BounceEdge(screenWidth, screenHeight);
    ball.BounceRect(paddle.GetRect());

    for (auto& brick : bricks) {
        if (!brick.IsActive()) continue;
        if (ball.BounceRect(brick.GetRect())) {
            brick.SetActive(false);
            score += brick.IsGolden() ? 100 : 10;
            break;
        }
    }
    
    bool anyActive = false;
    for (auto& brick : bricks) {
        if (brick.IsActive()) { anyActive = true; break; }
    }
    if (!anyActive) state = GameState::Victory;
}

void GameManager::DrawGame() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    if (state == GameState::Start) {
        DrawText("BREAKOUT", screenWidth / 2 - 80, screenHeight / 2 - 140, 40, DARKGRAY);
        bool hover = CheckCollisionPointRec(GetMousePosition(), startButton);
        DrawRectangleRec(startButton, hover ? LIGHTGRAY : GRAY);
        DrawText("START", (int)(startButton.x + 70), (int)(startButton.y + 18), 24, BLACK);
    } else if (state == GameState::DifficultySelect) {
        DrawText("SELECT DIFFICULTY", screenWidth / 2 - 150, screenHeight / 2 - 150, 32, DARKGRAY);

        bool hoverEasy = CheckCollisionPointRec(GetMousePosition(), easyButton);
        bool hoverNormal = CheckCollisionPointRec(GetMousePosition(), normalButton);
        bool hoverHard = CheckCollisionPointRec(GetMousePosition(), hardButton);

        DrawRectangle((int)easyButton.x, (int)easyButton.y, (int)easyButton.width, (int)easyButton.height, hoverEasy ? LIGHTGRAY : GREEN);
        DrawRectangle((int)normalButton.x, (int)normalButton.y, (int)normalButton.width, (int)normalButton.height, hoverNormal ? LIGHTGRAY : YELLOW);
        Color hardColor = hoverHard ? LIGHTGRAY : RED;
        DrawRectangle((int)hardButton.x, (int)hardButton.y, (int)hardButton.width, (int)hardButton.height, hardColor);

        DrawText("EASY", (int)(easyButton.x + 40), (int)(easyButton.y + 18), 22, BLACK);
        DrawText("NORMAL", (int)(normalButton.x + 30), (int)(normalButton.y + 18), 22, BLACK);
        DrawText("HARD", (int)(hardButton.x + 40), (int)(hardButton.y + 18), 22, BLACK);
    } else if (state == GameState::Settings) {
        DrawText("SETTINGS", screenWidth / 2 - 100, screenHeight / 2 - 160, 32, DARKGRAY);
        bool hoverRestart = CheckCollisionPointRec(GetMousePosition(), restartButton);
        bool hoverQuit = CheckCollisionPointRec(GetMousePosition(), quitButton);
        DrawRectangleRec(restartButton, hoverRestart ? LIGHTGRAY : GRAY);
        DrawRectangleRec(quitButton, hoverQuit ? LIGHTGRAY : GRAY);
        DrawText("SELECT DIFFICULTY", (int)(restartButton.x + 20), (int)(restartButton.y + 18), 20, BLACK);
        DrawText("QUIT", (int)(quitButton.x + 80), (int)(quitButton.y + 15), 22, BLACK);
    } else if (state == GameState::Victory) {
        DrawText("YOU WIN!", screenWidth / 2 - 120, screenHeight / 2 - 140, 40, DARKGRAY);
        char scoreText[64]; snprintf(scoreText, sizeof(scoreText), "Final Score: %d", score);
        DrawText(scoreText, screenWidth / 2 - 100, screenHeight / 2 - 60, 24, BLACK);
        bool hover = CheckCollisionPointRec(GetMousePosition(), restartButton);
        DrawRectangleRec(restartButton, hover ? LIGHTGRAY : GRAY);
        DrawText("RESTART", (int)(restartButton.x + 60), (int)(restartButton.y + 18), 24, BLACK);
    } else if (state == GameState::GameOver) {
        DrawText("GAME OVER!", screenWidth / 2 - 120, screenHeight / 2 - 140, 40, RED);
        char scoreText[64]; snprintf(scoreText, sizeof(scoreText), "Final Score: %d", score);
        DrawText(scoreText, screenWidth / 2 - 100, screenHeight / 2 - 60, 24, BLACK);
        bool hover = CheckCollisionPointRec(GetMousePosition(), restartButton);
        DrawRectangleRec(restartButton, hover ? LIGHTGRAY : GRAY);
        DrawText("RESTART", (int)(restartButton.x + 60), (int)(restartButton.y + 18), 24, BLACK);
    } else {
        const int wallThickness = 5;
        DrawRectangle(0, 0, wallThickness, screenHeight, GRAY);
        DrawRectangle(screenWidth - wallThickness, 0, wallThickness, screenHeight, GRAY);
        DrawRectangle(0, 0, screenWidth, wallThickness, GRAY);
        DrawRectangle(0, screenHeight - wallThickness, screenWidth, wallThickness, GRAY);
        ball.Draw();
        paddle.Draw();
        for (auto& brick : bricks) brick.Draw();
        char scoreText[32]; snprintf(scoreText, sizeof(scoreText), "Score: %d", score);
        DrawText(scoreText, screenWidth - 200, 20, 20, BLACK);
        for (int i = 0; i < lives; ++i) DrawRectangle(screenWidth - 200 + i * 30, 50, 10, 10, RED);

        const char* diffText = (difficulty == Difficulty::Easy) ? "Easy" : 
                               (difficulty == Difficulty::Normal) ? "Normal" : "Hard";
        DrawText(diffText, 20, 20, 20, BLACK);
    }
    EndDrawing();
}

void GameManager::Run() {
    SetTargetFPS(60);
    while (IsRunning()) {
        HandleInput();
        UpdateGame();
        DrawGame();
    }
}
