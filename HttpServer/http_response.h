// trivial
// Created by kiki on 2021/9/30.21:52

#pragma once

#include <string>
#include <string_view>
#include <map>
using std::string, std::string_view, std::map, std::to_string;

static constexpr int max_statusLine = 1024; //最大的状态行长度

/*
http响应格式：

-------------------------------------------------------------------
|	协议版本	|	空格	|	状态码	|	空格	|	状态码文字描述	| \r\n |	状态行
-------------------------------------------------------------------
key:value \r\n														响应头
......
key:value \r\n
-------------------------------------------------------------------
\r\n
-------------------------------------------------------------------
contents														   响应数据
*/
class HttpResponse
{
private:
	string protocol { "HTTP" };
	string version { "1.1" };
	int status_code { 200 };
	string status_word { "OK" };

	map<string, string> headers;

	string body;
public:
	string& Protocol() { return protocol; }
	string& Version() { return version; }
	string& Body() { return body; }
private:
	static const map<string, string> file2contenttype;

public:
	void encode(string& buf);

	void set_404()
	{
		status_code = 200;
		status_word = "OK";
		headers["Content-Type"] = file2contenttype.at("html");
		body = R"(<html>
						<head>
							<title>哎呀！没有您要查询的页面呢！</title>
						</head>
						<body>
							  <center>
								<h1>哎呀！没有您要查询的页面呢！</h1>
		                      </center>
		                       <hr><center>firmament服务器框架/1.0.0</center>
						</body>
				</html>)";
	}

	void set_contentType(string_view uri)
	{
		string_view file_name = uri.substr(uri.rfind('/') + 1); //请求的文件名
		string_view ext = file_name.substr(file_name.rfind('.') + 1); //文件的后缀

		auto iter = file2contenttype.find(string(ext));
		if (iter != file2contenttype.end()) { headers["Content-Type"] = iter->second; }
		else { headers["Content-Type"] = file2contenttype.at("txt"); }
	}


	void clear()
	{
		status_code = 200;
		status_word =  "OK";
		headers.clear();
		body.clear();
	}
};

const map<string, string> HttpResponse::file2contenttype = {{"vue",  "text/plain; charset=utf-8"},
															{"png",  "image/png"},
															{"html", "text/html; charset=utf-8"},
															{"jpg",  "image/jpeg"},
															{"txt",  "text/plain; charset=utf-8"},
															{"css",  "text/css"},
															{"js",   "application/javascript"},
															{"woff", "application/font-woff"},
															{"ttf",  "font/ttf"}};

void HttpResponse::encode(string& buf)
{
	buf.append(protocol).append("/").append(version).append(" ").append(to_string(status_code)).append(" ").append(status_word).append("\r\n");

	auto iter = headers.find("Content-Type");
	if (iter == headers.end()) { headers["Content-Type"] = file2contenttype.at("txt"); } //错误写法：iter->second = file2contenttype.at("txt")
	headers["Content-Length"] = to_string(body.size());
	for (const auto& header : headers)
	{
		buf.append(header.first).append(": ").append(header.second).append("\r\n");
	}

	buf.append("\r\n");

	buf.append(body);
}