/**
   simple asynchronous http client. with redirection implemented.
		frank May 24, 2018
*/

#ifndef ASYNCHTTPCLIENT_ASYNCHTTPCLIENT_H_
#define ASYNCHTTPCLIENT_ASYNCHTTPCLIENT_H_

#include <functional>
#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <asio.hpp>

using std::vector;
using std::string;
using std::pair;
using asio::ip::tcp;
using std::placeholders::_1;
using std::placeholders::_2;

class AsyncHttpClient: public std::enable_shared_from_this<AsyncHttpClient>
{
	using ahc_callback = std::function<void(std::shared_ptr<AsyncHttpClient>)>;
	enum HCStatus {
		INIT,
		RESOLVING,
		CONNECTING,
		SENDING_REQUEST,
		RECEIVING_RESPONSE,
		FINISHED
	};
public:
	AsyncHttpClient(asio::io_context& io_context, string url):
		status_code_(-1), redirection_times_(0),
		status_(INIT), io_context_(io_context),
		resolver_(io_context), socket_(io_context), url_(url)
	{
	}
public:
	void Start(ahc_callback cb);
	int Result(); // 0 for ok, otherwise failed
	string Err() { return err_; } // if Result returns non zero, Err return detail message
	string Response() { return response_; }
private:
	// callback of resolve
	void ResolveCallback(const asio::error_code& err,
		const tcp::resolver::results_type& endpoints);
	// callback of connect
	void ConnectCallback(const asio::error_code& err);
	// callback of sending request
	void SendRequestCallback(const asio::error_code& err);
	// got the first line of server response, example:
	void ServerResponseCallback(const asio::error_code& err, std::size_t bytes_transferred);
private:
	void Split(const std::string& str, const std::string& delimiters,
	        std::vector<std::string>& tokens); // split string by delim
	bool ParseStatusLine(const std::string& status_line); // should be like HTTP/1.n nnn blabla, return false if literal error
private:
	string protocol_; // http or https
	string host_; // such as www.qq.com, 129.168.0.1
	string path_; // url path, such as /, /index.html
	int status_code_; // in response, such as 200, 302 etc
	string cookie_;
	string location_; //redirection new location
	int redirection_times_; // in case of redirecting too much
private:
	HCStatus status_;
	asio::io_context& io_context_;
	tcp::resolver resolver_;
	tcp::socket socket_;
	string url_;
	vector<pair<string, string>> request_headers_;
	string request_;
	vector<pair<string, string>> response_headers_;
	string response_;
	ahc_callback finish_callback_;
	string err_;
};


#endif /* ASYNCHTTPCLIENT_ASYNCHTTPCLIENT_H_ */
