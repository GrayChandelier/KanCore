#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <future>
namespace KanCore::Utils
{
	class ThreadPool
	{
	private:
		std::vector<std::thread> workers;
		std::queue<std::function<void()>> tasks;

		std::mutex queueMutex;
		std::condition_variable condition;

		bool stopAll;

		void resolveException(const std::exception& ex)
		{
			printf("Failed to execute thread pool task: %s\n", ex.what());
		}
		void workerLoop()
		{
			while (true)
			{
				std::unique_lock<std::mutex> lock(queueMutex);

				condition.wait(lock, [this]() {return stopAll || !tasks.empty(); });

				if (tasks.empty() && stopAll) return;

				if (tasks.empty()) continue;

				auto task = tasks.front();
				tasks.pop();

				try
				{
					task();
				}
				catch (const std::exception& ex)
				{
					resolveException(ex);
				}
			}
		}
	public:

		template<typename F, typename... Args>
		auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>
		{
			using ReturnType = std::invoke_result_t<F, Args...>;

			auto taskPtr = std::make_shared<std::packaged_task<ReturnType()>>(
				std::bind(std::forward<F>(f), std::forward<Args>(args)...)
			);

			std::future<ReturnType> res = taskPtr->get_future();

			{
				std::lock_guard<std::mutex> lock(queueMutex);
				tasks.push([taskPtr]() { (*taskPtr)(); });
			}

			condition.notify_one();
			return res;
		}

		ThreadPool(size_t workersCount = std::thread::hardware_concurrency()) 
			: stopAll(false)
		{
			for (int it = 0; it < workersCount; it++)
				workers.push_back(std::thread([this]() {workerLoop(); }));
			
		}

		~ThreadPool()
		{
			{
				std::unique_lock<std::mutex> lock(queueMutex);
				stopAll = true;
			}
			condition.notify_all();
			for (auto& worker : workers)
				worker.join();
		}


	};
}