#pragma once
#include <chrono>
namespace KanCore::Utils
{
    class Stopwatch
    {
    private:
        using clock = std::chrono::steady_clock;
        clock::time_point startTime;
        clock::time_point lastCallTime;

    public:
        Stopwatch();

        void reset() noexcept;

        std::chrono::milliseconds elapsedSinceStartMs() const noexcept;

        std::chrono::milliseconds deltaMs() noexcept;
    };

}