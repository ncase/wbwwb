#include "Tween.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

namespace wb {

float clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

float easeQuadInOut(float t) {
    t = clamp01(t);
    return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

float easeCubicInOut(float t) {
    t = clamp01(t);
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

float easeCircInOut(float t) {
    t = clamp01(t);
    if (t < 0.5f) {
        return (1.0f - std::sqrt(1.0f - std::pow(2.0f * t, 2.0f))) / 2.0f;
    }
    return (std::sqrt(1.0f - std::pow(-2.0f * t + 2.0f, 2.0f)) + 1.0f) / 2.0f;
}

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

void TimerQueue::after(float seconds, std::function<void()> callback) {
    Timer timer{std::max(0.0f, seconds), std::move(callback)};
    if (updating_) {
        pendingTimers_.push_back(std::move(timer));
    } else {
        timers_.push_back(std::move(timer));
    }
}

void TimerQueue::clear() {
    timers_.clear();
    pendingTimers_.clear();
}

void TimerQueue::update(float dt) {
    updating_ = true;
    const std::size_t initialCount = timers_.size();
    std::size_t processed = 0;

    for (std::size_t i = 0; i < timers_.size() && processed < initialCount;) {
        timers_[i].remaining -= dt;
        ++processed;
        if (timers_[i].remaining <= 0.0f) {
            auto callback = std::move(timers_[i].callback);
            timers_.erase(timers_.begin() + static_cast<std::ptrdiff_t>(i));
            if (callback) {
                callback();
            }
        } else {
            ++i;
        }
    }

    updating_ = false;
    if (!pendingTimers_.empty()) {
        timers_.insert(timers_.end(), std::make_move_iterator(pendingTimers_.begin()), std::make_move_iterator(pendingTimers_.end()));
        pendingTimers_.clear();
    }
}

}  // namespace wb
