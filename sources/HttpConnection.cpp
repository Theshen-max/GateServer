#include "../headers/HttpConnection.h"
#include "../headers/LoginSystem.h"

char toHex(const char c)
{
	// 解析汉字，一个字节转换16进制，分成上4位，下4位, 两个字节
	// 最高15，最低0（ASCII码）
	return c > 9 ?  c + 55 : c + 48;

	// 可以尝试stringstream与std::to_chars
}

char fromHex(const char c)
{
	char ch;
	if (c >= 'A' && c <= 'F') ch = c - 'A' + 10;
	else if (c >= 'a' && c <= 'f') ch = c - 'a' + 10;
	else if (c >= '0' && c <= '9') ch = c - '0';
	else ch = 0;
	return ch;
}

std::string urlEncode(const std::string& str)
{
	std::string strTemp{};
	const size_t len = str.length();
	for (size_t i = 0; i < len; i++)
	{
		if (std::isalnum(str[i]) || str[i] == '-' || str[i] == '_' || str[i] == '.' || str[i] == '~')
			strTemp += str[i];
		else if (str[i] == ' ')
			strTemp += '+';
		else // 汉字字节
		{
			strTemp += '%';
			strTemp += toHex(str[i] >> 4); // 高四位
			strTemp += toHex(str[i] & 0x0F); // 低四位
		}
	}
	return strTemp;
}

std::string urlDecode(const std::string& str)
{
	std::string strTemp{};
	const size_t len = str.length();
	for (size_t i = 0; i < len; i++)
	{
		// 还原空格
		if (str[i] == '+') strTemp += ' ';
		// 遇到%说明后面两个16进制的字符需要转为完整字节
		else if (str[i] == '%')
		{
			assert(i + 2 < len);
			char highChar = fromHex(str[++i]);
			char lowChar = fromHex(str[++i]);
			// 位移运算结果是int, 相加字符后还是int, 需要转换
			strTemp += static_cast<char>((highChar << 4) + lowChar);
		}
		else // 普通字符（字母，数字，部分特殊符号）
			strTemp += str[i];
	}
	return strTemp;
}

// 构造函数
HttpConnection::HttpConnection(net::io_context& ioc, ssl::context& ssl_ctx):
	_socket(ioc, ssl_ctx),
	_stop(false)
{

}

HttpConnection::~HttpConnection()
{
	 std::cout << "HttpConnection Destructed" << std::endl;
}

void HttpConnection::close()
{
	if (_stop)
		return;
	_stop = true;

	auto self = shared_from_this();

	// 取消定时器，防止干扰关闭流程
	_deadline.cancel();

	_socket.async_shutdown([self](boost::system::error_code ec)
	{
		// 无论SSL shutdown是否成功,都要清理底层 TCP
		boost::system::error_code error;
		// 此时再关闭底层的 TCP 读写通道
		self->_socket.lowest_layer().shutdown(boost::asio::socket_base::shutdown_both, error);
		// 彻底释放文件描述符
		self->_socket.lowest_layer().close(error);
		if (error) {
			std::cerr << "Socket close error: " << error.message() << std::endl;
		}
	});
}

auto HttpConnection::getSocket() -> ssl::stream<tcp::socket>&
{
	return _socket;
}

auto HttpConnection::getRequest() -> http::request<http::dynamic_body>&
{
	return _request;
}

auto HttpConnection::getResponse() -> http::response<http::dynamic_body>&
{
	return _response;
}

auto HttpConnection::getGetParams() -> std::unordered_map<std::string, std::string>&
{
	return _get_params;
}


void HttpConnection::start()
{
	auto self = shared_from_this();
	_socket.async_handshake(ssl::stream_base::server, [self](beast::error_code ec)
	{
		if (ec)
		{
			// 过滤掉对端主动关闭引发的“良性”错误
			if (ec == boost::asio::error::eof ||
				ec == boost::asio::error::connection_reset ||
				ec == boost::asio::ssl::error::stream_truncated)
			{
				// 优雅处理：可以选择只打印一条普通的 Info 日志，或者干脆什么都不打印
				std::cout << "Client dropped connection during SSL handshake (Ignored)." << ec.what() << std::endl;
			}
			else
			{
				// 真正的 SSL 协议报错（如证书不受信任、加密套件不匹配等）
				std::cerr << "SSL handshake failed: " << ec.message() << " (" << ec.value() << ")" << std::endl;
			}
			return;
		}

		// 握手成功 → 开始读 HTTP
		self->doRead();
	});
}

void HttpConnection::doRead()
{
	auto self = shared_from_this();
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec, size_t bytes_transferred)
	{
		try
		{
			if (ec)
			{
				std::cout << "对端关闭或出现错误" << std::endl;
				self->close();
				return;
			}
			std::cout << "Read_Success" << std::endl;
			boost::ignore_unused(bytes_transferred); // 忽略已经传输的字节数据，直接调用回调函数
			self->handleRequest(); // 执行回调函数
			self->checkDeadline(); // 启动定时器，启动超时检测（可以检查慢客户端：接收太慢，导致服务器端send窗口满了，暂停write）
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
			throw e;
		}
	});
}

void HttpConnection::sendResponse()
{
	writeResponse();
}

void HttpConnection::checkDeadline()
{
	auto self = shared_from_this();
	// 每次启动前，重新设置 60 秒超时期限 （因为会复用Http连接，所以每次doRead()都要重新设置计时）
	_deadline.expires_after(std::chrono::seconds(60));
	_deadline.async_wait([self](beast::error_code ec)
	{
		if(ec == boost::asio::error::operation_aborted)
			return; // timer 被 cancel，不是超时

		std::cerr << "网关服务器写超时, 客户端接收太慢" << std::endl;
		// 会导致服务器端出现TIME_WAIT阶段，暂不考虑
		beast::error_code ignore;
		beast::get_lowest_layer(self->_socket).close(ignore);
	});
}

void HttpConnection::writeResponse()
{
	auto self = shared_from_this();
	_response.content_length(_response.body().size());
	// 将_response写入_socket，发送给对方
	http::async_write(_socket, _response, [self](beast::error_code ec, size_t bytes_transferred)
	{
		self->_deadline.cancel(); // 响应发送完毕，取消超时计时器
		if (ec)
		{
			// 如果写入发生错误，直接断开连接
			self->close();
			return;
		}
		if (!self->_response.keep_alive())
		{
			self->close();
		}

		// 清空上一次的请求和响应数据，为下一次请求做准备
		self->_request = {};
		self->_response = {};
		self->doRead();
	});
}

void HttpConnection::handleRequest()
{
	// 对方传送过来有版本号，需要设置版本
	_response.version(_request.version());
	// 复用http连接，底层TCP连接，严格说复用TCP连接，减少TCP连接三次握手的成本
	_response.keep_alive(_request.keep_alive());
	// 判断是不是get请求
	if (_request.method() == http::verb::get)
	{
		preparseGetParam();
		// 目标url，以及HttpConnection本身
		bool success = LoginSystem::getInstance()->handleGet(_get_url, shared_from_this());
		if (!success) // 失败
		{
			// 设置回应状态
			_response.result(http::status::not_found);
			// 设置回应类型
			_response.set(http::field::content_type, "text/plain; charset=utf-8");
			// 写入回应内容
			beast::ostream(_response.body()) << "url not found\r\n";
			// 写完发送回对方
			writeResponse();
			return;
		}

		// 成功
		// 设置回应状态
		_response.result(http::status::ok);
		// 设置回应类型
		_response.set(http::field::content_type, "text/plain; charset=utf-8");
		// 设置回应的服务器
		_response.set(http::field::server, "GateServer");
		writeResponse();
		return;
	}
	if (_request.method() == http::verb::post)
	{
		// 目标url，以及HttpConnection本身
		bool success = LoginSystem::getInstance()->handlePost(_request.target(), shared_from_this());
		if (!success) // 失败，且只能判断有没有URL
		{
			std::cout << "服务器没有注册该post请求的资源路径的URL" << std::endl;
			// 设置回应状态
			_response.result(http::status::not_found);
			// 设置回应类型
			_response.set(http::field::content_type, "text/plain; charset=utf-8");
			// 写入回应内容
			beast::ostream(_response.body()) << "url not found\r\n";
			// 写完发送回对方
			writeResponse();
			return;
		}
		// success不write，由handlePost注册的异步函数来回调传输数据，否则会传出空数据包
		// !!!!!!!!!!!! Post请求下，需要自行处理writeResponse(),否则一直不触发写入，导致客户端自动断连
	}
}


void HttpConnection::preparseGetParam()
{
	// 提取URI的资源路径 + 查询参数
	auto uri = _request.target(); // 因为是string_view所以可以避免多次拷贝
	// 查询（'?'的位置）
	size_t query_pos = uri.find('?');
	if (query_pos == std::string_view::npos)
	{
		_get_url = uri;
		return;
	}
	// 有查询参数
	// 资源路径如下
	_get_url = uri.substr(0, query_pos);
	// 查询参数列表
	std::string_view query_string = uri.substr(query_pos + 1);
	// 判断有没有'&'
	size_t pos{};
	while ((pos = query_string.find('&')) != std::string_view::npos)
	{
		// key=value格式
		auto pair = query_string.substr(0, pos);
		// 查找'='位置
		size_t eq_pos = pair.find('=');
		if (eq_pos != std::string_view::npos)
		{
			_get_params.emplace(
				urlDecode(std::string(pair.substr(0, eq_pos))),
				urlDecode(std::string(pair.substr(eq_pos + 1)))
			);
		}
		query_string.remove_prefix(pos + 1);
	}
	// 处理最后一个参数键值对
	// 这时候pos == std::string_view::npos
	if (!query_string.empty())
	{
		// key=value格式
		auto pair = query_string;
		// 查找'='位置
		size_t eq_pos = pair.find('=');
		if (eq_pos != std::string_view::npos)
		{
			_get_params.emplace(
				urlDecode(std::string(pair.substr(0, eq_pos))),
				urlDecode(std::string(pair.substr(eq_pos + 1)))
			);
		}
	}
}





