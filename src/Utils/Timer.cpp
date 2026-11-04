#include "Timer.h"

#include <utility>

Timer::Timer(Callback callback)
    : m_start(std::chrono::steady_clock::now()), m_callback(std::move(callback))
{
}

Timer::~Timer()
{
    if (m_callback)
    {
        const auto lifetime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_start).count();
        m_callback(lifetime);
    }
}
