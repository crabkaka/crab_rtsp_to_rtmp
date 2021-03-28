#pragma once

#include "rtsp_pull.hpp"
#include "rtmp_push.hpp"
#include "eventLoop.h"

class RtspManage
{
public:

	using onRender = std::function<void(uint8_t *data, int len)>;
	RtspManage(string rtsp_url, string rtmp_url, int id, EventLoop * el);
	~RtspManage();

	void start();
	void stop();
	void rtsp_connent();
	void rtsp_stop();

	shared_ptr<RtmpPush> rtmp_push_;
	void onConnect();
	void onDisconnect();

	void aliveCheck();

	void rtsp_reconnect();
private:

	ourRTSPClient* rtsp_client_;
	TaskScheduler* scheduler_;
	UsageEnvironment* env_;

	uint32_t pts = 0;
	bool is_init_video_ = false;
	uint32_t aliveCount_ = 1;
	string rtsp_url_;
	string rtmp_url_;

	double reconnect_time;
	uint32_t reconnect_ct_ = 0;
	int id_;

	EventLoop * eventLoop_;

};

