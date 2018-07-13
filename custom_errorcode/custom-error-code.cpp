/*
custom-error-code.cpp
                      frank Jul 12, 2018
*/
#include <type_traits>
#include <iostream>
#include <asio.hpp>

using namespace std;

int main()
{
	static_assert(std::is_same<asio::error_category, std::error_category>::value, "not same");
	asio::error_code err;
	return 0;
}


