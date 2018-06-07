/*
	simple https client
                      frank Jun 7, 2018
*/
#include <iostream>
#include <string>
#include <map>
#include <asio.hpp>
#include "AsyncHttpsClient.h"

using namespace std;
using namespace asio;

void OnFinish(std::shared_ptr<AsyncHttpsClient> client)
{
	std::cout<<"call back in finish, err: "<<client->Err()<<"\n";
}

int main(int argc, const char* argv[])
{
	if(argc < 3)
	{
		cout<<"usage: https-client host path\n";
		return -1;
	}

	std::map<string, std::shared_ptr<AsyncHttpsClient>> urls = {
			{"http://www.qq.com", nullptr},
			{"http://www.guanggoo.com/t/32230#reply87", nullptr},
			{"http://en.cppreference.com/w/cpp/regex/match_results", nullptr},
			};

	asio::ssl::context ctx(asio::ssl::context::sslv23);
	ctx.set_default_verify_paths();
	asio::io_context ioc;

    for(auto&& it : urls)
    {
    		it.second = std::make_shared<AsyncHttpsClient>(ioc, ctx, it.first);
    		it.second->Start(OnFinish);
    }

    ioc.run();

	return 0;
}


