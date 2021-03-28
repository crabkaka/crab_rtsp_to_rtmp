#pragma once
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavutil/error.h>
#include <libavutil/time.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/log.h>
}

#pragma comment ( lib,"avcodec.lib" )
#pragma comment ( lib,"avdevice.lib" )
#pragma comment ( lib,"avfilter.lib" )
#pragma comment ( lib,"avformat.lib" )
#pragma comment ( lib,"avutil.lib" )
#pragma comment ( lib,"postproc.lib" )
#pragma comment ( lib,"swresample.lib" )
#pragma comment ( lib,"swscale.lib" )