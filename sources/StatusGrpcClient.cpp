//
// Created by 27044 on 26-3-17.
//

#include "../headers/StatusGrpcClient.h"
#include "../headers/HttpConnection.h"

StatusGrpcClient::StatusGrpcClient()
{
	std::string ip_port = ConfigMgr::getInstance()["StatusServer"]["Host"] + ":" + ConfigMgr::getInstance()["StatusServer"]["Port"];
	std::cout << "状态服务，grpc的Channel连接的服务器ip_port: " << ip_port << std::endl;
	std::shared_ptr<Channel> channel = grpc::CreateChannel(ip_port, grpc::SslCredentials(grpc::SslCredentialsOptions()));
	// StatusService::NewStub()函数，内部就是std::make_unique<StatusService::Stub>(channel, grpc::StubOptions())
	_stub = StatusService::NewStub(channel);
}


void StatusGrpcClient::getChatServer(uint64_t uid, std::shared_ptr<HttpConnection> connection, const std::function<void(std::shared_ptr<GetChatServerRsp>)>& callback)
{
	std::shared_ptr<ClientContext> context = std::make_shared<ClientContext>();
	std::shared_ptr<GetChatServerReq> request = std::make_shared<GetChatServerReq>();
	std::shared_ptr<GetChatServerRsp> reply = std::make_shared<GetChatServerRsp>();
	request->set_uid(uid);
	_stub->async()->GetChatServer(context.get(), request.get(), reply.get(), [connection, context, request, reply, callback](Status status)
	{
		std::cout << "async getChatServer grpc request finished" << std::endl;
		if (!status.ok())
			reply->set_error(ErrorCodes::RPCFailed);

		boost::asio::post(connection->getSocket().lowest_layer().get_executor(), [connection, reply, callback]
		{
			callback(reply);
		});
	});
}

