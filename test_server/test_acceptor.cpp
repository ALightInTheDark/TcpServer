// trivial_test
// Created by kiki on 2021/12/2.10:44
#include <Acceptor.h>
#include <Reactor.h>
#include <Sockaddr_in.hpp>
#include <console_log.hpp>
using namespace std;

// telnet 127.0.0.1 5678
void new_conn(int conn_fd, const Sockaddr_in& peer_addr)
{
	log("对等方的IP和端口是：", peer_addr.IpPortString())

	write(conn_fd, "How are you?\n", 13);
	close(conn_fd);
}

int main()
{
	Reactor re;
	Acceptor acceptor(re.GetPoller(), Sockaddr_in(5678));
	acceptor.SetNewConnCallback(new_conn);

	re.React();
}