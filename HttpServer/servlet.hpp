// http
// Created by kiki on 2021/11/4.11:06

#pragma once

#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>
#include <string_view>
#include <filesystem>
#include <fnmatch.h> //字符串模糊匹配
#include "http_request.h"
#include "http_response.h"

using std::function, std::unique_ptr, std::unordered_map, std::pair, std::vector, std::make_unique;

struct Servlet
{
	virtual bool serve(HttpRequest& request, HttpResponse& response) const = 0;
	virtual ~Servlet() = default;
};

typedef function<bool(HttpRequest& request, HttpResponse& response)> Function;
struct FunctionServlet : Servlet
{
private:
	Function callback;
public:
	explicit FunctionServlet(Function cb) : callback(move(cb)) { }
	Function& GetFunction() { return callback; }

public:
	bool serve(HttpRequest& request, HttpResponse& response) const override
	{
		return callback(request, response);
	}
};

// struct WriteFileServlet : Servlet
// {
//     bool serve(HttpRequest& request, HttpResponse& response) const override
// 	{
// 		string resource_path = config.AbsoluteWebRoot() + request.Url();
// 		if (!std::filesystem::is_regular_file(resource_path)) { return false; }
// 		response.set_contentType(request.Url());
// 		write_file_to_buffer(resource_path, response.Body());
// 		return true;
// 	}
// };

struct DefaultServlet : Servlet
{
	bool serve(HttpRequest& request, HttpResponse& response) const override
	{
		response.set_404();
		return true;
	}
};


struct ServletDispatcher : public Servlet
{
private:
	unordered_map<string, unique_ptr<Servlet>> accurate_servlets;
	vector<pair<string, unique_ptr<Servlet>>> global_servlets;
	unique_ptr<Servlet> default_servlet;
public:
	ServletDispatcher() : default_servlet(make_unique<DefaultServlet>()) { }
public:
	bool serve(HttpRequest& request, HttpResponse& response) const override
	{
		return matchedServlet(request.Url()).serve(request, response);
	}

	const Servlet& matchedServlet(const string& url) const
	{
		auto iter = accurate_servlets.find(url);
		if (iter != accurate_servlets.end()) { return *(iter->second); }

		auto iter2 = find_if(global_servlets.begin(), global_servlets.end(), [url](const auto& pair){ return pair.first == url; } );
		if (iter2 != global_servlets.end()) { return *(iter2->second); }

		return *default_servlet;
	}

public:
	void AddServlet(const string& url, unique_ptr<Servlet> s) { accurate_servlets[url] = move(s); }
	void AddServlet(const string& url, Function cb) { accurate_servlets[url] = make_unique<FunctionServlet>(move(cb)); }

	void AddGlobalServlet(string_view url, unique_ptr<Servlet> s)
	{
		auto iter = find_if(global_servlets.begin(), global_servlets.end(), [url](const auto& pair){ return fnmatch(pair.first.data(), url.data(), 0) == 0; } );
		if (iter != global_servlets.end()) { iter->second = move(s); return; }
		global_servlets.emplace_back(url, move(s));
	}
	void AddGlobalServlet(string_view url, Function cb) { return AddGlobalServlet(url, make_unique<FunctionServlet>(move(cb))); }

	void DelServlet(const string& url) { accurate_servlets.erase(url); }
	void DelGlobalServlet(string_view url) { global_servlets.erase(find_if(global_servlets.begin(), global_servlets.end(), [url](const auto& pair){ return pair.first == url; })); }
};