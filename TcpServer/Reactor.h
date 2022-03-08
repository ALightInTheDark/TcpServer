// trivial
// Created by kiki on 2021/11/13.16:36
#pragma once
#include <noncopyable.hpp>
#include <FdGuard.hpp>
#include <console_log.hpp>
#include "EpollPoller.h"
#include "TimerContainer.h"

static thread_local Reactor* tes;

class Reactor : noncopyable
{
private:
	EpollPoller poller;
public:
	EpollPoller& GetPoller() { return poller; }

private:
	const thread::id tid {std::this_thread::get_id()};
public:
	[[nodiscard]] thread::id GetTid() const { return tid; }
	[[nodiscard]] bool InIOThread() const { return tid == std::this_thread::get_id(); }

private:
	bool running {true};
public:
	void React(); //只能在创建Reactor对象的线程中调用，不能跨线程调用

	void quit(); // todo: 使用shared_ptr<Reactor> 调用quit(),以确保线程安全。

private:
	mutable mutex mtx;
	vector<function<void()>> tasks;
	bool handling_tasks {false};
	void HandleTask();

public:
	void AddTask(function<void()> cb); // 加锁后向tasks中添加任务
	void Execute(function<void()> cb); // 被HandleEvent()或HandleTask()的中执行的任务函数调用。如果在io线程中,直接执行回调函数;否则执行AddTask()。

private:
	TimerContainer timer_container;
public:
	int i = 0;
	void CallAt(const TimeStamp time, const function<void()>& cb, Timer*& timer) {  Execute([this, time, cb, &timer](){timer_container.AddTimer(cb, time, 0.0, timer);}); } // 不能对这里的cb使用move, 否则会使得cb为空。
	void CallAfter(const double delay, const function<void()>& cb, Timer*& timer) { Execute([this, delay, cb, &timer](){timer_container.AddTimer(cb, AddSeconds(TimeStamp::Now(), delay), 0.0, timer);}); } // 经过delay秒后调用
	void CallEvery(const double interval, const function<void()>& cb, Timer*& timer) {  Execute([this, interval, cb, &timer](){timer_container.AddTimer(cb,AddSeconds(TimeStamp::Now(), interval), interval, timer);}); } // 每隔interval秒调用(第一次调用是在当前时间经过interval秒后)
	void CancelCall(Timer* p) { AddTask([this, p](){timer_container.CancelTimer(p);}); }
public:
	Reactor() : timer_container(poller)
	{
		if (tes != nullptr) { err("tid为 ", tid, " 的线程中创建了多个EventScheduler对象！") }
		tes = this; log("tid为 ", tid, " 的线程创建了EventScheduler对象，该线程成为IO线程。")
	}
	~Reactor()
	{
		tes = nullptr; //log("tid为 ", tid, " 的线程中的EventScheduler对象销毁，该IO线程将要退出。")
	}
};