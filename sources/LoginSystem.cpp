#include "../headers/LoginSystem.h"
#include "../headers/HttpConnection.h"
#include "../headers/VerifyGrpcClient.h"
#include "../headers/StatusGrpcClient.h"
#include "../headers/RedisMgr.h"
#include "../headers/MysqlMgr.h"
#include "../headers/JsonMgr.h"

bool LoginSystem::handleGet(const std::string& path, std::shared_ptr<HttpConnection> connection)
{
	if (!_get_handlers.contains(path))
	{
		// 没有注册对应的处理函数
		return false;
	}
	_get_handlers[path](connection);
	return true;
}

bool LoginSystem::handlePost(const std::string& path, std::shared_ptr<HttpConnection> connection)
{
	if (!_post_handlers.contains(path))
	{
		return false;
	}
	_post_handlers[path](connection);
	return true;
}

void LoginSystem::regGet(const std::string& url, const HttpHandler& handler)
{
	_get_handlers.emplace(url, handler);
}

void LoginSystem::regPost(const std::string& url, const HttpHandler& handler)
{
	_post_handlers.emplace(url, handler);
}

LoginSystem::LoginSystem()
{
	// 注册get请求处理器
	regGet("/get_test", [](std::shared_ptr<HttpConnection> connection)
	{
		beast::ostream(connection->getResponse().body()) << "receive get_test req" << std::endl;
		int i = 0;
		for (const auto& pair: connection->getGetParams())
		{
			i++;
			const auto&[key, value] = pair;
			beast::ostream(connection->getResponse().body()) << "param " << i << " key is " << key << " ";
			beast::ostream(connection->getResponse().body()) << "param " << i << " value is " << value << std::endl;
		}
	});
	// 注册post请求处理器，获取验证码
	regPost("/get_varifyCode", [](std::shared_ptr<HttpConnection> connection)
	{
		Defer defer([connection]
		{
			// 设置回应类型
			connection->getResponse().set(http::field::content_type, "text/json; charset=utf-8");
			// 设置回应的服务器
			connection->getResponse().set(http::field::server, "GateServer");
			// 异步写入http响应报文
			connection->sendResponse();
		});

		auto body_str = beast::buffers_to_string(connection->getRequest().body().data());
		std::cout << "receive body is " << body_str << std::endl;
		Json::Value src_root = JsonMgr::stringToJson(body_str);
		auto root = std::make_shared<Json::Value>();
		// 解析失败
		if (src_root.isMember("parse_error"))
		{
			std::cout << "Failed to parse JSON data, err is " << src_root["parse_error"] << std::endl;
			(*root)["error"] = ErrorCodes::Error_Json;
			std::string jsonStr = root->toStyledString();
			beast::ostream(connection->getResponse().body()) << jsonStr;
			return;
		}

		if (!src_root.isMember("email"))
		{
			std::cout << "Failed to parse JSON data, email is not the member " << std::endl;
			(*root)["error"] = ErrorCodes::Error_Json;
			std::string jsonStr = root->toStyledString();
			beast::ostream(connection->getResponse().body()) << jsonStr;
			return;
		}

		// 解析成功
		auto email = src_root["email"].asString();
		std::cout << "当时是获取验证码逻辑， email is " << email << std::endl;

		defer.setEnabled(false);
		// 注册grpc的异步回调函数
		VerifyGrpcClient::getInstance()->getVarifyCode(email, connection, [root, connection](std::shared_ptr<GetVarifyRsp> reply)
		{
			std::cout << "当前执行async grpc的回调函数" << std::endl;
			(*root)["error"] = reply->error();
			(*root)["email"] = reply->email();
			// json数据序列化
			std::string jsonStr = root->toStyledString();
			std::cout << "JSON对象root的序列化字符串: " << jsonStr << std::endl;
			beast::ostream(connection->getResponse().body()) << jsonStr;

			// 设置回应状态
			connection->getResponse().result(http::status::ok);
			// 设置回应类型
			connection->getResponse().set(http::field::content_type, "text/json; charset=utf-8");
			// 设置回应的服务器
			connection->getResponse().set(http::field::server, "GateServer");
			std::cout << "post应答完成，（应该与root对象的序列化一致）response的包体内容为: " << beast::buffers_to_string(connection->getResponse().body().data()) << std::endl;
			connection->sendResponse();
		});
	});
	// 注册post请求处理器，注册用户
	regPost("/user_register", [](std::shared_ptr<HttpConnection> connection)
	{
		Defer defer([connection]
		{
			// 设置回应类型
			connection->getResponse().set(http::field::content_type, "text/json; charset=utf-8");
			// 设置回应的服务器
			connection->getResponse().set(http::field::server, "GateServer");
			// 异步写入http响应报文
			connection->sendResponse();
		});

		std::cout << "当前执行用户注册的逻辑" << std::endl;

		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;

		auto body_str = boost::beast::buffers_to_string(connection->getRequest().body().data()); // 收到的请求头包体转化为string
		if (bool parse_success = reader.parse(body_str, src_root); !parse_success) // 反序列化解析到src_root
		{
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json; // 新Json文档写入数据
			std::string jsonStr = root.toStyledString();	// 序列化
			beast::ostream(connection->getResponse().body()) << jsonStr; // 序列化Json数据写入响应包包体
			return;
		}
		// 查找redis中email对应的验证码是否合理
		auto getVarifyOptional = RedisMgr::getRedis().get(std::string(CODE_PREFIX) + src_root["email"].asString());
		if (!getVarifyOptional)	// 没有该邮箱的验证码（要么没获取，要么过期）
		{
			std::cout << "Failed to get varify code!" << std::endl;
			root["error"] = ErrorCodes::VarifyExpired;
			std::string jsonStr = root.toStyledString();
			beast::ostream(connection->getResponse().body()) << jsonStr;
			return;
		}
		if (const std::string& varifyCode = getVarifyOptional.value(); varifyCode != src_root["varifyCode"].asString()) // 验证码错误
		{
			std::cout << "varify code is not correct!" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeError; // 验证码错误
			std::string jsonStr = root.toStyledString();
			beast::ostream(connection->getResponse().body()) << jsonStr;
			return;
		}

		const std::string& user = src_root["user"].asString();
		const std::string& password = src_root["passwd"].asString();
		const std::string& email = src_root["email"].asString();
		const std::string& salt = src_root["salt"].asString();

		// 访问Mysql查找相关数据
		std::optional<uint64_t> uid = MysqlMgr::getInstance()->registerUser(user, password, email);
		if (uid.has_value() && (uid.value() == 0 || uid.value() == -1))
		{
			std::cout << "user or email exists" << std::endl;
			root["error"] = ErrorCodes::UserExited;
			std::string jsonStr = root.toStyledString();
			beast::ostream(connection->getResponse().body()) << jsonStr;
			return;
		}

		connection->getResponse().result(http::status::ok);
		root["error"] = ErrorCodes::Success;
		root["email"] = email;
		root["user"] = user;
		root["uuid"] = uid.value();
		std::string jsonStr = root.toStyledString();
		std::cout << "当前注册逻辑里的Json数据: " << jsonStr << std::endl;
		beast::ostream(connection->getResponse().body()) << jsonStr;
	});
	// 注册post请求处理器，重置密码
	regPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection)
	{
		Defer defer([connection]
		{
			// 设置回应类型
			connection->getResponse().set(http::field::content_type, "text/json; charset=utf-8");
			// 设置回应的服务器
			connection->getResponse().set(http::field::server, "GateServer");
			// 异步写入http响应报文
			connection->sendResponse();
		});

		auto body_str = boost::beast::buffers_to_string(connection->getRequest().body().data());
		std::cout << "receive body is " << body_str << std::endl;
		std::cout << "当前执行重置密码的逻辑" << std::endl;
		Json::Value src_root = JsonMgr::stringToJson(body_str);
		Json::Value root;
		if (src_root.isMember("parse_error"))
		{
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonStr = JsonMgr::jsonToString(root);
			beast::ostream(connection->getResponse().body()) << jsonStr;
			return;
		}

		auto email = src_root["email"].asString();
		auto user = src_root["user"].asString();
		auto newPasswd = src_root["passwd"].asString();
		auto varifyCode = src_root["varifyCode"].asString();

		// 查找redis中email对应的验证码是否合理
		/// get的返回值是sw::redis::OptionalString (Optional<std::string>)
		auto getVarifyOptional = RedisMgr::getRedis().get(std::string(CODE_PREFIX) + email);
		if (!getVarifyOptional)
		{
			std::cout << "get varify code expired!" << std::endl;
			root["error"] = ErrorCodes::VarifyExpired;
			std::string jsonStr = JsonMgr::jsonToString(root);
			beast::ostream(connection->getResponse().body()) << jsonStr;
			return;
		}

		if (varifyCode != getVarifyOptional.value())
		{
			std::cout << "varify code is not correct!" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeError;
			std::string jsonStr = JsonMgr::jsonToString(root);
			beast::ostream(connection->getResponse().body()) << jsonStr;
			return;
		}

		// 查询数据库，判断用户名和邮箱是否匹配
		if (!MysqlMgr::getInstance()->checkEmail(user, email))
		{
			std::cout << "User not match the email" << std::endl;
			root["error"] = ErrorCodes::EmailNotMatch;
			std::string jsonStr = JsonMgr::jsonToString(root);
			beast::ostream(connection->getResponse().body()) << jsonStr;
			return;
		}

		// 更新密码为传入的最新密码
		if (!MysqlMgr::getInstance()->updatePassword(email, newPasswd))
		{
			std::cout << "Password update failed!" << std::endl;
			root["error"] = ErrorCodes::PasswdUpFailed;
			std::string jsonStr = JsonMgr::jsonToString(root);
			beast::ostream(connection->getResponse().body()) << jsonStr;
			return;
		}

		// 全部判断完成，更新操作完成，返回响应
		std::cout << "succeed to update pwd" << std::endl;
		root["error"] = ErrorCodes::Success;
		root["email"] = email;
		root["user"] = user;
		root["passwd"] = newPasswd;
		root["varifyCode"] = varifyCode;
		std::string jsonStr = JsonMgr::jsonToString(root);
		beast::ostream(connection->getResponse().body()) << jsonStr;
		return;
	});
	// 注册post请求处理器，用户登录
	regPost("/user_login", [](std::shared_ptr<HttpConnection> connection)
	{
		// 如果当前处理器，没有异步操作，可以使用Defer类
		Defer defer([connection]
		{
			connection->sendResponse();
		});
		// 解析Json字符串并反序列化
		auto body_str = beast::buffers_to_string(connection->getRequest().body().data());
		std::cout << "receive body is " << body_str << std::endl;
		std::cout << "当前执行登录的逻辑" << std::endl;
		Json::Value src_root = JsonMgr::stringToJson(body_str);
		Json::Value root;
		if (src_root.isMember("parse_error"))
		{
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonStr = JsonMgr::jsonToString(root);
			beast::ostream(connection->getResponse().body()) << jsonStr;
			return;
		}

		UserInfo userInfo;
		auto user = src_root["user"].asString();
		auto password = src_root["password"].asString();

		// 数据库查询判断用户名与密码是否匹配
		if (!MysqlMgr::getInstance()->checkPwd(user, password, userInfo))
		{
			std::cout << "password check failed!" << std::endl;
			root["error"] = ErrorCodes::LoginError;
			std::string jsonStr = JsonMgr::jsonToString(root);
			beast::ostream(connection->getResponse().body()) << jsonStr;
			return;
		}

		// 上面都是同步查询，下面是异步操作，需要关闭defer，不然会输出空
		defer.setEnabled(false);
		// 查询StatusServer是否找到合适的连接
		StatusGrpcClient::getInstance()->getChatServer(userInfo.uid, connection, [userInfo, connection](std::shared_ptr<GetChatServerRsp> reply)
		{
			Defer defer([connection]
			{
				// 设置回应类型
				connection->getResponse().set(http::field::content_type, "text/json; charset=utf-8");
				// 设置回应的服务器
				connection->getResponse().set(http::field::server, "StatusServer");
				// 延迟写入
				connection->sendResponse();
			});
			Json::Value root;
			if (reply->error() == ErrorCodes::RPCFailed)
			{
				std::cout << "grpc get char server failed, error is " << reply->error() << std::endl;
				root["error"] = ErrorCodes::RPCFailed;
				std::string jsonStr = JsonMgr::jsonToString(root);
				beast::ostream(connection->getResponse().body()) << jsonStr;
				return;
			}
			std::cout << "get chat server success, uid is " << userInfo.uid << std::endl;
			root["error"] = 0;
			root["email"] = userInfo.email;
			root["user"] = userInfo.username;
			root["uid"] = userInfo.uid;
			root["token"] = reply->token();
			root["host"] = reply->host();
			root["port"] = reply->port();
			std::string jsonStr = JsonMgr::jsonToString(root);
			beast::ostream(connection->getResponse().body()) << jsonStr;
			connection->getResponse().result(http::status::ok);
		});
	});
}
