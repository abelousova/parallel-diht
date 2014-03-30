#include <vector>
#include <memory>
#include <boost\thread.hpp>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>

template<class Key, class Value>
class Node {
	typedef std::shared_ptr<Node> NodePtr;
	typedef std::shared_ptr<Value> ValuePtr;
	typedef boost::detail::spinlock SpinMutex;
public:
	Node(const Key& key, const ValuePtr& value) : 
		_key(key), 
		_value(value) {
		_mutex.unlock();
	}

	void lock() { _mutex.lock(); }
	void unlock() { _mutex.unlock(); }
	bool try_lock() { return _mutex.try_lock(); }

	const Key& getKey() const { return _key; }
	void setKey(const Key& newKey) { _key = newKey; }
	ValuePtr getValue() const { return _value; }

private:
	Key _key;
	ValuePtr _value;

	SpinMutex _mutex;
};

template <class Key, class Value, class Compare = std::less<Key>>
class ConcurrentPriorityQueue {
	typedef Node<Key, Value> HNode;
	typedef std::shared_ptr<HNode> NodePtr;
	typedef std::shared_ptr<Value> ValuePtr;
	typedef boost::shared_mutex ReadWriteLock;
	typedef std::vector<std::shared_ptr<std::atomic<bool>>> AtomicBoolVector;
	typedef boost::mutex Mutex;
	typedef boost::condition_variable Condition;
	typedef boost::unique_lock<Mutex> UniqueLock;
	typedef boost::lock_guard<Mutex> LockGuard;
public:
	ConcurrentPriorityQueue(Compare comp = Compare()): compare(comp), closingFlag_(false), lastIndex(-1) {	}
	ConcurrentPriorityQueue(const std::vector<std::pair<Key, Value>>& vector);
	
	ConcurrentPriorityQueue(const ConcurrentPriorityQueue& queue) = delete;
	ConcurrentPriorityQueue& operator= (const ConcurrentPriorityQueue& queue) = delete;

	bool isEmpty();
	void insert(const Key& key, const Value& value) { insert(key, std::make_shared<Value>(value)); }
	void insert(const Key& key, const ValuePtr& value);
	ValuePtr extractNext();
	void setClosingFlag();
private:
	Compare compare;

	ReadWriteLock changeSizeMutex;
	ReadWriteLock extractInsertMutex;
	std::mutex conditionalMutex_;
	std::condition_variable condition_;

	std::vector<NodePtr> heap;
	bool closingFlag_;

	AtomicBoolVector isNodeLockedVector;

	size_t heapify(const size_t index);
	void heapifyWithoutLock(const size_t index);
	void pushUp(size_t index);
	std::atomic<int> lastIndex;

	int getParent(const size_t index);
	int getLeft(const size_t index);
	int getRight(const size_t index);

	NodePtr lockNodeByIndexSizeNotLocked(const size_t index);
	NodePtr lockNodeByIndexSizeLocked(const size_t index);

	enum Child{left, right, none};
	void unlockChildren(NodePtr leftNodePtr, NodePtr rightNodePtr, const size_t index, const Child swappedChild);	
};

#define QUEUE ConcurrentPriorityQueue<Key, Value, Compare>

template <class Key, class Value, class Compare>
int QUEUE::getParent(const size_t index) {
	return (index + 1) / 2 - 1;
}

template <class Key, class Value, class Compare>
int QUEUE::getLeft(const size_t index) {
	return 2 * (index + 1) - 1;
}

template <class Key, class Value, class Compare>
int QUEUE::getRight(const size_t index) {
	return 2 * (index + 1);
}

template <class Key, class Value, class Compare>
QUEUE::ConcurrentPriorityQueue(const std::vector<std::pair<Key, Value>>& vector) {
	for (size_t i = 0; i < vector.size(); ++i) {
		heap.emplace_back(Node<Key, Value>(key, std::make_shared<Value>(value)));
	}
	for (size_t i = (heap.size() + 1) / 2 - 1; i >= 0; --i) {
		heapifyWithoutLock(i);
	}
}

template <class Key, class Value, class Compare>
void QUEUE::heapifyWithoutLock(const size_t index) {
	const NodePtr currentNodePtr = heap[index];
	NodePtr leftNodePtr;
	NodePtr rightNodePtr;

	if (getLeft(index) < heap.size()) {
		leftNodePtr = heap[getLeft(index)];
	}
	else {
		return;
	}
	if (getRight(index) < heap.size()) {
		rightNodePtr = heap[getRight(index)];
	}
	else {
		rightNodePtr = nullptr;
	}

	int minIndex;
	NodePtr minNodePtr = currentNodePtr;

	if (compare(leftNodePtr->getKey(), currentNodePtr->getKey())) {
		minIndex = getLeft(index);
		minNodePtr = leftNodePtr;
	}

	if ((rightNodePtr != nullptr) && (compare(rightNodePtr->getKey(), minNodePtr->getKey()))) {
		minIndex = getRight(index);
		minNodePtr = rightNodePtr;
	}


	if (index != minIndex) {
		heap[index] = minNodePtr;
		heap[minIndex] = currentNodePtr;

		heapifyWithoutLock(minIndex);
	}
	else {
		return;
	}
}

template <class Key, class Value, class Compare>
typename QUEUE::NodePtr QUEUE::lockNodeByIndexSizeNotLocked(const size_t index) { 
	while (true) {
		bool expected = false;
		boost::lock_guard<ReadWriteLock> guard(changeSizeMutex);
		if (index >= heap.size()) {
			return nullptr;
		}
		if (isNodeLockedVector[index]->compare_exchange_strong(expected, true)) {
			break;
		}
	}
	return heap[index];
}

template <class Key, class Value, class Compare>
typename QUEUE::NodePtr QUEUE::lockNodeByIndexSizeLocked(const size_t index) {
	if (index >= heap.size()) {
		return nullptr;
	}
	while (true) {
		bool expected = false;
		if (isNodeLockedVector[index]->compare_exchange_strong(expected, true)) {
			break;
		}
	}
	return heap[index];
}

template <class Key, class Value, class Compare>
void QUEUE::unlockChildren(NodePtr leftNodePtr, NodePtr rightNodePtr, const size_t index, const Child swappedChild) {	
	switch (swappedChild) {
	case left:
		isNodeLockedVector[index]->store(false);
		if (rightNodePtr != nullptr) {
			isNodeLockedVector[getRight(index)]->store(false); 
		}
		break;
	case right:
		isNodeLockedVector[index]->store(false);
		isNodeLockedVector[getLeft(index)]->store(false);
		break;
	case none:
		isNodeLockedVector[getLeft(index)]->store(false);
		if (rightNodePtr != nullptr) {
			isNodeLockedVector[getRight(index)]->store(false);
		}
		break;
	default:
		std::cout << "error in unlockChildren()" << std::endl;
	}
}


template <class Key, class Value, class Compare>
size_t QUEUE::heapify(const size_t index) {
	if (index == lastIndex) {
		return index;
	}
	const NodePtr currentNodePtr = heap[index];

	NodePtr leftNodePtr = lockNodeByIndexSizeNotLocked(getLeft(index));
	NodePtr rightNodePtr = lockNodeByIndexSizeNotLocked(getRight(index));
	
	if (leftNodePtr == nullptr) {
		return index;
	}

	int minIndex;
	NodePtr minNodePtr = currentNodePtr;
	Child swappedChild = none;

	if (compare(leftNodePtr->getKey(), currentNodePtr->getKey())) {
		minIndex = getLeft(index);
		minNodePtr = leftNodePtr;
		swappedChild = left;
	}

	if ((rightNodePtr != nullptr) && (compare(rightNodePtr->getKey(), minNodePtr->getKey()))) {
		minIndex = getRight(index);
		minNodePtr = rightNodePtr;
		swappedChild = right;
	}

	
	if (swappedChild != none) {
		heap[index] = minNodePtr;
		heap[minIndex] = currentNodePtr;
		unlockChildren(leftNodePtr, rightNodePtr, index, swappedChild);

		return heapify(minIndex);
	}
	else {
		unlockChildren(leftNodePtr, rightNodePtr, index, none);
		return index;
	}
}

template <class Key, class Value, class Compare>
void QUEUE::pushUp(const size_t index) {
	if (index == 0) {
		return;
	}

	NodePtr currentNodePtr = heap[index];
	NodePtr parentNodePtr = heap[getParent(index)];

	if (compare(currentNodePtr->getKey(), parentNodePtr->getKey())) {
		heap[index] = parentNodePtr;
		heap[getParent(index)] = currentNodePtr;

		pushUp(getParent(index));
	}
}

template <class Key, class Value, class Compare>
void QUEUE::insert(const Key& key, const ValuePtr& value) {
	conditionalMutex_.lock();
	extractInsertMutex.lock();
	changeSizeMutex.lock();
	

	NodePtr currentNodePtr = std::make_shared<Node<Key, Value>>(Node<Key, Value>(key, value));	

	heap.push_back(currentNodePtr);
	isNodeLockedVector.push_back(std::make_shared<std::atomic<bool>>(false));
	++lastIndex;

	int index = heap.size() - 1;
	pushUp(index);

	
	changeSizeMutex.unlock();	
	extractInsertMutex.unlock();
	conditionalMutex_.unlock();

	condition_.notify_one();
}


template <class Key, class Value, class Compare>
typename QUEUE::ValuePtr QUEUE::extractNext() {
	NodePtr lastNodePtr;
	NodePtr rootNodePtr;
	{
		std::unique_lock<std::mutex> uniqueConditionLock(conditionalMutex_);
		condition_.wait(uniqueConditionLock, [this]()->bool {
			boost::shared_lock<boost::shared_mutex> changeSizeGuard(changeSizeMutex);
			bool isNotEmpty = (heap.size() > 0);
			return isNotEmpty || closingFlag_;
		});
		
		extractInsertMutex.lock_shared();
		rootNodePtr = lockNodeByIndexSizeNotLocked(0);
		boost::lock_guard<ReadWriteLock> guard(changeSizeMutex);

		if (rootNodePtr == nullptr) {
			extractInsertMutex.unlock_shared();
			return nullptr;
		}

		if (heap.size() == 1) {
			
			heap.pop_back();
			isNodeLockedVector.pop_back();
			--lastIndex;

			extractInsertMutex.unlock_shared();

			return rootNodePtr->getValue();
		}

		lastNodePtr = lockNodeByIndexSizeLocked(heap.size() - 1);		

		heap[0] = lastNodePtr;

		heap.pop_back();
		isNodeLockedVector.pop_back();
		--lastIndex;	
	}

	size_t finalIndex = heapify(0);

	isNodeLockedVector[finalIndex]->store(false);
		
	extractInsertMutex.unlock_shared();
	return rootNodePtr->getValue();
}

template <class Key, class Value, class Compare>
void QUEUE::setClosingFlag() {
	boost::lock_guard<std::mutex> guard(conditionalMutex_);
	closingFlag_ = true;
	condition_.notify_all();
}