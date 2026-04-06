#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "raylib.h"

class GameObject {
protected:
    Vector2 position;
public:
    GameObject(Vector2 pos) : position(pos) {}
    virtual ~GameObject() = default;
    Vector2 GetPosition() const { return position; }
    void SetPosition(Vector2 pos) { position = pos; }
    virtual void Draw() = 0;
};

#endif
