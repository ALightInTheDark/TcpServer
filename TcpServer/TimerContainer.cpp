// trivial_test
// Created by kiki on 2021/12/4.19:44
#include "TimerContainer.h"
#include "file_operation.h"
#include "Reactor.h"

void TimerContainer::AddTimer(function<void()> cb, TimeStamp expire_time, double interval, Timer*& timer)
{
	if (!cb) { err("TimerContainer::AddTimer: add null function") return; }

	unique_ptr<Timer> timer_ptr =  make_unique<Timer>(move(cb), expire_time, interval);
	timer = timer_ptr.get();

	bool earliest_changed = insert(move(timer_ptr));
	if (earliest_changed) { reset_timerfd(timer_fd.fd, long(timer->Expiration())); }
}
void TimerContainer::CancelTimer(Timer* timer)
{
	auto pair = timer_map.equal_range(timer->Expiration());
	if (pair.first == end(timer_map) && calling_expired_timers) // 某一时刻a，b两个定时器都超时，都被移到了expired vector中，先执行a的回调函数，紧接着就去执行b的回调函数，而b的回调函数中就是移除a定时器。
	{
		canceling_timer_set.emplace(timer);
	}
	else if (pair.first == end(timer_map)) { err("取消一个不存在的定时器！") }

	for (auto it = pair.first; it != pair.second; ++it) // 同一时间戳上设置的定时器数目不会太多，这么做不会影响性能。
	{
		if (it->second.get() == timer) { timer_map.erase(it); break; } // log("从map中移除重复计时的定时器！！")
	}
}

bool TimerContainer::insert(unique_ptr<Timer> timer) // 应当在IO线程中被调用
{
	bool earliest_changed = false; // 最早到期时间是否改变
	if (timer_map.empty() || timer->Expiration() < cbegin(timer_map)->first) { earliest_changed = true; }

	timer_map.emplace(timer->Expiration(), move(timer));

	return earliest_changed;
}

void TimerContainer:: HandleRead() // 该函数应该在IO线程中被调用
{
	TimeStamp now(TimeStamp::Now()); // info("read_timerfd at ", now.ToFormatString())

	read_timerfd(timer_fd.fd); // 清除该事件，避免一直触发

	vector< pair<TimeStamp, unique_ptr<Timer>> > expired;
	auto end_iter = timer_map.upper_bound(now); // lower_bound : 第一个大于等于 upper_bound : 第一个大于
	copy(make_move_iterator(begin(timer_map)), make_move_iterator(end_iter), back_inserter(expired));
	timer_map.erase(begin(timer_map), end_iter);

	canceling_timer_set.clear();
	calling_expired_timers = true;
	for (const auto& pair : expired) {  pair.second->Run(); } // Timer的回调函数可能被延时执行。例如a，b两个timer都在第2秒超时，a先执行，耗时1秒，b就只能在第3秒的时候执行。
	calling_expired_timers = false;

	for (auto& pr : expired) // 超时的定时器中可能有重复触发的定时器，如果它们不在已取消定时器列表中，需要重置这些定时器
	{
		if (pr.second->Repeat() && canceling_timer_set.find(pr.second.get()) == end(canceling_timer_set))
		{
			pr.second->Restart(now);
			insert(move(pr.second)); // info("重启定时器！！！")
		}
		else { pr.second.reset(); } // 这行代码可以省略，因为expired临时对象销毁时，unique_ptr对象也跟着销毁。（另：unique_ptr.release()返回它管理的指针，并不delete该指针。）
	}

	if (!timer_map.empty())
	{
		TimeStamp next_expire = cbegin(timer_map)->first; // set，map没有front()成员函数; cbegin(timer_map)->second->Expiration();
		if (next_expire.UsSinceEpoch() > 0) { reset_timerfd(timer_fd.fd, long(next_expire)); }
	}
}


TimerContainer::TimerContainer(EpollPoller& poller)
: timer_fd(open_timerfd()), timerfd_register(poller, timer_fd.fd)
{
	timerfd_register.SetReadCallBack([this] { HandleRead(); });
	timerfd_register.InterestReadableEvent();
}



