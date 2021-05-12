#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>


namespace Engine {
class ThreadPool {
private:
	std::vector<std::thread> threads {};
	std::function<void(uint, void*)> task {};

	std::queue<void*> dataQueue {};

	std::mutex mutex {};

	std::condition_variable cvReady;
	std::condition_variable cvFinished;

	uint workingCount {};

	bool shouldTerminate {};


public:
	~ThreadPool() {
		terminate();
	}


	template <typename Task>
	void init(Task&& task, uint threadCount) {
		threads.resize(threadCount);

		this->task = task;

		workingCount = 0;
		shouldTerminate = false;

		for (uint threadIndex = 0; threadIndex < threadCount; threadIndex++) {
			threads[threadIndex] = std::thread(&ThreadPool::threadFunc, this, threadIndex);
		}
	}

	void appendData(void* data);

	int waitForAll();

	void terminate();


	inline uint getThreadCount() const {
		return threads.size();
	}


private:
	void threadFunc(uint threadIndex);
};
} // namespace Engine