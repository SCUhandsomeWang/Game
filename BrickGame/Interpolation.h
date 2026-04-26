#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include "raylib.h"
#include <cmath>

class InterpolationSmoothing {
public:
    struct InterpolatedObject {
        Vector2 currentPos;
        Vector2 targetPos;
        Vector2 lastPos;
        float interpolationSpeed;  // 平滑速度（0-1，越大越快）
        bool isInterpolating;
    };

    // 线性插值
    static Vector2 LerpVector(Vector2 from, Vector2 to, float t) {
        t = (t < 0) ? 0 : (t > 1) ? 1 : t;
        return {
            from.x + (to.x - from.x) * t,
            from.y + (to.y - from.y) * t
        };
    }

    // 平滑阶跃插值（Smoothstep）
    static Vector2 SmootstepVector(Vector2 from, Vector2 to, float t) {
        t = (t < 0) ? 0 : (t > 1) ? 1 : t;
        float smoothT = t * t * (3.0f - 2.0f * t);  // smoothstep函数
        return LerpVector(from, to, smoothT);
    }

    // 弹性缓动（Elastic ease out）
    static float EaseOutElastic(float t) {
        if (t == 0) return 0;
        if (t == 1) return 1;
        
        float p = 0.3f;
        float a = 1.0f;
        float s = p / 4.0f;
        
        return a * powf(2, -10 * t) * sinf((t - s) * (2 * PI) / p) + 1;
    }

    // 立方缓动（Ease out cubic）
    static float EaseOutCubic(float t) {
        float f = t - 1;
        return f * f * f + 1;
    }

    // 平方缓动（Ease out quad）
    static float EaseOutQuad(float t) {
        return 1 - (1 - t) * (1 - t);
    }

    // 更新插值对象位置
    static void UpdateInterpolation(InterpolatedObject& obj, float deltaTime) {
        if (!obj.isInterpolating) {
            obj.currentPos = obj.targetPos;
            return;
        }

        float distance = sqrtf((obj.targetPos.x - obj.currentPos.x) * (obj.targetPos.x - obj.currentPos.x) +
                               (obj.targetPos.y - obj.currentPos.y) * (obj.targetPos.y - obj.currentPos.y));

        // 如果距离很小，直接设置目标位置
        if (distance < 1.0f) {
            obj.currentPos = obj.targetPos;
            obj.isInterpolating = false;
            return;
        }

        // 基于速度进行插值
        float step = obj.interpolationSpeed * deltaTime;
        
        // 使用缓动函数
        if (step < 1.0f) {
            float easedStep = EaseOutQuad(step);
            obj.currentPos = LerpVector(obj.currentPos, obj.targetPos, easedStep);
        } else {
            obj.currentPos = obj.targetPos;
            obj.isInterpolating = false;
        }
    }

    // 设置新的目标位置并启动插值
    static void SetTargetPosition(InterpolatedObject& obj, Vector2 newTarget, float smoothness = 0.15f) {
        obj.lastPos = obj.currentPos;
        obj.targetPos = newTarget;
        obj.interpolationSpeed = smoothness;
        obj.isInterpolating = true;
    }

    // 为球创建插值对象
    static InterpolatedObject CreateBallInterpolation(Vector2 pos) {
        return {
            pos,           // currentPos
            pos,           // targetPos
            pos,           // lastPos
            0.2f,          // interpolationSpeed（球移动较快）
            false          // isInterpolating
        };
    }

    // 为板创建插值对象
    static InterpolatedObject CreatePaddleInterpolation(Vector2 pos) {
        return {
            pos,           // currentPos
            pos,           // targetPos
            pos,           // lastPos
            0.25f,         // interpolationSpeed（板移动稍微慢一点以获得更平滑的效果）
            false          // isInterpolating
        };
    }

    // 位置缓存用于预测
    class PositionPredictor {
    private:
        Vector2 lastPos;
        Vector2 lastVelocity;
        float predictionFactor;  // 预测因子（0-1）

    public:
        PositionPredictor() 
            : lastPos({0, 0}), lastVelocity({0, 0}), predictionFactor(0.1f) {}

        // 基于历史速度预测下一个位置
        Vector2 PredictNextPosition(Vector2 currentPos, float deltaTime) {
            Vector2 velocity = {
                currentPos.x - lastPos.x,
                currentPos.y - lastPos.y
            };

            // 计算平滑的速度
            lastVelocity.x = lastVelocity.x * 0.7f + velocity.x * 0.3f;
            lastVelocity.y = lastVelocity.y * 0.7f + velocity.y * 0.3f;

            lastPos = currentPos;

            // 预测位置
            return {
                currentPos.x + lastVelocity.x * predictionFactor * deltaTime,
                currentPos.y + lastVelocity.y * predictionFactor * deltaTime
            };
        }

        void SetPredictionFactor(float factor) {
            predictionFactor = (factor < 0) ? 0 : (factor > 1) ? 1 : factor;
        }

        Vector2 GetLastVelocity() const {
            return lastVelocity;
        }
    };
};

#endif // INTERPOLATION_H
