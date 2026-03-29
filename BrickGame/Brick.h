#ifndef BRICK_H
#define BRICK_H

#include "raylib.h"

class Brick {
private:
    Rectangle rect;
    bool active;
    bool isGolden;
public:
    Brick(float x, float y, float w, float h);
    void Draw();
    bool IsActive() { return active; }
    void SetActive(bool a) { active = a; }
    bool IsGolden() { return isGolden; }
    Rectangle GetRect() const;
};

#endif