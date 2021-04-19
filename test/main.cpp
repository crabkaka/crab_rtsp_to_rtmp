#include <iostream>
#include "app.hpp"

using namespace std;
using namespace crab;


int main()
{
	
	App::instance()->start();

	//while (true)
	//{
		App::instance()->add_and_start(1, "rtmp://192.168.0.95/live/test1", "rtsp://admin:a12345678@192.168.0.30/Streaming/Channels/101");
		App::instance()->add_and_start(2, "rtmp://192.168.0.95/live/test2", "rtsp://admin:a12345678@192.168.0.30/Streaming/Channels/101");
		App::instance()->add_and_start(3, "rtmp://192.168.0.95/live/test3", "rtsp://admin:a12345678@192.168.0.30/Streaming/Channels/101");
		App::instance()->add_and_start(4, "rtmp://192.168.0.95/live/test4", "rtsp://admin:a12345678@192.168.0.30/Streaming/Channels/101");
		App::instance()->add_and_start(5, "rtmp://192.168.0.95/live/test5", "rtsp://admin:a12345678@192.168.0.30/Streaming/Channels/101");
		/*this_thread::sleep_for(15s);
		App::instance()->stop_push(1);
		this_thread::sleep_for(5s);*/
	//}
	
	getchar();
	return 0;
}