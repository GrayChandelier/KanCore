#include "../include/Scheduler.hpp"
#include <iostream>

namespace KanCore::Utils
{
    bool Scheduler::ScheduledTask::operator<(const ScheduledTask& other) const
    {
        return executeAt > other.executeAt;
    }

    Scheduler::Scheduler()
        : worker([this](std::stop_token st) { mainloop(st); })
    {
    }

    Scheduler::~Scheduler()
    {
        worker.request_stop();
        cv.notify_all();
    }

    void Scheduler::mainloop(std::stop_token stop)
    {
        std::unique_lock lock(mutex);

        while (!stop.stop_requested())
        {
            if (tasks.empty())
            {
                cv.wait(lock, [&] {
                    return stop.stop_requested() || !tasks.empty();
                    });
                continue;
            }

            auto now = std::chrono::steady_clock::now();
            auto nextTime = tasks.top().executeAt;

            if (now < nextTime)
            {
                cv.wait_until(lock, nextTime);
                continue;
            }

            auto task = std::move(tasks.top().task);
            tasks.pop();

            lock.unlock();
            try
            {
                task();
            }
            catch (...)
            {
                std::cerr << "Scheduled task crashed\n";
            }
            lock.lock();
        }
    }
}
