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
	asio::io_context io; // 无论是post还是dispatch，其待调用的函数都是在io_context::run的线程里面运行
	asio::strand<asio::io_context::executor_type> str(io.get_executor()); // strand的用法，这里用来防止cout输出错乱

	thread t2([&io, &str]() {
		for(int i = 0; i < 10; ++i)
		{
			// 如果是调用dispatch的线程和调用io_context::run的线程不是同一个线程，则dispatch和post的效果是一样的。
			// 此处就是一样, dispatch就等于post，都会被post到另一个线程中去执行
//			io.post([&io, &str]() {
				io.dispatch(asio::bind_executor(str, []() {
					cout<<"dispatch, threadid:"<<std::this_thread::get_id()<<"\n";
				}));
				io.post(asio::bind_executor(str, []() {
					cout<<"post, threadid:"<<std::this_thread::get_id()<<"\n";
				}));
//			});
		}

//		cout<<"thread2 id:"<<std::this_thread::get_id()<<"\n";
	});

	t2.join();

	thread t1([&io, &str]() {
		int times = 0;
		while(++times < 10)
		{
			io.restart();
			io.run(); // 注意run调用的时机，如果run最先启动，起初没有任何事件，run会退出，后面post进去的回调都不会执行。
			for(int i = 0; i < 10; ++i)
			{
				io.post(asio::bind_executor(str, []() { cout<<"====>post, this one is after next dispatch\n"; } ));
				io.dispatch(asio::bind_executor(str, []() { cout<<"====>dispatch, this one will run first\n"; }));
			}
		}
	});

	//t2.join();
	t1.join();

}

int main()
{
	// OneThreadWithPoll();

	MultiThreadRun();

	return 0;
}


