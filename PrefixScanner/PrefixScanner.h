#include <vector>
#include <atomic>

#include <ThreadPool.h>
#include <ThreadSafeLogger.h>

template<class ValueType, class BinaryOperation>
class PrefixScanner {
private:
	ValueType neutralElement_;
	BinaryOperation binaryOperation_;

	class Scanner {
	private:
		std::vector<ValueType> inputVector_;
		std::vector<ValueType>& resultVector_;
		std::vector<std::shared_ptr<std::atomic<int>>> level_;
		ThreadPool<void> threadPool_;

		boost::mutex levelMutex_;
		size_t levelsNumber_;
		size_t extendedSize_;
		ValueType neutralElement_;

		void countLevelsNumber() {
			levelsNumber_ = 0;
			extendedSize_ = 1;
			while (extendedSize_ < inputVector_.size()) {
				extendedSize_ *= 2;
				++levelsNumber_;
			}
		}

		void busyWait(std::function<bool()> condition) {
			while (!condition()) {}
		}

		void upSweep() {
			size_t boxSize = 2;
			ValueType neutral = neutralElement_;
			for (size_t i = 0; i < levelsNumber_; ++i) {
				size_t boxNumber = extendedSize_ / boxSize;
				for (size_t j = 0; j < boxNumber; ++j) {
					threadPool_.addTask([boxSize, i, j, neutral, this] {
						const size_t right = (j + 1) * boxSize - 1;
						const size_t left = right - boxSize / 2;

						busyWait([this, i, left, right] {
							boost::lock_guard<boost::mutex> levelGuard(levelMutex_);
							return (*level_[left] == i) && (*level_[right] == i);
						});

						resultVector_[right] = BinaryOperation()(resultVector_[left], resultVector_[right]);
						level_[right]->store(i + 1);
					});
				}
				boxSize *= 2;
			}
		}

		void middleStep() {
			ValueType neutral = neutralElement_;
			threadPool_.addTask([this, neutral] {

				busyWait([this] {
					boost::lock_guard<boost::mutex> levelGuard(levelMutex_);
					return (*level_[extendedSize_ - 1] == levelsNumber_);
				});

				resultVector_.push_back(resultVector_.back());
				resultVector_[resultVector_.size() - 2] = neutral;

				level_[extendedSize_ - 1]->store(levelsNumber_ + 1);

				
			});
		}

		void downSweep() {
			size_t boxSize = extendedSize_;
			for (size_t i = 0; i < levelsNumber_; ++i) {
				size_t boxNumber = extendedSize_ / boxSize;
				for (size_t j = 0; j < boxNumber; ++j) {
					threadPool_.addTask([boxSize, i, j, this] {
						const size_t right = (j + 1) * boxSize - 1;
						const size_t left = right - boxSize / 2;

						busyWait([this, i, left, right] {
							boost::lock_guard<boost::mutex> levelGuatd(levelMutex_);
							return (*level_[right] == i + levelsNumber_ + 1);
						});
						
						ValueType temp = resultVector_[right];
						resultVector_[right] = BinaryOperation()(resultVector_[right], resultVector_[left]);
						resultVector_[left] = temp;

						level_[right]->store(i + levelsNumber_ + 2);
						level_[left]->store(i + levelsNumber_ + 2);
					});
				}
				boxSize /= 2;
			}
		}


	public:
		Scanner(std::vector<ValueType>& inputVector, std::vector<ValueType>& resultVector, ValueType neutralElement) :
			inputVector_(inputVector),
			resultVector_(resultVector),
			level_(0),
			extendedSize_(0),
			levelsNumber_(0),			
			neutralElement_(neutralElement),
			levelMutex_(),
			threadPool_(4)
		{ 

		}

		void countResultVector() {	
			countLevelsNumber();

			for (size_t i = 0; i < extendedSize_; ++i) {
				level_.emplace_back(new std::atomic<int>(0));
			}

			resultVector_ = inputVector_;
			resultVector_.resize(extendedSize_, neutralElement_);

			upSweep();

			middleStep();

			downSweep();
		}
	};

public:
	PrefixScanner(ValueType neutralElement) :
		neutralElement_(neutralElement)
	{ }

	void makeScan(std::vector<ValueType>& inputVector, std::vector<ValueType>& resultVector) {
		{
			Scanner scanner(inputVector, resultVector, neutralElement_);
			scanner.countResultVector();
		}		
		resultVector.erase(resultVector.begin());
		resultVector.erase(resultVector.begin() + inputVector.size(), resultVector.end());
	}
};