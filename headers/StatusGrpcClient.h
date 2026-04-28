//
// Created by 27044 on 26-3-17.
//

#ifndef STATUSGRPCCLIENT_H
#define STATUSGRPCCLIENT_H

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "message.pb.h"
#include "const.h"
#include "ConfigMgr.h"
#include "Singleton.h"

// 引入grpc相关类
using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
// 引入自定义proto文件的接口与资源类
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService; // 接口

// 状态上下文池（后面采用异步grpc接口，若无特别高并发需求，单个Channel足以）
class StatusConPool {};
class HttpConnection;
class StatusGrpcClient: public Singleton<StatusGrpcClient>
{
	friend Singleton<StatusGrpcClient>;
public:
	// 根据负载均衡算法，判断该用户（uid）应该在哪个ChatServer里，并返回相关信息
	void getChatServer(uint64_t uid, std::shared_ptr<HttpConnection> connection, const std::function<void(std::shared_ptr<GetChatServerRsp>)>& callback);
private:
	StatusGrpcClient();
	std::unique_ptr<StatusService::Stub> _stub;
};

#endif //STATUSGRPCCLIENT_H
