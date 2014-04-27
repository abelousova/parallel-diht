#include <queue>
#include <boost\thread.hpp>

template<class T>
class ConcurrentQueue {
private:
	std::queue<T> internalQueue;
	boost::mutex mutex;
	bool closingFlag;
	boost::condition_variable conditional;
public:
	ConcurrentQueue() : closingFlag(false) {}
	
	void push(const T& value) {
		boost::lock_guard<boost::mutex> guard(mutex);
		internalQueue.push(value);
		conditional.notify_one();
	}

	bool pop(T& value) {
		boost::unique_lock<boost::mutex> unique(mutex);
		conditional.wait(unique, [this]()->bool {
			return !internalQueue.empty() || closingFlag;
		});
		if (!internalQueue.empty()) {
			value = internalQueue.front();
			internalQueue.pop();
			return true;
		}
		else {
			return false;
		}
		
	}

	void setClosingFlag() {
		closingFlag = true;
		conditional.notify_all();
	}
};