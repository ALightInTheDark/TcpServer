// FoundationClasses
// Created by kiki on 2021/11/6.21:17
#pragma once
#include "noncopyable.hpp"
#include "./cpp_header/cpp_container.h"
#include "./cpp_header/cpp_functional.h"
#include "./cpp_header/cpp_thread.h"
using std::runtime_error;

class ThreadPool : noncopyable
{
private:
	mutex mtx;
	condition_variable cv;

private:
	vector<thread> threads;
	deque<function<void()>> tasks_queue;
public:
	[[nodiscard]] size_t GetThreadCount() const { return threads.size(); }

private:
	bool running {true};

public:
	explicit ThreadPool(size_t count = thread::hardware_concurrency())
	{
		threads.reserve(count);
		for (size_t i = 0; i < count; ++i)
		{
			threads.emplace_back
			(
				[this, count]()
				{
					while(true)
					{
						function<void()> task;
						{
							unique_lock<mutex> lock(this->mtx); // 保护running变量

							this->cv.wait(lock, [this](){ return !this->running || !this->tasks_queue.empty(); }); // 只要不满足后面的条件，就一直阻塞下去
							if(!this->running && tasks_queue.empty()) { return; } // running为false时，不会wait; 确保所有任务都完成，见测试
							task = move(this->tasks_queue.front());
							this->tasks_queue.pop_front();
						}
						task();
					}
				}
			);
		}

		print();
	}

	auto add_task(function<void(void)> task)
	{
		{
			lock_guard<mutex> lock(mtx);
			if (!running) { throw runtime_error("add task on stopped thread_pool"); }
			tasks_queue.emplace_back(move(task));
		}

		cv.notify_one();
	}

	~ThreadPool()
	{
		{
			lock_guard<mutex> lock(mtx); // 思考：这里有必要加锁吗？
			running = false;
		}

		cv.notify_all();

		for (auto& thread : threads)
		{
			thread.join();
		}
	}

	void print() const
	{
		size_t n = GetThreadCount();
		debug("计算线程的数量是", n, "它们的tid如下")
		for (int i = 0; i < n; ++i)
		{
			debug(threads[i].get_id())
		}
	}
};

/*
当匿名函数返回false时才阻塞线程，阻塞时自动释放锁。
当匿名函数返回true且受到通知时解阻塞，然后加锁。
*/
