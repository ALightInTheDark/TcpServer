// trivial_test
// Created by kiki on 2021/12/6.15:59
#pragma once
#include "../TcpServer/TcpServer.h"
#include <iostream>
using std::cout, std::endl;
#include "servlet.hpp"
#include "../base/thread_pool.hpp"
#include <functional>
#include <utility>
using std::bind;
static int i = 0;

class HttpServer
{
private:
	TcpServer server;

private:
	ServletDispatcher servlet;
	HttpRequest request;
	HttpResponse response;

private:
	ThreadPool pool {8};
private:
	static void OnConnection(shared_ptr<TcpConnection> conn)
	{
		if (conn->IsClose())
		{
			cout << "连接 " << conn->connection_name << " 将要关闭！" << endl;
		}
		else
		{
			cout << "建立了名为 " << conn->connection_name << " 的连接！";
			cout << "服务器地址为 " << conn->self_addr.IpPortString() << " ";
			cout << "对等方地址为 " << conn->peer_addr.IpPortString() << endl;
			info(++i)
		}
	}

	void OnMessageArrived(shared_ptr<TcpConnection> conn)
	{
		//cout << "连接名为 " << conn->connection_name; cout << " 收到了" << conn->read_buffer.ReadableBytes() << "字节的数据！ " << endl;
		//info(conn->read_buffer.StringView())

		pool.add_task
		(
			[conn]() // 这里不能捕获conn的引用！对于shared_ptr, 慎重捕获引用。
			{
				conn->read_buffer.RetrieveAsString();

				conn->context = string();
				any_cast<string&>(conn->context) = R"(<html>
						<head>
							<title>哎呀！没有您要查询的页面呢！</title>
						</head>
						<body>
							  <center>
								<h1>哎呀！没有您要查询的页面呢！</h1>
		                      </center>
		                       <hr><center>firmament服务器框架/1.0.0</center>
						</body>
				</html>)";

				conn->SendAcrossThreads(any_cast<string&>(conn->context).data(), any_cast<string&>(conn->context).size());
				//conn->ShutDownAcrossThreads();
			}
		);

	}


public:
	void add_servlet(const string& url, Function cb) { servlet.AddServlet(url, std::move(cb)); }
	HttpServer(const Sockaddr_in& addr, string name, size_t io_thread_count) : server(addr, std::move(name), io_thread_count) { }

	void start()
	{
		// server.SetConnectionCallback(OnConnection);
		server.SetMessageCallback([this](auto && PH1) { OnMessageArrived(std::forward<decltype(PH1)>(PH1)); });

		server.start();
	}
};