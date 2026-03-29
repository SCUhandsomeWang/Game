#ifndef PADDLE_H
#define PADDLE_H

#include "raylib.h"

class Paddle {
private:
    Rectangle rect;
    float minY;  // 允许的最小 Y 坐标
    float maxY;  // 允许的最大 Y 坐标
public:
    Paddle(float x, float y, float w, float h);
    void Draw();
    void MoveLeft(float speed);
    void MoveRight(float speed);
    void SetCenterX(float cx);
    void SetCenter(float cx, float cy);  // 同时设置 X 和 Y，并限制范围
    void SetMovementBounds(float minY, float maxY);
    Rectangle GetRect() const;
};

#endif