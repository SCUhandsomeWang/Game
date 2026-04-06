#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include <vector>

enum class GameState { Start, Playing, Settings, Victory, GameOver, DifficultySelect };
enum class Difficulty { Easy = 1, Normal = 2, Hard = 3 };

class GameManager {
private:
    int screenWidth, screenHeight;
    GameState state;
    Ball ball;
    Paddle paddle;
    std::vector<Brick> bricks;
    int score, lives;
    Difficulty difficulty;

    Rectangle startButton, restartButton, quitButton;
    Rectangle easyButton, normalButton, hardButton;

    void CreateBricks();
    void ResetGame();
    void ResetBall();
    void UpdateGame();
    void DrawGame();
    void HandleInput();
    void InitializeDifficultyButtons();
    void ApplyDifficultySettings();

public:
    GameManager(int width, int height);
    void Run();
    bool IsRunning() const { return !WindowShouldClose(); }
};

#endif
