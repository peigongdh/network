/*config.cpp
				frank May 30, 2018
*/
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "config.h"

using namespace std;

void StringSplit(const std::string& str, const std::string& delimiters,
        std::vector<std::string>& tokens)
{
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    string::size_type pos = str.find_first_of(delimiters, lastPos);
    while (string::npos != pos || string::npos != lastPos)
    {
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        lastPos = str.find_first_not_of(delimiters, pos);
        pos = str.find_first_of(delimiters, lastPos);
    }
}

std::string& StringTrim(std::string &str)
{
    if (str.empty())
    {
        return str;
    }

    str.erase(0, str.find_first_not_of(" "));
    str.erase(str.find_last_not_of(" ") + 1);
    return str;
}

bool SSConfig::ReadConfig(std::string file_name)
{
	ifstream f(file_name);
	if(! f)
	{
		std::cerr<<"failed to open file: "<<file_name<<"\n";
		return false;
	}

	std::map<string, string> values;
	std::string line;
	while(std::getline(f, line))
	{
		line = StringTrim(line);
		if(line[0] == '#' || (line[0] == '/' && line[1] == '/'))
		{
			continue;
		}

		std::vector<string> vs;
		StringSplit(line, " ", vs);

		if(vs.size() != 3)
		{
			continue;
		}

		string name = vs[0];
		string value = vs[2];

		values[name] = value;
	}

	server_ip = values["server_ip"];
	server_port = std::stoi(values["server_port"]);
	local_port = std::stoi(values["local_port"]);
	shift_steps = std::stoi(values["shift_steps"]);

	std::cout<<"init, server_ip="<<server_ip<<", server_port="<<server_port<<", local_port="<<local_port<<", shift_steps="<<shift_steps<<"\n";
	server_endpoint = asio::ip::tcp::endpoint(asio::ip::address::from_string(server_ip), server_port);

	return true;
}
