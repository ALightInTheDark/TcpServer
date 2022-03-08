// trivial_test
// Created by kiki on 2021/11/28.21:40
#pragma once
#include <chrono>
#include <string>
#include <algorithm>
#include <sys/time.h>
using std::swap, std::string;
using namespace std::chrono;

class ChronoTimeStamp
{
private:
	long us_since_epoch;  //微秒
public:
	explicit ChronoTimeStamp(long us = 0) : us_since_epoch(us) { }
	[[nodiscard]] long UsSinceEpoch() const { return us_since_epoch; }

public:
	static ChronoTimeStamp Now()
	{
		return ChronoTimeStamp(duration_cast<microseconds>(system_clock::now().time_since_epoch()).count());
	}

	void swap(ChronoTimeStamp& other) { ::swap(us_since_epoch, other.us_since_epoch); }

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
		if (ret == NULL) { return {}; }

		char buf[64] = {0};
		int usec = static_cast<int>(us_since_epoch % 1000'000);
		snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d", stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday, stm.tm_hour, stm.tm_min, stm.tm_sec, usec);

		return buf;
	}
};

//返回秒数
double TimeDifference(const ChronoTimeStamp& high, const ChronoTimeStamp& low) { return static_cast<double>(high.UsSinceEpoch() - low.UsSinceEpoch()) / 1000'000; }
//增加秒数
ChronoTimeStamp AddSeconds(ChronoTimeStamp ts, double seconds) { return ChronoTimeStamp(ts.UsSinceEpoch() + static_cast<long>(seconds * 1000'000)); }
bool operator<(const ChronoTimeStamp& lhs, const ChronoTimeStamp& rhs) { return lhs.UsSinceEpoch() < rhs.UsSinceEpoch(); }
bool operator==(const ChronoTimeStamp& lhs, const ChronoTimeStamp& rhs) { return lhs.UsSinceEpoch() == rhs.UsSinceEpoch(); }
