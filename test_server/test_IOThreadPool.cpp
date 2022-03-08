// trivial_test
// Created by kiki on 2021/11/29.18:00
#include <IOThreadPool.h>
using namespace std;

int main()
{
	// unique_ptr<IOThreadPool> pool = make_unique<IOThreadPool>();
	// pool->print();
	Reactor reactor;
	IOThreadPool pool(reactor);

	auto& es = pool.GetReactor();
	log("pool.ThreadCount()", pool.ThreadCount())
	log("pool.ThreadCount()", pool.Index())

	//es.Execute([](){trace(std::this_thread::get_id())});
	Timer* timer;
	es.CallAfter(2, [](){trace(std::this_thread::get_id())}, timer);
	Timer* timer2;
	es.CallAfter(4, [](){trace(std::this_thread::get_id())}, timer2);
	Timer* timer3;
	es.CallAfter(6, [](){trace(std::this_thread::get_id())}, timer3);
	Timer* timer4;
	es.CallAfter(8, [&reactor](){reactor.quit();}, timer4);

	reactor.React();
}
