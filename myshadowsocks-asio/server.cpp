/*
		server.cpp
				frank May 30, 2018
*/
#include <iostream>
#include "session.h"
#include "config.h"

int main()
{
	std::cout<<"server listen on port: "<<SSConfig::Instance().server_port<<"\n";
	asio::io_context io_context;
	Socks5Server ss(io_context, SSConfig::Instance().server_endpoint);
	ss.ServerStart();
	io_context.run();

	return 0;
}

