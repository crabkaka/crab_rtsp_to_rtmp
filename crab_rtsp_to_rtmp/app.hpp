#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>



using namespace std;

#ifdef CRAB_TRANSFORM_IMPORT
#define CRAB_TRANSFORM_API  __declspec(dllimport)
#else
#define CRAB_TRANSFORM_API  __declspec(dllexport)
#endif

class RtspManage;
class EventLoop;

namespace crab 
{

	class CRAB_TRANSFORM_API App
	{
	public:
		App & operator=(const App &) = delete;
		App(const App &) = delete;

		static App* instance()
		{
			static App * single = new App();
			return single;
		}

		void start();
		void add(int id, string &&rtmp_url, string &&rtsp_url);
		void add_and_start(int id, string &&rtmp_url, string &&rtsp_url);
		void alive_check();

		void stop_push(int id);

	private:
		friend class RtspManage;
		App();
		~App();
		unordered_map<int, shared_ptr<RtspManage>> push_map_;
		mutex map_mutex_;
		EventLoop* el;

		
	};

	

}
