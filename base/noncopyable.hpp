// trivial
// Created by kiki on 2021/11/13.12:25
#pragma once

// public继承表示is-a, private继承表示implemented-in-terms-of（依据...实现）。
// 因此noncopyable类应当被子类private继承。
class noncopyable
{
protected: // 创建noncopyable对象是没有意义的，它只被其它类继承。
	noncopyable() = default;
	~noncopyable() = default;
public:
	noncopyable(const noncopyable&) = delete;
	noncopyable& operator=(const noncopyable&) = delete;
	// noncopyable类虽然是基类，但它并非用来实现多态，不必将析构函数设置为virtual。
};

