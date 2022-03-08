// trivial_test
// Created by kiki on 2021/11/28.22:47
#include <Reactor.h>
#include <iostream>
using namespace std;

Reactor* re;

void thread_func()
{
	cout << "es->InIOThread():" << boolalpha << re->InIOThread() << endl;
	re->React();

}

int main() // 在非IO线程中运行Reactor.React(),程序异常退出
{
	cout << this_thread::get_id() << endl;

	Reactor reactor;

	re = &reactor;
	thread trd(thread_func);
	this_thread::sleep_for(chrono::seconds(4));
	re->quit();
	trd.join();
}