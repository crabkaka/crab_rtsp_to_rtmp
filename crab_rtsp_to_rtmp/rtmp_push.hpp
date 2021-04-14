#pragma once

#pragma once

#include <iostream>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "timestamp.hpp"
#include "audio_transcode.hpp"

using namespace std;
using namespace crab;

enum ENC_DATA_TYPE {
	VIDEO_DATA,
	AUDIO_DATA,
	META_DATA
};

struct EncData
{
	EncData(void) :_data(NULL), _dataLen(0),_dts(0) {}
	~EncData()
	{
		delete[] _data;
	}
	uint8_t*_data;
	int _dataLen;
	uint32_t _dts;
	ENC_DATA_TYPE _type;
};

class RtmpPush
{
public:
	using onDisconnect = std::function<void()>;
	using onConnect = std::function<void()>;
	RtmpPush(string url);
	void reconnect();
	void connect();
	void disconnect();
	void run();
	void callDisconnect();
	void callConnect();

	void start();
	void stop();
	void push_video_frame(uint8_t* pdata, int len, uint32_t ts);
	void push_audio_frame(uint8_t* pdata, int len, uint32_t ts);
	void getH264Nal(uint8_t* pdata, int len, uint32_t ts);
	void on_audio_data(uint8_t* data, int len, uint32_t ts);
	void on_aac_data(uint8_t* data,int len );


	void onVideoInfo(int width, int height, int fps, string codec, string sps, string pps);
	void onAudioInfo(int sample, int channel, string codec);
	onDisconnect onDisconnect_;
	onConnect onConnect_;

	uint32_t alive_ct_;

	bool is_connected_;

private:


	//* For RTMP
	// 0 = Linear PCM, platform endian
	// 1 = ADPCM
	// 2 = MP3
	// 7 = G.711 A-law logarithmic PCM
	// 8 = G.711 mu-law logarithmic PCM
	// 10 = AAC
	// 11 = Speex
	char sound_format_;
	// 0 = 5.5 kHz
	// 1 = 11 kHz
	// 2 = 22 kHz
	// 3 = 44 kHz
	char sound_rate_;
	// 0 = 8-bit samples
	// 1 = 16-bit samples
	char sound_size_;
	// 0 = Mono sound
	// 1 = Stereo sound
	char sound_type_;


	string url_;
	void * rtmp_;
	bool stop_;

	bool is_reconnect_;
	bool is_call_disconnect_;
	bool has_idr_;
	bool is_send_i_;
	bool is_init_video_;
	bool is_push_i_;
	bool is_push_sps_;
	bool is_push_pps_;
	bool is_need_transcode_;
	bool is_transcode_init_;
	mutex queue_mutex_;
	mutex call_dis_mutex_;

	condition_variable condition_;
	shared_ptr<thread> thread_;
	queue<shared_ptr<EncData>> queue_;

	string sps_;
	string pps_;

	Timestamp _startTimePoint;

	shared_ptr<audio_transcode> audio_transcode_;

	int sample_rate_;
	int channel_;

	int disconnect_ct_;

	char h264_header_[4] = { 0,0,0,1 };

};
