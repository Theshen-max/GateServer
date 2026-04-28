# GateServer

**分布式 IM 系统的入口安全网关**

## 项目目标
作为客户端接入系统的第一道屏障，负责处理短连接 HTTP/HTTPS 请求，执行用户注册、登录鉴权及微服务间的逻辑路由分发。

## 技术栈
- **网络模型**: Boost.Asio (异步多线程 Reactor)
- **协议解析**: Boost.Beast (HTTP/HTTPS)
- **远程调用**: gRPC (作为 Client 调用 VarifyServer/StatusServer)
- **存储交互**: Redis (验证码瞬时校验), MySQL (用户持久化凭据)

## 核心职责
- **业务路由**: 动态映射 URL（如 `/user_login`, `/get_varifyCode`）到对应的逻辑处理器。
- **负载均衡分配**: 在用户登录通过后，请求 `StatusServer` 为其分发当前负载最轻的 `ChatServer` 节点。
- **安全加固**: 集成 SSL/TLS 证书卸载，确保用户凭据在公网传输过程中的安全性。
- **异步解耦**: 采用单例模式管理各类 RPC 客户端，所有 IO 操作均不阻塞主事件循