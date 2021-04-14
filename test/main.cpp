#include <iostream>
#include "app.hpp"

using namespace std;
using namespace crab;


int main()
{
	
	App::instance()->start();

	while (true)
	{
		App::instance()->add_and_start(1, "rtmp://192.168.0.95/live/test", "rtsp://admin:a12345678@192.168.0.30/Streaming/Channels/101");
		this_thread::sleep_for(15s);
		App::instance()->stop_push(1);
		this_thread::sleep_for(5s);
	}
	
	getchar();
	return 0;
}