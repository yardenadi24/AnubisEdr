#pragma once
#include <Windows.h>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>
#include <queue>
#include <functional>
#include <future>
#include <type_traits>

using namespace std;

namespace utils{

using Task = function<void()>;

class ThreadPool {
public:
	ThreadPool(int capacity) :capacity_(capacity), stopped_(false)
	{
		for (int i = 0; i < capacity_; i++)
		{
			
			// Push thread to the vector with the worker job
			// Emplace back construct the object so we avoid
			// extra copy
			workers_.emplace_back([this]() {
				this->Worker();
				});
		}

	}

	~ThreadPool()
	{
		stopped_ = true;
		cv_.notify_all();
		for (auto& t : workers_)
		{
			t.join();
		}
	}


	// typename... Args : Means variadic template, meaning it accepts 0 or more types
	// packed to Args
	template<typename F, typename... Args>
	future<typename invoke_result<F, Args...>::type> PushTask(F&& f, Args&&... args)
	{
		using return_type = typename invoke_result<F, Args...>::type;

		// Create packaged task that encapsulates the callable and its args
		// Allowing invoking async and retrieving the result in the future
		shared_ptr<packaged_task<return_type()>> task(
			new packaged_task<return_type()>
			(
				// Binds f to the args, creating the callable
				// forwards helps us treat the values as
				// their original state lval or rval.
				bind(forward<F>(f), forward<Args>(args)...)
			)
		);

		future<return_type> res = task->get_future();
		unique_lock<mutex> lk(mutex_);
		tasks_.emplace([task]() {(*task)(); });
		lk.unlock();

		cv_.notify_one();
		return res;
	}



private:
	void Worker()
	{
		while (true)
		{
			Task task;
			unique_lock<mutex> lk(mutex_);
			cv_.wait(lk,[this]()
				{
					return	(!this->tasks_.empty()) || this->stopped_;
				});

			// Check if we are here because
			// we are stopping with no tasks pending
			if (this->stopped_ && this->tasks_.empty())
			{
				// In this case just return
				return;
			}

			// otherwise take the task
			task = move(this->tasks_.front());
			this->tasks_.pop();

			// we have the task we can release the lock
			lk.unlock();

			// run task safely
			task();
		}
	}
	queue<Task> tasks_;
	vector<thread> workers_;

	mutex mutex_;
	condition_variable cv_;
	
	int capacity_;
	bool stopped_;
};

}