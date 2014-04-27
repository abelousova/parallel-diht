#include <cassert>
#include <algorithm>
#include <numeric>

#include "PrefixScanner.h"

void intTest() {
	const size_t testSize = 2000;

	class Sum {
	public:
		int operator()(int a, int b) {
			return a + b;
		}
	};

	PrefixScanner<int, Sum> scanner(0);
	std::vector<int> input;
	std::vector<int> output;
	for (int i = 0; i < testSize; ++i) {
		input.push_back(i);
	}
	scanner.makeScan(input, output);

	std::vector<int> rightAnswer(input.size());
	std::partial_sum(input.begin(), input.end(), rightAnswer.begin(), Sum());

	for (size_t i = 0; i < rightAnswer.size(); ++i) {
		assert(rightAnswer[i] == output[i]);
	}
}

void vectorTest() {
	const size_t testSize = 2000;
	const size_t vectorSize = 5;

	class Sum {
	public:
		std::vector<int> operator()(std::vector<int>& a, std::vector<int>& b) {
			std::vector<int> result;
			for (int i = 0; i < a.size(); ++i) {
				result.push_back(a[i] + b[i]);
			}
			return result;
		}
	};

	std::vector<int> zero;
	for (int i = 0; i < vectorSize; ++i) {
		zero.push_back(0);
	}

	PrefixScanner<std::vector<int>, Sum> scanner(zero);
	std::vector<std::vector<int>> input;
	std::vector<std::vector<int>> output;
	for (int i = 0; i < testSize; ++i) {
		std::vector<int> vec;
		for (int i = 0; i < vectorSize; ++i) {
			vec.push_back(i);
		}
		input.push_back(vec);
	}
	scanner.makeScan(input, output);

	std::vector<std::vector<int>> rightAnswer(input.size());
	std::partial_sum(input.begin(), input.end(), rightAnswer.begin(), Sum());
	for (size_t i = 0; i < rightAnswer.size(); ++i) {
		for (size_t j = 0; j < rightAnswer[i].size(); ++j) {
			assert(output[i][j] == rightAnswer[i][j]);
		}
	}
}

void permutationTest() {
	const size_t permutationSize = 3;

	typedef std::vector<int> Permutation;

	class PermutationMultiply {
	public:
		Permutation operator()(Permutation& a, Permutation& b) {
			Permutation result;
			for (size_t i = 0; i < a.size(); ++i) {
				result.push_back(b[a[i]]);
			}
			return result;
		}
	};

	Permutation identical;
	for (size_t i = 0; i < permutationSize; ++i) {
		identical.push_back(i);
	}

	PrefixScanner<Permutation, PermutationMultiply> scanner(identical);
	std::vector<Permutation> input;
	std::vector<Permutation> output;
	
	Permutation permutation;
	for (size_t i = 0; i < permutationSize; ++i) {
		permutation.push_back(i);
	}
	do {
		input.push_back(permutation);
	} while (std::next_permutation(permutation.begin(), permutation.end()));
	scanner.makeScan(input, output);

	std::vector<Permutation> rightAnswer(input.size());
	std::partial_sum(input.begin(), input.end(), rightAnswer.begin(), PermutationMultiply());

	for (size_t i = 0; i < rightAnswer.size(); ++i) {
		for (size_t j = 0; j < rightAnswer[i].size(); ++j) {
			assert(output[i][j] == rightAnswer[i][j]);
		}
	}
}

int main() {
	intTest();
	std::cout << "Success in int test" << std::endl;
	vectorTest();
	std::cout << "Success in vector test" << std::endl;
	permutationTest();
	std::cout << "Success in permutation test" << std::endl;
}