// trivial
// Created by kiki on 2022/3/1.23:47
#include <stdio.h>  // snprintf
#include <signal_operation.cpp>
#include "TcpServer.h"

void TcpServer::EstablishNewConnection(int conn_fd, const Sockaddr_in& peer_addr) // 只在IO线程中调用
{
	if (!main_reactor.InIOThread()) { fatal("EstablishNewConnection未在main_reactor线程中执行！") }

	char buf[32];
	snprintf(buf, sizeof buf, ":-%s#%d", port.data(), next_connId);
	++next_connId;
	string conn_name = name + buf;

	Reactor& reactor = IO_threadpool.GetReactor();
	shared_ptr<TcpConnection> conn(make_shared<TcpConnection>(reactor, conn_name, conn_fd, listen_addr, peer_addr));
	conn->SetConnCallback(connection_callback);
	conn->SetMessageCallback(message_callback);
	conn->SetWriteCompleteCallback(writeComplete_callback);
	conn->SetCloseCallback([this](auto && PH1) { RemoveConnection(std::forward<decltype(PH1)>(PH1)); });

	connection_map[conn_name] = conn;

	reactor.Execute([conn] { conn->ConnectionEstablished(); });
}

void TcpServer::RemoveConnection(shared_ptr<TcpConnection> conn)
{
	main_reactor.Execute // 单个reactor时，直接调用下面的函数
	(
		[this, conn]() // fixme:这里能不能捕获conn的引用？
		{
			connection_map.erase(conn->connection_name); // 只被main_reactor线程调用，无需加锁。
			conn->GetReactor().AddTask([conn] { conn->OnDestroyConnection(); });
		}
	);
}

TcpServer::TcpServer(const Sockaddr_in& _listen_addr, string nm, size_t sub_reactor_count)
: listen_addr(_listen_addr), port(listen_addr.PortString()),
  name(move(nm)),
  IO_threadpool(main_reactor, sub_reactor_count),
  acceptor(main_reactor.GetPoller(), listen_addr)
{
	acceptor.SetNewConnCallback([this](auto && PH1, auto && PH2) { EstablishNewConnection(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });

	ignore_SIGPIPE();
}
TcpServer::~TcpServer()
{
	for (auto& [conn_name, s_ptr] : connection_map)
	{
		shared_ptr<TcpConnection> conn = s_ptr;
		s_ptr.reset();
		conn->GetReactor().Execute([conn] { conn->OnDestroyConnection(); });
	}
}