//
// Created by 27044 on 26-3-4.
//

#ifndef LOGINSYSTEM_H
#define LOGINSYSTEM_H

#include "const.h"
class HttpConnection;
using HttpHandler = std::function<void(std::shared_ptr<HttpConnection>)>;

class LoginSystem: public std::enable_shared_from_this<LoginSystem>, public Singleton<LoginSystem>
{
	friend class Singleton<LoginSystem>;
public:
	bool handleGet(const std::string&, std::shared_ptr<HttpConnection>); // 处理get请求

	bool handlePost(const std::string&,  std::shared_ptr<HttpConnection>); // 处理post请求

	void regGet(const std::string&, const HttpHandler&); // 注册get请求的回调函数

	void regPost(const std::string&, const HttpHandler&); // 注册post请求的回调函数
private:
	LoginSystem();
	// get请求的处理函数集合, 不同url，调用不同处理函数（参数是该请求的HttpConnection）
	std::map<std::string, HttpHandler> _getHandlers;
	// post请求的处理函数集合, 不同url，调用不同处理函数（参数是该请求的HttpConnection）
	std::map<std::string, HttpHandler> _postHandlers;
};
#endif //LOGINSYSTEM_H
