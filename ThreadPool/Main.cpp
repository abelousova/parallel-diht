#include "ThreadPool.h"

#include <cstdlib>

int main() {	
	/*
	ThreadPool<int, int> threadPool(10);
	std::vector<std::future<int>> futureVector;
	for (size_t i = 0; i < 50000; ++i) {
		futureVector.push_back(threadPool.addTask([i]() -> int {
			return i;
		}, i));
	}
	
	std::cout << futureVector[1].get() << std::endl;
	*/

	const size_t arraySize = 100;

	ThreadPool<int, int> threadPool(4);
	std::future<int> result[arraySize][arraySize];
	int a[arraySize][arraySize];
	int b[arraySize][arraySize];

	for (size_t i = 0; i < arraySize; ++i) {
		for (size_t j = 0; j < arraySize; j++) {
			a[i][j] = rand() % 10;
			b[i][j] = rand() % 10;
		}
	}

	size_t k = 0;
	for (size_t i = 0; i < arraySize; ++i) {
		for (size_t j = 0; j < arraySize; j++) {
			result[i][j] = threadPool.addTask([=, &a, &b]()->int {
				int res = 0;
				for (size_t index = 0; index < arraySize; ++index) {
					res += a[i][index] * b[index][j];
				}
				return res;
			}, k);
			k++;
		}
	}

	/*
	for (size_t i = 0; i < arraySize; ++i) {
		for (size_t j = 0; j < arraySize; j++) {
			std::cout << result[i][j].get() << "  ";
		}
		std::cout << std::endl;
	}
	*/
}