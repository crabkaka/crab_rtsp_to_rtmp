#include "push_manage.hpp"
#include "app.hpp"



RtspManage::RtspManage(string rtsp_url, string rtmp_url, int id, EventLoop* el)
	:rtmp_push_(new RtmpPush(rtmp_url)),
	rtsp_url_(rtsp_url),
	rtmp_url_(rtmp_url),
	reconnect_time(10.0),
	rtsp_client_(nullptr),
	id_(id),
	eventLoop_(el)
{
	scheduler_ = BasicTaskScheduler::createNew();
	env_ = BasicUsageEnvironment::createNew(*scheduler_);
	rtmp_push_->onConnect_ = std::bind(&RtspManage::onConnect, this);
	rtmp_push_->onDisconnect_ = std::bind(&RtspManage::onDisconnect, this);
}

RtspManage::~RtspManage()
{

}

void RtspManage::rtsp_connent()
{
	rtsp_client_ = ourRTSPClient::createNew(*env_, rtsp_url_.c_str());
	rtsp_client_->scheduler_ = scheduler_;
	rtsp_client_->env_ = env_;
	rtsp_client_->video_cb_ = std::bind(&RtmpPush::getH264Nal, rtmp_push_.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	rtsp_client_->audio_cb_ = std::bind(&RtmpPush::on_audio_data, rtmp_push_.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	rtsp_client_->on_video_codec_info_ = std::bind(&RtmpPush::onVideoInfo, rtmp_push_.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
	rtsp_client_->on_audio_codec_info_ = std::bind(&RtmpPush::onAudioInfo, rtmp_push_.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	rtsp_client_->start();
}

void RtspManage::aliveCheck()
{
	if (!rtmp_push_->is_connected_) return;
	if (rtmp_push_->alive_ct_ > 0)
	{
		rtmp_push_->alive_ct_ = 0;
	}
	else
	{
		rtmp_push_->callDisconnect();
	}
}

void RtspManage::rtsp_reconnect()
{
	aliveCount_ = 1;
	rtsp_stop();
	eventLoop_->setTimeout(5000, [&]() {
		rtsp_connent();
	});
}

void RtspManage::rtsp_stop()
{
	if (rtsp_client_)
	{
		rtsp_client_->stop();
		delete rtsp_client_;
		rtsp_client_ = nullptr;
	}

}

void RtspManage::onConnect()
{
	reconnect_time = 15.0;
	rtsp_connent();
}


void RtspManage::rtmp_reconnect(int id)
{
	auto it = App::instance()->push_map_.find(id);
	if (it != App::instance()->push_map_.end())
	{
		it->second->rtmp_push_->reconnect();
	}
}

void RtspManage::onDisconnect()
{
	rtsp_stop();
	int delay = reconnect_time * 1000;
	eventLoop_->setTimeout(delay, std::bind(&RtspManage::rtmp_reconnect,this, id_));
	
}


void RtspManage::start()
{
	rtmp_push_->start();
}

void RtspManage::stop()
{
	rtsp_stop();
	env_->reclaim();
	delete scheduler_;
	rtmp_push_->stop();

}