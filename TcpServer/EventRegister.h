// trivial
// Created by kiki on 2021/11/13.16:52
#pragma once
#include <sys/epoll.h>
#include <cpp_header/cpp_smartptr.h>
#include <noncopyable.hpp>
#include <console_log.hpp>

class EpollPoller;

class EventRegister : noncopyable
{
private:
	EpollPoller& poller;

private:
	const int fd; // fd
	int events {0}; // fd上关注的事件
	int triggered_events {0}; // fd上到来的事件
	int state {-1}; // 是否已经注册到EpollPoller
public:
	[[nodiscard]] int Fd() const { return fd; }
	[[nodiscard]] int Events() const { return events; }
	[[nodiscard]] int TriggeredEvents() const { return triggered_events; }
	void SetTriggeredEvents(int t) { triggered_events = t; } // 被PEpollPoller调用
	[[nodiscard]] int State() const { return state; } // 被EpollPoller调用
	void SetState(int i) { state = i; } // 被EpollPoller调用

public:
	void Remove();
	void Update();
public:
	static const int none_event = 0;
	static const int read_event = EPOLLIN | EPOLLPRI;
	static const int write_event = EPOLLOUT;
	void InterestReadableEvent() { events |= read_event; Update(); } // 关注fd的可读事件
	void InterestWritableEvent() { events |= write_event; Update(); } // 关注fd的可写事件
	void UnInterestReadableEvent() { events &= ~read_event; Update(); } // 取消关注fd的可读事件
	void UnInterestWritableEvent() { events &= ~write_event; Update(); } // 取消关注fd的可读事件
	void UnInterestAllEvents() { if (events == none_event){ return; } events = none_event; Update(); } // 取消关注fd的所有事件
	[[nodiscard]] bool MonitoringReadable() const { return events & read_event; }
	[[nodiscard]] bool MonitoringWritable() const { return events & write_event; }
	[[nodiscard]] bool MonitoringNothing() const { return events == none_event; }

private:
	function<void()> connection_callback;
	function<void()> read_callback;
	function<void()> write_callback;
	function<void()> error_callback;
public:
	void SetCloseCallBack(function<void()> cb) { connection_callback = move(cb); }
	void SetReadCallBack(function<void()> cb) { read_callback = move(cb); }
	void SetWriteCallBack(function<void()> cb) { write_callback = move(cb); }
	void SetErrorCallBack(function<void()> cb) { error_callback = move(cb); }

private:
	weak_ptr<void> hold_ptr; // 与TcpConnection对象的生存期有关
	bool held {false};
public:
	void hold(const shared_ptr<void>& obj) { hold_ptr = obj; held = true; }

public:
	void HandleEvent();

public:
	EventRegister(EpollPoller& epoller, int f) : poller(epoller), fd(f) { } // log("创建了一个EventRegister对象")
	~EventRegister() { UnInterestAllEvents(); Remove(); }
};




