/*
		server.cpp
				frank May 30, 2018
*/
#include <iostream>
#include "session.h"
#include "config.h"

#ifdef __linux__
#include <unistd.h>
void daemon()
{
    pid_t pid;
    if ((pid = fork()) != 0)
    {
        exit(0);
    }

    signal(SIGHUP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    setsid();

    if((pid = fork()) != 0)
    {
        exit(0);
    }

    umask(0);
    chdir("/");

    for(int i=0;i<65535;i++)
    	close(i);
}
#endif

// usage: server -c proxy.conf
int main(int argc, const char* argv[])
{
	if(argc != 3 || std::string(argv[1]) != "-c")
	{
		std::cerr<<"usage: local -c proxy.conf"<<"\n";
		return 1;
	}

	if(! SSConfig::Instance().ReadConfig(argv[2]))
	{
		return 1;
	}

#ifdef __linux__
	daemon();
#endif

	std::cout<<"server listen on port: "<<SSConfig::Instance().ServerPort()<<"\n";
	asio::io_context io_context(1); // one thread
	Socks5Server ss(io_context, SSConfig::Instance().ServerEndpoint());
	ss.ServerStart();
	io_context.run();

	return 0;
}

