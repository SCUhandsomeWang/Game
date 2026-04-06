#ifndef VISUALOBJECT_H
#define VISUALOBJECT_H

#include "GameObject.h"

class VisualObject : public GameObject {
protected:
    Color color;
    bool visible;
public:
    VisualObject(Vector2 pos = { 0, 0 }, Color col = WHITE, bool vis = true)
        : GameObject(pos), color(col), visible(vis) {}
    
    virtual ~VisualObject() = default;
    
    Color GetColor() const { return color; }
    void SetColor(Color col) { color = col; }
    
    bool IsVisible() const { return visible; }
    void SetVisible(bool vis) { visible = vis; }
};

#endif
