// trivial
// Created by kiki on 2021/12/31.10:03
#include <stdio.h>
#include <stdlib.h>
#include<unistd.h>

void daemon()
{
	pid_t pid = fork();
	if (pid == -1) { perror("fork failed"); exit(EXIT_FAILURE); }
	else if (pid > 0) { exit(EXIT_SUCCESS); } // 父进程直接退出。子进程被init进程收养

	if (setsid() == -1) { perror("fork failed"); exit(EXIT_FAILURE); } //只有子进程才能执行到此。让子进程摆脱原会话->原进程组->原控制终端 的控制

	chdir("/"); // 子进程继承了父进程的当前工作目录,可能会造成一些问题

	umask(0); // 子进程继承了父进程的文件权限掩码; 把文件权限掩码设置为0，增强守护进程的灵活性

	int fd = open("/dev/null", O_RDWR); // 必要时可以关闭所有从父进程继承而来的文件描述符。
	if (fd == -1) { perror("open /dev/null failed"); exit(EXIT_FAILURE); }
	if (dup2(fd, STDIN_FILENO) == -1) { perror("dup2(fd, STDIN_FILENO) failed"); exit(EXIT_FAILURE); }
	if (dup2(fd, STDOUT_FILENO) == -1) { perror("dup2(fd, STDIN_FILENO) failed"); exit(EXIT_FAILURE); } // 不关闭标准错误，以便错误日志能够输出到控制台
	if (close(fd) == -1) { perror("close /dev/null failed"); exit(EXIT_FAILURE); }
}