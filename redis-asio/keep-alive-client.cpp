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

enum class ParseResult
{
	PR_CONTINUE,
	PR_FINISHED,
	PR_ERROR
};

class AbstractReplyItem: public std::enable_shared_from_this<AbstractReplyItem>
{
public:
	virtual string ToString() = 0;
	virtual ParseResult Feed(char c) = 0;
	virtual ~AbstractReplyItem() {}
};

// array,  start with *
class ArrayItem: public AbstractReplyItem
{
public:
	string ToString() override
	{
		return "";
	}

	ParseResult Feed(char c) override
	{
		return ParseResult::PR_ERROR;
	}
private:
	vector<std::shared_ptr<AbstractReplyItem>> items_;
};

//simple line of string, start with +, -, :
class OneLineString: public AbstractReplyItem
{
	enum class OLS_STATUS { PARSING_STRING,
		EXPECT_LF // got \r, expect \n
	};
public:
	string ToString() override
	{
		return content_;
	}

	ParseResult Feed(char c) override
	{
		switch(status_)
		{
		case OLS_STATUS::PARSING_STRING:
			if(c == '\r')
			{
				status_ = OLS_STATUS::EXPECT_LF;
			}
			else
			{
				content_.push_back(c);
			}
			return ParseResult::PR_CONTINUE;
			break;
		case OLS_STATUS::EXPECT_LF:
			if(c == '\n')
			{
				return ParseResult::PR_FINISHED;
			}
			else
			{
				return ParseResult::PR_ERROR;
			}
			break;
		}

		return ParseResult::PR_ERROR; // impossible
	}
protected:
	OLS_STATUS status_ = OLS_STATUS::PARSING_STRING;
	string content_;
};

// start with +
class SimpleStringItem: public OneLineString
{
};

// start with -
class ErrString: public OneLineString
{
};

// string with length, start with $
class BulkString: public AbstractReplyItem
{
	enum class BULK_STATUS {
		BS_EXPECT_LENGTH, // first line, 0 for empty string, -1 for null
		BS_EXPECT_LENGTH_LF, // first line of \n
		BS_EXPECT_CONTENT,
		BS_EXPECT_CONTENT_LF // next line of \n
	};
public:
	string ToString() override
	{
		return content_;
	}

	ParseResult Feed(char c) override
	{
		switch(status_)
		{
		case BULK_STATUS::BS_EXPECT_LENGTH:
			if(c == '-' or std::isdigit(c))
			{
				length_line_.push_back(c);
			}
			else if(c == '\r')
			{
				status_ = BULK_STATUS::BS_EXPECT_LENGTH_LF;
			}
			else
			{
				// error
			}
			return ParseResult::PR_CONTINUE;
			break;
		case BULK_STATUS::BS_EXPECT_LENGTH_LF:
			if(c != '\n')
			{
				// err
			}
			length_ = std::stoi(length_line_);
			if(length_ == 0 || length_ == -1)
			{
				return ParseResult::PR_FINISHED;
			}
			else if(length_ < 0)
			{
				// err
			}
			status_ = BULK_STATUS::BS_EXPECT_CONTENT;
			return ParseResult::PR_CONTINUE;
			break;
		case BULK_STATUS::BS_EXPECT_CONTENT:
			if(c == '\r')
			{
				status_ = BULK_STATUS::BS_EXPECT_CONTENT_LF;
			}
			else
			{
				content_.push_back(c);
			}
			return ParseResult::PR_CONTINUE;
			break;
		case BULK_STATUS::BS_EXPECT_CONTENT_LF:
			if(c != '\n')
			{
				//err
			}
			return ParseResult::PR_FINISHED;
			break;
		}
		return ParseResult::PR_ERROR;
	}
private:
	BULK_STATUS status_ = BULK_STATUS::BS_EXPECT_LENGTH;
	int length_ = 0;
	string length_line_; // length, save temporarily as a string
	string content_;
};

class NumberItem: public OneLineString
{
public:
	ParseResult Feed(char c) override
	{
		ParseResult pr = OneLineString::Feed(c);
		if(pr == ParseResult::PR_FINISHED)
		{
			number_ = std::stoi(content_);
		}
		return pr;
	}
private:
	int number_ = -1;
};

class RedisCli;
std::list<std::shared_ptr<RedisCli>> free_clients; // available clients, others are busy

class RedisCli: public std::enable_shared_from_this<RedisCli>
{
public:
	RedisCli(int id, asio::io_context& io, asio::ip::tcp::endpoint ep):
		id_(id), io_(io), socket_(io_), server_ep_(ep) {}

	void Start()
	{
		auto self = shared_from_this();
		socket_.async_connect(server_ep_, [self, this](const asio::error_code& err) {
			if(err)
			{
				cout<<"connect err: "<<err.message()<<"\n";
			}

			cout<<"id="<<id_<<" connected...\n";
			free_clients.push_back(self);
		});
	}

	int Id() const { return id_; }

	void SendAndReceive(const string& cmd)
	{
		send_buff_ = cmd;
		asio::async_write(socket_, asio::buffer(send_buff_), [this](const asio::error_code& err, size_t len) {
			cout<<"sent...\n";

			if(err)
			{
				cout<<"send err:"<<err.message()<<"\n";
				return;
			}

			socket_.async_read_some(asio::buffer(recv_buff_), std::bind(&RedisCli::RecvData, this,
					std::placeholders::_1, std::placeholders::_2));
		});
	}

	void RecvData(const asio::error_code& err, size_t len)
	{
		cout<<"recving data...\n";
		if(err)
		{
			cout<<"recv err: "<<err.message()<<"\n";
			return;
		}

		for(char* p = recv_buff_; p < recv_buff_ + len; ++p)
		{
			if(! item_)
			{
				switch(*p)
				{
				case '*':
					item_ = std::make_shared<ArrayItem>();
					break;
				case '+':
					item_ = std::make_shared<SimpleStringItem>();
					break;
				case '-':
					item_ = std::make_shared<ErrString>();
					break;
				case '$':
					item_ = std::make_shared<BulkString>();
					break;
				case ':':
					item_ = std::make_shared<NumberItem>();
					break;
				}
			}
			else
			{
				ParseResult pr = item_->Feed(*p);
				if(pr == ParseResult::PR_FINISHED)
				{
					cout<<item_->ToString()<<"\n";
				}
			}
		}
	}
private:
	int id_;
	asio::io_context& io_;
	asio::ip::tcp::socket socket_;
	asio::ip::tcp::endpoint server_ep_;
	string send_buff_;
	char recv_buff_[1024];
	std::shared_ptr<AbstractReplyItem> item_;
};

//simulate a queue and the queue generates cmd.
//pretend sometimes the queue is empty.
class RequestIter
{
public:
	RequestIter()
	{
		commands_ = {"*2\r\n$3\r\nget\r\n$3\r\naaa\r\n",
			//"*3\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$7\r\nmyvalue\r\n",
			//"*3\r\n$3\r\nset\r\n$3\r\naaa\r\n$3\r\nbbb\r\n",
			//"*2\r\n$3\r\nget\r\n$3\r\naaa\r\n",
			//"*3\r\n$3\r\nset\r\nccc\r\n$5\r\nddddd\r\n"
				};
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
		cmd = commands_[0];//commands_[di(dre_)];
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

//	std::function<void(std::shared_ptr<RedisCli>, const asio::error_code&)> finish;
//	finish = [&finish, &io_context, &server_endpoint](std::shared_ptr<RedisCli> p, const asio::error_code& err) {
//		std::cout<<"client["<<p->Id()<<"] finish its work\n";
//		/*
//		if(!err)
//		{
//			free_clients.push_back(p);
//		}
//		else
//		{
//			std::cout<<"client["<<p->Id()<<"] get err: "<<err.message()<<"\n";
//			// make a new client with the same id
//			std::shared_ptr<RedisCli> new_ptr = std::make_shared<RedisCli>(p->Id(), io_context, server_endpoint, finish);
//			free_clients.push_back(new_ptr);
//		}*/
//	};

	RequestIter ri;

	for(int i = 1; i <= 1; ++i)
	{
		std::shared_ptr<RedisCli> p = std::make_shared<RedisCli>(i, io_context, server_endpoint);
		p->Start();
	}

	cout<<"start...\n";

	while(1)
	{
		std::string cmd;
		if(!free_clients.empty() && ri.NextCommand(cmd))
		{
			std::shared_ptr<RedisCli> p = free_clients.front();
			free_clients.pop_front();
			p->SendAndReceive(cmd);
		}
		io_context.run();// run_for(std::chrono::milliseconds(10));
	}

	return 0;
}

#endif /* REDIS_ASIO_REDIS_CLIENT_CPP_ */
