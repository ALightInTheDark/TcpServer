// trivial
// Created by kiki on 2021/9/30.21:22

#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>
#include <string.h>
#include <vector>
using std::string, std::map, std::string_view;

/*
http1.1协议中定义了9种方法(方法名区分大小写)，我们只实现必要的三种方法：
GET: 请求特定的资源
HEAD: 请求特定的资源，但服务器不会返回响应体。测试用。
POST: 向服务器提交表单或上上传文件。
*/
enum class HttpMethod  //http方法
{
	UNKNOWN, GET, HEAD, POST
};

/*
HTTP请求格式:

-------------------------------------------------------------------
|	method	|	空格	|	URL	|	空格	|	协议版本	|	\r\n	|		状态行, 如   GET /favicon.ico HTTP/1.1\r\n
-------------------------------------------------------------------
key:value \r\n														请求头
......
key:value \r\n
-------------------------------------------------------------------
\r\n
-------------------------------------------------------------------
contents														   请求体
*/

//if else语句繁琐，似乎用状态机简化了程序的分支
//一个状态机中的状态过多，可以使用主从状态机，但状态机的结构更为复杂
//状态约定俗成用大写字母，为了提高可读性，这里用小写字母。
enum class DecodeState 
{
//-------------------------------------------------状态行
	prepare_parsing_method,
	parsing_method,
	
	//访问百度翻译的请求 URL: https://fanyi.baidu.com/translate?aldtype=16047&query=&keyfrom=baidu&smartresult=dict&lang=auto2zh
	prepare_parsing_url, 
	parsing_url, //解析https://fanyi.baidu.com/translate
	prepare_parsing_urlParamKey, //遇到?  开始解析请求参数
	parsing_urlParamKey, //请求参数之间用&连接，每个请求参数的格式为key=value, value可以为空
	prepare_parsing_urlParamValue,
	parsing_urlParamValue,

	prepare_parsing_protocol,
	parsing_protocol,
	prepare_parsing_version,
	prepare_version,
	separator_encountered,
//-----------------------------------------------请求头
	parsing_header_key,
	blank_encountered,
	colon_encountered,
	parsing_header_value,//值

	cr_encountered,
	lf_encountered,
//-----------------------------------------------请求体
	parse_body,
//-----------------------------------------------
	cr_lf_cr
};


/*
HTTP请求格式:

-------------------------------------------------------------------
|	method	|	空格	|	URL	|	空格	|	协议版本	|	\r\n	|		状态行
-------------------------------------------------------------------
key:value \r\n														请求头
......
key:value \r\n
-------------------------------------------------------------------
\r\n
-------------------------------------------------------------------
contents														   请求数据
*/
class HttpRequest
{
private:
	DecodeState decode_state = DecodeState::prepare_parsing_method;	//解析状态
private:
	string method;
	string url; //请求路径,不包含请求参数
	map<string, string> request_params; //请求参数
	string protocol;
	string version;

	map<string, string> headers;

	size_t content_length = 0; string body;
public:
	[[nodiscard]] const string& Method() const { return method; }
	[[nodiscard]] const string& Url() const { return url; }
	const string& Body() { return body; }
public:
	bool decode(string_view src); //解析http协议

	void clear()
	{
		decode_state = DecodeState::prepare_parsing_method;

		method.clear();
		url.clear();
		request_params.clear();
		protocol.clear();
		version.clear();
		headers.clear();
		content_length = 0; body.clear();
	}
public:
	string_view all_data;
};

/*
HTTP请求格式:

-------------------------------------------------------------------
|	请求方法	|	空格	|	URL	|	空格	|	协议版本	|	\r\n	|		状态行
-------------------------------------------------------------------
key:value \r\n														请求头
......
key:value \r\n
-------------------------------------------------------------------
\r\n
-------------------------------------------------------------------
contents														   请求数据
*/
bool HttpRequest::decode(string_view src)
{
	all_data = src;

	clear();

	string param_key;
	string param_value;
	string header_key;
	string header_value;

	for (auto ch : src)
	{
		switch (decode_state)
		{
			case DecodeState::prepare_parsing_method:
			{
				if (ch == '\r' || ch == '\n' || isblank(ch)) { } //跳过空格、回车换行
				else if (isupper(ch)) { decode_state = DecodeState::parsing_method; method.push_back(ch); } //遇到第一个字符，开始解析方法
				else { return false; } //方法必须是大写。方法名区分大小写
				break;
			}
			case DecodeState::parsing_method:
			{
				if (isupper(ch)) { method.push_back(ch); } //把大写字符添加到方法中
				else if (isblank(ch)) { decode_state = DecodeState::prepare_parsing_url; } //遇到空格，解析方法结束，准备解析URL
				else { return false; }
				break;
			}
			case DecodeState::prepare_parsing_url:
			{
				if (ch == '/') { url.push_back(ch); decode_state = DecodeState::parsing_url; } //url以/开头
				else if (isblank(ch)) { }
				else { return false; }
				break;
			}
			case DecodeState::parsing_url: //解析请求路径
			{
				if (ch == '?') { decode_state = DecodeState::prepare_parsing_urlParamKey; } //解析请求的key
				else if (isblank(ch)) { decode_state = DecodeState::prepare_parsing_protocol; } //遇到空格，则解析完毕请求路径，开始解析协议
				else { url.push_back(ch); }
				break;
			}
			case DecodeState::prepare_parsing_urlParamKey:
			{
				if (isblank(ch) || ch == '\n' || ch == '\r') { return false; }
				else { param_key.push_back(ch); decode_state = DecodeState::parsing_urlParamKey; }
				break;
			}
			case DecodeState::parsing_urlParamKey:
			{
				if (ch == '=') { decode_state = DecodeState::prepare_parsing_urlParamValue; }
				else if (isblank(ch)) { return false; }
				else { param_key.push_back(ch); }
				break;
			}
			case DecodeState::prepare_parsing_urlParamValue:
			{
				if (isblank(ch) || ch == '\n' || ch == '\r') { return false; }
				else { param_value.push_back(ch); decode_state = DecodeState::parsing_urlParamValue; }
				break;
			}
			case DecodeState::parsing_urlParamValue:
			{
				if (ch == '&') { request_params.insert({param_key, param_value}); param_key.clear(); param_value.clear(); decode_state = DecodeState::prepare_parsing_urlParamKey; }
				else if (isblank(ch)) { request_params.insert({param_key, param_value}); decode_state = DecodeState::prepare_parsing_protocol; }
				else { param_value.push_back(ch); }
				break;
			}
			case DecodeState::prepare_parsing_protocol:
			{
				if (isblank(ch)) { }
				else { protocol.push_back(ch); decode_state = DecodeState::parsing_protocol; }
				break;
			}
			case DecodeState::parsing_protocol:
			{
				if (ch == '/') { decode_state = DecodeState::prepare_parsing_version; }
				else { protocol.push_back(ch); }
				break;
			}
			case DecodeState::prepare_parsing_version:
			{
				if (isdigit(ch)) { version.push_back(ch); decode_state = DecodeState::prepare_version; }
				else { return false; }
				break;
			}
			case DecodeState::prepare_version: //协议解析，如果不是数字或者. 就不对
			{
				if (ch == '\r') { decode_state = DecodeState::cr_encountered; }
				else if (ch == '.') { version.push_back(ch); decode_state = DecodeState::separator_encountered; }
				else if (isdigit(ch)) { version.push_back(ch); }
				else { return false; }
				break;
			}
			case DecodeState::separator_encountered:
			{
				if (isdigit(ch)) { version.push_back(ch); decode_state = DecodeState::prepare_version; } //遇到版本分割符，字符必须是数字，其他情况都是错误
				else { return false; }
				break;
			}
			case DecodeState::parsing_header_key:
			{

				if (isblank(ch)) { decode_state = DecodeState::blank_encountered; } //冒号前可能有空格
				else if (ch == ':') { decode_state = DecodeState::colon_encountered; }
				else { header_key.push_back(ch); }
				break;
			}
			case DecodeState::blank_encountered:
			{
				if (isblank(ch)) { }
				else if (ch == ':') { decode_state = DecodeState::colon_encountered; }
				else { return false; }
				break;
			}
			case DecodeState::colon_encountered:
			{
				if (isblank(ch)) { } //冒号后可能有空格
				else { header_value.push_back(ch); decode_state = DecodeState::parsing_header_value; } //开始处理值
				break;
			}
			case DecodeState::parsing_header_value:
			{
				if (ch == '\r') { headers.insert({header_key, header_value}); header_key.clear(); header_value.clear(); decode_state = DecodeState::cr_encountered; }
				header_value.push_back(ch);
				break;
			}
			case DecodeState::cr_encountered:
			{
				if (ch == '\n') { decode_state = DecodeState::lf_encountered; }
				else { return false; }
				break;
			}
			case DecodeState::lf_encountered:
			{
				if (ch == '\r') { decode_state = DecodeState::cr_lf_cr; }
				else if (isblank(ch)) { return false; }
				else { header_key.push_back(ch); decode_state = DecodeState::parsing_header_key; }
				break;
			}
			case DecodeState::cr_lf_cr:
			{
				if (ch == '\n')
				{
					auto iter = headers.find("Content-Length");
					if (iter != headers.end())
					{
						content_length = atoll(iter->second.c_str());
						if (content_length > 0) { body.reserve(content_length); decode_state = DecodeState::parse_body; }
						else { return true; }
					}
					else { return true; }
				}
				else { return false; }
				break;
			}
			case DecodeState::parse_body:
			{
				body.push_back(ch);
				if (body.size() == content_length) { return true; }
				break;
			}
			default:
				return false;
		}
	}

	return false;
}



