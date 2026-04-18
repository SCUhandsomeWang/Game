#ifndef PHYSICALOBJECT_H
#define PHYSICALOBJECT_H

#include "GameObject.h"

class PhysicalObject : virtual public GameObject {
protected:
    Vector2 velocity;
    float radius;

public:
    PhysicalObject(Vector2 pos = { 0, 0 }, Vector2 vel = { 0, 0 }, float r = 0.0f)
        : GameObject(pos), velocity(vel), radius(r) {
    }

    Vector2 GetVelocity() const {
        return velocity;
    }

    void SetVelocity(Vector2 vel) {
        velocity = vel;
    }

    float GetRadius() const {
        return radius;
    }

    void SetRadius(float r) {
        radius = r;
    }
};

#endif