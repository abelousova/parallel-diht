#include "ThreadPool.h"

#include <cassert>
#include <vector>

typedef std::vector<std::vector<int>> intMatrix;
typedef std::vector<std::vector<std::future<int>>> futureIntMatrix;

intMatrix oneThreadMultiply(intMatrix& a, intMatrix& b) {
	intMatrix result;

	result.resize(a.size());
	for (size_t i = 0; i < a.size(); i++) {
		result[i].resize(a.size());
	}

	for (size_t i = 0; i < a.size(); ++i) {
		for (size_t j = 0; j < a.size(); j++) {
			result[i][j] = 0;
			for (size_t index = 0; index < a.size(); ++index) {
				result[i][j] += a[i][index] * b[index][j];
			}				
		}
	}
	return result;
}

intMatrix generateMatrix(size_t arraySize) {
	intMatrix result;

	result.resize(arraySize);
	for (size_t i = 0; i < arraySize; i++) {
		result[i].resize(arraySize);
	}

	for (size_t i = 0; i < arraySize; ++i) {
		for (size_t j = 0; j < arraySize; j++) {
			result[i][j] = rand() % 10;
		}
	}
	return result;
}

futureIntMatrix multyTreadMultiply(intMatrix& a, intMatrix& b) {
	ThreadPool<int, int> threadPool(4);
	futureIntMatrix result;

	result.resize(a.size());
	for (size_t i = 0; i < a.size(); i++) {
		result[i].resize(a.size());
	}

	size_t k = 0;
	for (size_t i = 0; i < a.size(); ++i) {
		for (size_t j = 0; j < a.size(); j++) {
			result[i][j] = threadPool.addTask([=, &a, &b]()->int {
				int res = 0;
				for (size_t index = 0; index < a.size(); ++index) {
					res += a[i][index] * b[index][j];
				}
				return res;
			}, k);
			k++;
		}
	}
	return result;
}

void compare(intMatrix& a, futureIntMatrix& b) {
	for (size_t i = 0; i < a.size(); ++i) {
		for (size_t j = 0; j < a.size(); j++) {
			assert(b[i][j].get() == a[i][j]);
		}
	}
}

void matrixTest() {
	intMatrix a = generateMatrix(50);
	intMatrix b = generateMatrix(50);

	intMatrix oneThreadResult = oneThreadMultiply(a, b);
	futureIntMatrix multyTreadResult = multyTreadMultiply(a, b);

	compare(oneThreadResult, multyTreadResult);
}

int main() {	
	matrixTest();
}