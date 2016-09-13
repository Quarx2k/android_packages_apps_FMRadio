//
// Created by agent on 11.09.2016.
//

#ifndef BROADCOMFMAPP_LOCAL_DEFINES_H
#define BROADCOMFMAPP_LOCAL_DEFINES_H

#include "android/log.h"

#define ALOGE(...)  __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define ALOGI(...)  __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define TAG "Init fm lib"

#endif //BROADCOMFMAPP_LOCAL_DEFINES_H
