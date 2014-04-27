#include "HierarchicalMutex.h"
#include <stdexcept>

thread_local std::set<int> HierarchicalMutex::lastPriority_;
std::set<int> HierarchicalMutex::usedPriorities_;

HierarchicalMutex::HierarchicalMutex(size_t priority) {
    if (priority == 0) {
		throw std::logic_error("Error: priority must be larger than 0");
	}
	if (usedPriorities_.find(priority) != usedPriorities_.end()) {
		throw std::logic_error("Error: mutex with prority " + std::to_string(priority) + " already exists");
	}
	priority_ = priority;
	usedPriorities_.insert(priority);
}

void HierarchicalMutex::checkPriority() {
	if (lastPriority_.empty()) {
        return;
	}
    if (priority_ < *lastPriority_.rbegin()) {
		throw std::logic_error("Error: try to lock mutex with priority: " + std::to_string(priority_) +
			" but the last locked mutex priority is " + std::to_string(*lastPriority_.rbegin()));
	}
}

void HierarchicalMutex::lock() {
	checkPriority();
	internalMutex_.lock();
	lastPriority_.insert(priority_);
}

bool HierarchicalMutex::try_lock() {
	checkPriority();
	return internalMutex_.try_lock();
}

void HierarchicalMutex::unlock() {
	internalMutex_.unlock();
	lastPriority_.erase(priority_);
}
