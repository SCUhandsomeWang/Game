#include "Brick.h"
#include <cstdlib>
#include <ctime>

Brick::Brick(float x, float y, float w, float h) {
    rect = { x, y, w, h };
    active = true;
    // 20% 概率成为金色砖块
    isGolden = (rand() % 100) < 20;
}

void Brick::Draw() {
    if (active) {
        if (isGolden) {
            DrawRectangleRec(rect, GOLD);
        }
        else {
            DrawRectangleRec(rect, GREEN);
        }
    }
}

Rectangle Brick::GetRect() const {
    return rect;
}
