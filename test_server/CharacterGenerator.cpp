// trivial_test
// Created by kiki on 2021/12/5.20:27
#include <TcpServer.h>

string str;
string get_data()
{
	string message;

	string line;
	for (int i = 33; i < 127; ++i)
	{
		line.push_back(char(i));
	}
	line += line;

	for (size_t i = 0; i < 127-33; ++i)
	{
		message += line.substr(i, 72) + '\n';
	}

	return message;
}

void conn_info(const shared_ptr<TcpConnection>& conn)
{
	if (conn->IsClose())
	{
		trace("连接 ", conn->connection_name, " 将要关闭！")
	}
	else
	{
		trace("建立了名为 ", conn->connection_name, " 的连接！", "服务器地址为 ", conn->self_addr.IpPortString(), " ", "对等方地址为 ", conn->peer_addr.IpPortString())
		conn->Send(str.data(), str.size());
	}

}

void serve(const shared_ptr<TcpConnection>& conn)
{
	trace("连接名为 ", conn->connection_name, " 收到了", conn->read_buffer.ReadableBytes(), "字节的数据！ ")
	conn->read_buffer.RetrieveAsString();
	conn->Send(str.data(), str.size());
}

void on_write_complete(const shared_ptr<TcpConnection>& conn)
{
	conn->Send(str.data(), str.size());
}

// telnet 192.168.177.187 5678
// nc 192.168.177.187 5678



int main()
{
	str = get_data();
	TcpServer server(Sockaddr_in(5678), "trivial服务器");
	server.SetConnectionCallback(conn_info);
	server.SetMessageCallback(serve);
	server.SetWriteCompleteCallback(on_write_complete);
	server.start();
}