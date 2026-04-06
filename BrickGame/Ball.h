#ifndef BALL_H
#define BALL_H

#include "raylib.h"

class Ball {
private:
    Vector2 position;
    Vector2 speed;
    Vector2 initialSpeed;
    float radius;
    float maxSpeedMultiplier;
public:
    Ball(Vector2 pos, Vector2 sp, float r);
    void Move();
    void Draw();
    void BounceEdge(int screenWidth, int screenHeight);
    bool BounceRect(const Rectangle& rect);
    Vector2 GetPosition() const { return position; }
    void ResetPosition(Vector2 pos) { position = pos; }
    bool IsOutOfBounds(int screenHeight) const { return position.y - radius > screenHeight; }
    void IncreaseSpeed();
    void ResetSpeed() { speed = initialSpeed; maxSpeedMultiplier = 1.0f; }
};

#endif