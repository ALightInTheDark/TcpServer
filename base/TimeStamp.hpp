// trivial
// Created by kiki on 2021/11/13.11:13
#pragma once
#include <string>
#include <algorithm>
#include <sys/time.h> // typedef long __time_t typedef __time_t time_t
using std::swap, std::string;

class TimeStamp
{
private:
	long us_since_epoch;  //微秒
public:
	explicit TimeStamp(long us = 0) : us_since_epoch(us) { } // 不使用unsigned long long, 以便两个时间戳可以相减得负数
	[[nodiscard]] long UsSinceEpoch() const { return us_since_epoch; }
	explicit operator long() const { return us_since_epoch; } // 类型转换函数

public:
	static TimeStamp Now()
	{
		struct timeval tv{};
		int ret = gettimeofday(&tv, NULL); //int gettimeofday(struct timeval *tv, struct timezone *tz);
		if (ret == -1) { perror("gettimeofday"); return TimeStamp(); }
		return TimeStamp(tv.tv_sec * 1000'000 + tv.tv_usec);
	}

	void swap(TimeStamp& other) { ::swap(us_since_epoch, other.us_since_epoch); }

	[[nodiscard]] string ToSecondsString() const
	{
		char buf[64] = {0};
		snprintf(buf, sizeof(buf), "%ld s %ld us", us_since_epoch / 1000'000, us_since_epoch % 1000'000);
		return buf;
	}
	[[nodiscard]] string ToFormatString() const
	{
		time_t sec = us_since_epoch / 1000'000;

		struct tm stm {};
		struct tm* ret = gmtime_r(&sec, &stm); // _r为线程安全的函数
		if (ret == NULL) { perror("gmtime_r"); return {}; }

		int usec = static_cast<int>(us_since_epoch % 1000'000);
		char buf[64] = {0};
		snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d", stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday, stm.tm_hour, stm.tm_min, stm.tm_sec, usec);

		return buf;
	}
};

//返回秒数
static double TimeDifference(const TimeStamp& high, const TimeStamp& low) { return static_cast<double>(high.UsSinceEpoch() - low.UsSinceEpoch()) / 1000'000; }
//增加秒数
static TimeStamp AddSeconds(TimeStamp ts, double seconds) { return TimeStamp(ts.UsSinceEpoch() + static_cast<long>(seconds * 1000'000)); }
static bool operator<(const TimeStamp& lhs, const TimeStamp& rhs) { return lhs.UsSinceEpoch() < rhs.UsSinceEpoch(); }
static bool operator==(const TimeStamp& lhs, const TimeStamp& rhs) { return lhs.UsSinceEpoch() == rhs.UsSinceEpoch(); }
