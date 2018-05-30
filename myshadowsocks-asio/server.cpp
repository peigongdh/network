/*
		server.cpp
				frank May 30, 2018
*/
#include <iostream>
#include "session.h"
#include "config.h"

// usage: server -c proxy.conf
int main(int argc, const char* argv[])
{
	if(argc != 3 || std::string(argv[1]) != "-c")
	{
		std::cerr<<"usage: local -c proxy.conf"<<"\n";
		return 1;
	}

	if(! SSConfig::Instance().ReadConfig(argv[2]))
	{
		return 1;
	}

	std::cout<<"server listen on port: "<<SSConfig::Instance().ServerPort()<<"\n";
	asio::io_context io_context;
	Socks5Server ss(io_context, SSConfig::Instance().ServerEndpoint());
	ss.ServerStart();
	io_context.run();

	return 0;
}

