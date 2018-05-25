/*
 simple async http client, retrieve html from a web server,
 without considering redirection.

                      frank May 24, 2018
*/
#include <regex>
#include "AsyncHttpClient2.h"

void AsyncHttpClient::Start(ahc_callback cb)
{
	finish_callback_ = cb;

	// parse url
	std::regex re("(https?)://([^/]+)((/.*)*)");
	std::smatch sm;
	if(! std::regex_match(url_, sm, re))
	{
		err_ = "invalid url";
		finish_callback_(shared_from_this());
		return;
	}

	protocol_ = sm[1];
	host_ = sm[2];
	path_ = sm[3].str() == "" ? "/" : sm[3].str();

	//std::cout<<"parsed, protocol="<<protocol_<<", host="<<host_<<", path="<<path_<<"\n";

    // asynchronous resolve host name to ip address
    resolver_.async_resolve(host_, protocol_,
        std::bind(&AsyncHttpClient::ResolveCallback, this,
        		_1, _2));
}

void AsyncHttpClient::ResolveCallback(asio::error_code const & err,
		tcp::resolver::results_type const & endpoints)
{
	if(! err)
	{
		//std::cout<<url_<<"finished resolving"<<"\n";
		// connect to the server
		asio::async_connect(socket_, endpoints,
	          std::bind(&AsyncHttpClient::ConnectCallback, this, _1));
	}
	else
	{
		err_ = err.message();
		finish_callback_(shared_from_this());
	}
}

void AsyncHttpClient::ConnectCallback(asio::error_code const & err)
{
	if(! err)
	{
		std::cout<<"connected"<<"\n";

		request_.append("GET ").append(path_).append(" HTTP/1.1\r\n");
		request_.append("Host: ").append(host_).append("\r\n");
		request_.append("Accept: */*\r\n");
		if(! cookie_.empty())
		{
			request_.append("Cookie: ").append(cookie_).append("\r\n");
		}
		request_.append("Connection: Close\r\n\r\n");

		asio::async_write(socket_, asio::buffer(&request_[0], request_.size()),
				std::bind(&AsyncHttpClient::SendRequestCallback, this, _1));
	}
	else
	{
		err_ = err.message();
		finish_callback_(shared_from_this());
	}
}

void AsyncHttpClient::SendRequestCallback(asio::error_code const & err)
{
	if(! err)
	{
		std::cout<<"finished sending request\n";

		// read all data, until the server finish sending and close the socket
		asio::async_read(socket_, asio::dynamic_buffer(response_),
				std::bind(&AsyncHttpClient::ServerResponseCallback, this, _1, _2));
	}
	else
	{
		err_ = err.message();
		finish_callback_(shared_from_this());
	}
}

void AsyncHttpClient::ServerResponseCallback(asio::error_code const & err, std::size_t bytes_transferred)
{
	if(! err)
	{
		std::cout<<response_.substr(0, bytes_transferred)<<"\n";
	}
	else if(err.value() == 2) // close by server
	{
		// now the whole html content is stored in response_, including response headers.
	}
	else
	{
		err_ = err.message();
	}

	// parse the whole response, check the status code. if the code is 302, handle the redirection.
	// check if the response header is complete.
	const size_t header_end = response_.find("\r\n\r\n"); // attention, not find_first_of
	if(header_end == string::npos) // no \r\n\r\n, the header is not complete
	{
		err_ = "header is not complete";
		finish_callback_(shared_from_this());
		return;
	}

	// parse the response header line by line
	const string response_header = response_.substr(0, header_end);
	//std::cout<<"response header:\n"<<response_header<<"\n";
	//std::cout<<"=======================================\n";
	std::vector<std::string> vec_headers;
	Split(response_header, "\r\n", vec_headers);
	// the first line is the status code, 200 is ok, 302 needs redirection
	const std::string& status_line = vec_headers[0];
	if(! ParseStatusLine(status_line))
	{
		finish_callback_(shared_from_this());
		return;
	}
	// parse other header lines
	for(size_t index = 1; index < vec_headers.size(); ++index)
	{
		const string& line = vec_headers[index];
		size_t pos = line.find(": ");
		string name = line.substr(0, pos);
		string value = line.substr(pos + 2, string::npos);
		//std::cout<<"get line: ("<<name<<"="<<value<<")\n";
		if(name == "Set-Cookie")
		{
			cookie_ = value;
		}
		else if(name == "Location")
		{
			location_ = value;
			url_ = value;
		}
	}

	if(status_code_ == 302 && ! location_.empty() && redirection_times_ < 2) // now do redirection
	{
		++ redirection_times_;
		std::cout<<status_line<<"\n";
		std::cout<<"302, now start redirection to "<<location_<<"\n";
		Start(finish_callback_);
	}
	else
	{
		finish_callback_(shared_from_this());
	}
}

void AsyncHttpClient::Split(const std::string& str, const std::string& delimiters,
        std::vector<std::string>& tokens)
{
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

bool AsyncHttpClient::ParseStatusLine(std::string const & status_line)
{
	std::regex re("HTTP/1\\.\\d (\\d+) [\\w ]+");
	std::smatch sm;
	if(! std::regex_match(status_line, sm, re))
	{
		err_ = "invalid status line";
		return false;
	}

	status_code_ = std::stoi(sm[1].str());
	return true;
}
