// trivial
// Created by kiki on 2021/11/22.23:45
#pragma once
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include "./cpp_header/cpp_container.h"

struct Sockaddr_in
{
	struct sockaddr_in addr{};

	Sockaddr_in() = default;
	Sockaddr_in(const Sockaddr_in&) = default; // 拷贝构造
	Sockaddr_in& operator=(const Sockaddr_in&) = default; // 赋值运算符

	explicit Sockaddr_in(unsigned short port)
	{
		addr.sin_family = PF_INET;
		addr.sin_addr.s_addr = get_local_ip();
		addr.sin_port = htons(port);
	}
	explicit Sockaddr_in(const struct sockaddr_in& a) : addr(a)
	{

	}
	Sockaddr_in(string_view ip, short port)
	{
		addr.sin_family = PF_INET;
		int ret = inet_pton(PF_INET, ip.data(), &addr.sin_addr);
		if (ret != 1) { perror("Sockaddr_in::inet_pton"); exit(EXIT_FAILURE); }
		addr.sin_port = htons(port);
	}

	[[nodiscard]] unsigned int Ip() const { return addr.sin_addr.s_addr; }
	[[nodiscard]] unsigned short Port() const { return addr.sin_port; }

	[[nodiscard]] string PortString() const
	{
		char buf[32] = {0};
		unsigned short port = ntohs(addr.sin_port);
		snprintf(buf, sizeof buf, "%d", port);
		return buf;
	}
	[[nodiscard]] string IpString() const
	{
		char buf[32]  = {0};
		const char* ret = inet_ntop(PF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(sizeof buf));
		if (ret == NULL) { perror("Sockaddr_in::IpString::inet_pton"); exit(EXIT_FAILURE); }
		return buf;
	}
	[[nodiscard]] string IpPortString() const
	{
		return IpString() + ", " + PortString();
	}

	static in_addr_t get_local_ip()
	{
		int sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(sockfd == -1) { perror("get_local_ip::socket"); return -1;}

		struct ifreq ireq{};
		strcpy(ireq.ifr_name, "eth0");

		ioctl(sockfd, SIOCGIFADDR, &ireq);

		auto* host = (struct sockaddr_in*)&ireq.ifr_addr;

		close(sockfd);

		return host->sin_addr.s_addr;
	}
};

//  int inet_pton(int af, const char *src, void *dst); 成功返回1，失败返回0或-1
// const char* inet_ntop(int af, const void *src, char *dst, socklen_t size); 失败返回NULL
