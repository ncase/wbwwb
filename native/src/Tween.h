#pragma once

#include <functional>
#include <vector>

namespace wb {

float clamp01(float value);
float easeQuadInOut(float t);
float easeCubicInOut(float t);
float easeCircInOut(float t);
float lerp(float a, float b, float t);

class TimerQueue {
public:
    void after(float seconds, std::function<void()> callback);
    void clear();
    void update(float dt);

private:
    struct Timer {
        float remaining = 0.0f;
        std::function<void()> callback;
    };

    std::vector<Timer> timers_;
    std::vector<Timer> pendingTimers_;
    bool updating_ = false;
};

}  // namespace wb
