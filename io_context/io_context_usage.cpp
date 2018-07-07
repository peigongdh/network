/*
demonstrate usage of poll post run and poll
                      frank Jul 7, 2018
*/

#include <iostream>
#include <functional>
#include <thread>
#include <mutex>
#include <chrono>
#include "asio.hpp"

using namespace std;

int main()
{
	asio::io_context ios;

	short s = 0;

	auto f = ([&ios, &s]() {
		for(int i = 0; i < 10; ++i)
		{
			ios.post([i, &s]() { std::cout<<"post i="<<i<<"\n";
				++s;
			});
			ios.post([i, &s]() {
				std::cout<<"dispatch i ="<<i<<"\n";
				++s;
			});
			// std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	});

	while(++s)
	{
		ios.restart();
		ios.poll();
		ios.post([s]() { cout<<s; cout.flush(); });
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	return 0;
}


