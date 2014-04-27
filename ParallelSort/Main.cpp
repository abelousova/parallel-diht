#include <algorithm>

#include "ParallelSort.h"

void intTest() {
	std::vector<int> testArray;
	for (size_t i = 0; i < 1000; i++) {
		testArray.push_back(rand() % 1000);
	}

	std::vector<int> sortedArray = testArray;
	std::sort(sortedArray.begin(), sortedArray.end());
	{
		ParallelQuickSorter<int> sorter;
		sorter.parallelSort(testArray);
	}

	for (size_t i = 0; i < testArray.size(); ++i) {
		assert(sortedArray[i] == testArray[i]);
	}
}

int main() {
	intTest();
	return 0;
}