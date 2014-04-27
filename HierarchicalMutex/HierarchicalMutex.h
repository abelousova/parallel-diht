#pragma once

#include <map>
#include <set>
#include <boost/thread.hpp>

class HierarchicalMutex {
public:
	HierarchicalMutex(size_t priority);
	void lock();
	bool try_lock();
	void unlock();
private:
	boost::mutex internalMutex_;
	int priority_;
	thread_local static std::set<int> lastPriority_;
	static std::set<int> usedPriorities_;

	void checkPriority();
};
