#include <iostream>
#include "eventLoop.h"
#include "push_manage.hpp"
#include <future>
#include "timestamp.hpp"
#include <iomanip>
using namespace std;





int main()
{

	EventLoop loop;

	RtspManage * t = new RtspManage("rtsp://admin:a12345678@192.168.0.30/Streaming/Channels/101","rtmp://192.168.0.95/live/test",1, &loop);
	t->start();
	loop.start();
	getchar();
	return 0;
}