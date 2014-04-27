#include <iostream>
#include <chrono>

#include "HierarchicalMutex.h"

void diningPhilosophersGoodEmulation() {
    const int numberOfPhilosophers = 5;
	
	boost::mutex coutMutex;

	std::vector<boost::thread> philosophers;
	std::vector<std::shared_ptr<HierarchicalMutex>> forks;

	for (size_t i = 0; i < numberOfPhilosophers; ++i) {
		forks.push_back(std::make_shared<HierarchicalMutex>(i + 1));
	}

	for (size_t i = 0; i < numberOfPhilosophers; ++i) {
		philosophers.emplace_back([i, &forks, numberOfPhilosophers, &coutMutex] {
			while (true) {
				size_t secondFork = (i == numberOfPhilosophers - 1) ? 0 : i + 1;
            
				if (i > secondFork) {	
					forks[secondFork]->lock();
                    forks[i]->lock();
				}
				else {
					forks[i]->lock();
					forks[secondFork]->lock();
				}

				{
					boost::lock_guard<boost::mutex> coutGuard(coutMutex);
					std::cout << "I am eating! Sincerely yours, thread " << boost::this_thread::get_id() << std::endl;
				}
				boost::this_thread::sleep(boost::posix_time::seconds(2));
	
				forks[secondFork]->unlock();
                forks[i]->unlock();	

				boost::this_thread::sleep(boost::posix_time::seconds(3));
            }            
		});
	}

	std::for_each(philosophers.begin(), philosophers.end(), std::mem_fn(&boost::thread::join));
}

void diningPhilosophersBadEmulation() {
	const int numberOfPhilosophers = 5;
	
	boost::mutex coutMutex;

	std::vector<boost::thread> philosophers;
	std::vector<std::shared_ptr<HierarchicalMutex>> forks;

	for (size_t i = 0; i < numberOfPhilosophers; ++i) {
		forks.push_back(std::make_shared<HierarchicalMutex>(i + 1));
	}

	for (size_t i = 0; i < numberOfPhilosophers; ++i) {
		philosophers.emplace_back([=, &forks, &coutMutex] {
			while (true) {
				size_t secondFork = (i == numberOfPhilosophers - 1) ? 0 : i + 1;

				forks[i]->lock();
				forks[secondFork]->lock();

				{
					boost::lock_guard<boost::mutex> coutGuard(coutMutex);
					std::cout << "I am eating! Sincerely yours, thread " << boost::this_thread::get_id() << std::endl;
				}
				boost::this_thread::sleep(boost::posix_time::seconds(2));

				forks[i]->unlock();
				forks[secondFork]->unlock();
				
				boost::this_thread::sleep(boost::posix_time::seconds(3));
			}
		});
	}

	std::for_each(philosophers.begin(), philosophers.end(), std::mem_fn(&boost::thread::join));
}

int main(int argc,char* argv[]) {
	if (argc < 2 || std::string("bad") != argv[1]) {
		diningPhilosophersGoodEmulation();
	} else {
		diningPhilosophersBadEmulation();
	}		
    return 0;
}

