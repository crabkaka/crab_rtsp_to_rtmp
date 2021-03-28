#pragma once

#include "filter_aresample.hpp"
#include "headers_ffmpeg.h"
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <functional>
#include <iostream>
#include <fstream>

using namespace std;

namespace crab
{


	class AudioPacket
	{
	public:
		AudioPacket(uint8_t *data, int len, uint32_t ts)
			:buffer_(new uint8_t[len]),
			size_(len),
			ts_(ts)
		{
			
		}
		~AudioPacket()
		{

		}
		std::shared_ptr<uint8_t> buffer_;
		int size_;
		uint32_t ts_;

	};

	struct AudioInfo
	{
		int channel;
		int sample_rate;
		AVCodecID codec;
		AVSampleFormat fmt;
	};


	typedef struct {
		uint8_t *buff;
		int size;
		int sample_in;
	}AUDIO_SAMPLE;

	

	class audio_transcode
	{
	public:

		typedef std::function<void(uint8_t *data, int len)> on_transcode_data;
		audio_transcode();
		~audio_transcode();


		void start();

		void stop();

		void pushPacket(uint8_t *data, int len,uint32_t ts);

		void run();

		int init(AudioInfo &decode_info, AudioInfo &encode_info);

		void transcode(shared_ptr<AudioPacket> pkt);

		void encode(uint8_t * data, int len);

		on_transcode_data on_transcode_data_ = nullptr;

		void on_filter_aresample_error(int error, int);
		

		void on_filter_aresample_data(AVFrame * frame, int index);
	private:

		bool stop_;
		mutex queue_mutex_;
		condition_variable condition_;
		shared_ptr<thread> thread_;
		queue<shared_ptr<AudioPacket>> queue_;


		AVCodec *encoder_;
		AVCodecContext *encoder_ctx_;


		AVCodec *decoder_;
		AVCodecContext *decoder_ctx_;

	
		AVPacket* pkt_ = nullptr;
		AVPacket* encode_pkt_ = nullptr;
		AVFrame* decodeFrame_ = nullptr;
		AVFrame* resampleFrame_ = nullptr;
		AVFrame* encodeFrame_ = nullptr;

		uint8_t * encode_buf_;
		int encode_buf_size_;

		AUDIO_SAMPLE* re_sample_;
		int size_;

		uint32_t pts_ = 0;
		shared_ptr<filter_aresample> filter_resample_;
	};

}