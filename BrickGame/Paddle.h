#ifndef PADDLE_H
#define PADDLE_H

#include "GameObject.h"

class Paddle : public GameObject {
private:
    float width;
    float height;
    Color color;
    bool useMouseControl;
    float minBoundX;
    float maxBoundX;

    void ClampToBounds() {
        float rightLimit = maxBoundX - width;
        if (rightLimit < minBoundX) {
            rightLimit = minBoundX;
        }
        if (position.x < minBoundX) {
            position.x = minBoundX;
        }
        if (position.x > rightLimit) {
            position.x = rightLimit;
        }
    }

public:
    Paddle(float x, float y, float w, float h, Color c = BLUE)
        : GameObject({ x, y }), width(w), height(h), color(c), useMouseControl(true), minBoundX(0.0f), maxBoundX((float)GetScreenWidth()) {
        if (maxBoundX <= minBoundX) {
            maxBoundX = 800.0f;
        }
        ClampToBounds();
    }

    void Update() override {
        if (!useMouseControl) {
            return;
        }
        position.x = (float)GetMouseX() - width / 2.0f;
        ClampToBounds();
    }

    void Draw() override {
        DrawRectangleRec(GetRect(), color);
    }

    void SetCenterX(float cx) {
        position.x = cx - width / 2.0f;
        ClampToBounds();
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
        ClampToBounds();
    }

    void SetControlEnabled(bool enabled) {
        useMouseControl = enabled;
    }

    void SetBounds(float minX, float maxX) {
        minBoundX = minX;
        maxBoundX = maxX;
        if (maxBoundX <= minBoundX) {
            maxBoundX = minBoundX + width + 1.0f;
        }
        ClampToBounds();
    }

    float GetCenterX() const {
        return position.x + width / 2.0f;
    }
};

#endif