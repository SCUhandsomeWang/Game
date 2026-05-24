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
    int type;

public:
    // type: 0 = empty(not used), 1 = normal, 2 = gold, 3 = harmful/damage
    Brick(float x, float y, float w, float h, int t = 1)
        : GameObject({ x, y }), width(w), height(h), color(GREEN), active(true), isGolden(false), type(t) {
        if (type == 2) {
            isGolden = true;
            color = GOLD;
        }
        else if (type == 3) {
            isGolden = false;
            color = RED;
        }
        else {
            color = GREEN;
        }
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

    int GetType() const {
        return type;
    }

    Rectangle GetRect() const {
        return { position.x, position.y, width, height };
    }
};

#endif