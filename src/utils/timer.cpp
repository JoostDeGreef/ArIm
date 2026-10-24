#include "timer.h"

#include <utility>

Timer::Timer(Callback callback)
    : start_(std::chrono::steady_clock::now()), callback_(std::move(callback))
{
}

Timer::~Timer()
{
    if (callback_)
    {
        const auto lifetime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_).count();
        callback_(lifetime);
    }
}
