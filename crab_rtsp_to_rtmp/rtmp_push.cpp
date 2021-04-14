#include "rtmp_push.hpp"
#include "srs_librtmp.h"
#define ADTS_SIZE 7

RtmpPush::RtmpPush(string url)
	:url_(url),
	stop_(false),
	is_connected_(false),
	is_reconnect_(false),
	is_call_disconnect_(false),
	has_idr_(false),
	is_send_i_(false),
	is_init_video_(false),
	is_push_i_(false),
	is_need_transcode_(false),
	is_transcode_init_(false),
	disconnect_ct_(0),
	sound_format_(10),
	sound_rate_(3),	// 3 = 44 kHz
	sound_size_(1),	// 1 = 16-bit samples
	sound_type_(1)	// 0 = Mono sound; 1 = Stereo sound
{

}

void RtmpPush::connect()
{
	rtmp_ = srs_rtmp_create(url_.c_str());
	srs_rtmp_set_timeout(rtmp_, 3000, 3000);
	if (srs_rtmp_handshake(rtmp_) != 0) {
		srs_human_trace("%s simple handshake failed.", url_.c_str());
		goto rtmp_destroy;
	}

	if (srs_rtmp_connect_app(rtmp_) != 0) {
		srs_human_trace("%s connect vhost/app failed.", url_.c_str());
		goto rtmp_destroy;
	}

	if (srs_rtmp_publish_stream(rtmp_) != 0) {
		srs_human_trace("%s publish stream failed.", url_.c_str());
		goto rtmp_destroy;
	}
	srs_human_trace("%s publish stream success", url_.c_str());

	is_connected_ = true;
	callConnect();
	return;

rtmp_destroy:
	is_call_disconnect_ = false;
	callDisconnect();
}

void RtmpPush::disconnect()
{
	is_connected_ = false;
	is_transcode_init_ = false;
	while (!queue_.empty())
	{
		queue_.pop();
	}

	if (rtmp_)
	{
		srs_rtmp_destroy(rtmp_);
		rtmp_ = nullptr;
	}
}

void RtmpPush::callConnect()
{
	alive_ct_ = 1;
	onConnect_();
	is_call_disconnect_ = false;
}



void RtmpPush::callDisconnect()
{
	lock_guard<mutex> lock(call_dis_mutex_);

	++disconnect_ct_;

	if (is_call_disconnect_) return;

	disconnect();
	onDisconnect_();
	is_call_disconnect_ = true;
	is_push_i_ = false;
}


void RtmpPush::reconnect()
{
	is_reconnect_ = true;
	condition_.notify_one();
}


void RtmpPush::start()
{
	thread_.reset(new thread(&RtmpPush::run, this));

}

void RtmpPush::stop()
{
	stop_ = true;
	condition_.notify_one();
	thread_->join();
	while (!queue_.empty())
	{
		queue_.pop();
	}
}

void RtmpPush::run()
{
	connect();
	while (true)
	{
		shared_ptr<EncData> pkt;
		{
			std::unique_lock<std::mutex> lock(queue_mutex_);
			condition_.wait(lock,
				[this] { return stop_ || !queue_.empty() || is_reconnect_; });

			if (stop_){
				disconnect();
				return;
			}

			if (is_reconnect_)
			{
				connect();
				is_reconnect_ = false;
				continue;
			}
			pkt = std::move(queue_.front());
			queue_.pop();
		}

		if (pkt->_type == VIDEO_DATA) {

			char *ptr = (char*)pkt->_data;
			int len = pkt->_dataLen;
			int ret = 0;
			ret = srs_h264_write_raw_frames(rtmp_, ptr, len, pkt->_dts, pkt->_dts);

			bool is_send_i_success = true;
			

			if (ret != 0) {
				if (srs_h264_is_dvbsp_error(ret)) {
					srs_human_trace("ignore drop video error, code=%d", ret);
				}
				else if (srs_h264_is_duplicated_sps_error(ret)) {
					srs_human_trace("ignore duplicated sps, code=%d", ret);
				}
				else if (srs_h264_is_duplicated_pps_error(ret)) {
					srs_human_trace("ignore duplicated pps, code=%d", ret);
				}
				else {
					is_send_i_success = false;
					srs_human_trace("send h264 raw data failed. ret=%d", ret);
					callDisconnect();
				}
			}
			else
			{
				++alive_ct_;
			}

			if ((ptr[4] == 0x65) && is_send_i_success)
			{
				is_send_i_ = true;
			}

		}
		else if (pkt->_type == AUDIO_DATA) {
			int ret = 0;

			if ((ret = srs_audio_write_raw_frame(rtmp_,
				sound_format_, sound_rate_, sound_size_, sound_type_,
				(char*)pkt->_data, pkt->_dataLen, pkt->_dts)) != 0) {
				srs_human_trace("send audio raw data failed. ret=%d", ret);
				callDisconnect();
			}
		}
	}
}

void RtmpPush::push_video_frame(uint8_t* pData, int nLen, uint32_t ts)
{

	shared_ptr<EncData> pdata(new EncData());
	pdata->_data = new uint8_t[nLen + 4];
	memcpy(pdata->_data, h264_header_, 4);
	memcpy(pdata->_data + 4, pData, nLen);
	pdata->_dataLen = nLen + 4;
	pdata->_type = VIDEO_DATA;
	pdata->_dts = ts;
	{
		std::unique_lock<std::mutex> lock(queue_mutex_);
		queue_.push(std::move(pdata));
	}
	condition_.notify_one();
}


void print_hex(void  const *data,int len)
{
	uint8_t * dd = (uint8_t *)data;
	for (int i = 0; i < len; ++i)
	{
		printf("%#x ", dd[i]);
	}
	printf("\n\n");
}

void RtmpPush::getH264Nal(uint8_t* pData, int nLen, uint32_t ts)
{
	if (!is_connected_)
	{
		return;
	}

	if ((pData[0] == 0x67) || (pData[0] == 0x68))
	{
		return;
	}
	
	uint32_t tts= _startTimePoint.elapsed();
	if (pData[0] == 0x65 && !is_push_i_)
	{
		_startTimePoint.reset();
		tts = _startTimePoint.elapsed();
		push_video_frame((uint8_t *)sps_.data(), sps_.length(), tts);
		push_video_frame((uint8_t *)pps_.data(), pps_.length(), tts);
		is_push_i_ = true;
	
	}

	//push first frame is I frame
	if (!is_push_i_)
	{
		return;
	}
	
	push_video_frame(pData, nLen, tts);
	
}

static uint32_t AACSampleRate[16] =
{
	96000, 88200, 64000, 48000,
	44100, 32000, 24000, 22050,
	16000, 12000, 11025, 8000,
	7350, 0, 0, 0 /*reserved */
};

int inline aac_set_adts_head(uint8_t *buf, int size, int sample_rate, int channel_conf)
{
	int objecttype = 2;
	int sample_rate_index = 0;
	for (sample_rate_index = 0; sample_rate_index < 16; sample_rate_index++)
	{
		if (AACSampleRate[sample_rate_index] == sample_rate)
			break;
	}
	if (sample_rate_index == 16)
		return -1; // error

	uint8_t byte;
	if (size < 7)
	{
		return -1;
	}
	buf[0] = 0xff;
	buf[1] = 0xf1;
	byte = 0;
	byte |= (objecttype & 0x03) << 6;
	byte |= (sample_rate_index & 0x0f) << 2;
	byte |= (channel_conf & 0x07) >> 2;
	buf[2] = byte;
	byte = 0;
	byte |= (channel_conf & 0x07) << 6;
	byte |= (ADTS_SIZE + size) >> 11;
	buf[3] = byte;
	byte = 0;
	byte |= (ADTS_SIZE + size) >> 3;
	buf[4] = byte;
	byte = 0;
	byte |= ((ADTS_SIZE + size) & 0x7) << 5;
	byte |= (0x7ff >> 6) & 0x1f;
	buf[5] = byte;
	byte = 0;
	byte |= (0x7ff & 0x3f) << 2;
	buf[6] = byte;
	return 0;
}


void RtmpPush::push_audio_frame(uint8_t* pData, int nLen, uint32_t ts)
{
	if (!is_send_i_) return;

	uint32_t tts = _startTimePoint.elapsed();
	shared_ptr<EncData> pdata(new EncData());
	pdata->_data = new uint8_t[nLen + ADTS_SIZE];
	uint8_t adts[ADTS_SIZE];
	if (aac_set_adts_head(adts, nLen, sample_rate_, channel_) != 0) return;
	memcpy(pdata->_data, adts, ADTS_SIZE);
	memcpy(pdata->_data + ADTS_SIZE, pData, nLen);
	pdata->_dataLen = nLen + ADTS_SIZE;
	pdata->_type = AUDIO_DATA;
	pdata->_dts = tts;
	{
		std::unique_lock<std::mutex> lock(queue_mutex_);
		queue_.push(std::move(pdata));
	}
	condition_.notify_one();
}



void RtmpPush::on_audio_data(uint8_t* pData, int nLen, uint32_t ts)
{
	if (!is_connected_) return;

	if (is_need_transcode_)
	{
		if(is_transcode_init_)
			audio_transcode_->pushPacket(pData, nLen,ts);
	}
	else
	{
		push_audio_frame(pData,nLen,ts);
	}
	
}


void RtmpPush::onVideoInfo(int width, int height, int fps, string codec, string sps, string pps)
{
	if (is_init_video_) return;

	sps_ = sps;
	pps_ = pps;

	is_init_video_ = true;
}

void RtmpPush::onAudioInfo(int sample, int channel, string codec)
{
	channel_ = channel;
	sample_rate_ = sample;
	if (codec == "MPEG4-GENERIC" && sample == 44100)
	{
		is_need_transcode_ = false;
	}
	else
	{
		is_need_transcode_ = true;
		audio_transcode_.reset(new audio_transcode());
		AudioInfo decoder_info;
		decoder_info.channel = channel;
		decoder_info.sample_rate = sample;
		if (codec == "MPEG4-GENERIC")
		{
			decoder_info.codec = AV_CODEC_ID_AAC;
			decoder_info.fmt = AV_SAMPLE_FMT_FLTP;
		}
		else if (codec == "PCMU")
		{
			decoder_info.codec = AV_CODEC_ID_PCM_MULAW;
			decoder_info.fmt = AV_SAMPLE_FMT_S16;
		}
		else if (codec == "PCMA")
		{
			decoder_info.codec = AV_CODEC_ID_PCM_ALAW;
			decoder_info.fmt = AV_SAMPLE_FMT_S16;
		}
		else if (codec == "G726-16")
		{
			decoder_info.codec = AV_CODEC_ID_ADPCM_G726;
			decoder_info.fmt = AV_SAMPLE_FMT_S16;
		}
		else
		{
			return;
		}
		AudioInfo encoder_info;
		encoder_info.channel = 1;
		encoder_info.sample_rate = 44100;
		encoder_info.codec = AV_CODEC_ID_AAC;
		encoder_info.fmt = AV_SAMPLE_FMT_FLTP;
		
		int ret = audio_transcode_->init(decoder_info, encoder_info);

		if (ret) return;

		audio_transcode_->on_transcode_data_ = std::bind(&RtmpPush::on_aac_data,this,std::placeholders::_1,std::placeholders::_2);

		audio_transcode_->start();

		is_transcode_init_ = true;
	}



	//OutputDebugPrintf("sample = %d ,channel = %d,codec = %s\n",sample,channel,codec.c_str());
}

void RtmpPush::on_aac_data(uint8_t* data, int len)
{
	push_audio_frame(data, len, _startTimePoint.elapsed());
}