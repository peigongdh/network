/*
demonstrate usage of poll post run and poll
                      frank Jul 7, 2018
*/

#include <iostream>
#include <functional>
#include <thread>
#include <mutex>
#include <chrono>
#include <stdint.h>
#include "asio.hpp"

using namespace std;

// 单线程，为了主循环不阻塞，可以调用io_context::poll
void OneThreadWithPoll()
{
	asio::io_context ios;

	uint16_t s = 0;
	while(++s)
	{
		ios.restart(); // 注意调restart，否则后面的回调都不会执行
		ios.poll();  // poll不会阻塞，可以在自己的主循环里面调用
		ios.post([s]() { cout<<s; cout.flush(); });
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

// 在单独一个线程中调用run，在其它线程中post
void MultiThreadRun()
{
	asio::io_context io;

	thread t2([&io]() {
		for(int i = 0; i < 10; ++i)
		{
			io.dispatch([]() {
				cout<<"dispatch, threadid:"<<std::this_thread::get_id()<<"\n";
			});
			io.post([]() {
				cout<<"post, threadid:"<<std::this_thread::get_id()<<"\n";
			});
		}

		cout<<"thread2 id:"<<std::this_thread::get_id()<<"\n";
	});

	thread t1([&io]() {
		cout<<"run in thread:"<<std::this_thread::get_id()<<"\n";
		io.run(); // 注意run调用的时机，如果run最先启动，起初没有任何事件，run会退出，后面post进去的回调都不会执行。
	});

	t2.join();
	t1.join();

}

int main()
{
	// OneThreadWithPoll();

	MultiThreadRun();

	return 0;
}


