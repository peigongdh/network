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
	bool ReadConfig(std::string file_name);
public:
	asio::ip::tcp::endpoint& ServerEndpoint()
	{
		return server_endpoint;
	}

	short LocalPort() { return local_port; }

	short ServerPort() { return server_port; }

	size_t ShiftSteps() { return shift_steps; }
public:
	static SSConfig& Instance()
	{
		static SSConfig instance;
		return instance;
	}
private:
	SSConfig() {}
	SSConfig(const SSConfig&) = delete;
	SSConfig& operator=(const SSConfig&) = delete;
	SSConfig(SSConfig&&) = delete;
	SSConfig& operator=(SSConfig&&) = delete;
private:
	std::string server_ip;
	short server_port;
	short local_port; // local server is 127.0.0.1
	size_t shift_steps; // for crypting
	asio::ip::tcp::endpoint server_endpoint; // ip::tcp::endpoint(ip::address::from_string(...), ...)
};



#endif /* MYSHADOWSOCKS_ASIO_CONFIG_H_ */
