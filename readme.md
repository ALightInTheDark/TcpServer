# Tcp Server

这是一个受muduo库启发设计的，multiple reactors + thread pool（即IO线程池+计算线程池）模式的c++~~低性能~~高性能Tcp服务器。使用的EPOLL模式为LT。

# IO复用

如果为每一条连接分配一个进程（线程）以提供服务，那么使用阻塞IO是高效的：数据未到来时线程阻塞，不会消耗cpu资源。

如果我们想避免频繁地创建、销毁线程，并减少创建的线程数量，那么就需要使用线程池，让一条空闲线程为多个客户端的连接提供服务

这种情况下，不能使用阻塞IO：假设某线程为5个客户端连接提供服务，线程首先从第一个客户端读取数据（或向它发送数据），如果数据尚未到来，那么线程就会阻塞，其余4个客户端有数据到来，也不能得到处理。我们必须使用非阻塞IO。这种情况下，第一个客户端的数据没有到来时，非阻塞IO直接返回，我们便能从第二个客户端读取数据；如果第二个客户端的数据没有到来，非阻塞IO直接返回，我们便能从第三个客户端读取数据......最坏的情况下，我们需要进行5次read系统调用。

像上面这样，遍历每个连接，通过read或write系统调用逐个判断该连接上是否有数据到来，存在一个很大的问题：进行的系统调用次数过多，影响性能。如果总的连接数非常多，但活跃的连接数较少时，尤为明显。我们希望通过一次系统调用（即IO复用），就能够获取有事件到来的连接。epoll系统调用实现了这一点。

# 类的设计

## 辅助类

ApplicationBuffer，应用层缓冲区.类似于队列，向尾部写，从头部读出数据。

Sockaddr_in，数据成员为`struct sockaddr_in addr;`成员函数实现了数字形式的ip地址、端口与字符串形式的相互转换。

## ThreadPool

c++11 API实现的，生产者消费者模型的计算线程池。

构造函数中，向线程数组emplace_back线程对象，线程对象执行的线程函数为，在无限循环中：加锁，running变量为true且任务队列为空时，阻塞，等待被条件变量唤醒；当running变量为false，或任务队列不为空时，被唤醒；running变量为false且任务队列为空时则退出循环；从任务队列中取出一个函数对象；解锁，执行函数对象。

添加任务时，加锁，将函数对象添加到任务队列中，释放锁；令条件变量唤醒一个线程。

析构函数中，加锁，将running变量设置为false，解锁；令条件变量唤醒所有线程；线程数组中的所有线程都调用join()。

## EventRegister

我们将文件描述符fd装到一个类中。

对于listen fd，我们封装了 Acceptor类；

对于conn fd，我们封装了TcpConnection类；

对于timer fd，我们封装了TimerContainer类；

对于event fd, 我们封装了EpollPoller类。

这些类都组合了一个EventRegister对象。

EventRegister类的数据成员有：

- fd, fd上关注的事件，fd上到来的事件。（上面那些类负责关闭fd，EventRegister不负责关闭fd。）

  ```
  const int fd; // EventRegister不控制fd的生命周期, 而是由Acceptor或TcpConnection控制。
  int events {0}; // 关注的事件
  int triggered_events {0}; // fd上到来的事件
  ```

- fd上有事件到来时，执行的响应函数。成员函数HandleEvent()根据triggered_events调用下面相应的响应函数。

  ```
  function<void()> connection_callback;
  function<void()> read_callback;
  function<void()> write_callback;
  function<void()> error_callback;	
  ```



## EpollPoller

EpollPoller封装了epoll系统调用。

我们先来认识下列epoll系统调用：

```cpp
int epoll_create(int size); //  Linux 2.6.8后，size参数不起作用，但必须大于0
int epoll_create1(int flags); // flags指定为EPOLL_CLOEXEC

int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event);

int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);
/*
epoll_wait调用会阻塞直到：一个文件描述符上产生了感兴趣的event；经过了timeout毫秒，返回0；被signal handler打断。timeout=-1 将一直阻塞，直到有event到来；timeout=0 时立即返回，如果没有可用的event则返回0。
*/
struct epoll_event
{
uint32_t     events;  //Epoll events
epoll_data_t data;    //User data variable
};
typedef union epoll_data
{
    void*        ptr;
    int          fd;
    uint32_t     u32;
    uint64_t     u64;
} 
epoll_data_t;
```



EpollPoller的数据成员有：

epoll文件描述符（在构造函数中通过epoll_create1()获取），epoll_event数组，EventRegister指针数组。

```cpp
FdGuard epoll_fd; 
vector<struct epoll_event> events_vector;
vector<EventRegister*> active_registers;
```

成员函数`void Register(EventRegister* er)`和`void Unregister(EventRegister* er)`获取EventRegister中的fd、fd上感兴趣的事件，构造struct epoll_event结构体，并令epoll_event.data.ptr指向EventRegister。接着调用epoll_ctl()，将fd添加到epoll红黑树中。

EventRegister类拥有EpollPoller的引用poller，EventRegister的`Remove()`和`Update()`成员函数调用了 `poller.Register(this)`和`poller.Unregister(this)`.

成员函数`poll()`调用`epoll_wait()`，将有事件发生的fd的相关信息保存在events_vector中，如果events_vector满了，就将它的大小加倍（ epoll_wait的返回值等于events_vector.size() ）；接着遍历events_vector，从epoll_event中的data.ptr获取EventRegister，将EventRegister的triggered_events设置为epoll_event.events，并将EventRegister添加到active_registers中。

成员函数`HandleActiveEvents()`首先调用`poll()`，接着遍历active_registers，调用EventRegister的HandleEvent()成员函数。最后，清空active_registers。



EpollPoller还拥有数据成员wakeup_fd及其对应的wakeup_register，

```cpp
FdGuard wakeup_fd;
EventRegister wakeup_register;
```

在构造函数中有

```cpp
EpollPoller() : wakeup_fd(open_eventfd()), wakeup_register(*this, wakeup_fd.fd)
{
	wakeup_register.SetReadCallBack
	(
		[this]()
		{
			unsigned long long one = 1;
			ssize_t ret = read(wakeup_fd.fd, &one, sizeof one);
			if (ret != sizeof one) { perror("EpollPoller::wakeup_register.SetReadCallBack::read"); }
		}
	);
	wakeup_register.InterestReadableEvent();
}
```

成员函数WakeUp()向wakeup_fd中写入1，以唤醒阻塞的epoll_wait。



## Acceptor

Acceptor的数据成员如下：

```cpp
const FdGuard listen_fd;
EventRegister new_conn_register;
	
FdGuard dummy_fd; // 处理EMFILE
```

在构造函数中创建监听套接字，绑定服务器的地址，开启监听；打开null文件，保存在dummy_fd。

连接到来时产生可读事件，new_conn_register的可读事件响应函数被设置为：调用accept4()得到一个非阻塞的连接套接字，并调用上层程序注册的回调函数，相关数据成员如下：

```cpp
private:
   function<void(int conn_fd, const Sockaddr_in& peer_addr)> new_conn_callback; 
public:
   void SetNewConnCallback(function<void(int conn_fd, const Sockaddr_in& peer_addr)> cb) 
   { new_conn_callback = move(cb); }
```

如果发生EMFILE错误，则关闭dummy_fd，接收并关闭客户端的连接，再次打开null文件，保存在dummy_fd。



## TimerContainer

在base文件夹下，实现了类TimeStamp和ChronoTimeStamp。它的数据成员为`long us_since_epoch`。

静态成员函数Now()获取当前时间点距离纪元时间点的微妙数（前一个类通过gettimeofday函数获取，后一个类c++11 chrono库获取。经测试，这两种实现几乎没有性能差异。），将它转化为TimeStamp对象后返回。

成员函数ToSecondsString()将us_since_epoch转化为秒数字符串，成员函数ToFormatString()通过gmtime_r()将us_since_epoch转化为年月日时分秒格式的字符串。

全局函数TimeDifference()计算两个TimeStamp相差的秒数。全局函数AddSeconds将TimeStamp增加指定的秒数。



在base文件夹下，实现了类Timer。它的数据成员如下：

```cpp
const function<void()> callback; //定时器回调函数
TimeStamp expiration; //下一次的超时时刻
const double interval; //超时时间间隔，如果是一次性定时器，该值为0，表示不重复
```

成员函数Run()执行callback。如果interval大于0，成员函数Restart()计算下一次的超时时刻。



TimerContainer是管理定时器Timer的容器。它的数据成员如下：

```cpp
multimap<TimeStamp, unique_ptr<Timer>> timer_map; // 时间戳->在时间戳上超时的定时器
set<Timer*> canceling_timer_set; // 自有妙用
```

multimap默认从小到大排序，最早的超时时间排在前面。在一个时间戳上可能有多个超时的定时器，因此不能用map。

成员函数AddTimer()构造定时器，将其插入timer_map中，返回定时器指针。如果timer_map原先为空，或新定时器的超时时间小于timer_map中第一个元素的超时时间，则需要更新最早超时时间。

成员函数CancelTimer(Timer* timer)根据传入的定时器指针，通过`timer_map.equal_range(timer->Expiration())`，在timer_map

中找到要取消的定时器，并移除。如果未找到，将其置于canceling_timer_set中。

成员函数HandleRead()处理超时事件。当有超时事件到来时，获取当前时刻，通过`timer_map.upper_bound(now)`获得超时时刻小于当前时刻的所有定时器，并通过make_move_iterator将它们移入`vector< pair<TimeStamp, unique_ptr<Timer>> > expired`中。清空canceling_timer_set，遍历expired，执行定时器的回调函数。（假设某一时刻a，b两个重复启动的定时器都超时，都被移到了expired vector中，先执行a的回调函数，紧接着就去执行b的回调函数，而b的回调函数中就是移除a定时器。b的回调函数在timer_map中找不到a，于是将a添加到canceling_timer_set中。）再次遍历expired，如果其中的定时器是重复触发的，且不在canceling_timer_set中，再次计算其超时时刻，并插入timer_map中。



TimerContainer使用timerfd通知超时。

```c
int timerfd_create(int clockid, int flags);
int timerfd_settime(int fd, int flags, const struct itimerspec* new_value, struct itimerspec* old_value);
int timerfd_gettime(int fd, struct itimerspec* curr_value);
```

timerfd_create把时间变成一个文件描述符，该文件在定时器超时的那一刻变得可读，可以用统一的方式处理IO事件和超时事件。

相关的数据成员如下：

```cpp
FdGuard timer_fd;
EventRegister timerfd_register;
```

timerfd_register的可读事件响应函数即为HandleRead。HandleRead中首先从timer_fd中读出数据，避免一直触发；接着处理超时事件；最后调用timerfd_settime重置超时事件。



## Reactor

Reactor包装了EpollPoller，并提供了任务队列，以使其它拥有Reactor对象指针的线程可以向IO线程添加任务。

```cpp
mutable mutex mtx;
vector<function<void()>> tasks;
```

成员函数AddTask()加锁后向tasks添加函数对象，接着解锁。

成员函数HandleTask()加锁后将tasks中的任务移动到一个临时队列，解锁后执行临时队列中的任务。这样做减小了临界区的长度，不会阻塞其它线程的AddTask()，也避免了死锁（任务队列中的回调函数很可能又调用了AddTask()  ）


成员函数React()执行

```cpp
while (running)
{
	poller.HandleActiveEvents();
	HandleTask();
}
```

跨线程调用AddTask()后，React()可能执行到第3行，并阻塞在epoll调用；或者React()执行到第四行，接着又去执行第三行，并阻塞在epoll调用。

如果HandleTask()中执行的函数，又调用了AddTask()，那么HandleTask()执行完后，又去执行第三行，很可能阻塞在epoll调用，使新添加的任务无法立即执行。

上述两中情况下，AddTask()中需要调用poller.WakeUp()，以唤醒阻塞的线程。

只有第三行代码poller.HandleActiveEvents();中的回调函数，调用了AddTask，才不需要唤醒。



## IOThreadPool

拥有Reactor对象，以Reactor.React()作为线程函数的对象，成为IO线程。一个IIO线程最多只能有一个Reactor对象。

main函数所在的主线程必须拥有一个Reactor对象，即 main reactor，否则主线程就什么都不干，成为空转线程了。

IOThreadPool的数据成员如下：

```cpp
vector<thread> thread_vector;
vector<Reactor*> sub_reactor_vector;
```

在构造函数中向thread_vector中emplace_back线程函数。

```cpp
for (size_t i = 0; i < thread_count; ++i)
{
	thread_vector.emplace_back(......);
    
    unique_lock<mutex> ulock(mtx);
	cv.wait(ulock, sub_reactor_vector.size() != thread_count);
}	
```

线程函数中，创建一个栈上的Reactor对象，而后加锁，将栈对象保存在sub_reactor_vector中，如果创建够了指定数量的线程，便让条件变量唤醒构造函数，最后解锁。

注意，thread_vector[i]线程所拥有的线程对象不一定是sub_reactor_vector[i]。

成员函数GetReactor()以轮叫的方式，返回Reactor对象的引用：

```cpp
Reactor& IOThreadPool::GetReactor()
{
	......

	Reactor* es = sub_reactor_vector[index];

	++index;
	if (index == sub_reactor_vector.size()) { index = 0; }

	return *es;
}
```



## TcpConnection

TcpConnection是最复杂的类，但也不过200多行代码。

其中基本的数据成员为：

```cpp
FdGuard conn_fd;
EventRegister event_register;
Reactor& reactor;

ConnectionState state {ConnectionState::Connecting};
```

IO事件发生后执行的回调函数有

```cpp
function<void (shared_ptr<TcpConnection>)> connection_callback; // 连接建立和断开时执行的回调函数。
function<void (shared_ptr<TcpConnection>)> message_callback; // 消息到来后执行的回调函数。
function<void (shared_ptr<TcpConnection>)> write_complete_callback; // 消息发送完毕后执行的回调函数。
	
function<void (shared_ptr<TcpConnection>, size_t)> buffer_full_callback; // 接收缓冲区满后执行的回调函数
function<void (shared_ptr<TcpConnection>)> close_callback; // 连接关闭时执行的回调函数
```

event_register的响应函数被设置为：

可读事件：HandleRead()，从connfd中读取数据到应用层缓冲区中。如果对等方关闭连接，调用HandleClose()；如果发生错误，调用HandleError()；否则，调用message_callback(shared_from_this())，该回调函数的参数是本对象的智能指针，因此它在执行时，TcpConnection对象一定不会被析构。

可写事件：HandleWrite()，向connfd中写入应用层缓冲区的数据。如果应用层缓冲区的数据全部被发送出去，调用event_register.UnInterestWritableEvent()停止关注EPOLLOUT事件、调用write_complete_callback、如果TcpConnection的状态为Disconnecting，则调用shutdown关闭写端。

对等方关闭连接（EPOLLHUP）事件：HandleClose()，将状态改为Disconnected，调用connection_callback和close_callback。

错误事件：HandleError()，通过getsockopt()获取错误原因并打印。

成员函数Send()由用户调用。如果状态不为Connected，则返回。该函数首先尝试向内核缓冲区中写，全部写完则调用`reactor.AddTask([this]{write_complete_callback(shared_from_this());} );`。这里不能直接调用write_complete_callback，因为write_complete_callback中可能又调用了Send()，造成无限递归。否则，将数据写入到应用层缓冲区中，如果应用缓冲区满，就将buffer_full_callback加入reactor的任务队列中。如果之前没有关注过EPOLLOUT事件，就通过`event_register.InterestWritableEvent();`关注。

对于大流量的应用层服务，可能会频繁地关注、停止关注EPOLLOUT事件，使得性能下降。

成员函数SendAcrossThreads()调用了`reactor.AddTask([this, data, len] { Send(data, len); } );`，该函数可以被计算线程池跨线程调用。

成员函数ShutDown()关闭连接的写端。如果仍关注着可写事件，那么只将状态修改为Disconnecting便返回，以便HandleWrite()发送完剩下的数据。该函数同样有ShutDownAcrossThreads()版。

成员函数close()将状态修改为Disconnecting，将HandleClose()加入reactor的任务队列。



## TcpServer

终于到了最后一个类。我们的Tcp Server就要完成了。

TcpServer重要的数据成员如下：

```cpp
Reactor main_reactor; // acceptor所属的event_scheduler, 即main reactor。
IOThreadPool IO_threadpool; // sub reactors
Acceptor acceptor;
map<string, shared_ptr<TcpConnection>> connection_map; // 连接列表： 连接名-连接对象的指针
```

TcpServer聚合了多个TcpConnection，因为TcpConnection销毁时，TcpServer不随之销毁，仍在接收连接。

成员函数EstablishNewConnection()被设置为acceptor中的NewConnCallback。该函数中通过IO_threadpool.GetReactor()获得一个Reactor，并用它作为参数，创建一个TcpConnection shared_ptr对象，将用户设置的connection_callback、message_callback、writeComplete_callback，设置为TcpConnection中对应的回调函数，将TcpConnection中的CloseCallback设置为RemoveConnection() （见下）。接着，这个TcpConnection 对象被添加到connection_map中。最后，让Reactor执行（reactor.Execute() ）TcpConnection中的ConnectionEstablished()函数。该函数中将状态设置为Connected，令event_register持有一个TcpConnection的shared ptr（event_register执行事件到来时的回调函数时，它所属的TcpConnection对象一定不会被销毁）， 并关注可读事件，并调用connection_callback。

RemoveConnection()成员函数中，main_reactor执行（main_reactor.Execute）以下任务：从connection_map中删除TcpConnection，把TcpConnection的成员函数OnDestroyConnection()添加到TcpConnection所在的reactor的任务队列中。该函数将状态设置为Disconnected，并调用connection_callback。

# SIGPIPE

退出码为128+x时，发生了与Linux信号x相关的严重错误。

进程异常结束，退出代码为 141时，141-128=13,13号信号为SIGPIPE，表明收到了SIGPIPE信号。

对等方发送FIN段关闭连接后，read系统调用返回0，但我们仍能够通过write系统调用发送数据。此时对等方会向我们发送RST段。如果再次调用write发送数据，就会产生SIGPIPE信号，使进程退出。令程序忽略此信号即可。

# 测试

开发环境为

- Ubuntu-20.04，wsl2
- 9.4.0
- cmake 3.16.3

我们在tcp server的基础上，实现了简单的http协议模块，用户通过ServletDispatcher.AddServlet()，指定响应给客户端的内容。默认发送的内容为

```html
<html>
    <head>
    	<title>哎呀！没有您要查询的页面呢！</title>
    </head>
    <body>
        <center>
        <h1>哎呀！没有您要查询的页面呢！</h1>
        </center>
        <hr><center>firmament服务器框架/1.0.0</center>
    </body>
</html>
```

16个IO线程：

ApacheBench,	ab -c10000 -n100000 http://xxx.xxx.xxx.xxx:5678/

```bash
ab -c10000 -n100000 http://xxx.xxx.xxx.xxx:5678/
This is ApacheBench, Version 2.3 <$Revision: 1843412 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking xxx.xxx.xxx.xxx (be patient)
Completed 10000 requests
Completed 20000 requests
Completed 30000 requests
Completed 40000 requests
Completed 50000 requests
Completed 60000 requests
Completed 70000 requests
Completed 80000 requests
Completed 90000 requests
Completed 100000 requests
Finished 100000 requests


Server Software:
Server Hostname:        xxx.xxx.xxx.xxx
Server Port:            5678

Document Path:          /
Document Length:        326 bytes

Concurrency Level:      10000
Time taken for tests:   3.149 seconds
Complete requests:      100000
Failed requests:        0
Total transferred:      40600000 bytes
HTML transferred:       32600000 bytes
Requests per second:    31751.66 [#/sec] (mean)
Time per request:       314.944 [ms] (mean)
Time per request:       0.031 [ms] (mean, across all concurrent requests)
Transfer rate:          12589.04 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0  149 115.3    141    1217
Processing:    54  153  27.3    153     243
Waiting:        0  117  28.9    123     196
Total:        104  302 119.0    288    1388

Percentage of the requests served within a certain time (ms)
  50%    288
  66%    313
  75%    318
  80%    321
  90%    339
  95%    341
  98%    351
  99%   1318
 100%   1388 (longest request)
```

WebBench， ./web_bench -c 20000 -t 60 -k http://xxx.xxx.xxx.xxx:5678/

```bash
Requset: GET / HTTP/1.1
User-Agent: WebBench v1.5
Host: xxx.xxx.xxx.xxx
Connection: keep-alive

===================================开始压测===================================
GET http://xxx.xxx.xxx.xxx:5678/ (Using HTTP/1.1)
客户端数量 20000, 每个进程运行 60秒


速度: 391741 请求/秒, 15881310 字节/秒.
请求: 23504482 成功, 0 失败
```

8个IO线程，8个计算线程：

```bash
Benchmarking xxx.xxx.xxx.xxx (be patient)
Completed 10000 requests
Completed 20000 requests
Completed 30000 requests
Completed 40000 requests
Completed 50000 requests
Completed 60000 requests
Completed 70000 requests
Completed 80000 requests
Completed 90000 requests
Completed 100000 requests
Finished 100000 requests


Server Software:
Server Hostname:        xxx.xxx.xxx.xxx
Server Port:            5678

Document Path:          /
Document Length:        0 bytes

Concurrency Level:      10000
Time taken for tests:   10.904 seconds
Complete requests:      100000
Failed requests:        0
Total transferred:      32600000 bytes
HTML transferred:       0 bytes
Requests per second:    9171.12 [#/sec] (mean)
Time per request:       1090.379 [ms] (mean)
Time per request:       0.109 [ms] (mean, across all concurrent requests)
Transfer rate:          2919.71 [Kbytes/sec] received
```



```bash
./web_bench -c 10000 -t 60  -k http://xxx.xxx.xxx.xxx:5678/
Requset: GET / HTTP/1.1
User-Agent: WebBench v1.5
Host: xxx.xxx.xxx.xxx
Connection: keep-alive


===================================开始压测===================================
GET http://xxx.xxx.xxx.xxx:5678/ (Using HTTP/1.1)
客户端数量 10000, 每个进程运行 60秒

速度: 92903 请求/秒, 30286432 字节/秒.
请求: 5574194 成功, 0 失败
```



# 其它

对于大流量的应用程序，TcpServer不断生成数据，调用TcpConnection的send()发送。如果对等方接收不及时，
内核发送缓冲区满，就会将应用层数据添加到TcpConnection的应用层发送缓冲区。长此以往，会撑爆应用层发送缓冲区。
此时需要设置WriteCompleteCallback和buffer_full_callback。例如在WriteCompleteCallback回调后，才能继续发送数据、在buffer_full_callback被回调后，关闭连接。

断开空闲连接的方法：每个TcpConnection保存最后一次收到消息的时间；在MainReactor中设置一个每秒执行一次的定时器，定时器回调函数中遍历所有的连接，踢掉不活跃的连接；在连接所在的SubReactor中为它注册一个一次性定时器，超时后断开连接，如果收到消息则重置定时器；上述两种方法性能开销大，可以使用时间轮或升序链表改进。

