#ifndef BALL_H
#define BALL_H

#include "GameObject.h"

class Ball : public GameObject {
private:
    Vector2 velocity;
    Vector2 initialVelocity;
    float radius;
    Color color;
    bool visible;
    int scoreValue;
    float maxSpeedMultiplier;
    float speedFactor;
    bool launched;

public:
    Ball(Vector2 pos, Vector2 vel, float r, Color c = RED, bool show = true, int score = 10)
        : GameObject(pos), velocity(vel), initialVelocity(vel), radius(r), color(c), visible(show), scoreValue(score), maxSpeedMultiplier(1.0f), speedFactor(1.0f), launched(false) {
    }

    void Update() override {
        if (!launched) {
            return;
        }

        position.x += velocity.x * speedFactor;
        position.y += velocity.y * speedFactor;

        if (position.x - radius <= 5) {
            velocity.x = -velocity.x;
            position.x = 5 + radius;
        }
        if (position.x + radius >= GetScreenWidth() - 5) {
            velocity.x = -velocity.x;
            position.x = GetScreenWidth() - 5 - radius;
        }
        if (position.y - radius <= 5) {
            velocity.y = -velocity.y;
            position.y = 5 + radius;
        }
    }

    void Draw() override {
        if (!visible) {
            return;
        }
        DrawCircle((int)position.x, (int)position.y, radius, color);
    }

    bool BounceRect(const Rectangle& rect) {
        if (CheckCollisionCircleRec(position, radius, rect)) {
            if (position.x < rect.x || position.x > rect.x + rect.width) {
                velocity.x = -velocity.x;
            }
            else {
                velocity.y = -velocity.y;
            }
            IncreaseSpeed();
            return true;
        }
        return false;
    }

    bool IsOutOfBounds(int screenHeight) const {
        return position.y + radius >= screenHeight;
    }

    void ResetPosition(Vector2 pos) {
        position = pos;
    }

    void ResetSpeed() {
        velocity = initialVelocity;
        maxSpeedMultiplier = 1.0f;
    }

    void IncreaseSpeed() {
        if (maxSpeedMultiplier < 3.0f) {
            maxSpeedMultiplier += 0.1f;
            if (maxSpeedMultiplier > 3.0f) {
                maxSpeedMultiplier = 3.0f;
            }
            velocity.x = initialVelocity.x * maxSpeedMultiplier * (velocity.x > 0 ? 1 : -1);
            velocity.y = initialVelocity.y * maxSpeedMultiplier * (velocity.y > 0 ? 1 : -1);
        }
    }

    int GetScoreValue() const {
        return scoreValue;
    }

    void SetScoreValue(int value) {
        scoreValue = value;
    }

    Vector2 GetVelocity() const {
        return velocity;
    }

    void SetVelocity(Vector2 vel) {
        velocity = vel;
        initialVelocity = vel;
    }

    float GetRadius() const {
        return radius;
    }

    void SetColor(Color c) {
        color = c;
    }

    void SetSpeedFactor(float factor) {
        speedFactor = factor;
    }

    bool IsLaunched() const {
        return launched;
    }

    void Launch() {
        launched = true;
    }

    void ResetLaunchState() {
        launched = false;
    }
};

#endif