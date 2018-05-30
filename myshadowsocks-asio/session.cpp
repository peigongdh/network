/*session.cpp
				frank May 30, 2018
*/

#include "session.h"
#include "encrypt.h"
#include "config.h"

#define LogErrorCode(msg, ec) do { \
		std::cerr<<"[<<local_fd = "<< local_socket_.native_handle() <<"], line="<<__LINE__<<", "<<msg  ; \
		if(ec) \
		{ \
			std::cerr<<" err: "<<ec.message(); \
		} \
		std::cerr<<"\n"; \
		}while(0);

#define LogErr(msg) do { \
		std::cerr<<"[<<local_fd = "<< local_socket_.native_handle() <<"], line="<<__LINE__<<", "<<msg<<"\n"; \
		}while(0);

#define CLOSE_EC else { Close(ec); }

void SSSession::LocalHandShake()
{
}

void SSSession::ServerHandShake()
{
}

void SSSession::TransferData(asio::ip::tcp::socket& sock1,
		asio::ip::tcp::socket& sock2, buffer_type& buff1, buffer_type& buff2)
{
}

void SSSession::LocalStart()
{
	auto self = shared_from_this();
	local_socket_.async_receive(asio::buffer(local_buffer_), [self, this](const asio::error_code& ec, size_t len) {
		if(! ec) // expeted to be 05 01 00
		{
			//check the value
			if(len < 3 || local_buffer_[0] != 0x05 || local_buffer_[1] != 0x01 || local_buffer_[2] != 0x00)
			{
				LogErr("recv 05 01 00 err");
				return;
			}

			// connect to server
			remote_socket_.async_connect(SSConfig::Instance().server_endpoint, [self, this](const asio::error_code& ec) {
				if(! ec)
				{
					// connected to server, send encrypted 05 01 00
					Cypher::Instance().Encrypt(local_buffer_, 3);
					asio::async_write(remote_socket_, asio::buffer(local_buffer_, 3), [self, this](const asio::error_code& ec, size_t len) {
						if(! ec)
						{
							// send to server ok, receive from server, expected to be encrypted 05 00
							if(len < 2)
							{
								LogErr("receive 05 00 from remote err");
								return;
							}

							Cypher::Instance().Decrypt(remote_buffer_, 2);
							if(remote_buffer_[0] != 0x05 || remote_buffer_[1] != 0x00)
							{
								LogErr("receive from remote, not 05 00");
								return;
							}

							// send to client
							asio::async_write(local_socket_, asio::buffer(remote_buffer_, 2), [self, this](const asio::error_code& ec, size_t len) {
								if(! ec)
								{
									// receive 05 01 00 01/03 ... from client
									local_socket_.async_receive(local_buffer_, [self, this](const asio::error_code& ec, size_t len) {
										if(! ec)
										{
											// decrypt and send to server
											if(len < 5)
											{

											}
										} CLOSE_EC
									});
								} CLOSE_EC
							});
						} CLOSE_EC
					});
				} CLOSE_EC
			});
		} CLOSE_EC
	});
}

void SSSession::ServerStart()
{
	auto self = shared_from_this();
	local_socket_.async_receive(asio::buffer(local_buffer_), [self, this](const asio::error_code& ec, size_t len)
			{
				if(! ec) // expeted to be encrypted 05 01 00
				{

				}
				else
				{
					Close(ec);
				}
			});
}

void SSSession::Close(const asio::error_code& ec)
{
}

//============================================================
void Socks5Server::LocalStart()
{
	auto p = std::make_shared<SSSession>(io_context_);
	acceptor_.async_accept(p->local_socket_, [p, this](const asio::error_code& err)
			{
				if(! err)
				{
					p->LocalStart();
					LocalStart();
				}
				else
				{
					std::cout<<"accept error, msg:"<<err.message()<<"\n";
				}
			});
}

void Socks5Server::ServerStart()
{
	auto p = std::make_shared<SSSession>(io_context_);
	acceptor_.async_accept(p->local_socket_, [p, this](const asio::error_code& err)
			{
				if(! err)
				{
					p->ServerStart();
					ServerStart();
				}
				else
				{
					std::cout<<"accept error, msg:"<<err.message()<<"\n";
				}
			});
}

