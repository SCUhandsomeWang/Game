#ifndef BRICK_H
#define BRICK_H

#include "PhysicalObject.h"
#include "VisualObject.h"

class Brick : public PhysicalObject, public VisualObject {
private:
    Rectangle rect;
    bool active;
    bool isGolden;
    int scoreValue;
public:
    Brick(float x = 0.0f, float y = 0.0f, float w = 60.0f, float h = 18.0f,
          Color col = GREEN, bool golden = false, int score = 10)
        : PhysicalObject({ x, y }, { 0, 0 }, 0), VisualObject({ x, y }, col, true),
          rect({ x, y, w, h }), active(true), isGolden(golden), scoreValue(score) {}
    
    void Draw() {
        if (active && visible) {
            DrawRectangleRec(rect, color);
        }
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
        return rect;
    }
    
    int GetScoreValue() const {
        return scoreValue;
    }
    
    void SetScoreValue(int score) {
        scoreValue = score;
    }
};

#endif
