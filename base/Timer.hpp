// trivial
// Created by kiki on 2021/11/13.11:06
#pragma once
#include "TimeStamp.hpp"
#include "noncopyable.hpp"
#include "./cpp_header/cpp_thread.h"

static atomic<unsigned long long> timer_count = 0; //当前创建的定时器总数。可能有多个线程同时创建Timer对象,因此用原子变量

class Timer : noncopyable
{
private:
	const function<void()> callback; //定时器回调函数
	TimeStamp expiration; //下一次的超时时刻

	const double interval; //超时时间间隔，如果是一次性定时器，该值为0，表示不重复

	const unsigned long long sequence; //定时器序号
public:
	[[nodiscard]] TimeStamp Expiration() const { return expiration; }
	[[nodiscard]] unsigned long long Sequence() const { return sequence; }
	[[nodiscard]] bool Repeat() const { return interval > 0; }

public:
	Timer(function<void()> cb, TimeStamp when, double _interval = 0) : callback(move(cb)), expiration(when), interval(_interval), sequence(++timer_count) { }

	void Run() const { callback(); }

	void Restart(TimeStamp now) //计算下一个超时时刻.
	{
		if (interval > 0) { expiration = AddSeconds(now, interval); }
		else { expiration = TimeStamp(); }
	}
};
