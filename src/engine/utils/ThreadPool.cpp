#include "ThreadPool.hpp"


namespace Engine {
void ThreadPool::appendData(void* pData) {
	std::unique_lock lock(mutex);
	dataQueue.push(pData);
	cvReady.notify_one();
}


void ThreadPool::terminate() {
	std::unique_lock lock(mutex);
	shouldTerminate = true;
	cvReady.notify_all();
	lock.unlock();

	for (auto& thread : threads) {
		thread.join();
	}
}


int ThreadPool::waitForAll() {
	std::unique_lock lock(mutex);
	cvFinished.wait(lock, [&]() {
		return dataQueue.empty() && (workingCount == 0);
	});

	return 0;
}


void ThreadPool::threadFunc(uint threadIndex) {
	while (!shouldTerminate) {
		std::unique_lock lock(mutex);
		cvReady.wait(lock, [&]() {
			return shouldTerminate || !dataQueue.empty();
		});

		if (!dataQueue.empty()) {
			workingCount++;

			void* pData = dataQueue.front();
			dataQueue.pop();

			lock.unlock();

			task(threadIndex, pData);

			lock.lock();

			workingCount--;
			cvFinished.notify_one();
		}
	}
}
} // namespace Engine