// trivial_test
// Created by kiki on 2021/11/28.22:58
#include "EventRegister.h"
#include "EpollPoller.h"

void EventRegister::Update() { poller.Register(this); }
void EventRegister::Remove()
{
	if (!MonitoringNothing()) { err("将一个关注着事件的EventRegister移出了Poller！！！") }
	poller.Unregister(this);
}

void EventRegister::HandleEvent()
{
	shared_ptr<void> guard;
	if (held) { guard = hold_ptr.lock(); if (!guard) { return; } }

	if (triggered_events & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) { if (read_callback) { read_callback(); } } // EPOLLRDHUP : 对等方关闭连接或半连接。对该事件的处理应放在POLLIN前。否则POLLIN会处理该事件。
	else if (triggered_events & EPOLLOUT) { if (write_callback) { write_callback(); } }
	else if ( (triggered_events & EPOLLHUP) && !(triggered_events & EPOLLIN) ) { if (connection_callback) { connection_callback(); } }
	else if (triggered_events & (EPOLLERR)) { if (error_callback) { error_callback(); } }
}
