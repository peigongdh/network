/*
general-client.cpp
                      frank Jul 1, 2018
*/
#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include "asio.hpp"

using namespace std;

// one shot async client, exe one cmd, get the result, then destroy
class OneShotClient: public std::enable_shared_from_this<OneShotClient>
{
public:
	OneShotClient(asio::io_context& io, asio::ip::tcp::endpoint ep): io_(io), ep_(ep), socket_(io) {}

	void ExecuteCmd(const string& cmd)
	{
		send_buff_ = cmd; // copy and save
		socket_.async_connect(ep_, [this](const asio::error_code& err) {
			if(err)
			{
				cout<<"connect err: "<<err.message()<<"\n";
				return;
			}

			asio::async_write(socket_, asio::buffer(send_buff_), [this](const asio::error_code& err, size_t len) {
				if(err)
				{
					cout<<"send err: "<<err.message()<<"\n";
					return;
				}

				socket_.async_read_some(asio::buffer(recv_buff_), [this](const asio::error_code& err, size_t len) {
					cout<<"got result: "<<string(recv_buff_, len)<<"\n";
				});
			});
		});
	}
private:
	asio::io_context& io_;
	asio::ip::tcp::endpoint ep_; // server address
	asio::ip::tcp::socket socket_;
	string send_buff_;
	char recv_buff_[1024];
};

int main()
{
	asio::io_context io;
	asio::ip::tcp::endpoint ep(asio::ip::address::from_string("127.0.0.1"), 6379);

	auto p = std::make_shared<OneShotClient>(io, ep);
	p->ExecuteCmd("*2\r\n$3\r\nget\r\n$3\r\naaa\r\n");

	io.run();
	return 0;
}


