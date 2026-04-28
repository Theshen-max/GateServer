#include "../headers/VerifyGrpcClient.h"
#include "../headers/ConfigMgr.h"
#include "../headers/HttpConnection.h"

VerifyGrpcClient::VerifyGrpcClient()
{
	std::string ip_port = ConfigMgr::getInstance()["VarifyServer"]["Host"] + ":" + ConfigMgr::getInstance()["VarifyServer"]["Port"];
	std::cout << "验证码服务，gRPC的Channel连接的服务器ip:port是 : " << ip_port << std::endl;

	// 创建通道
	std::shared_ptr<Channel> channel = grpc::CreateChannel(ip_port, grpc::InsecureChannelCredentials());
	// VarifyService::NewStub()函数内部就是std::make_unique<VarifyService::Stub>(channel);
	_stub = VarifyService::NewStub(channel);
}

void VerifyGrpcClient::getVarifyCode(const std::string& email, std::shared_ptr<HttpConnection> connection, const std::function<void(std::shared_ptr<GetVarifyRsp>)>& callback)
{
	// ClientContext context; // 客户端上下文
	// GetVarifyRsp reply; // 回应报文段
	// GetVarifyReq request; // 请求报文段
	// request.set_email(email); // 设置请求头消息
	//
	// // 这里采用基础的同步RPC（需要优化成异步RPC/回调RPC_推荐）
	// Status status = _stub->GetVarifyCode(&context, request, &reply);
	// if (status.ok())
	// {
	// 	reply.set_error(ErrorCodes::Success);
	// 	return reply;
	// }
	// else
	// {
	// 	reply.set_error(ErrorCodes::RPCFailed);
	// 	return reply;
	// }

	auto clientContextPtr = std::make_shared<ClientContext>();
	auto request = std::make_shared<GetVarifyReq>(); // 请求报文段
	auto reply = std::make_shared<GetVarifyRsp>(); // 响应报文段
	request->set_email(email); // 设置请求头消息

	_stub->async()->GetVarifyCode(clientContextPtr.get(), request.get(), reply.get(), [clientContextPtr, request, reply, connection, callback](Status status)
	{
		std::cout << "async grpc request finished" << std::endl;
		if (!status.ok())
			reply->set_error(ErrorCodes::RPCFailed);

		// 切回 Asio 所在的底层线程！！！
		boost::asio::post(connection->getSocket().lowest_layer().get_executor(), [callback, reply]
		{
			callback(reply);
		});
	});
}


