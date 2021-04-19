#pragma once

#include "evpp/platform_config.h"

#ifdef __cplusplus
//#define GOOGLE_GLOG_DLL_DECL           // 使用静态glog库时，必须定义这个
//#define GLOG_NO_ABBREVIATED_SEVERITIES // 没这个编译会出错,传说因为和Windows.h冲突
#include <time.h>
#include <vector>
//#include <glog/logging.h>

#ifdef GOOGLE_STRIP_LOG

#if GOOGLE_STRIP_LOG == 0
#define LOG_TRACE LOG(INFO)
#define LOG_DEBUG LOG(INFO)
#define LOG_INFO  LOG(INFO)
#define DLOG_TRACE LOG(INFO) << __PRETTY_FUNCTION__ << " this=" << this << " "
#else
#define LOG_TRACE if (false) LOG(INFO)
#define LOG_DEBUG if (false) LOG(INFO)
#define LOG_INFO  if (false) LOG(INFO)
#define DLOG_TRACE if (false) LOG(INFO)
#endif

#if GOOGLE_STRIP_LOG <= 1
#define LOG_WARN  LOG(WARNING)
#define DLOG_WARN LOG(WARNING) << __PRETTY_FUNCTION__ << " this=" << this << " "
#else
#define LOG_WARN  if (false) LOG(WARNING)
#define DLOG_WARN if (false) LOG(WARNING)
#endif

#define LOG_ERROR LOG(ERROR)
#define LOG_FATAL LOG(FATAL)

#else
#define DLOG_TRACE if(false) std::cout << __PRETTY_FUNCTION__ << " this=" << this << " \n"
#define DLOG_WARN if(false) std::cout << __FILE__ << ":" << __LINE__ << " \n"
#define LOG_TRACE if(false) std::cout << __FILE__ << ":" << __LINE__ << " \n"
#define LOG_DEBUG if(false) std::cout << __FILE__ << ":" << __LINE__ << " \n"
#define LOG_INFO  if(false) std::cout << __FILE__ << ":" << __LINE__ << " \n"
#define LOG_WARN  if(false) std::cout << __FILE__ << ":" << __LINE__ << " \n"
#define LOG_ERROR std::cout << __FILE__ << ":" << __LINE__ << " \n"
#define LOG_FATAL std::cout << __FILE__ << ":" << __LINE__ << " \n"
#define CHECK_NOTnullptr(val) LOG_ERROR << "'" #val "' Must be non nullptr";
#endif
#endif // end of define __cplusplus

//#ifdef _DEBUG
//#ifdef assert
//#undef assert
//#endif
//#define assert(expr)  { if (!(expr)) { LOG_FATAL << #expr ;} }
//#endif