/**
 	a general redis client.

		frank Jun 21, 2018
*/

#ifndef REDIS_ASIO_REDIS_CLIENT_CPP_
#define REDIS_ASIO_REDIS_CLIENT_CPP_

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <memory>
#include <random>
#include <list>
#include "asio.hpp"

using namespace std;

class AbstractReplyItem
{
public:
	virtual string ToString() = 0;
	virtual int Feed(char c) = 0;
	virtual ~AbstractReplyItem() {}
};

// array,  start with *
class ArrayItem: public AbstractReplyItem, public std::enable_shared_from_this<ArrayItem>
{
private:
	vector<std::shared_ptr<AbstractReplyItem>> items_;
};

// start with +
class SimpleStringItem: public AbstractReplyItem, public std::enable_shared_from_this<ArrayItem>
{
private:
	string content_;
};

// start with -
class ErrString: public AbstractReplyItem, public std::enable_shared_from_this<ArrayItem>
{
private:
	string content_;
};

// string with length, start with $
class BulkString: public AbstractReplyItem, public std::enable_shared_from_this<ArrayItem>
{
private:
	int length_;
	string content;
};

class NumberItem: public AbstractReplyItem, public std::enable_shared_from_this<ArrayItem>
{
private:
	int number_;
};

class RedisCli: public std::enable_shared_from_this<RedisCli>
{
	using callback = std::function<void(std::shared_ptr<RedisCli>, const asio::error_code&)>;
	enum CLIENT_STATUS {
		CS_INITED = 0,
		CS_CONNECTING,
		CS_CONNECTED, // ready for send and recv data
		CS_SENDING,
		CS_RECEIVING
	};
public:
	RedisCli(int id, asio::io_context& io_context, asio::ip::tcp::endpoint server_endpoint, callback cb): id_(id),
		status_(CS_INITED), io_context_(io_context), socket_(io_context_),
		server_endpoint_(server_endpoint), do_finish_(cb) {}

	int Id() const { return id_;}

	void Start()
	{
		status_ = CS_CONNECTING;
		auto self = shared_from_this();
		socket_.async_connect(server_endpoint_, [this, self](const asio::error_code& err) {
			if(err) // connect error
			{
				do_finish_(self, err);
				return;
			}

			status_ = CS_CONNECTED;
		});
	}

	void SendAndReceive(const std::string& line)
	{
		status_ = CS_SENDING;
		send_buff_ = line;
		auto self = shared_from_this();
		asio::async_write(socket_, asio::buffer(line), [this, self](const asio::error_code& err, size_t len){
			if(err) // send err
			{
				do_finish_(self, err);
				return;
			}

			status_ = CS_RECEIVING;
			socket_.async_read_some(asio::buffer(recv_buff_),
					std::bind(&RedisCli::DoReceive, this, std::placeholders::_1, std::placeholders::_2));
		});
	}
private:
	// recv response, parse by state machine
	void DoReceive(const asio::error_code& err, size_t len)
	{
		// redis protocol: https://redis.io/topics/protocol
		//* bulk, show as array. *-1 is empty array
		//+ simple string
		//- err string
		//$ string with length. $0 is empty string, $-1 is nil
		//: number
	}
private:
	int id_;
	CLIENT_STATUS status_;
	asio::io_context& io_context_;
	asio::ip::tcp::socket socket_;
	std::string send_buff_;
	char recv_buff_[1024];
	int line_count_; // receive multi line
	std::string recv_data_;
	asio::ip::tcp::endpoint server_endpoint_;
	callback do_finish_; // callback after one shot of sending and receiving
};

//simulate a queue and the queue generates cmd.
//pretend sometimes the queue is empty.
class RequestIter
{
public:
	RequestIter()
	{
		commands_ = {"*3\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$7\r\nmyvalue\r\n",
			"*3\r\n$3\r\nset\r\naaa\r\n$3\r\nbbb\r\n",
			"*2\r\n$3\r\nget\r\n$3\r\naaa\r\n",
			"*3\r\n$3\r\nset\r\nccc\r\n$5\r\nddddd\r\n"};
	}

	// get next command, if no command at the moment, return false
	bool NextCommand(std::string& cmd)
	{
		std::uniform_int_distribution<int> d(0, 100);
		if(d(dre_) < 80)
		{
			return false;
		}

		std::uniform_int_distribution<int> di(0, commands_.size()-1);
		cmd = commands_[di(dre_)];
		return true;
	}
private:
	std::vector<std::string> commands_;
	std::default_random_engine dre_;
};

int main()
{
	asio::ip::tcp::endpoint server_endpoint(asio::ip::address::from_string("127.0.0.1"), 6379);
	asio::io_context io_context(1);

	std::list<std::shared_ptr<RedisCli>> free_clients; // available clients, others are busy
	std::function<void(std::shared_ptr<RedisCli>, const asio::error_code&)> finish;
	finish = [&finish, &free_clients, &io_context, &server_endpoint](std::shared_ptr<RedisCli> p, const asio::error_code& err) {
		std::cout<<"client["<<p->Id()<<"] finish its work\n";
		if(!err)
		{
			free_clients.push_back(p);
		}
		else
		{
			std::cout<<"client["<<p->Id()<<"] get err: "<<err.message()<<"\n";
			// make a new client with the same id
			std::shared_ptr<RedisCli> new_ptr = std::make_shared<RedisCli>(p->Id(), io_context, server_endpoint, finish);
			free_clients.push_back(new_ptr);
		}
	};

	RequestIter ri;

	for(int i = 1; i <= 10; ++i)
	{
		std::shared_ptr<RedisCli> p = std::make_shared<RedisCli>(i, io_context, server_endpoint, finish);
		free_clients.push_back(p);
		p->Start();
	}

	while(1)
	{
		std::string cmd;
		if(!free_clients.empty() && ri.NextCommand(cmd))
		{
			std::shared_ptr<RedisCli> p = free_clients.front();
			free_clients.pop_front();
			p->SendAndReceive(cmd);
		}
		io_context.run_for(std::chrono::milliseconds(10));
	}

	return 0;
}

#endif /* REDIS_ASIO_REDIS_CLIENT_CPP_ */
