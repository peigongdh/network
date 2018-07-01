/*
    one shot redis client. connect to server, send cmd, recv response, and then disconnect.
    the client should be destroyed after each time.
                      frank Jul 1, 2018
*/
#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <map>
#include "asio.hpp"

using namespace std;
using std::placeholders::_1;
using std::placeholders::_2;

class ArrayItem;
class SimpleStringItem;
class ErrString;
class NumberItem;
class BulkString;

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

				socket_.async_read_some(asio::buffer(recv_buff_), std::bind(&OneShotClient::ReceiveResponse, this, _1, _2));
			});
		});
	}
private:
	void ReceiveResponse(const asio::error_code& err, size_t len)
	{
		if(err)
		{
			cout<<"recv err: "<<err.message()<<"\n";
			return;
		}

		// cout<<"got result: "<<string(recv_buff_, len)<<"\n";

		for(char* p = recv_buff_; p < recv_buff_ + len; ++p)
		{
			if(! parse_item_)
			{
				parse_item_ = CreateItem(*p);
				continue;
			}

			ParseResult pr = parse_item_->Feed(*p);
			if(pr == ParseResult::PR_FINISHED)
			{
				cout<<"get parse result: "<<parse_item_->ToString()<<"\n";
				return;
			}
			else if(pr == ParseResult::PR_ERROR)
			{
				cout<<"parse error\n";
				return;
			}
		}

		// not finished, recv again
		socket_.async_read_some(asio::buffer(recv_buff_), std::bind(&OneShotClient::ReceiveResponse, this, _1, _2));
	}

	// factory method
	std::shared_ptr<AbstractReplyItem> CreateItem(char c)
	{
		std::map<char, std::function<AbstractReplyItem*()>> factory_func = {
				{'*', []() { return new ArrayItem; } },
				{'+', []() { return new SimpleStringItem; } },
				{'-', []() { return new ErrString; } },
				{':', []() { return new NumberItem; } },
				{'$', []() { return new BulkString; } },
		};

		auto it = factory_func.find(c);
		if(it != factory_func.end())
		{
			return std::shared_ptr<AbstractReplyItem>(it->second());
		}

		return nullptr;
	}
private:
	asio::io_context& io_;
	asio::ip::tcp::endpoint ep_; // server address
	asio::ip::tcp::socket socket_;
	string send_buff_;
	char recv_buff_[1024];
	std::shared_ptr<AbstractReplyItem> parse_item_;
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


