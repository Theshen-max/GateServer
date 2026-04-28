#ifndef VERIFYGRPCCLIENT_H
#define VERIFYGRPCCLIENT_H
/*
*	gRPC 官方结论：Stub 是线程安全的，可以多线程同时调用。
	gRPC 做并发，只是提高QPS，创建多个Channel来提高吞吐量
 */
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "const.h"
#include "Singleton.h"

class HttpConnection;

// 引入grpc相关类
using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
// 引入自定义proto文件的接口与资源类
using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService; // 接口

class VerifyGrpcClient: public Singleton<VerifyGrpcClient>, public std::enable_shared_from_this<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>;
public:
	void getVarifyCode(const std::string& email, std::shared_ptr<HttpConnection> connection, const std::function<void(std::shared_ptr<GetVarifyRsp>)>& callback);

private:
	VerifyGrpcClient();
	std::unique_ptr<VarifyService::Stub> _stub;
};
#endif //VERIFYGRPCCLIENT_H
