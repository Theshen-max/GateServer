//
// Created by 27044 on 26-3-4.
//

#include "../headers/CServer.h"
#include "../headers/HttpConnection.h"
#include "../headers/AsioIOServicePool.h"

CServer::CServer(net::io_context& ioc, const unsigned short& port) :
	_ioc(ioc),
	_acceptor(ioc, tcp::endpoint(tcp::v4(), port)),
	_ssl_ctx(ssl::context::tlsv12_server)
{
	initSSL();
}

void CServer::start()
{
	auto self = shared_from_this(); // 延长生命周期

	// 获取连接池的io_context，创建新连接，并且创建HttpConnection类管理这个连接（因为是短连接，无需保存）
	auto& iocFromPool = AsioIOServicePool::getInstance()->getIOService();
	auto connection = std::make_shared<HttpConnection>(iocFromPool, _ssl_ctx);

	// 拷贝捕获self,延长CServer在回调函数时的生命周期
	_acceptor.async_accept(connection->getSocket().lowest_layer(), [self, connection](beast::error_code ec)
	{
		// 出错放弃这链接，进行监听其他链接
		try
		{
			if (ec)
			{
				// operation_aborted代表用户主动关闭服务器
				if (ec == boost::asio::error::operation_aborted)
				{
					std::cout << "Acceptor closed, stopping accept loop." << std::endl;
					return;
				}
				std::cerr << "Accept error: " << ec.message() << std::endl;
				self->start();
				return;
			}

			// 未出错
			std::cout << "Created_Connection, Client IP: "
					  << connection->getSocket().lowest_layer().remote_endpoint().address().to_string()
					  << ":"
					  << connection->getSocket().lowest_layer().remote_endpoint().port()
					  << std::endl;
			connection->start();
			// CServer继续监听新连接
			self->start();
		}
		catch (const std::exception& e)
		{

		}
	});
}

void CServer::initSSL()
{
	_ssl_ctx.use_certificate_chain_file("server.crt");
	_ssl_ctx.use_private_key_file("server.key", ssl::context::pem);
}

