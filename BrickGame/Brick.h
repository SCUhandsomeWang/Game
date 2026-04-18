#ifndef BRICK_H
#define BRICK_H

#include "GameObject.h"
#include <cstdlib>

class Brick : public GameObject {
private:
    float width;
    float height;
    Color color;
    bool active;
    bool isGolden;

public:
    Brick(float x, float y, float w, float h, Color c = GREEN)
        : GameObject({ x, y }), width(w), height(h), color(c), active(true), isGolden((rand() % 100) < 20) {
    }

    void Update() override {
    }

    void Draw() override {
        if (!active) {
            return;
        }
        DrawRectangleRec(GetRect(), isGolden ? GOLD : color);
    }

    bool IsActive() const {
        return active;
    }

    void SetActive(bool a) {
        active = a;
    }

    bool IsGolden() const {
        return isGolden;
    }

    Rectangle GetRect() const {
        return { position.x, position.y, width, height };
    }
};

#endif