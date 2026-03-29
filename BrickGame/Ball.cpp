#include "Ball.h"
#include "raylib.h"

Ball::Ball(Vector2 pos, Vector2 sp, float r)
    : position(pos), speed(sp), initialSpeed(sp), radius(r), maxSpeedMultiplier(1.0f) {
}

void Ball::Move() {
    position.x += speed.x;
    position.y += speed.y;
}

void Ball::Draw() {
    DrawCircle((int)position.x, (int)position.y, radius, RED);
}

void Ball::BounceEdge(int screenWidth, int screenHeight) {
    if (position.x - radius <= 5) {
        speed.x = -speed.x;
        position.x = 5 + radius;
    }
    if (position.x + radius >= screenWidth - 5) {
        speed.x = -speed.x;
        position.x = screenWidth - 5 - radius;
    }
    if (position.y - radius <= 5) {
        speed.y = -speed.y;
        position.y = 5 + radius;
    }
    if (position.y + radius >= screenHeight - 5) {
        speed.y = -speed.y;
        position.y = screenHeight - 5 - radius;
    }
}

bool Ball::BounceRect(const Rectangle& rect) {
    if (CheckCollisionCircleRec(position, radius, rect)) {
        if (position.x < rect.x || position.x > rect.x + rect.width) {
            speed.x = -speed.x;
        }
        else {
            speed.y = -speed.y;
        }
        IncreaseSpeed();
        return true;
    }
    return false;
}

void Ball::IncreaseSpeed() {
    if (maxSpeedMultiplier < 3.0f) {
        maxSpeedMultiplier += 0.1f;
        if (maxSpeedMultiplier > 3.0f) {
            maxSpeedMultiplier = 3.0f;
        }
        speed.x = initialSpeed.x * maxSpeedMultiplier * (speed.x > 0 ? 1 : -1);
        speed.y = initialSpeed.y * maxSpeedMultiplier * (speed.y > 0 ? 1 : -1);
    }
}