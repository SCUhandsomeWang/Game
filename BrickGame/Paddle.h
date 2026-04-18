#ifndef PADDLE_H
#define PADDLE_H

#include "GameObject.h"

class Paddle : public GameObject {
private:
    float width;
    float height;
    Color color;

public:
    Paddle(float x, float y, float w, float h, Color c = BLUE)
        : GameObject({ x, y }), width(w), height(h), color(c) {
    }

    void Update() override {
        position.x = (float)GetMouseX() - width / 2.0f;
        if (position.x < 0) position.x = 0;
        if (position.x + width > GetScreenWidth()) {
            position.x = GetScreenWidth() - width;
        }
    }

    void Draw() override {
        DrawRectangleRec(GetRect(), color);
    }

    void SetCenterX(float cx) {
        position.x = cx - width / 2.0f;
        if (position.x < 0) position.x = 0;
        if (position.x + width > GetScreenWidth()) {
            position.x = GetScreenWidth() - width;
        }
    }

    Rectangle GetRect() const {
        return { position.x, position.y, width, height };
    }

    float GetWidth() const {
        return width;
    }

    void SetWidth(float newWidth) {
        float centerX = position.x + width / 2.0f;
        width = newWidth;
        position.x = centerX - width / 2.0f;
        if (position.x < 0) {
            position.x = 0;
        }
        if (position.x + width > GetScreenWidth()) {
            position.x = GetScreenWidth() - width;
        }
    }
};

#endif