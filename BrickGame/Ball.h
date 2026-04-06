#ifndef BALL_H
#define BALL_H

#include "PhysicalObject.h"
#include "VisualObject.h"

class Ball : public PhysicalObject, public VisualObject {
private:
    Vector2 initialSpeed;
    float maxSpeedMultiplier;
    int scoreValue;
public:
    Ball(Vector2 pos = { 400, 300 }, Vector2 sp = { 2, 2 }, float r = 10.0f, 
         Color col = RED, int score = 10)
        : PhysicalObject(pos, sp, r), VisualObject(pos, col, true), 
          initialSpeed(sp), maxSpeedMultiplier(1.0f), scoreValue(score) {}
    
    void Move() {
        position.x += velocity.x;
        position.y += velocity.y;
    }
    
    void Draw() {
        if (visible) {
            DrawCircle((int)position.x, (int)position.y, radius, color);
        }
    }
    
    void BounceEdge(int screenWidth, int screenHeight) {
        if (position.x - radius <= 5) {
            velocity.x = -velocity.x;
            position.x = 5 + radius;
        }
        if (position.x + radius >= screenWidth - 5) {
            velocity.x = -velocity.x;
            position.x = screenWidth - 5 - radius;
        }
        if (position.y - radius <= 5) {
            velocity.y = -velocity.y;
            position.y = 5 + radius;
        }
        if (position.y + radius >= screenHeight - 5) {
            velocity.y = -velocity.y;
            position.y = screenHeight - 5 - radius;
        }
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
        return position.y - radius > screenHeight;
    }
    
    void IncreaseSpeed() {
        if (maxSpeedMultiplier < 3.0f) {
            maxSpeedMultiplier += 0.1f;
            if (maxSpeedMultiplier > 3.0f) {
                maxSpeedMultiplier = 3.0f;
            }
            velocity.x = initialSpeed.x * maxSpeedMultiplier * (velocity.x > 0 ? 1 : -1);
            velocity.y = initialSpeed.y * maxSpeedMultiplier * (velocity.y > 0 ? 1 : -1);
        }
    }
    
    void ResetSpeed() {
        velocity = initialSpeed;
        maxSpeedMultiplier = 1.0f;
    }
    
    int GetScoreValue() const { return scoreValue; }
    void SetScoreValue(int score) { scoreValue = score; }
};

#endif
