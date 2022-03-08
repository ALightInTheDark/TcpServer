// trivial
// Created by kiki on 2021/11/14.15:14
#pragma once
#include <noncopyable.hpp>
#include <FdGuard.hpp>
#include <console_log.hpp>
#include "EventRegister.h"
class Sockaddr_in;
class EpollPoller;

class Acceptor : noncopyable
{
private:
	const FdGuard listen_fd;
	EventRegister new_conn_register; // 可读事件到来时接收连接，并执行下面的new_conn_callback。

	FdGuard dummy_fd; // 处理EMFILE

private:
	function<void(int conn_fd, const Sockaddr_in& peer_addr)> new_conn_callback; // 被TcpServer设置为TcpServer::EstablishNewConnection。Acceptor将accept得到的conn_fd和对等方的地址传入该回调函数。
public:
	void SetNewConnCallback(function<void(int conn_fd, const Sockaddr_in& peer_addr)> cb) { new_conn_callback = move(cb); }

public:
	Acceptor(EpollPoller& epoller, const Sockaddr_in& listen_address);
	~Acceptor() = default;

	void start_listen();
};

