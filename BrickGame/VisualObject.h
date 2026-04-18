#ifndef VISUALOBJECT_H
#define VISUALOBJECT_H

#include "GameObject.h"

class VisualObject : virtual public GameObject {
protected:
    Color color;
    bool visible;

public:
    VisualObject(Vector2 pos = { 0, 0 }, Color c = WHITE, bool v = true)
        : GameObject(pos), color(c), visible(v) {
    }

    Color GetColor() const {
        return color;
    }

    void SetColor(Color c) {
        color = c;
    }

    bool IsVisible() const {
        return visible;
    }

    void SetVisible(bool v) {
        visible = v;
    }
};

#endif