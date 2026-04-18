#ifndef POWERUP_H
#define POWERUP_H

#include "GameObject.h"

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

#endif
