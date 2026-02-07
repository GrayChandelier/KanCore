#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <future>
#include <atomic>
#include <semaphore>
#include <optional>
#include <memory>

namespace KanCore::Threads
{
    using Task = std::function<void()>;
    using TaskPriority = uint16_t;

    struct TaskUnit
    {
        Task task;
        TaskPriority priority;

        struct Comparator
        {
            bool operator()(const TaskUnit& first, const TaskUnit& second) const
            {
                return first.priority > second.priority;
            }
        };

        TaskUnit(TaskUnit&& other) noexcept
            : task(std::move(other.task)), priority(other.priority) {
        }
        TaskUnit& operator=(TaskUnit&& other) noexcept
        {
            task = std::move(other.task);
            priority = other.priority;
            return *this;
        }
        TaskUnit() = default;
        TaskUnit(const TaskUnit&) = delete;
        TaskUnit& operator=(const TaskUnit&) = delete;
    };

    template<typename T>
    class LockFreeQueue
    {
    private:
        std::vector<std::optional<T>> buffer;
        std::atomic<size_t> head{ 0 };
        std::atomic<size_t> tail{ 0 };
        size_t capacity;

    public:
        explicit LockFreeQueue(size_t cap = 1024) : buffer(cap), capacity(cap) {}

        bool push(T item)
        {
            size_t currentTail = tail.load(std::memory_order_relaxed);
            size_t nextTail = (currentTail + 1) % capacity;

            if (nextTail == head.load(std::memory_order_acquire))
                return false; 

            buffer[currentTail] = std::move(item);
            tail.store(nextTail, std::memory_order_release);
            return true;
        }

        std::optional<T> pop()
        {
            size_t currentHead = head.load(std::memory_order_relaxed);

            if (currentHead == tail.load(std::memory_order_acquire))
                return std::nullopt; 

            std::optional<T> item = std::move(buffer[currentHead]);
            buffer[currentHead].reset();

            head.store((currentHead + 1) % capacity, std::memory_order_release);
            return item;
        }

        bool isEmpty() const
        {
            return head.load() == tail.load();
        }
    };

    class ThreadPool
    {
    private:
        using Semaphore = std::counting_semaphore<>;
        Semaphore semaphore{ 0 };

        std::priority_queue<TaskUnit, std::vector<TaskUnit>, TaskUnit::Comparator> nonLockFreeTasks;
        LockFreeQueue<TaskUnit> sortedLockFreeTasks{ 4096 };
        std::mutex enqueueMutex;

        std::vector<std::thread> workers;
        std::atomic<bool> stopped{ false };

        void workerLoop()
        {
            while (!stopped.load())
            {
                semaphore.acquire(); 

                auto optTask = sortedLockFreeTasks.pop();
                if (!optTask.has_value())
                    continue;

                try
                {
                    optTask->task();
                }
                catch (const std::exception& ex)
                {
                    printf("Task failed: %s\n", ex.what());
                }
            }
        }

    public:
        ThreadPool(size_t workersCount = std::thread::hardware_concurrency() - 1)
        {
            for (size_t i = 0; i < workersCount; ++i)
                workers.emplace_back([this]() { workerLoop(); });
        }

        void stop()
        {
            stopped.store(true);

            for (size_t i = 0; i < workers.size(); ++i)
                semaphore.release();

            for (auto& w : workers)
                w.join();
        }

        template<typename F, typename... Args>
        auto enqueue(TaskPriority priority, F&& f, Args&&... args)
            -> std::future<std::invoke_result_t<F, Args...>>
        {
            using ReturnType = std::invoke_result_t<F, Args...>;

            // создаём task, который захватывает аргументы
            auto taskPtr = std::make_shared<std::packaged_task<ReturnType()>>(
                [func = std::forward<F>(f), ... capturedArgs = std::forward<Args>(args)]() mutable {
                    return func(capturedArgs...);
                }
            );

            std::future<ReturnType> res = taskPtr->get_future();

            TaskUnit unit;
            unit.task = [taskPtr]() { (*taskPtr)(); };
            unit.priority = priority;

            TaskUnit topTask;
            {
                std::lock_guard lock(enqueueMutex);
                nonLockFreeTasks.emplace(std::move(unit));
                topTask = std::move(const_cast<TaskUnit&>(nonLockFreeTasks.top()));
                nonLockFreeTasks.pop();
            }

            while (!sortedLockFreeTasks.push(std::move(topTask)))
                std::this_thread::yield();

            semaphore.release();
            return res;
        }


        ~ThreadPool()
        {
            stop();
        }
    };
}
