#include <fstream>

#include "ThreadSafeLogger.h"

void oneThreadTest() {
	const int messageNumber = 2000;
	std::ofstream out("oneThreadLog.txt");
	ThreadSafeLogger logger(out);
	try {
		for (size_t i = 0; i < messageNumber; ++i) {
			std::string message = "message " + std::to_string(i) + "\n";
			logger << message;
		}
	}
	catch (std::logic_error& e) {
		std::cout << e.what();
	}
}

void multyThreadTest() {
	const size_t messageNumber = 10000;
	const size_t threadNumber = 10;
	std::ofstream out("multiplyThreadLog.txt");

	std::vector<boost::thread> threadVector;
	ThreadSafeLogger logger(out);

	for (size_t i = 0; i < threadNumber; ++i) {
		threadVector.emplace_back([messageNumber, i, &logger] {
			for (size_t j = 0; j < messageNumber; ++j) {
				std::string message = "message " + std::to_string(j) + " from thread " + std::to_string(i) + "\n";
				logger << message;
			}
		});
	}
	std::for_each(threadVector.begin(), threadVector.end(), std::mem_fn(&boost::thread::join));
}

int main() {
	oneThreadTest();
	std::cout << "One thread test completed" << std::endl;
	multyThreadTest();
	std::cout << "Several threads test completed" << std::endl;
	return 0;
}