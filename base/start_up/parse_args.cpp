// trivial
//created by zda on 2021/10/30/15:54

/*
int getopt(int argc, char* const argv[], const char* optstring);

int getopt_long(int argc, char* const argv[], const char* optstring, const struct option* longopts, int* longindex);
plus版getopt, 增加了解析长选项的功能如：--help

char* optstring = “ab:c::”
a       表示选项a没有参数
b:     表示选项b有且必须加参数
c::   表示选项c可以有参数，也可以没有参数
*/

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <tuple>
#include <optional>
#include <thread>
using std::optional, std::tuple, std::get;

void print_help()
{
	printf("-a 监听地址\n");
	printf("-p 监听端口\n");
	printf("-i IO线程数（不包括监听客户端连接的主线程）\n");
	printf("-c 计算线程数\n");
	printf("-d 启动守护进程，后台运行\n");
	printf("-s 停止服务器\n");
	printf("-h 帮助\n");
}
auto parse_args(int argc, char* const argv[])
{
	tuple<optional<string>, optional<unsigned short>, optional<size_t>, optional<size_t>, optional<bool>, optional<bool>> tup;

	const char* opt_string = "a::l::m::o::s::t::c::a::";
	int opt;
	while ((opt = getopt_long(argc, argv, opt_string)) != -1)
	{
		switch (opt)
		{
			case 'a' : get<0>(tup) = optarg; break; // optarg是全局变量
			case 'p' : get<1>(tup) = atoi(optarg); break;
			case 'i' : get<2>(tup) = optarg; break;
			case 'c' : get<3>(tup) = optarg; break;
			case 'd' : get<4>(tup) = optarg; break;
			case 's' : get<5>(tup) = optarg; break;
			case 'h' : print_help(); exit(EXIT_SUCCESS); break;
			default: printf("解析到未知的命令行参数！\n"); exit(EXIT_FAILURE); break;
		}
	}

	return tup;
}