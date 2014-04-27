#include <iostream>
#include <atomic>
#include <memory>
#include <fstream>

#include <boost/thread.hpp>

class ThreadSafeLogger {
private:
	static const size_t BUFF_SIZE_ = 1024;

	typedef std::string** Buffer;
	typedef boost::detail::spinlock Spinlock;

	Spinlock bufferMutex_;

	std::ostream& out_;
	Buffer buffer_;
	std::atomic<size_t> bufferPtr_;

	void flush(Buffer buffer) {
		for (size_t i = 0; i < BUFF_SIZE_; ++i) {
			out_ << (*buffer)[i];
		}
		delete[] buffer[0];
		delete buffer;
	}
public:
	ThreadSafeLogger(std::ostream& out) :
		out_(out),
		buffer_(new std::string*)
	{ 
		(*buffer_) = new std::string[BUFF_SIZE_];
		bufferMutex_.unlock();
	}

	ThreadSafeLogger& operator<< (const std::string& message) {
		while (true)
		{
			int index;
			Buffer localBuffer;

			{
				boost::lock_guard<Spinlock> bufferGuard(bufferMutex_);
				localBuffer = buffer_;
				index = bufferPtr_.fetch_add(1);
			}
			if (index < BUFF_SIZE_) {
				(*buffer_)[index] = message;
				break;
			}
			else {
				boost::unique_lock<Spinlock> bufferGuard(bufferMutex_);
				Buffer newBuffer(new std::string*);
				newBuffer[0] = new std::string[BUFF_SIZE_];
				if (localBuffer == buffer_) {
					bufferPtr_.store(1);
					buffer_ = newBuffer;
					bufferGuard.unlock();
					(*buffer_)[0] = message;
					flush(localBuffer);
				}
				else {
					bufferGuard.unlock();
					delete[] newBuffer[0];
					delete newBuffer;
				}
			}
		}

		return *this;
	}
	
	~ThreadSafeLogger() {
		flush(buffer_);
	}
};
