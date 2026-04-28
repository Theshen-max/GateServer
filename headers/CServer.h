#ifndef CSERVER_H
#define CSERVER_H
#include "const.h"

class CServer: public std::enable_shared_from_this<CServer>
{
public:
	// io_context用于底层socket判断事件是否完成（调度器，监听树）
	CServer(net::io_context& ioc, const unsigned short& port);

	void start();

private:
	void initSSL();

	net::io_context& _ioc; // 服务器上下文（引用成员数据，类默认没有拷贝赋值函数）
	ssl::context _ssl_ctx; // SSL上下文（保证https + TLS）
	tcp::acceptor _acceptor; // 接收器（底层accept函数）
};
#endif //CSERVER_H
