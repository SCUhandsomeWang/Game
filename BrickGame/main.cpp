#include "raylib.h"
#include "Ball.h"
#include "Paddle.h"
#include "Brick.h"
#include "PowerUp.h"
#include <vector>
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

int main() {
    srand((unsigned int)time(nullptr));

    ScoreCalculator scoreCalculator;
    printf("普通砖块得分: %d\n", scoreCalculator.CalculateScore(1));
    printf("金色砖块+3连击得分: %d\n", scoreCalculator.CalculateScore(2, 3));

    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Brick Breaker Game");
    SetExitKey(KEY_NULL);

    Difficulty selectedDifficulty = Difficulty::Normal;
    DifficultyConfig config = GetDifficultyConfig(selectedDifficulty);

    std::vector<GameObject*> objects;
    std::vector<Brick*> bricks;
    std::vector<Ball*> balls;
    std::vector<PowerUp*> powerUps;
    Paddle* paddle = nullptr;

    float brickWidth = 60.0f;
    float brickHeight = 18.0f;
    const int wallThickness = 5;
    const int gapX = 8;
    const int gapY = 6;
    int rowsCount = config.rows;

    int score = 0;
    int lives = config.lives;
    int combo = 0;
    int scoreMultiplier = 1;
    bool widePaddleActive = false;
    bool frenzyActive = false;
    float widePaddleEndTime = 0.0f;
    float frenzyEndTime = 0.0f;
    float basePaddleWidth = config.paddleWidth;

    struct TrailParticle {
        Vector2 pos;
        Vector2 vel;
        float life;
        float maxLife;
        float size;
        Color color;
    };

    struct Shockwave {
        Vector2 pos;
        float radius;
        float maxRadius;
        float life;
        float maxLife;
        Color color;
    };

    std::vector<TrailParticle> trailParticles;
    std::vector<Shockwave> shockwaves;

    auto ClearObjects = [&]() {
        for (GameObject* object : objects) {
            delete object;
        }
        objects.clear();
        bricks.clear();
        balls.clear();
        powerUps.clear();
        trailParticles.clear();
        shockwaves.clear();
        paddle = nullptr;
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

    auto SpawnTrailParticle = [&](Vector2 pos, Color c, float speedMul = 1.0f) {
        float vx = ((float)(rand() % 1000) / 1000.0f - 0.5f) * 60.0f * speedMul;
        float vy = ((float)(rand() % 1000) / 1000.0f - 0.5f) * 60.0f * speedMul;
        float life = 0.2f + ((float)(rand() % 1000) / 1000.0f) * 0.35f;
        float size = 2.0f + ((float)(rand() % 1000) / 1000.0f) * 3.2f;
        trailParticles.push_back({ pos, { vx, vy }, life, life, size, c });
    };

    auto SpawnPickupBurst = [&](Vector2 pos, PowerUpType type) {
        Color innerColor = { 40, 240, 255, 255 };
        Color outerColor = { 60, 140, 255, 255 };
        float innerMaxRadius = 70.0f;
        float outerMaxRadius = 110.0f;
        float innerLife = 0.42f;
        float outerLife = 0.56f;
        int particleCount = 20;

        if (type == PowerUpType::WidePaddle) {
            innerColor = { 120, 255, 160, 255 };
            outerColor = { 70, 210, 255, 255 };
            innerMaxRadius = 82.0f;
            outerMaxRadius = 124.0f;
            innerLife = 0.48f;
            outerLife = 0.62f;
            particleCount = 24;
        }
        else if (type == PowerUpType::Frenzy) {
            innerColor = { 255, 95, 210, 255 };
            outerColor = { 255, 175, 80, 255 };
            innerMaxRadius = 92.0f;
            outerMaxRadius = 140.0f;
            innerLife = 0.54f;
            outerLife = 0.72f;
            particleCount = 30;
        }

        for (int i = 0; i < particleCount; ++i) {
            float angle = ((float)i / (float)particleCount) * 2.0f * PI;
            float speed = 110.0f + (float)(rand() % 100);
            float life = 0.35f + ((float)(rand() % 1000) / 1000.0f) * 0.4f;
            float size = 2.0f + ((float)(rand() % 1000) / 1000.0f) * 4.0f;
            Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
            Color c = (i % 2 == 0) ? innerColor : outerColor;
            trailParticles.push_back({ pos, vel, life, life, size, c });
        }

        shockwaves.push_back({ pos, 10.0f, innerMaxRadius, innerLife, innerLife, innerColor });
        shockwaves.push_back({ pos, 16.0f, outerMaxRadius, outerLife, outerLife, outerColor });
    };

    auto UpdateVfx = [&](float dt) {
        for (size_t i = 0; i < trailParticles.size();) {
            TrailParticle& p = trailParticles[i];
            p.life -= dt;
            if (p.life <= 0.0f) {
                trailParticles.erase(trailParticles.begin() + (int)i);
                continue;
            }
            p.pos.x += p.vel.x * dt;
            p.pos.y += p.vel.y * dt;
            p.vel.x *= 0.97f;
            p.vel.y *= 0.97f;
            ++i;
        }

        for (size_t i = 0; i < shockwaves.size();) {
            Shockwave& s = shockwaves[i];
            s.life -= dt;
            if (s.life <= 0.0f) {
                shockwaves.erase(shockwaves.begin() + (int)i);
                continue;
            }
            float progress = 1.0f - s.life / s.maxLife;
            s.radius = 10.0f + (s.maxRadius - 10.0f) * progress;
            ++i;
        }
    };

    auto DrawVfx = [&]() {
        for (const TrailParticle& p : trailParticles) {
            float alpha = p.life / p.maxLife;
            DrawCircleV(p.pos, p.size, Fade(p.color, alpha * 0.9f));
        }

        for (const Shockwave& s : shockwaves) {
            float alpha = s.life / s.maxLife;
            DrawRing(s.pos, s.radius - 2.0f, s.radius + 2.0f, 0, 360, 56, Fade(s.color, alpha * 0.7f));
        }
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
        paddle = new Paddle(350, 550, basePaddleWidth, 20);
        objects.push_back(paddle);
        CreateBricks();
        score = 0;
        lives = config.lives;
        combo = 0;
        scoreMultiplier = 1;
        widePaddleActive = false;
        frenzyActive = false;
        widePaddleEndTime = 0.0f;
        frenzyEndTime = 0.0f;
    };

    auto ResetBall = [&]() {
        if (paddle == nullptr) {
            return;
        }

        for (Ball* ball : balls) {
            RemoveObject(ball);
            delete ball;
        }
        balls.clear();

        Rectangle paddleRect = paddle->GetRect();
        SpawnBall({ paddleRect.x + paddleRect.width / 2, paddleRect.y - 20 }, config.ballSpeed, 10, false);
    };

    CreateGameObjects();

    SetTargetFPS(60);

    enum class State { Start, Playing, Settings, Victory, GameOver };
    State state = State::Start;

    Rectangle startButton = { (float)(screenWidth / 2 - 100), (float)(screenHeight / 2 - 30), 200.0f, 60.0f };
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
        float dt = GetFrameTime();

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

            bool hover = CheckCollisionPointRec(mp, startButton);
            DrawNeonButton(startButton, "START", hover, false, neonCyan);

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

            const char* currentLabel = GetDifficultyLabel(selectedDifficulty);
            char selectedText[64];
            snprintf(selectedText, sizeof(selectedText), "Current: %s", currentLabel);
            int selectedTextSize = 18;
            int selectedTextWidth = MeasureText(selectedText, selectedTextSize);
            DrawText(selectedText, screenWidth / 2 - selectedTextWidth / 2, (int)hardButton.y + 70, selectedTextSize, Fade(neonCyan, 0.95f));

            EndDrawing();

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverEasy) {
                selectedDifficulty = Difficulty::Easy;
            }
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverNormal) {
                selectedDifficulty = Difficulty::Normal;
            }
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoverHard) {
                selectedDifficulty = Difficulty::Hard;
            }
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hover) {
                CreateGameObjects();
                state = State::Playing;
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
                CreateGameObjects();
                state = State::Playing;
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
                CreateGameObjects();
                state = State::Playing;
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
                CreateGameObjects();
                state = State::Playing;
            }

            continue;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            state = State::Settings;
            continue;
        }

        if (IsKeyPressed(KEY_SPACE)) {
            for (Ball* ball : balls) {
                if (!ball->IsLaunched()) {
                    ball->Launch();
                }
            }
        }

        for (GameObject* object : objects) {
            object->Update();
        }

        if (paddle != nullptr) {
            Rectangle paddleRect = paddle->GetRect();
            for (Ball* ball : balls) {
                if (!ball->IsLaunched()) {
                    ball->SetPosition({ paddleRect.x + paddleRect.width / 2, paddleRect.y - ball->GetRadius() - 1.0f });
                }
            }
        }

        UpdateVfx(dt);

        float now = (float)GetTime();
        if (widePaddleActive && now >= widePaddleEndTime) {
            widePaddleActive = false;
            if (paddle != nullptr) {
                paddle->SetWidth(basePaddleWidth);
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

        if (paddle != nullptr) {
            Rectangle paddleRect = paddle->GetRect();

            for (Ball* ball : balls) {
                if (!ball->IsLaunched()) {
                    continue;
                }
                ball->BounceRect(paddleRect);
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
                        combo++;
                        int brickType = brick->IsGolden() ? 2 : 1;
                        score += scoreCalculator.CalculateScore(brickType, combo) * scoreMultiplier;

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

                Color puColor = neonCyan;
                if (pu->GetType() == PowerUpType::WidePaddle) {
                    puColor = { 120, 255, 160, 255 };
                }
                else if (pu->GetType() == PowerUpType::Frenzy) {
                    puColor = neonPink;
                }
                SpawnTrailParticle(pu->GetPosition(), puColor, 0.7f);

                if (CheckCollisionRecs(pu->GetRect(), paddleRect)) {
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
                        paddle->SetWidth(basePaddleWidth * 2.0f);
                    }
                    else if (type == PowerUpType::Frenzy) {
                        frenzyActive = true;
                        frenzyEndTime = now + 10.0f;
                        scoreMultiplier = 4;
                        for (Ball* ball : balls) {
                            ball->SetSpeedFactor(2.0f);
                        }
                    }

                    SpawnPickupBurst(pu->GetPosition(), type);

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
                combo = 0;
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

        BeginDrawing();
        DrawSciFiBackground(uiTime);

        DrawRectangle(0, 0, (int)wallThickness, screenHeight, Fade(neonBlue, 0.9f));
        DrawRectangle(screenWidth - (int)wallThickness, 0, (int)wallThickness, screenHeight, Fade(neonBlue, 0.9f));
        DrawRectangle(0, 0, screenWidth, (int)wallThickness, Fade(neonBlue, 0.9f));
        DrawRectangle(0, screenHeight - (int)wallThickness, screenWidth, (int)wallThickness, Fade(neonBlue, 0.9f));

        DrawVfx();

        for (GameObject* object : objects) {
            object->Draw();
        }

        char scoreText[48];
        snprintf(scoreText, sizeof(scoreText), "SCORE  %d", score);
        DrawHudCard({ 560, 12, 220, 40 }, scoreText, neonCyan);

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
            DrawHudCard({ 290, 58, 220, 40 }, "PRESS SPACE TO LAUNCH", neonBlue);
        }

        DrawHudCard({ 560, 58, 220, 40 }, "LIVES", neonBlue);

        int livesY = 70;
        for (int i = 0; i < lives; ++i) {
            DrawRectangleRounded({ (float)(618 + i * 30), (float)livesY, 16, 12 }, 0.35f, 4, Fade(neonPink, 0.9f));
        }

        EndDrawing();
    }

    ClearObjects();
    CloseWindow();
    return 0;
}