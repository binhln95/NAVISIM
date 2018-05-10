//
// Created by 8460LK on 3/23/2018.
//

#ifndef NAVISIM_LOG_H
#define NAVISIM_LOG_H

#include <stdio.h>
#include <stddef.h>
#include <android/log.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,"LOG_DEBUG", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"LOG_ERRORS", __VA_ARGS__)
#endif //NAVISIM_LOG_H
