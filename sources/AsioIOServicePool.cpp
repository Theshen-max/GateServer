#include "../headers/AsioIOServicePool.h"

AsioIOServicePool::AsioIOServicePool(size_t size):
	_ioServices(size),
	_nextIOService(0)
{
	for (int i = 0; i < size; ++i)
		_workers.emplace_back(_ioServices[i].get_executor());

	for (int i = 0; i < size; ++i)
	{
		_jthreads.emplace_back([this, i]
		{
			_ioServices[i].run();
		});
	}
}

AsioIOServicePool::~AsioIOServicePool()
{
	stop();
}

AsioIOServicePool::IOService& AsioIOServicePool::getIOService()
{
	boost::asio::io_context& ioContext = _ioServices[_nextIOService++];
	_nextIOService %= _ioServices.size();
	return ioContext;
}

void AsioIOServicePool::stop()
{
	if (!_isStopped.load(std::memory_order_relaxed))
	{
		_isStopped.store(true, std::memory_order_relaxed);

		// 仅仅work.reset只能标志这io_context在run()后，队列为空直接返回，并不代表直接让其从run()状态退出
		for (int i = 0; i < _workers.size(); ++i)
		{
			_workers[i].reset();
			_ioServices[i].stop();
		}
	}
}


