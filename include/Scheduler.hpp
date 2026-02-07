#pragma once
#include <chrono>
#include <condition_variable>
#include <functional>
#include <queue>
#include <mutex>
#include <thread>
#include <stop_token>

namespace KanCore::Utils
{
    class Scheduler
    {
    private:
        struct ScheduledTask
        {
            std::chrono::steady_clock::time_point executeAt;
            std::function<void()> task;

            bool operator<(const ScheduledTask& other) const;
        };

        std::priority_queue<ScheduledTask> tasks;
        std::mutex mutex;
        std::condition_variable cv;
        std::jthread worker;

        void mainloop(std::stop_token stop);

    public:
        Scheduler();
        ~Scheduler();

        template<typename F, typename... Args>
        void schedule(std::chrono::steady_clock::duration delay,
            F&& f, Args&&... args)
        {
            auto execTime = std::chrono::steady_clock::now() + delay;
            Task task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

            {
                std::lock_guard lock(mutex);
                tasks.push({ execTime, std::move(task) });
            }
            cv.notify_one();
        }
    };
}
