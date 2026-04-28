#ifndef ASIOIOSERVICEPOOL_H
#define ASIOIOSERVICEPOOL_H

#include <boost/asio/io_context.hpp>
#include <vector>
#include <thread>
#include <atomic>
#include "Singleton.h"

class AsioIOServicePool : public Singleton<AsioIOServicePool>
{
	friend class Singleton<AsioIOServicePool>;
public:
	// 旧版本里叫io_service，后面改名为io_context
	using IOService = boost::asio::io_context;
	using Work = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

	~AsioIOServicePool();
	// 使用round-robin方式返回一个io_context
	IOService& getIOService();
	void stop();

private:
	AsioIOServicePool(size_t size = 2);
	std::vector<IOService> _ioServices;		// io_context的数量
	std::vector<std::jthread> _jthreads;		// 对应线程的数量
	std::vector<Work> _workers;		//	对应管理io_context的executor_work_guard的数量
	std::size_t _nextIOService;		// 下一个io_context的序号
	std::atomic<bool> _isStopped = false;
};

#endif //ASIOIOSERVICEPOOL_H
