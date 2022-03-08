// trivial
// Created by kiki on 2022/3/3.21:31
#include "IOThreadPool.h"

void IOThreadPool::Start()
{
	if (thread_count == 0) { return; } // 这个判断很重要，否则线程会阻塞在cv.wait(ulock);

	for (size_t i = 0; i < thread_count; ++i)
	{
		thread_vector.emplace_back // thread_vector[i]线程所拥有的线程对象不一定是sub_reactor_vector[i]
		(
			[this]()
			{
				Reactor reactor; // 这里可以创建栈上的对象。

				{
					lock_guard<mutex> gurad(mtx); // 此处必须加锁，否则会出现double free or corruption (out) Aborted。 详见test中的测试。

					this->sub_reactor_vector.push_back(&reactor);
					if (this->sub_reactor_vector.size() == this->thread_count) { this->cv.notify_one(); }
				}

				reactor.React();
			}
		);
	}

	unique_lock<mutex> ulock(mtx);
	cv.wait(ulock, [this](){return sub_reactor_vector.size() == thread_count;}); // 等待所有IO线程都创建完毕。
}

IOThreadPool::IOThreadPool(Reactor& reactor, size_t count) : main_reactor(reactor), thread_count(count) // trace("IO线程池将要调用Start()！")
{
	thread_vector.reserve(count);
	sub_reactor_vector.reserve(count);
	Start();
	print();
}

IOThreadPool::~IOThreadPool() // log("开始执行IOThreadPool的析构函数！") log("IOThreadPool的析构函数执行完毕！")
{
	for (int i = 0; i < thread_count; ++i)
	{
		sub_reactor_vector[i]->quit();
	}
	for (int i = 0; i < thread_count; ++i)
	{
		thread_vector[i].join();
	}
}

Reactor& IOThreadPool::GetReactor()
{
	if (sub_reactor_vector.empty()) { return main_reactor; }

	Reactor* es = sub_reactor_vector[index];

	++index;
	if (index == sub_reactor_vector.size()) { index = 0; }

	return *es;
}

void IOThreadPool::print()
{
	if (sub_reactor_vector.size() != thread_count) { fatal("print") }
	if (thread_vector.size() != thread_count) { fatal("print") }
	for (int i = 0; i < thread_count; ++i)
	{
		log(thread_vector[i].get_id(), "-----", sub_reactor_vector[i]->GetTid())
	}
	trace("创建IO线程池完毕！")
}
// 错误写法：还是在主线程中创建的sub_reactors对象。sub_reactors对象必须在IO线程中创建。
/*
class IOThreadPool : noncopyable
{
private:
	size_t thread_count;
	int index {-1};
private:
	Reactor main_reactor;
	thread main_reactor_thread;

	vector<thread> sub_reactor_threads;
	vector<Reactor> sub_reactors;

private:
	explicit IOThreadPool(size_t thread_num = 16)
	: main_reactor_thread([ObjectPtr = &main_reactor] { ObjectPtr->React(); }),
	  thread_count(thread_num), sub_reactors(thread_num)
	{
		sub_reactor_threads.reserve(thread_num);
		for (int i = 0; i < thread_num; ++i)
		{
			sub_reactor_threads.emplace_back([capture0 = &sub_reactors[i]] { capture0->React(); });
		}
	}
public:
	~IOThreadPool()= default;

private:

public:
	static Reactor& GetMainReactor()
	{
		static IOThreadPool io_thread_pool{16};
		return io_thread_pool.main_reactor;
	}
	static Reactor& GetSubReactor()
	{
		static IOThreadPool io_thread_pool{16};
		return io_thread_pool.sub_reactors[++io_thread_pool.index % io_thread_pool.thread_count];
	}
};
*/

// 析构函数的两种错误写法：
/*错误写法：
	* 	for (auto& trd : thread_vector)
		{
			trd.join();
		}
		for (auto& es : sub_reactor_vector)
		{
			es->quit();
		}
		第二个for循环永远不会执行
	*/
/*错误写法
for (int i = 0; i < thread_count; ++i)
{
	sub_reactor_vector[i]->quit();
	thread_vector[i].join();
}
sub_reactor_vector[i]不一定是thread_vector[i]线程所拥有的线程对象。
*/