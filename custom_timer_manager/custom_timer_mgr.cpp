/*
 * custom_timer_mgr.cpp
 *
 *  Created on: Jul 19, 2018
 *      Author: frank
 */
#include <iostream>
#include "asio.hpp"
#include "timer_mgr2.h"

int main()
{
	asio::io_context io;
	asio::steady_timer t(io, asio::chrono::microseconds(500));
	t.async_wait([](const asio::error_code&) { std::cout<<"it's time.\n"; } );
	io.run();
	return 0;
}



