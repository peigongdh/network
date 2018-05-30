/*local.cpp
				frank May 30, 2018
*/
#include <iostream>
#include "encrypt.h"
#include "config.h"
#include "session.h"

int main()
{
	std::cout<<"local listen at port:"<<SSConfig::Instance().local_port<<"\n";
	asio::ip::tcp::endpoint ep(asio::ip::tcp::v4(), SSConfig::Instance().local_port);
	asio::io_context io_context;
	Socks5Server ss(io_context, ep);
	ss.LocalStart();
	io_context.run();
	return 0;
}


