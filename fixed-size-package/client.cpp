/*
 * client.cpp
 *
 *  Created on: Jun 21, 2018
 *      Author: frank
 */

#include <iostream>
#include <string>
#include <functional>
#include <memory>
#include <random>
#include "asio.hpp"
#include "package.h"

using namespace std;

class PersudoQueue
{
public:
	// get a pkg, if the queue is empty, return false
	bool Recv(Package& pkg)
	{
//		std::uniform_int_distribution<int> d(0, 100);
//		if(d(dre_) < 50)
//		{
//			return false;
//		}

		std::uniform_int_distribution<uint32_t> d2(0, MAX_BODY_LEN);
		uint32_t random_len = d2(dre_);
		pkg.header.body_len = random_len;
		return true;
	}
public:
	static PersudoQueue& Instance()
	{
		static PersudoQueue q;
		return q;
	}
private:
	PersudoQueue() {}
	std::default_random_engine dre_;
};

// a connection to server
class Session: public std::enable_shared_from_this<Session>
{
public:
	Session(asio::io_context& io, const asio::ip::tcp::endpoint ep): //io_context_(io),
		socket_(io), server_endpoint_(ep) {}
	void Start()
	{
		auto self = shared_from_this();
		socket_.async_connect(server_endpoint_, [self, this](const asio::error_code& err) {
			if(err) // connect err
			{
				std::cout<<"connect err:"<<err.message()<<"\n";
				return;
			}

			// receive pkg from queue, and then send it
			SendQueuePkg();
		});
	}

	void SendQueuePkg()
	{
		auto self = shared_from_this();
		bool has_pkg = PersudoQueue::Instance().Recv(request_);
		if(! has_pkg) // the queue is empty now , sleep for a while
		{

		}
		else // send request to server, and then recv response
		{
			asio::async_write(socket_, asio::buffer(&request_, sizeof(PkgHeader)+request_.header.body_len),
					[self, this](const asio::error_code& err, size_t len) {
				if(err) // send error, maybe the server is down
				{
					std::cout<<"send request err:"<<err.message()<<"\n";
					socket_.close();
					return;
				}

				// now recv from server.
				// recv header first
				asio::async_read(socket_, asio::buffer(&response_, sizeof(PkgHeader)),
						[self, this](const asio::error_code& err, size_t len) {
					if(err) // read err
					{
						std::cout<<"recv header err: "<<err.message()<<"\n";
						socket_.close();
						return;
					}

					// recv body
					if(response_.header.body_len > MAX_BODY_LEN)
					{
						std::cout<<"invalid body len: "<<response_.header.body_len<<"\n";
						socket_.close();
						return;
					}

					asio::async_read(socket_, asio::buffer(&response_.body, response_.header.body_len),
							[self, this](const asio::error_code& err, size_t len) {
						if(err)
						{
							std::cout<<"recv body err: "<<err.message()<<"\n";
							socket_.close();
							return;
						}

						// recv response ok, procece the response
						std::cout<<"got response, body len: "<<response_.header.body_len<<"\n";

						// now get next pkg from queue
						SendQueuePkg();
					});
				});
			});
		}
	}
private:
//	asio::io_context& io_context_;
	asio::ip::tcp::socket socket_;
	asio::ip::tcp::endpoint server_endpoint_;
	Package request_;
	Package response_;
};

int main()
{
	asio::io_context io(1);
	asio::ip::tcp::endpoint ep(asio::ip::address::from_string("127.0.0.1"), 12345);
	std::shared_ptr<Session> p = std::make_shared<Session>(io, ep);
	p->Start();
	io.run();
	return 0;
}
