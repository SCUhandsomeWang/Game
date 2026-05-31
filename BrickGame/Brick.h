#ifndef BRICK_H
#define BRICK_H

#include "GameObject.h"
#include <cstdlib>

// Brick: 关卡中的单个砖块。
// 是什么: 保存尺寸、颜色、是否激活与类型（普通/金砖/有害）。
// 为什么: 以轻量对象表示每个砖块，便于关卡数据（激活状态、道具掉落）管理与序列化。
// 怎么用: 使用构造函数指定位置与类型；游戏逻辑在击中砖块后调用 `SetActive(false)`
// 并根据 `GetType()` / `IsGolden()` 决定得分或特殊行为；调用 `GetRect()` 参与碰撞检测。


class Brick : public GameObject {
private:
    float width;
    float height;
    Color color;
    bool active;
    bool isGolden;
    int type;

public:
    // type: 0 = empty(not used), 1 = normal, 2 = gold, 3 = harmful/damage
    Brick(float x, float y, float w, float h, int t = 1)
        : GameObject({ x, y }), width(w), height(h), color(GREEN), active(true), isGolden(false), type(t) {
        if (type == 2) {
            isGolden = true;
            color = GOLD;
        }
        else if (type == 3) {
            isGolden = false;
            color = RED;
        }
        else {
            color = GREEN;
        }
    }

    void Update() override {
    }

    void Draw() override {
        if (!active) {
            return;
        }
        DrawRectangleRec(GetRect(), isGolden ? GOLD : color);
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

    int GetType() const {
        return type;
    }

    Rectangle GetRect() const {
        return { position.x, position.y, width, height };
    }
};

#endif