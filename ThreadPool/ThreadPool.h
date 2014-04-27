#include <vector>
#include <map>
#include <future>
#include <stdexcept>

#include "ConcurrentQueue.h"

template <class Result>
class ThreadPool {
	typedef std::packaged_task<Result()> Task;
	typedef std::shared_ptr<Task> TaskPtr;
	typedef ConcurrentQueue<TaskPtr> Queue;
	typedef std::map<size_t, boost::shared_future<Result>> ResultMap;
	typedef std::vector<boost::thread> ThreadVector;
public:
	ThreadPool(size_t numberOfThreads) {
		for (size_t i = 0; i < numberOfThreads; ++i) {
			threads_.emplace_back(ThreadFunction(queue_, i));
		}
	}
	~ThreadPool() {
		queue_.setClosingFlag();
		std::for_each(threads_.begin(), threads_.end(), std::mem_fn(&boost::thread::join));
	}

	template <class TaskFunctor>
	std::future<Result> addTask(const TaskFunctor& task) {
		TaskPtr taskPtr = std::make_shared<Task>(task);
		queue_.push(taskPtr);
		return taskPtr->get_future();
	}

	Result getResult(const size_t taskId) const;
private:
	Queue queue_;
	ResultMap results_;
	ThreadVector threads_;

	class ThreadFunction {
	public:
		ThreadFunction(Queue& queue, size_t threadId) :
			queue_(queue),
			threadId_(threadId)
		{ }

		void operator()() {
			std::shared_ptr<Task> task;
			while (queue_.pop(task)) {
				(*task)();
			}
		}
	private:
		Queue& queue_;
		size_t threadId_;
	};
};