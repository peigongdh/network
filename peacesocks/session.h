/*
	session.h
				frank May 30, 2018
*/

#ifndef MYSHADOWSOCKS_ASIO_SESSION_H_
#define MYSHADOWSOCKS_ASIO_SESSION_H_

#include <iostream>
#include <memory>
#include "asio.hpp"

//using namespace asio;

// client ---data---> local ---encrypted data---> server ---decrypted data---> remote

class SSSession : public std::enable_shared_from_this<SSSession>
{
	friend class Socks5Server;
	static const int BUFFER_SIZE = 512;
public:
	SSSession(asio::io_context& ioc):local_socket_(ioc), remote_socket_(ioc), resolver_(ioc) {}
	void LocalStart(); // only called in the "local" module
	void ServerStart(); // only called in the "server" module
private:
	using buffer_type = char [BUFFER_SIZE];
	//recv data from sock1, into buff, then encrypt, then send to sock2
	void RecvEncryptSend(asio::ip::tcp::socket& sock1, asio::ip::tcp::socket& sock2, buffer_type& buff);
	//recv data from sock1, into buff, then decrypt, then send to sock2
	void RecvDecryptSend(asio::ip::tcp::socket& sock1, asio::ip::tcp::socket& sock2, buffer_type& buff);
	void Close(const asio::error_code& ec);
private:
	asio::ip::tcp::socket local_socket_; // listen as server for client, or for local
	asio::ip::tcp::socket remote_socket_;
	asio::ip::tcp::resolver resolver_;
private:
	std::string dest_host_;
	std::string dest_port_;
	char local_buffer_[BUFFER_SIZE]; // receive from local_socket_
	char remote_buffer_[BUFFER_SIZE]; // receive from remote_socket_
};

class Socks5Server
{
public:
	Socks5Server(asio::io_context& ioc, const asio::ip::tcp::endpoint& ep) : io_context_(ioc), acceptor_(ioc, ep) {}
	void LocalStart();
	void ServerStart();
private:
	asio::io_context& io_context_;
	asio::ip::tcp::acceptor acceptor_;
};

#endif /* MYSHADOWSOCKS_ASIO_SESSION_H_ */
