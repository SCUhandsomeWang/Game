#ifndef PHYSICALOBJECT_H
#define PHYSICALOBJECT_H

#include "GameObject.h"

class PhysicalObject : virtual public GameObject {
protected:
    Vector2 velocity;
    float radius;
public:
    PhysicalObject(Vector2 pos, Vector2 vel, float r) 
        : GameObject(pos), velocity(vel), radius(r) {}
    virtual ~PhysicalObject() = default;
    Vector2 GetVelocity() const { return velocity; }
    void SetVelocity(Vector2 vel) { velocity = vel; }
    float GetRadius() const { return radius; }
};

#endif
