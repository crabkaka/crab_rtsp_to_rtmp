#include "audio_transcode.hpp"
#include "error_define.h"
#include <algorithm>

namespace crab
{

	audio_transcode::audio_transcode()
	{

	}
	audio_transcode::~audio_transcode()
	{
		stop();
		avcodec_close(encoder_ctx_);
		avcodec_free_context(&encoder_ctx_);
		avcodec_close(decoder_ctx_);
		avcodec_free_context(&decoder_ctx_);

		av_frame_free(&decodeFrame_);
		av_frame_free(&encodeFrame_);

		av_packet_free(&pkt_);
		av_packet_free(&encode_pkt_);

		av_free(encode_buf_);

		delete re_sample_;
	
	}


	void audio_transcode::start()
	{
		thread_.reset(new thread(&audio_transcode::run, this));
	}

	void audio_transcode::stop()
	{
		stop_ = true;
		filter_resample_->stop();
		condition_.notify_one();
		thread_->join();
	}

	void audio_transcode::pushPacket(uint8_t *data, int len,uint32_t ts)
	{
		shared_ptr<AudioPacket> pkt(new AudioPacket(data, len,ts));
		memcpy(pkt->buffer_.get(), data, len);
		{
			std::unique_lock<std::mutex> lock(queue_mutex_);
			queue_.push(std::move(pkt));
		}
		condition_.notify_one();
	}

	void audio_transcode::run()
	{
		while (true)
		{
			shared_ptr<AudioPacket> pkt;
			{
				std::unique_lock<std::mutex> lock(queue_mutex_);
				condition_.wait(lock,
					[this] { return stop_ || !queue_.empty(); });

				if (stop_) return;

				pkt = std::move(queue_.front());
				queue_.pop();
			}
			transcode(pkt);
		}
	}

	void  audio_transcode::transcode(shared_ptr<AudioPacket> pkt)
	{
		pkt_->data = pkt->buffer_.get();
		pkt_->size = pkt->size_;

		int ret = avcodec_send_packet(decoder_ctx_, pkt_);
		if (ret < 0) return;

		ret = avcodec_receive_frame(decoder_ctx_, decodeFrame_);
		if (ret < 0) return;

		av_packet_unref(pkt_);


		filter_resample_->add_frame(decodeFrame_);

	}

	void audio_transcode::on_filter_aresample_error(int error, int)
	{
		printf("on filter amix audio error:%d", error);
	}

	void audio_transcode::on_filter_aresample_data(AVFrame * frame, int index)
	{

		AUDIO_SAMPLE *resamples = re_sample_;

		int copied_len = 0;
		int sample_len = av_samples_get_buffer_size(NULL, frame->channels, frame->nb_samples, (AVSampleFormat)frame->format, 1);

		int remain_len = sample_len;

		//for data is planar,should copy data[0] data[1] to correct buff pos
		if (av_sample_fmt_is_planar((AVSampleFormat)frame->format) == 0 || frame->channels == 1) {
			while (remain_len > 0) {

				//cache pcm
				copied_len = min(resamples->size - resamples->sample_in, remain_len);
				if (copied_len) {
					memcpy(resamples->buff + resamples->sample_in, frame->data[0] + sample_len - remain_len, copied_len);
					resamples->sample_in += copied_len;
					remain_len = remain_len - copied_len;
				}

				//got enough pcm to encoder,resample and mix
				if (resamples->sample_in == resamples->size) {
					encode(resamples->buff, resamples->size);

					resamples->sample_in = 0;
				}
			}
		}
		else {//resample size is channels*frame->linesize[0],for 2 channels
			while (remain_len > 0) {
				copied_len = min(resamples->size - resamples->sample_in, remain_len);

				if (copied_len) {
					memcpy(resamples->buff + resamples->sample_in / 2, frame->data[0] + (sample_len - remain_len) / 2, copied_len / 2);
					memcpy(resamples->buff + resamples->size / 2 + resamples->sample_in / 2, frame->data[1] + (sample_len - remain_len) / 2, copied_len / 2);
					resamples->sample_in += copied_len;
					remain_len = remain_len - copied_len;
				}

				if (resamples->sample_in == resamples->size) {
					encode(resamples->buff, resamples->size);

					resamples->sample_in = 0;
				}
			}
		}
	}


	void audio_transcode::encode(uint8_t * data, int len)
	{
		av_packet_unref(encode_pkt_);

		memcpy(encode_buf_,data,encode_buf_size_);
		encodeFrame_->pts = pts_++;
		int ret = avcodec_send_frame(encoder_ctx_, encodeFrame_);

		ret = avcodec_receive_packet(encoder_ctx_, encode_pkt_);

		if(on_transcode_data_ && encode_pkt_->size > 0)
			on_transcode_data_(encode_pkt_->data, encode_pkt_->size);
		
	}

	int audio_transcode::init(AudioInfo &decode_info, AudioInfo &encode_info)
	{
		int err = AE_NO;
		int ret = 0;

		do {
			decoder_ = avcodec_find_decoder(decode_info.codec);
			if (!decoder_) {
				err = AE_FFMPEG_FIND_ENCODER_FAILED;
				break;
			}

			decoder_ctx_ = avcodec_alloc_context3(decoder_);
			if (!decoder_ctx_) {
				err = AE_FFMPEG_ALLOC_CONTEXT_FAILED;
				break;
			}

			decoder_ctx_->codec_type = AVMEDIA_TYPE_AUDIO;
			decoder_ctx_->sample_rate = decode_info.sample_rate;
			decoder_ctx_->time_base.den = decode_info.sample_rate;
			decoder_ctx_->time_base.num = 1;
			decoder_ctx_->channels = decode_info.channel;
			decoder_ctx_->channel_layout = av_get_default_channel_layout(decode_info.channel);
			decoder_ctx_->sample_fmt = decode_info.fmt;
			if (decode_info.codec == AV_CODEC_ID_ADPCM_G726)
			{
				decoder_ctx_->bits_per_coded_sample = 2;
			}
			

			if (avcodec_open2(decoder_ctx_, decoder_, NULL) != 0) {
				err = AE_FFMPEG_OPEN_CODEC_FAILED;
				break;
			}
			pkt_ = av_packet_alloc();
			av_init_packet(pkt_);

			encode_pkt_ = av_packet_alloc();
			av_init_packet(encode_pkt_);
			decodeFrame_ = av_frame_alloc();

			encoder_ = avcodec_find_encoder(encode_info.codec);

			if (!encoder_) {
				err = AE_FFMPEG_FIND_ENCODER_FAILED;
				break;
			}
			encoder_ctx_ = avcodec_alloc_context3(encoder_);

			encoder_ctx_->sample_rate = encode_info.sample_rate;
			encoder_ctx_->sample_fmt = encode_info.fmt;
			encoder_ctx_->channels = encode_info.channel;
			encoder_ctx_->channel_layout = av_get_default_channel_layout(encode_info.channel);
			encoder_ctx_->bit_rate = 32000;
			encoder_ctx_->time_base.den = encode_info.sample_rate;
			encoder_ctx_->time_base.num = 1;
			encoder_ctx_->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
			encoder_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;


			ret = avcodec_open2(encoder_ctx_, encoder_, NULL);
			if (ret < 0) {
				err = AE_FFMPEG_OPEN_CODEC_FAILED;
				break;
			}

			encodeFrame_ = av_frame_alloc();

			encodeFrame_->channels = encode_info.channel;
			encodeFrame_->nb_samples = encoder_ctx_->frame_size;
			encodeFrame_->channel_layout = av_get_default_channel_layout(encode_info.channel);
			encodeFrame_->format = encode_info.fmt;
			encodeFrame_->sample_rate = encode_info.sample_rate;


			encode_buf_size_ = av_samples_get_buffer_size(NULL, encode_info.channel, encoder_ctx_->frame_size, encode_info.fmt, 1);
			encode_buf_ = (uint8_t*)av_malloc(encode_buf_size_);

			 avcodec_fill_audio_frame(encodeFrame_, encode_info.channel, encode_info.fmt, encode_buf_, encode_buf_size_, 0);


			FILTER_CTX ctx_in = { 0 }, ctx_out = { 0 };
			ctx_in.time_base = decoder_ctx_->time_base;
			ctx_in.channel_layout = decoder_ctx_->channel_layout;
			ctx_in.nb_channel = decoder_ctx_->channels;
			ctx_in.sample_fmt = decoder_ctx_->sample_fmt;
			ctx_in.sample_rate = decoder_ctx_->sample_rate;

			ctx_out.time_base = { 1,AV_TIME_BASE };
			ctx_out.channel_layout = encoder_ctx_->channel_layout;
			ctx_out.nb_channel = encoder_ctx_->channels;
			ctx_out.sample_fmt = encoder_ctx_->sample_fmt;
			ctx_out.sample_rate = encoder_ctx_->sample_rate;
			filter_resample_.reset(new filter_aresample());

			filter_resample_->init(ctx_in, ctx_out, 1);

			filter_resample_->registe_cb(
			std::bind(&audio_transcode::on_filter_aresample_data, this, std::placeholders::_1, std::placeholders::_2),
				std::bind(&audio_transcode::on_filter_aresample_error, this, std::placeholders::_1, std::placeholders::_2));
			filter_resample_->start();

			re_sample_ = new AUDIO_SAMPLE();
			re_sample_->size = av_samples_get_buffer_size(
				NULL, encoder_ctx_->channels, encoder_ctx_->frame_size, encoder_ctx_->sample_fmt, 1);
			re_sample_->buff = new uint8_t[re_sample_->size];

			
		} while (0);

		return err;
	}

}