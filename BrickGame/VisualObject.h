#ifndef VISUALOBJECT_H
#define VISUALOBJECT_H

#include "GameObject.h"

class VisualObject : virtual public GameObject {
protected:
    Color color;
    bool visible;
public:
    VisualObject(Vector2 pos, Color col, bool vis) 
        : GameObject(pos), color(col), visible(vis) {}
    virtual ~VisualObject() = default;
    Color GetColor() const { return color; }
    void SetColor(Color col) { color = col; }
    bool IsVisible() const { return visible; }
    void SetVisible(bool vis) { visible = vis; }
};

#endif
