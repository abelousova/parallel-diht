#include <vector>
#include <map>
#include <future>
#include <stdexcept>

#include "CPQueue.h"

template <class Result, class Priority, class Compare = std::less<Priority>>
class ThreadPool {
	typedef std::packaged_task<Result()> Task;
	typedef std::shared_ptr<Task> TaskPtr;
	typedef CPQueue<Priority, Task, Compare> Queue;
	typedef std::map<size_t, boost::shared_future<Result>> ResultMap;
	typedef std::vector<boost::thread> ThreadVector;
public:
	ThreadPool(size_t numberOfThreads);
	~ThreadPool();

	template <class TaskFunctor>
	std::future<Result> addTask(const TaskFunctor& task, const Priority& priority);
	
	Result getResult(const size_t taskId) const;
private:
	Queue queue_;
	ResultMap results_;
	ThreadVector threads_;

	class ThreadFunction {
	public:
		ThreadFunction(Queue& queue, size_t threadId):
			queue_(queue),
			threadId_(threadId)
		{ }
		
		void operator()() {
			std::shared_ptr<Task> task;
			std::cout << "thread " << threadId_ << " started" << std::endl;
			while (queue_.extractNext(task, threadId_)) {
				(*task)();
			}
			std::cout << "thread " << threadId_ << " finished" << std::endl;
		}
	private:
		Queue& queue_;
		size_t threadId_;
	};
};

#define THREADPOOL ThreadPool<Result, Priority, Compare>

template <class Result, class Priority, class Compare>
THREADPOOL::ThreadPool(size_t numberOfThreads) {
	for (size_t i = 0; i < numberOfThreads; ++i) {
		threads_.emplace_back(ThreadFunction(queue_, i));
	}
}

template <class Result, class Priority, class Compare>
template <class TaskFunctor>
std::future<Result> THREADPOOL::addTask(const TaskFunctor& task, const Priority& priority) {
	TaskPtr taskPtr = std::make_shared<Task>(task);
	queue_.insert(priority, taskPtr);
	return taskPtr->get_future();
}




template <class Result, class Priority, class Compare>
THREADPOOL::~ThreadPool() {
	queue_.setClosingFlag();
	std::for_each(threads_.begin(), threads_.end(), std::mem_fn(&boost::thread::join));
}