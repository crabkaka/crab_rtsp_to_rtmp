#pragma once

#include <functional>
#include <thread>
#include <memory>
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

enum Codec 
{
	AAC = 97,
	H264 = 264,
	H265 =265,
	
};


class StreamClientState {
public:
	StreamClientState();
	~StreamClientState();

public:
	MediaSubsessionIterator* iter;
	MediaSession* session;
	MediaSubsession* subsession;
	TaskToken streamTimerTask;
	double duration;
};

using namespace std;
using onReceivePkt =  std::function<void(uint8_t *data,  int len,uint32_t ts)>;
using onVideoCodecInfo = std::function<void(int width, int height, int fps, string codec, string sps, string pps)>;
using onAudioCodecInfo =  std::function<void(int sample, int channel, string codec)>;


class ourRTSPClient : public RTSPClient 
{
public:
	static ourRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel = 0,
		char const* applicationName = NULL,
		portNumBits tunnelOverHTTPPortNum = 0);
	void start();
	void stop();
	void run();
	virtual ~ourRTSPClient();

	std::function<void()> onConnect_;
	std::function<void()> onDisconnect_;
protected:
	ourRTSPClient(UsageEnvironment& env,char const* rtspURL,
		int verbosityLevel = 1, char const* applicationName = "live555", portNumBits tunnelOverHTTPPortNum = 0);

public:

	TaskScheduler* scheduler_;
	UsageEnvironment* env_;
	StreamClientState scs;

	char eventLoopWatchVariable_ = 0;

	uint32_t rtspClientCount_ = 0;

	onReceivePkt video_cb_;
	onReceivePkt audio_cb_;
	onVideoCodecInfo on_video_codec_info_;
	onAudioCodecInfo on_audio_codec_info_;

	shared_ptr<thread> thread_;
};




