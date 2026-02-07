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

		void resolveException(const std::exception& ex);
		void workerLoop();
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


		ThreadPool(size_t workersCount = std::thread::hardware_concurrency());

		~ThreadPool();


	};
}