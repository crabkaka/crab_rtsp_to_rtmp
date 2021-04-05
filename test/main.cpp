#include <iostream>
#include "app.hpp"

using namespace std;
using namespace crab;


int main()
{
	App::instance()->add_and_start(1,"rtmp://192.168.0.95/live/test","rtsp://admin:a12345678@192.168.0.30/Streaming/Channels/101");
	App::instance()->start();
	return 0;
}