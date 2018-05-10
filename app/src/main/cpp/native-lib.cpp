#include <jni.h>
#include <string>
#include <log.h>
#include <native-lib.h>
#include <iostream>

using namespace std;


extern "C"{
#include "gpssim.h"
#include "lime_main.h"
}
// test

//int limeFd;
//const char* usbDevPath;

extern "C"
JNIEXPORT jstring

JNICALL
Java_com_navis_navisim_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */ instance) {
    jobject mainActivityObj = env->NewGlobalRef(instance);
    jclass clz = env->GetObjectClass(instance);
    jclass mainActivityClz = (jclass)env->NewGlobalRef(clz);
    jmethodID timerId = env->GetMethodID(mainActivityClz, "callBack", "(Ljava/lang/String;)V");
    char *msg = "BinhLN from cpp";
    jstring javaMsg = env->NewStringUTF(msg);
    env->CallVoidMethod(mainActivityObj, timerId, javaMsg);
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT jint

JNICALL
Java_com_navis_navisim_MainActivity_runSimulator(
        JNIEnv *env, jobject instance,jstring fPath, jint dFileDescription, jstring usbDevicePath,
        jint vid, jint pid/* this */) {
    const char *spath;
    spath = env->GetStringUTFChars(fPath , NULL) ;
    externalStoragePath = env->GetStringUTFChars(fPath , NULL) ;

    limeFd =  (int) dFileDescription;
    limeVid = vid;
    limePid = pid;
    usbDevPath = env->GetStringUTFChars(usbDevicePath , NULL);
    LOGD("%s\n",spath);

//    int res = runCore((char*)spath);
    int res = initLimeSDR();
    if(!res)
        LOGD("Initializing success");
    else{
        LOGD("Initializing error");
        return -1;
    }

//    res = tx_task((char*)spath);
    res = limeMain();
    LOGD(" Finish generating: %d", (int)res);
    return (int)res;
}

extern "C"
JNIEXPORT jint

JNICALL
Java_com_navis_navisim_MainActivity_generateData(
        JNIEnv *env, jobject instance,
        jstring fPath,
        jstring trajectory,
        jint flag,
        jint time) {
    const char *spath;
    spath = env->GetStringUTFChars(fPath , NULL) ;
    char *path = (char*) spath;

    const char *jtraj;
    jtraj = env->GetStringUTFChars(trajectory, NULL);
    char *traj = (char*) jtraj;
//    LOGD("%s\n",spath);
    int flagCore = (int)flag;
    int timeCore = (int)time;
//    int res = runCore((char*)spath, flagCore, timeCore);
    int result = runCore3(path, traj, flagCore, timeCore);
    return result;
//    LOGD(" Result: %d", (int)res);
//    return (int)res;
}
