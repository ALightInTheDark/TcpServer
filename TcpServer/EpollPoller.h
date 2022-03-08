// trivial
// Created by kiki on 2021/11/13.17:58
#pragma once
#include <FdGuard.hpp>
#include <noncopyable.hpp>
#include <cpp_header/cpp_container.h>
#include "EventRegister.h"

class EpollPoller : noncopyable
{
private:
	FdGuard epoll_fd;
	vector<struct epoll_event> events_vector;
	vector<EventRegister*> active_registers;

public:
	void poll(int ms_timeout = 10'000); // 超时时间默认为10秒
	void HandleActiveEvents();

private:
	void epoll_control(int operation, EventRegister* er) const;
	static const int first_register = -1; // EventRegister为首次注册
	static const int registered = 1; // EventRegister已经注册
	static const int unregistered = 2; // EventRegister尚未注册
public:
	void Register(EventRegister* er) const; // 被EventRegister调用
	void Unregister(EventRegister* er) const; // 被EventRegister调用

private:
	FdGuard wakeup_fd;
	EventRegister wakeup_register;
public:
	void WakeUp() const
	{
		unsigned long long one = 1;
		ssize_t ret = write(wakeup_fd.fd, &one, sizeof one);
		if (ret != sizeof one) { perror("EpollPoller::WakeUp::write"); }
	}

public:
	explicit EpollPoller();
	~EpollPoller() = default;
};


