// trivial_test
// Created by kiki on 2021/12/4.20:03
#include <iostream>
#include <Reactor.h>
#include <console_log.hpp>

int num = 0;
Reactor* global_es;

void print(const char* msg)
{
	info(TimeStamp::Now().ToFormatString(), msg);

	if (++num == 20)
	{
		global_es->quit();
	}
}

void cancel(Timer* timer)
{
	global_es->CancelCall(timer);
	info(TimeStamp::Now().ToFormatString(), "global_es->CancelCall(timer)")
}


int main()
{
	info("main函数所在的线程ID为:", std::this_thread::get_id())
	info("当前时间为：", TimeStamp::Now().ToFormatString())

	Reactor es;
	global_es = &es; // es是main函数中的局部对象，IDE可能会报错，但其实程序是正确的

	Timer* timer;
	es.CallAfter(1, [](){print("once1");}, timer);
	es.CallAfter(1.5, [](){print("once1.5");}, timer);
	es.CallAfter(2.5, [](){print("once2.5");}, timer);
	es.CallAfter(3.5, [](){print("once3.5");}, timer);

	Timer* t45; es.CallAfter(4.5, [](){print("once4.5");}, t45); // 被取消，不输出
	es.CallAfter(4.2, [t45](){cancel(t45);}, t45);
	es.CallAfter(4.8, [t45](){cancel(t45);}, t45); // 无事发生

	es.CallEvery(2, [](){print("every2");}, t45); // 输出13个every2
	Timer* t3; es.CallEvery(3, [](){print("every3");}, t3); // 输出3个every3
	es.CallAfter(9.001, [t3](){cancel(t3);}, t3);

	es.React();

	print("main es exits");
}


/*
Reactor* glo_es;

int n = 0;
Timer* tr;

void busy_loop() { long i = 10000000000; while(--i){} }
void info_()
{
	info("every 2", "当前时间为：", TimeStamp::Now().ToFormatString())
	busy_loop();
}
void cance()
{
	info("cancel", "当前时间为：", TimeStamp::Now().ToFormatString())
	glo_es->CancelCall(tr);
	glo_es->CallAfter(10, [](){glo_es->quit();}, tr);
}

int main()
{
	Reactor es;
	glo_es = &es;

	info("main函数所在的线程ID为:", std::this_thread::get_id())
	info("当前时间为：", TimeStamp::Now().ToFormatString())

	es.CallEvery(2, info_, tr);
	Timer* tm;
	es.CallAfter(6, cance, tm); // 新创建的定时器，传入的指针追号和之前的不同。

	es.React();

	info("当前时间为：", TimeStamp::Now().ToFormatString(), "main es exits")
}
*/

