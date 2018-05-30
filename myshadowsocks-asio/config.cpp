/*config.cpp
				frank May 30, 2018
*/
#include <iostream>
#include <regex>
#include <fstream>
#include <string>
#include "config.h"

using namespace std;

bool SSConfig::ReadConfig(std::string file_name)
{
	ifstream f(file_name);
	if(! f)
	{
		std::cerr<<"failed to open file: "<<file_name<<"\n";
		return false;
	}

	// read aaa = bbb
	std::regex reg("([\\w_]+)[ ]*=[ ]*([^ ]+)[ ]*");

	std::string line;
	while(std::getline(f, line))
	{
		std::smatch match;
		if(std::regex_match(line, match, reg))
		{
			//cout<<match[1].str()<<" = "<<match[2].str()<<"\n";
			string name = match[1].str();
			string value = match[2].str();

			//std::cout<<"reading config:"<<name<<" = "<<value<<"\n";

			if(name == "server_ip")
			{
				server_ip = value;
			}
			else if(name == "server_port")
			{
				server_port = std::stoi(value);
			}
			else if(name == "local_port")
			{
				local_port = std::stoi(value);
			}
			else
			{
				std::cerr<<"invalid config line: "<<line<<"\n";
				return false;
			}
		}
	}

	std::cout<<"init, server_ip="<<server_ip<<", server_port="<<server_port<<"\n";
	server_endpoint = asio::ip::tcp::endpoint(asio::ip::address::from_string(server_ip), server_port);

	return true;
}
