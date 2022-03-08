// trivial
// Created by kiki on 2021/11/13.12:31
#pragma once
#include <noncopyable.hpp>
#include <FdGuard.hpp>
#include <Timer.hpp>
#include <console_log.hpp>
#include "EventRegister.h"
class Reactor;

class TimerContainer : noncopyable // TimerContainer可以做成抽象类，用红黑树、最小堆、时间轮、排序链表、链表等数据结构实现。
{
private:
	multimap<TimeStamp, unique_ptr<Timer>> timer_map; // 在一个时间戳上可能有多个超时的定时器，因此不能用map。
	set<Timer*> canceling_timer_set;

private:
	FdGuard timer_fd;
	EventRegister timerfd_register;
	void HandleRead(); // timerfd_register注册的响应函数。该函数应该在IO线程中被调用
private:
	bool calling_expired_timers {false};

public:
	void AddTimer(function<void()> cb, TimeStamp when, double interval, Timer*& timer); // 不必加锁，只会被拥有TimerContainer的IO线程调用
	void CancelTimer(Timer* timer); // 不必加锁，只会被拥有TimerContainer的IO线程调用

	bool insert(unique_ptr<Timer> timer); // 应当在IO线程中被调用

public:
	explicit TimerContainer(EpollPoller& poller);
	~TimerContainer() = default;
};