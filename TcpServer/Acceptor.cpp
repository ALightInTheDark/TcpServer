// trivial
// Created by kiki on 2022/3/1.23:26
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <Sockaddr_in.hpp>
#include <file_operation.h>
#include "Acceptor.h"
#include "EpollPoller.h"


Acceptor::Acceptor(EpollPoller& epoller, const Sockaddr_in& listen_address)
: listen_fd(open_socket()), new_conn_register(epoller, listen_fd.fd), dummy_fd(open_null())
{
	int ret = ::bind(listen_fd.fd, (struct sockaddr*)&listen_address.addr, static_cast<socklen_t>(sizeof(struct sockaddr_in)));
	if (ret == -1) { perror("Acceptor::ctor::bind"); exit(EXIT_FAILURE); }

	new_conn_register.SetReadCallBack
	(
		[this]()
		{
			Sockaddr_in peer_addr{};
			auto len = static_cast<socklen_t>(sizeof peer_addr.addr);
			int conn_fd = accept4(listen_fd.fd, (struct sockaddr*)&peer_addr.addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);

			if (conn_fd != -1)
			{
				if (new_conn_callback) { new_conn_callback(conn_fd, peer_addr); }
				else { close(conn_fd); }
			}
			else
			{
				if (errno == EMFILE)
				{
					close(dummy_fd.fd);
					dummy_fd.fd = accept(listen_fd.fd, NULL, NULL);
					close(dummy_fd.fd);
					dummy_fd.fd = open_null();

					err("accept: EMFILE")
				}
			}
		}
	);

	//start_listen();
}

void Acceptor::start_listen()
{
	int ret = listen(listen_fd.fd, SOMAXCONN);
	if (ret == -1) { perror("Acceptor::start_listen::listen"); exit(EXIT_FAILURE); } // 可能是端口被占用

	new_conn_register.InterestReadableEvent(); info("Acceptor开始监听连接！！！")
}