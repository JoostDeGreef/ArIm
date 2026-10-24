#pragma once

#include <chrono>
#include <cstdint>
#include <functional>

class Timer
{
public:
    using Callback = std::function<void(std::int64_t)>;

    explicit Timer(Callback callback);

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    ~Timer();

private:
    using ClockTime = std::chrono::steady_clock::time_point;

    ClockTime start_;
    Callback callback_;
};

