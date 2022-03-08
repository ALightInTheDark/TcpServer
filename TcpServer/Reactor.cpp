// trivial
// Created by kiki on 2022/3/1.23:44
#include "Reactor.h"

void Reactor::React()
{
	if (!InIOThread()) { fatal("tid为 ", tid, " 的线程启动了其它IO线程的事件循环！") }
	while (running)
	{
		poller.HandleActiveEvents();
		HandleTask();
	}
}

void Reactor::quit() // 使用shared_ptr<Reactor> 调用quit(),以确保线程安全。
{
	running = false;
	if (!InIOThread()) { poller.WakeUp(); } // log("tid为 ", tid, " 的线程中的EventScheduler对象将要结束事件循环。")
}

void Reactor::HandleTask()
{
	vector<function<void()>> temp;
	handling_tasks = true;
	{
		lock_guard<mutex> lock(mtx);
		temp.swap(tasks);
	}

	for (const auto& cb : temp) { cb(); }
	handling_tasks = false;
}

void Reactor::AddTask(function<void()> cb) // trace("AddTask! InIOThread = ", InIOThread() ? "true" : "false")
{
	{
		lock_guard<mutex> lock(mtx);
		tasks.push_back(move(cb));
	}
	if (!InIOThread() || handling_tasks) { poller.WakeUp(); }
}
void Reactor::Execute(function<void()> cb)
{
	if (InIOThread()) { cb(); }
	else { AddTask(move(cb)); }
}