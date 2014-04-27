#include <boost\thread.hpp>

#include "ConcurrentPriorityQueue.h"

template <class Key, class Value, class Compare = std::not1(std::less<Key>)>
class CPQueue {
	typedef std::shared_ptr<Value> ValuePtr;
public:
	CPQueue() { }
	void insert(const Key& key, const ValuePtr value);
	bool extractNext(std::shared_ptr<Value>& value, size_t threadId);
	void setClosingFlag();
private:
	ConcurrentPriorityQueue<Key, Value, Compare> queue_;
};

template <class Key, class Value, class Compare>
void CPQueue<Key, Value, Compare>::insert(const Key& key, const ValuePtr value) {
	queue_.insert(key, value);
}

template <class Key, class Value, class Compare>
bool CPQueue<Key, Value, Compare>::extractNext(std::shared_ptr<Value>& value, size_t threadId) {	
	value = queue_.extractNext();
	if (value == nullptr) {
		return false;
	}
	else {
		return true;
	}
}

template <class Key, class Value, class Compare>
void CPQueue<Key, Value, Compare>::setClosingFlag() {
	queue_.setClosingFlag();
}