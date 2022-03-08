// trivial
// Created by kiki on 2021/11/13.11:12
#include <iostream>
#include <vector>
#include <TimeStamp.hpp>
#include <ChronoTimeStamp.hpp>
using namespace std;

template <typename T>
void benchmark()
{
	vector<T> stamps;
	stamps.reserve(100'0000); // 一百万个元素

	for (int i = 0; i < 100'0000; ++i)
	{
		stamps.emplace_back(T::Now());
	}

	cout << stamps.front().ToFormatString() << "------" << stamps.front().ToSecondsString() << endl;
	cout << stamps.back().ToFormatString() << "------" << stamps.back().ToSecondsString() << endl;
	cout << "push_back 100'0000 TimeStamp cost seconds : " << TimeDifference(stamps.back(), stamps.front()) << endl;

	int big_gap {};
	long long first = stamps.front().UsSinceEpoch();
	for (int i = 1; i < 100'0000; ++i)
	{
		long long second = stamps[i].UsSinceEpoch();
		long long diff = second - first;
		first = second;
		if (diff < 0) { cout << "时间反转！！！" << endl; }
		else if (diff < 100) { }
		else { ++big_gap; }
	}
	cout << "big_gap count: " << big_gap << "---" << "small_gap count: " << 100'0000 - big_gap << endl;
}

int main()
{
	TimeStamp now(TimeStamp::Now()); // 拷贝构造函数
	ChronoTimeStamp c_now = ChronoTimeStamp::Now(); // 拷贝赋值

	cout << "test copy ctor " << now.ToFormatString() << "------" << now.ToSecondsString() << endl;
	cout << "test assign operator " << c_now.ToFormatString() << "------" << c_now.ToSecondsString() << endl;

	cout << "----------------------" << endl;
	cout << TimeDifference(now, AddSeconds(now, 1)) << endl;
	cout << TimeDifference(c_now, AddSeconds(c_now, 1)) << endl;

	cout << "----------------------" << endl;
	benchmark<TimeStamp>();
	cout << "----------------------" << endl;
	benchmark<ChronoTimeStamp>();
}