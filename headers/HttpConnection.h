//
// Created by 27044 on 26-3-3.
//

#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include "const.h"

class HttpConnection: public std::enable_shared_from_this<HttpConnection>
{
public:
	explicit HttpConnection(net::io_context& ioc, ssl::context& ctx);

	~HttpConnection();

	void start();

	void close();

	auto getSocket() -> ssl::stream<tcp::socket>&;

	auto getRequest() -> http::request<http::dynamic_body>&;

	auto getResponse() -> http::response<http::dynamic_body>&;

	auto getGetParams() -> std::unordered_map<std::string, std::string>&;

	void sendResponse();

private:

	void doRead();

	void checkDeadline();

	void writeResponse();

	void handleRequest();

	void preparseGetParam();

	ssl::stream<tcp::socket> _socket;// 本地socket（用于与客户端通信的socket）

	beast::flat_buffer _buffer{8192}; // 缓冲区

	http::request<http::dynamic_body> _request; // 请求

	http::response<http::dynamic_body> _response; // 回复

	net::steady_timer _deadline{_socket.get_executor(), std::chrono::seconds(60)};	// 判断超时的定时器

	std::string _get_url;

	std::unordered_map<std::string, std::string> _get_params;

	bool _stop;
};

#endif //HTTPCONNECTION_H
