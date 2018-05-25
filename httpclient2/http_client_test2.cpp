/*
http_client_test.cpp
                      frank May 24, 2018
*/
#include <iostream>
#include <map>
#include <string>
#include <memory>

#include "AsyncHttpClient2.h"

using namespace std;

void OnFinish(std::shared_ptr<AsyncHttpClient> client)
{
	std::cout<<"call back in finish, err: "<<client->Err()<<"\n";
	std::cout<<"html:\n";
	std::cout<<client->Response()<<"\n";
}

int main()
{
	std::map<string, std::shared_ptr<AsyncHttpClient>> urls = {
			{"http://km.oa.com", nullptr},
			//{"http://www.guanggoo.com/t/32230#reply87", nullptr},
			//{"http://en.cppreference.com/w/cpp/regex/match_results", nullptr},
			};
    asio::io_context io_context;
    for(auto&& it : urls)
    {
    		it.second = std::make_shared<AsyncHttpClient>(io_context, it.first);
    		it.second->Start(OnFinish);
    }

    io_context.run();

    return 0;
}



