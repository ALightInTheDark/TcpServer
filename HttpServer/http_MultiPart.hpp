//created by zda on 2021/10/11/22:50

#pragma once

#include <string>
#include <string_view>
#include <map>
#include <vector>
#include "http_request.h"
using std::string, std::string_view, std::map, std::vector;

/*
Multipart/form-data：浏览器用表单向服务器传文件


POST / HTTP/1.1
Host: localhost:8080
Connection: keep-alive
Content-Length: 12345678
Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryWdDAe6hxfa4nl2Ig
Accept-Encoding: gzip, deflate, br

------WebKitFormBoundaryWdDAe6hxfa4nl2Ig													boundary分割表单数据项
Content-Disposition: form-data; name="submit-name"											表单数据头
jack																						表单数据体
------WebKitFormBoundaryWdDAe6hxfa4nl2Ig
Content-Disposition: form-data; name="file1"; filename="out.png"							Disposition：排列
Content-Type: image/png
binary-data
------WebKitFormBoundaryWdDAe6hxfa4nl2Ig
Content-Disposition: form-data; name="file2"; filename="2.png"
Content-Type: image/png
binary-data-2
------WebKitFormBoundaryWdDAe6hxfa4nl2Ig--
*/
enum class DecodeMultipartState
{
	boundary_encountered,
	parsing_boundary,

	header_encountered,
	header_field_encountered,
	parsing_field,
	header_value_encountered,
	parsing_value,
	header_value_finished,
	header_finished,

	body_encountered,
	parsing_body,

	finish
};

class HttpMultiPart
{
private:
	DecodeMultipartState decode_state = DecodeMultipartState::boundary_encountered;
private:
	struct FormDataItem
	{
		map<string, string> header;
		string body;
	};
	vector<FormDataItem> formData;
	string boundary;

public:
	bool decode(string_view src)
	{
		string matched_boundary;
		auto matchBoundaryChar =
		[&matched_boundary, this]()
		{
			if (matched_boundary.size() == boundary.size() || matched_boundary.size() == boundary.size() + 1) { return ch == '-'; }
			if (matched_boundary.size() >= boundary.size() + 2) { return false; }
			return boundary[matched_boundary.size()] == ch;
		};
		auto isBoundaryFinallyEnd =
		[this, &matched_boundary]()
		{ return boundary.size() + 2 == matched_boundary.size(); }
		auto isFullBoundary =
		[this, &matched_boundary]() { return boundary.size() == matched_boundary.size(); }

		string header_key;
		string header_value;

		if (src.empty()) { return true; }

		for (char ch : src)
		{
			switch (decode_state)
			{
				case DecodeMultipartState::boundary_encountered:
				{
					if (matchBoundaryChar(ch)) { matched_boundary.push_back(ch); decode_state = DecodeMultipartState::parsing_boundary; }
					else { return false; }
					break;
				}
				case DecodeMultipartState::parsing_boundary:
				{
					if (ch == '\r') { matched_boundary.clear(); decode_state = DecodeMultipartState::header_encountered; }
					else if (matchBoundaryChar(ch)) { matched_boundary.push_back(ch); }
					else { return false; }
					break;
				}
				case DecodeMultipartState::header_encountered:
				{
					if (ch == '\n') { decode_state = DecodeMultipartState::header_field_encountered; formData.emplace_back(); }
					else { return false; }
					break;
				}
				case DecodeMultipartState::header_field_encountered:
				{
					if (isspace(ch)) { }
					else { decode_state = DecodeMultipartState::parsing_field; header_key.clear(); header_key.push_back(ch); }
					break;
				}
				case DecodeMultipartState::parsing_field:
				{
					if (isspace(ch) || ch == ':') { decode_state = DecodeMultipartState::header_value_encountered; }
					else if (ch == '\r' || ch == '\n') { return false; }
					else { header_key.push_back(ch); }
					break;
				}
				case DecodeMultipartState::header_value_encountered:
				{
					if (isspace(ch)) { }
					else if (ch == '\r' || ch == '\n') { return false; }
					else { decode_state = DecodeMultipartState::parsing_value; header_value.clear(); header_value.push_back(ch); }
					break;
				}
				case DecodeMultipartState::parsing_value:
				{
					if (ch == '\r') { formData.back().header.emplace(header_key, header_value); decode_state = DecodeMultipartState::header_value_finished; }
					else { header_value.push_back(ch); }
					break;
				}
				case DecodeMultipartState::header_value_finished:
				{
					if (ch == '\n') { decode_state = DecodeMultipartState::header_finished; }
					else { return false; }
					break;
				}
				case DecodeMultipartState::header_finished:
				{
					if (ch == '\r') { decode_state = DecodeMultipartState::body_encountered; }
					else { decode_state = DecodeMultipartState::parsing_field; header_key.clear(); header_key.push_back(ch); }
					break;
				}
				case DecodeMultipartState::body_encountered:
				{
					if (ch == '\n') { decode_state = DecodeMultipartState::parsing_body;}
					else { return false; }
					break;
				}
				case DecodeMultipartState::parsing_body:
				{
					if (matchBoundaryChar(ch)) { matched_boundary.push_back(ch); }
					else if (isFullBoundary() && ch == '\r') { matched_boundary.clear(); decode_state = DecodeMultipartState::header_encountered; }
					else if (isBoundaryFinallyEnd() && ch == '\r') { decode_state = DecodeMultipartState::finish; }
					else
					{
						if (!matched_boundary.empty()) { formData.back().body.append(matched_boundary); matched_boundary.clear(); }
						if (matchBoundaryChar(ch)) { matched_boundary.push_back(ch); }
						else { formData.back().body.push_back(ch); }
					}
					break;
				}
				case DecodeMultipartState::finish:
				{
					if (ch == '\n') { return true; }
					else { return false; }
				}
				default:
					return false;
			}
		}

		return false;
	}

	/*
	------WebKitFormBoundaryWdDAe6hxfa4nl2Ig
	Content-Disposition: form-data; name="file1"; filename="out.png"
	Content-Type: image/png
	binary-data
	*/
	static map<string, string> parseContentDisposition(string_view line)
	{
		string_view pairs = line.substr(line.find_first_of(';') + 1);
		if (pairs.empty()) { return {}; }


		map<string, string> result;
		string key;
		string value;

		int state = 0;
		for (char ch : pairs)
		{
			switch (state)
			{
				case 0:
				{
					if (!isspace(ch) && ch != ';') { key = ch; state = 1;}
					break;
				}
				case 1:
				{
					if (ch == '=') { state = 2; }
					else { key.push_back(ch);}
					break;
				}
				case 2:
				{
					if (ch == '"') { value.clear(); state = 3;}
					break;
				}
				case 3:
				{
					if (ch == '"') { state = 0; result.emplace(key, value); }
					else { value.push_back(ch); }
					break;
				}
			}
		}
		return result;
	}

	void fromHttpRequest(HttpRequest* request) //从httpRequest转换过来
	{
		if (request == nullptr && request->Body().empty()) { return; }
		
		//Content-Type: multipart/form-data; boundary=----WebKitFormBoundarySAJsB3ZsTmgE6job
		string contentType = request->Header("Content-Type");
		if (contentType.empty()) { return; }
		
		int colonIndex = contentType.find_first_of(';');
		if (contentType.substr(0, colonIndex) != "multipart/form-data") { return; }
		
		//找出boundary
		int equalIndex = contentType.find_last_of('=');
		string boundary = contentType.substr(equalIndex + 1);
		//去掉空格
		boundary.erase(0, boundary.find_first_not_of(' '));
		boundary.erase(boundary.find_last_not_of(' ') + 1);

		this->setBoundary(boundary);
		this->decode(request->Body());//解析
	}



	void setBoundary(const string &boundary) //设置分割线
	{
		boundary = "\r\n--" + boundary;//加上前面的\r\n--，方便状态机处理
		matched_boundary.append("\r\n");//初始化_matchedBoundaryBuffer
	}
};
