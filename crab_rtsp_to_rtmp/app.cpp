#include "app.hpp"
#include "push_manage.hpp"
#include "eventLoop.h"

namespace crab
{

	App* App::instance()
	{
		static App * single = new App();
		return single;
	}
	App::App()
	{
		el = new EventLoop();
	}

	App::~App()
	{
		delete el;
	}

	void App::start()
	{
		el->setInterval(10000,std::bind(&App::alive_check,this));

		thread_.reset(new thread(std::bind(&App::work,this)));
		
	}

	void App::work()
	{
		el->start();
	}

	void App::add(int id, string &&rtmp_url, string &&rtsp_url)
	{
		push_map_.emplace(id, shared_ptr<RtspManage>(new RtspManage(std::move(rtsp_url), std::move(rtmp_url), id, el)));
	}

	void App::add_and_start(int id, string &&rtmp_url, string &&rtsp_url)
	{
		lock_guard<mutex> lock(map_mutex_);
		auto it = push_map_.find(id);
		if (it != push_map_.end())
		{
			return;
		}
		add(id, std::move(rtmp_url), std::move(rtsp_url));

		push_map_[id]->start();
	}

	void App::alive_check()
	{
		lock_guard<mutex> lock(map_mutex_);
		for (auto it : push_map_)
		{
			it.second->aliveCheck();
		}
	}

	void App::stop_push(int id)
	{
		lock_guard<mutex> lock(map_mutex_);
		auto it = push_map_.find(id);
		if (it != push_map_.end())
		{
			it->second->stop();
		}
		push_map_.erase(id);
	}
}