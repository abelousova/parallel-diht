#include <vector>

#include "ThreadPool.h"

template<class T, class Compare = std::less<T>>
class ParallelQuickSorter {
public:
	ParallelQuickSorter(): threadPool(4) {	}

	void parallelSort(std::vector<T>& vector) {
		comparator = Compare();
		if (vector.size() == 0) {
			return;
		}
		threadPool.addTask([&vector, this] {
			quickSort(vector, 0, vector.size() - 1);
		});
	}
private:
	Compare comparator;
	ThreadPool<void> threadPool;

	size_t partition(std::vector<T>& vector, size_t left, size_t right) {
		size_t randomIndex = rand() % (right - left + 1) + left;
		std::swap(vector[randomIndex], vector[right]);
		T pivot = vector[right];
		int i = left - 1;
		for (size_t j = left; j < right; ++j) {
			if (comparator(vector[j], pivot)) {
				++i;
				std::swap(vector[i], vector[j]);
			}
		}
		std::swap(vector[i + 1], vector[right]);
		return i + 1;
	}

	void quickSort(std::vector<T>& vector, size_t left, size_t right) {
		int middle = partition(vector, left, right);
		if (int(left) < middle - 1) {
			threadPool.addTask([=, &vector, this] {
				quickSort(vector, left, middle - 1);
			});
		}
		if (middle + 1 < right) {
			threadPool.addTask([=, &vector, this] {
				quickSort(vector, middle + 1, right);
			});
		}
	}
};