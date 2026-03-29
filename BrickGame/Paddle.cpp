#include "Paddle.h"

Paddle::Paddle(float x, float y, float w, float h) {
    rect = { x, y, w, h };
    minY = 100.0f;   // Default minimum Y
    maxY = 520.0f;   // Default maximum Y (screenHeight - height)
}

void Paddle::Draw() {
    DrawRectangleRec(rect, BLUE);
}

void Paddle::MoveLeft(float speed) {
    rect.x -= speed;
    if (rect.x < 0) rect.x = 0;
}

void Paddle::MoveRight(float speed) {
    rect.x += speed;
    if (rect.x + rect.width > GetScreenWidth())
        rect.x = GetScreenWidth() - rect.width;
}

void Paddle::SetCenterX(float cx) {
    rect.x = cx - rect.width / 2.0f;
    if (rect.x < 0) rect.x = 0;
    if (rect.x + rect.width > GetScreenWidth()) rect.x = GetScreenWidth() - rect.width;
}

void Paddle::SetCenter(float cx, float cy) {
    // Set X coordinate, constrained to screen width
    rect.x = cx - rect.width / 2.0f;
    if (rect.x < 0) rect.x = 0;
    if (rect.x + rect.width > GetScreenWidth()) rect.x = GetScreenWidth() - rect.width;

    // Set Y coordinate, constrained to movement range
    rect.y = cy - rect.height / 2.0f;
    if (rect.y < minY) rect.y = minY;
    if (rect.y + rect.height > maxY) rect.y = maxY - rect.height;
}

void Paddle::SetMovementBounds(float minY, float maxY) {
    this->minY = minY;
    this->maxY = maxY;
}

Rectangle Paddle::GetRect() const {
    return rect;
}
