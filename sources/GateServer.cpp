#include <iostream>
#include "../headers/CServer.h"
#include "../headers/ConfigMgr.h"

int main()
{
	try
	{
		net::io_context ioc{1};
		boost::asio::signal_set signals{ioc, SIGINT, SIGTERM};
		signals.async_wait([&ioc](const boost::system::error_code& ec, int signal_number)
		{
			if (ec) return; // 出错
			ioc.stop();
		});

		unsigned short gatePort;
		std::string gatePortStr = ConfigMgr::getInstance()["GateServer"]["Port"];
		std::from_chars(gatePortStr.data(), gatePortStr.data() + gatePortStr.size(), gatePort);

		std::make_shared<CServer>(ioc, gatePort)->start();
		std::cout << "Gate Server listen on port: " << gatePort << std::endl;
		ioc.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE; // 1
	}
	return EXIT_SUCCESS; // 0
}