#ifndef POWERUP_H
#define POWERUP_H

#include "GameObject.h"
#include <vector>

enum class PowerUpType {
    SplitBalls,
    WidePaddle,
    Frenzy
};

class PowerUp : public GameObject {
private:
    PowerUpType type;
    float speedY;
    float size;
    bool active;

public:
    PowerUp(Vector2 pos, PowerUpType powerUpType, float dropSpeed = 2.2f)
        : GameObject(pos), type(powerUpType), speedY(dropSpeed), size(18.0f), active(true) {
    }

    PowerUp() : GameObject({0,0}), type(PowerUpType::SplitBalls), speedY(2.2f), size(18.0f), active(false) {}

    void Reset(Vector2 pos, PowerUpType powerUpType, float dropSpeed = 2.2f) {
        position = pos;
        type = powerUpType;
        speedY = dropSpeed;
        size = 18.0f;
        active = true;
    }

    void Update() override {
        position.y += speedY;
    }

    void Draw() override {
        if (!active) {
            return;
        }

        Color c = SKYBLUE;
        const char* label = "S";
        if (type == PowerUpType::WidePaddle) {
            c = LIME;
            label = "W";
        }
        else if (type == PowerUpType::Frenzy) {
            c = ORANGE;
            label = "F";
        }

        Rectangle rect = GetRect();
        DrawRectangleRounded(rect, 0.25f, 6, c);
        int fontSize = 18;
        int textW = MeasureText(label, fontSize);
        DrawText(label, (int)(rect.x + rect.width / 2 - textW / 2), (int)(rect.y + rect.height / 2 - fontSize / 2), fontSize, BLACK);
    }

    Rectangle GetRect() const {
        return { position.x - size / 2.0f, position.y - size / 2.0f, size, size };
    }

    PowerUpType GetType() const {
        return type;
    }

    bool IsActive() const {
        return active;
    }

    void SetActive(bool value) {
        active = value;
    }

    bool IsOutOfScreen(int screenHeight) const {
        return position.y - size / 2.0f > screenHeight;
    }
};

// Simple object pool for PowerUp to reduce frequent allocations.
namespace PowerUpPool {
    static std::vector<PowerUp*> s_pool;

    static PowerUp* Acquire(Vector2 pos, PowerUpType type, float dropSpeed = 2.2f) {
        if (!s_pool.empty()) {
            PowerUp* p = s_pool.back();
            s_pool.pop_back();
            p->Reset(pos, type, dropSpeed);
            return p;
        }
        return new PowerUp(pos, type, dropSpeed);
    }

    static void Release(PowerUp* p) {
        if (p == nullptr) return;
        p->SetActive(false);
        s_pool.push_back(p);
    }

    static void ClearPool() {
        for (PowerUp* p : s_pool) {
            delete p;
        }
        s_pool.clear();
    }
}

#endif
