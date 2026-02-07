#include "../include/Stopwatch.hpp"

namespace KanCore::Utils
{
    Stopwatch::Stopwatch()
        : startTime(clock::now()),
        lastCallTime(startTime)
    {
    }

    void  Stopwatch::reset() noexcept
    {
        startTime = lastCallTime = clock::now();
    }

    std::chrono::milliseconds Stopwatch::elapsedSinceStartMs() const noexcept
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            clock::now() - startTime
        );
    }

    std::chrono::milliseconds Stopwatch::deltaMs() noexcept
    {
        auto now = clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastCallTime
        );
        lastCallTime = now;
        return elapsed;
    }
}