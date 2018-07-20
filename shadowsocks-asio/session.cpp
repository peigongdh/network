/*session.cpp
				frank May 30, 2018
*/

#include "session.h"
#include "encrypt.h"
#include "config.h"

#define LogErr(msg) do { \
		std::cerr<<"[<<local_fd = "<< local_socket_.native_handle() <<"], line="<<__LINE__<<", "<<msg<<"\n"; \
		}while(0);

#define CLOSE_EC else { Close(ec); }

#define ERROR_RETURN  if(ec) { Close(ec); return; }

void SSSession::RecvEncryptSend(asio::ip::tcp::socket& sock1,
		asio::ip::tcp::socket& sock2, buffer_type& buff)
{
	auto self = shared_from_this();
	sock1.async_receive(asio::buffer(buff), [self, this, &sock1, &sock2, &buff](const asio::error_code& ec, size_t len) {
		ERROR_RETURN
		Cypher::Instance().Encrypt(buff, len);
		asio::async_write(sock2, asio::buffer(buff, len), [self, this, &sock1, &sock2, &buff](const asio::error_code& ec, size_t len) {
			ERROR_RETURN
			RecvEncryptSend(sock1, sock2, buff);
		});
	});
}

void SSSession::RecvDecryptSend(asio::ip::tcp::socket& sock1,
		asio::ip::tcp::socket& sock2, buffer_type& buff)
{
	auto self = shared_from_this();
	sock1.async_receive(asio::buffer(buff), [self, this, &sock1, &sock2, &buff](const asio::error_code& ec, size_t len) {
		ERROR_RETURN
		Cypher::Instance().Decrypt(buff, len);
		asio::async_write(sock2, asio::buffer(buff, len), [self, this, &sock1, &sock2, &buff](const asio::error_code& ec, size_t len) {
			ERROR_RETURN
			RecvDecryptSend(sock1, sock2, buff);
		});
	});
}

void SSSession::LocalStart()
{
	auto self = shared_from_this();
	local_socket_.async_receive(asio::buffer(local_buffer_), [self, this](const asio::error_code& ec, size_t len) {
		ERROR_RETURN
		//check the value
		if(len < 3 || local_buffer_[0] != 0x05 || local_buffer_[1] != 0x01 || local_buffer_[2] != 0x00)
		{
			LogErr("recv 05 01 00 err");
			return;
		}

		// connect to server
		remote_socket_.async_connect(SSConfig::Instance().ServerEndpoint(), [self, this](const asio::error_code& ec) {
			//std::cout<<"=====>"<<SSConfig::Instance().ServerEndpoint().address().to_string()<<"\n";
			ERROR_RETURN
			// connected to server, send encrypted 05 01 00
			Cypher::Instance().Encrypt(local_buffer_, 3);
			asio::async_write(remote_socket_, asio::buffer(local_buffer_, 3), [self, this](const asio::error_code& ec, size_t len) {
				ERROR_RETURN
				// send to server ok, receive from server, expected to be encrypted 05 00
				remote_socket_.async_receive(asio::buffer(remote_buffer_), [self, this](const asio::error_code& ec, size_t len) {
					ERROR_RETURN
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
						ERROR_RETURN
						// receive 05 01 00 01/03 ... from client
						local_socket_.async_receive(asio::buffer(local_buffer_), [self, this](const asio::error_code& ec, size_t len) {
							ERROR_RETURN
							// decrypt and send to server
							if(len < 5)
							{
								LogErr("receive 05 01 00 01/03 ... from client err");
								return;
							}

							// encrypt and send to remote
							Cypher::Instance().Encrypt(local_buffer_, len);
							asio::async_write(remote_socket_, asio::buffer(local_buffer_, len), [self, this](const asio::error_code& ec, size_t len) {
								ERROR_RETURN
								// receive encrypted 05 00 00 ... from remote
								remote_socket_.async_receive(asio::buffer(remote_buffer_), [self, this](const asio::error_code& ec, size_t len) {
									ERROR_RETURN
									Cypher::Instance().Decrypt(remote_buffer_, len);
									// reply to client
									asio::async_write(local_socket_, asio::buffer(remote_buffer_, len), [self, this](const asio::error_code& ec, size_t len) {
										ERROR_RETURN
										// now handshake is over, begin to transfer data
										RecvEncryptSend(local_socket_, remote_socket_, local_buffer_);
										RecvDecryptSend(remote_socket_, local_socket_, remote_buffer_);
									});
								});
							});
						});
					});
				});
			});
		});
	});
}

void SSSession::ServerStart()
{
	auto self = shared_from_this();
	local_socket_.async_receive(asio::buffer(local_buffer_), [self, this](const asio::error_code& ec, size_t len) {
		ERROR_RETURN
		// expeted to be encrypted 05 01 00
		if(len < 3)
		{
			LogErr("recv 05 01 00 length err");
			local_socket_.close();
			return;
		}
		// decrypt data
		Cypher::Instance().Decrypt(local_buffer_, len);
		if(local_buffer_[0] != 0x05 || local_buffer_[1] != 0x01 || local_buffer_[2] != 0x00)
		{
			LogErr("05 01 00 content not right");
			local_socket_.close();
			return;
		}
		// send encrypted 05 00 to local
		local_buffer_[1] = 0x00;
		Cypher::Instance().Encrypt(local_buffer_, 2);
		asio::async_write(local_socket_, asio::buffer(local_buffer_, 2), [self, this](const asio::error_code& ec, size_t len) {
			ERROR_RETURN
			//receive encrypted 05 01 00 01 .... from local
			local_socket_.async_receive(asio::buffer(local_buffer_), [self, this](const asio::error_code& ec, size_t len) {
				ERROR_RETURN
				if(len < 5)
				{
					LogErr("recv encryted 05 01 00 01 ... length too short");
					local_socket_.close();
					return;
				}

				Cypher::Instance().Decrypt(local_buffer_, len);

				if(local_buffer_[1] != 0x01)// now only 0x01 is supported
				{
					LogErr("unsupported cmd");
					local_socket_.close();
					return;
				}

				if(local_buffer_[1] == 0x01)
				{
					if(local_buffer_[3] == 0x01) // 05 01 00 01 ip[4] port[2]
					{
						if(len != 10)
						{
							std::cerr<<"connect with ip length error, not 10, but "<<len<<"\n";
							local_socket_.close();
							return;
						}

						uint32_t int_ip = *(uint32_t*)(&local_buffer_[4]);
						dest_host_ = asio::ip::address_v4(ntohl(int_ip)).to_string();
						uint16_t int_port = *(uint16_t*)(&local_buffer_[8]);
						dest_port_ = std::to_string(ntohs(int_port));
						//std::cout<<"connect with ip[4]="<<dest_host_<<", port="<<dest_port_<<"\n";
					}
					else if(local_buffer_[3] == 0x03) // 05 01 00 03 host_len host[host_len] port[2]
					{
						uint8_t host_len = local_buffer_[4];
						if(len != (uint8_t)7 + host_len)
						{
							std::cerr<<"connect with hostname length error, not "<<7+host_len<<", but "<<len<<"\n";
							return;
						}

						dest_host_ = std::string(&local_buffer_[5], host_len);
						uint16_t int_port = *(uint16_t*)(&local_buffer_[len-2]);
						dest_port_ = std::to_string(ntohs(int_port));
//						size_t pos = dest_host_.find("google.com");
//						if(pos != std::string::npos)
//						{
//							dest_host_.replace(pos, std::string("google.com").length(), "google.ca");
//						}
						std::cout<<"connect with host="<<dest_host_<<", port="<<dest_port_<<"\n";
					}

					// resolve the domain
					asio::ip::tcp::resolver::query q(dest_host_, dest_port_);
					resolver_.async_resolve(q, [self, this](const asio::error_code& ec, asio::ip::tcp::resolver::results_type results) {
						ERROR_RETURN
						// connect to dest
						remote_socket_.async_connect(*results.begin(), [self, this](const asio::error_code& ec) {
							ERROR_RETURN
							// send encrypted 05 00 00 ... to local
							char resp[] = {0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
							memcpy(local_buffer_, resp, sizeof(resp));
							Cypher::Instance().Encrypt(local_buffer_, sizeof(resp));
							asio::async_write(local_socket_, asio::buffer(local_buffer_, sizeof(resp)), [self, this](const asio::error_code& ec, size_t len) {
								ERROR_RETURN
								// socks5 handshake over, now begin to transfer data
								RecvEncryptSend(remote_socket_, local_socket_, remote_buffer_);
								RecvDecryptSend(local_socket_, remote_socket_, local_buffer_);
							});
						});
					});
				}
			});
		});
	});
}

void SSSession::Close(const asio::error_code& ec)
{
//	if(ec != asio::error::eof)
//	{
//		std::cerr<<"err: "<<ec.message()<<"\n";
//	}
	local_socket_.close();
	remote_socket_.close();
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
			std::cerr<<"accept error, msg:"<<err.message()<<"\n";
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
			std::cout<<"new conn, from ip:"<<p->local_socket_.remote_endpoint().address().to_string()<<"\n";
			p->ServerStart();
			ServerStart();
		}
		else
		{
			std::cerr<<"accept error, msg:"<<err.message()<<"\n";
		}
	});
}

