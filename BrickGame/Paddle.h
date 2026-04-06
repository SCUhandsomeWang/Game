#ifndef PADDLE_H
#define PADDLE_H

#include "PhysicalObject.h"
#include "VisualObject.h"

class Paddle : public PhysicalObject, public VisualObject {
private:
    Rectangle rect;
    float minY;
    float maxY;
public:
    Paddle(float x = 350.0f, float y = 550.0f, float w = 100.0f, float h = 20.0f,
           Color col = BLUE)
        : PhysicalObject({ x, y }, { 0, 0 }, 0), VisualObject({ x, y }, col, true),
          rect({ x, y, w, h }), minY(100.0f), maxY(520.0f) {}
    
    void Draw() {
        if (visible) {
            DrawRectangleRec(rect, color);
        }
    }
    
    void MoveLeft(float speed) {
        rect.x -= speed;
        if (rect.x < 0) rect.x = 0;
        position.x = rect.x;
    }
    
    void MoveRight(float speed, int screenWidth) {
        rect.x += speed;
        if (rect.x + rect.width > screenWidth)
            rect.x = screenWidth - rect.width;
        position.x = rect.x;
    }
    
    void SetCenterX(float cx, int screenWidth) {
        rect.x = cx - rect.width / 2.0f;
        if (rect.x < 0) rect.x = 0;
        if (rect.x + rect.width > screenWidth) rect.x = screenWidth - rect.width;
        position.x = rect.x;
    }
    
    void SetCenter(float cx, float cy, int screenWidth) {
        rect.x = cx - rect.width / 2.0f;
        if (rect.x < 0) rect.x = 0;
        if (rect.x + rect.width > screenWidth) rect.x = screenWidth - rect.width;
        
        rect.y = cy - rect.height / 2.0f;
        if (rect.y < minY) rect.y = minY;
        if (rect.y + rect.height > maxY) rect.y = maxY - rect.height;
        
        position.x = rect.x;
        position.y = rect.y;
    }
    
    void SetMovementBounds(float minYVal, float maxYVal) {
        minY = minYVal;
        maxY = maxYVal;
    }
    
    Rectangle GetRect() const {
        return rect;
    }
};

#endif
