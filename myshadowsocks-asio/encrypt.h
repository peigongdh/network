/*
	encrypt.h
				frank May 30, 2018
*/

#ifndef MYSHADOWSOCKS_ASIO_ENCRYPT_H_
#define MYSHADOWSOCKS_ASIO_ENCRYPT_H_

#include <cstddef> // size_t
#include <stdint.h>

// simple encrypt algorithm, just turn a,b,c,d to a+n,b+n,c+n,d+n....(z+n)%256
class Cypher
{
	static const int MAX_UCHAR = 256;
private:
	Cypher();
	Cypher(const Cypher&) = delete;
	Cypher& operator=(const Cypher&) = delete;
	Cypher(Cypher&&) = delete;
	Cypher& operator=(Cypher&&) = delete;
public:
	static Cypher& Instance()
	{
		static Cypher cy;
		return cy;
	}
public:
	void Encrypt(char* buff, size_t num);
	void Decrypt(char* buff, size_t num);
private:
	uint8_t encrypt_value[MAX_UCHAR]; // encrypt x to encrypt_value[x]
	uint8_t decrypt_value[MAX_UCHAR]; // decrypt x to decrypt_value[x]
};


#endif /* MYSHADOWSOCKS_ASIO_ENCRYPT_H_ */
