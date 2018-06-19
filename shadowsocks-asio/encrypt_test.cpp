/*encrypt_test.cpp
				frank May 30, 2018
*/
#include <iostream>
#include <string>
#include "encrypt.h"

using namespace std;

int main()
{
	Cypher& cy = Cypher::Instance();
	string buff = "12345678abc";
	cy.Encrypt(&buff[0], buff.length());
	cout<<"now buffer is"<<buff<<"\n";
	cy.Decrypt(&buff[0], buff.length());
	cout<<"after decrypt, buff:"<<buff<<"\n";
	return 0;
}


