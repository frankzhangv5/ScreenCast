#pragma once

#define LOG_TAG "mirror_server"

#ifdef __ANDROID__
#    undef NDEBUG
#    include <android/log.h>
#    define FMT_PUBLIC ""
#    define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#    define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#    define LOGW(...)  __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#    define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#elif defined(__OHOS__)
#    include "hilog/log.h"
#    define FMT_PUBLIC "{public}"
#    define LOGD(...)  OH_LOG_DEBUG(LOG_APP, __VA_ARGS__)
#    define LOGI(...)  OH_LOG_INFO(LOG_APP, __VA_ARGS__)
#    define LOGW(...)  OH_LOG_WARN(LOG_APP, __VA_ARGS__)
#    define LOGE(...)  OH_LOG_ERROR(LOG_APP, __VA_ARGS__)
#else
#    include <stdio.h>
#    define FMT_PUBLIC ""
#    define LOGD(...)  fprintf(stderr, LOG_TAG, __VA_ARGS__, "\n")
#    define LOGI(...)  fprintf(stderr, LOG_TAG, __VA_ARGS__, "\n")
#    define LOGW(...)  fprintf(stderr, LOG_TAG, __VA_ARGS__, "\n")
#    define LOGE(...)  fprintf(stderr, LOG_TAG, __VA_ARGS__, "\n")
#endif