#ifndef PADDLE_H
#define PADDLE_H

#include "GameObject.h"

// Paddle: 玩家控制的挡板实体。
// 是什么: 封装挡板位置、尺寸、输入控制（鼠标/禁用）与边界约束逻辑。
// 为什么: 将输入与边界限制放在挡板内部，简化游戏主循环与碰撞检测代码。
// 怎么用: 创建后每帧调用 `Update()`（若启用鼠标控制会根据鼠标 X 更新位置），使用
// `GetRect()` 获取用于碰撞检测的矩形，或通过 `SetCenterX()`/`SetWidth()` 调整外部状态。


class Paddle : public GameObject {
private:
    float width;
    float height;
    Color color;
    bool useMouseControl;
    float minBoundX;
    float maxBoundX;

    void ClampToBounds() {
        float rightLimit = maxBoundX - width;
        if (rightLimit < minBoundX) {
            rightLimit = minBoundX;
        }
        if (position.x < minBoundX) {
            position.x = minBoundX;
        }
        if (position.x > rightLimit) {
            position.x = rightLimit;
        }
    }

public:
    Paddle(float x, float y, float w, float h, Color c = BLUE)
        : GameObject({ x, y }), width(w), height(h), color(c), useMouseControl(true), minBoundX(0.0f), maxBoundX((float)GetScreenWidth()) {
        if (maxBoundX <= minBoundX) {
            maxBoundX = 800.0f;
        }
        ClampToBounds();
    }

    void Update() override {
        if (!useMouseControl) {
            return;
        }
        position.x = (float)GetMouseX() - width / 2.0f;
        ClampToBounds();
    }

    void Draw() override {
        DrawRectangleRec(GetRect(), color);
    }

    void SetCenterX(float cx) {
        position.x = cx - width / 2.0f;
        ClampToBounds();
    }

    Rectangle GetRect() const {
        return { position.x, position.y, width, height };
    }

    float GetWidth() const {
        return width;
    }

    void SetWidth(float newWidth) {
        float centerX = position.x + width / 2.0f;
        width = newWidth;
        position.x = centerX - width / 2.0f;
        ClampToBounds();
    }

    void SetControlEnabled(bool enabled) {
        useMouseControl = enabled;
    }

    void SetBounds(float minX, float maxX) {
        minBoundX = minX;
        maxBoundX = maxX;
        if (maxBoundX <= minBoundX) {
            maxBoundX = minBoundX + width + 1.0f;
        }
        ClampToBounds();
    }

    float GetCenterX() const {
        return position.x + width / 2.0f;
    }
};

#endif