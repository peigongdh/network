/*
	config.h
				frank May 30, 2018
*/

#ifndef MYSHADOWSOCKS_ASIO_CONFIG_H_
#define MYSHADOWSOCKS_ASIO_CONFIG_H_

#include <string>
#include "asio.hpp"

class SSConfig
{
public:
	std::string server_ip;
	short server_port;
	short local_port; // local server is 127.0.0.1
	asio::ip::tcp::endpoint server_endpoint; // ip::tcp::endpoint(ip::address::from_string(...), ...)
	asio::ip::tcp::endpoint& ServerEndpoint()
	{
		return server_endpoint;
	}
public:
	static SSConfig& Instance()
	{
		static SSConfig instance;
		return instance;
	}
private:
	SSConfig() {}
};



#endif /* MYSHADOWSOCKS_ASIO_CONFIG_H_ */
