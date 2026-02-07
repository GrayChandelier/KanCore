#include "../include/Threads.hpp"
#include <stdexcept>
#include <iostream>

namespace KanCore::Utils
{

	void ThreadPool::resolveException(const std::exception& ex)
	{
		std::cerr << "Failed to execute thread pool task: " << ex.what() << std::endl;
	}


	void ThreadPool::workerLoop()
	{
		while (true)
		{
			std::unique_lock<std::mutex> lock(queueMutex);

			condition.wait(lock, [this]() { return stopAll || !tasks.empty(); });

			if (tasks.empty() && stopAll)
				return;

			if (tasks.empty())
				continue;

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

	ThreadPool::ThreadPool(size_t workersCount)
		: stopAll(false)
	{
		for (int it = 0; it < workersCount; it++)
			workers.push_back(std::thread([this]() {workerLoop(); }));
	}
	ThreadPool::~ThreadPool()
	{
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			stopAll = true;
		}
		condition.notify_all();
		for (auto& worker : workers)
			worker.join();
	}


}