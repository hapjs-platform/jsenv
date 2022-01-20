/*
 * Copyright (c) 2021, the hapjs-platform Project Contributors
 * SPDX-License-Identifier: EPL-1.0
 */

#ifndef INSPECTOR_LOG_H
#define INSPECTOR_LOG_H

#include <dlfcn.h>
//#include <android/log.h>

extern "C" void __android_log_print(int, const char*, const char*, ...);
#define ANDROID_LOG_VERBOSE 2
#define ANDROID_LOG_DEBUG 3
#define ANDROID_LOG_INFO 4
#define ANDROID_LOG_WARN 5
#define ANDROID_LOG_ERROR 6

#define PROP_VALUE_MAX 92
#define LOG_TAG "Inspector"
#ifdef ANDROID
#define ALOG(TAG, LEVEL, ...)                                   \
  do {                                                          \
    __android_log_print(ANDROID_LOG_##LEVEL, TAG, __VA_ARGS__); \
  } while (0)
#else
#define ALOG(TAG, LEVEL, ...)
#endif
#define LOGD(...) ALOG(LOG_TAG, DEBUG, __VA_ARGS__)

#define ALOGV(TAG, ...) ALOG(TAG, VERBOSE, __VA_ARGS__)
#define ALOGD(TAG, ...) ALOG(TAG, DEBUG, __VA_ARGS__)
#define ALOGI(TAG, ...) ALOG(TAG, INFO, __VA_ARGS__)
#define ALOGW(TAG, ...) ALOG(TAG, WARN, __VA_ARGS__)
#define ALOGE(TAG, ...) ALOG(TAG, ERROR, __VA_ARGS__)

#endif
