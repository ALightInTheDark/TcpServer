// WebLibrary
// Created by kiki on 2021/8/8.18:17
#pragma once
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void ignore_SIGPIPE()
{
	struct sigaction action {};
	action.sa_handler = SIG_IGN;
	action.sa_flags = 0;
	int ret = sigemptyset(&action.sa_mask);
		if (ret == -1) { perror("ignore_SIGPIPE::sigemptyset"); exit(EXIT_FAILURE); }
	ret = sigaction(SIGPIPE, &action, NULL);
		if (ret == -1) { perror("ignore_SIGPIPE::sigaction"); exit(EXIT_FAILURE); }
}

/*
struct sigaction
{
    void (*sa_handler) (int); //指向信号处理函数的函数指针
    void (*sa_sigaction) (int, siginfo_t *, void *); //更详细的信号处理函数
    sigset_t sa_mask; //指定信号处理函数执行期间被屏蔽的信号
    int sa_flags; //指定信号处理的行为
    void (*sa_restorer) (void);
};

int sigaction(int signum, const struct sigaction* act, struct sigaction* oldact);
将信号signum的处理方式设置为act，旧的处理方式保存在oldact。成功返回0，失败返回-1
 */