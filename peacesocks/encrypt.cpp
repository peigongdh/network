/*encrypt.cpp
 frank May 30, 2018
 */
#include <iostream>
#include <algorithm>
#include "encrypt.h"
#include "config.h"

Cypher::Cypher()
{
	uint8_t data[MAX_UCHAR];
	uint8_t init = 0;
	std::generate_n(data, MAX_UCHAR, [&init]() { return init++; } ); // data[i] = i, 0 <= i <= 255

	const size_t shift_steps = SSConfig::Instance().ShiftSteps();
	std::rotate(data, data + shift_steps, data + sizeof(data));

	for(int i = 0; i < MAX_UCHAR; ++i)
	{
		encrypt_value[i] = data[i];
		decrypt_value[data[i]] = i;
	}
}

void Cypher::Encrypt(char* buff, size_t num)
{
	uint8_t* first = (uint8_t*)buff;
	for(uint8_t* p = first; p < first + num; ++p)
	{
		*p = encrypt_value[*p];
	}
}

void Cypher::Decrypt(char* buff, size_t num)
{
	uint8_t* first = (uint8_t*)buff;
	for(uint8_t* p = first; p < first + num; ++p)
	{
		*p = decrypt_value[*p];
	}
}
