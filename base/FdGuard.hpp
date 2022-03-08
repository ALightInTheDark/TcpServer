// trivial
// Created by kiki on 2021/11/23.9:19
#pragma once
#include <unistd.h> // close

struct FdGuard
{
	int fd;

	explicit FdGuard(int f) : fd(f){ }

	explicit operator int() { return fd; }

	~FdGuard() { close(fd); }
};