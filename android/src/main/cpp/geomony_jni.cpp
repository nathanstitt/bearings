#include <jni.h>
#include <string>
#include <memory>

#include "geomony/GeomonyCore.h"
#include "geomony/PlatformBridge.h"
#include "geomony/Logger.h"
#include "geomony/Location.h"
#include "geomony/Geofence.h"

static JavaVM* gJavaVM = nullptr;

class AndroidPlatformBridge : public geomony::PlatformBridge {
public:
    AndroidPlatformBridge(JNIEnv* env, jobject bridge)
        : bridge_(env->NewGlobalRef(bridge)) {
        jclass cls = env->GetObjectClass(bridge);
        startMethod_ = env->GetMethodID(cls, "onStartLocationUpdates", "(DD)V");
        stopMethod_ = env->GetMethodID(cls, "onStopLocationUpdates", "()V");
        dispatchMethod_ = env->GetMethodID(cls, "onDispatchEvent", "(Ljava/lang/String;Ljava/lang/String;)V");
        getDbPathMethod_ = env->GetMethodID(cls, "onGetDatabasePath", "()Ljava/lang/String;");
        startMotionMethod_ = env->GetMethodID(cls, "onStartMotionActivity", "()V");
        stopMotionMethod_ = env->GetMethodID(cls, "onStopMotionActivity", "()V");
        startGeofenceMethod_ = env->GetMethodID(cls, "onStartStationaryGeofence", "(DDD)V");
        stopGeofenceMethod_ = env->GetMethodID(cls, "onStopStationaryGeofence", "()V");
        startTimerMethod_ = env->GetMethodID(cls, "onStartStopTimer", "(I)V");
        cancelTimerMethod_ = env->GetMethodID(cls, "onCancelStopTimer", "()V");
        addGeofenceMethod_ = env->GetMethodID(cls, "onAddGeofence", "(Ljava/lang/String;DDDZZZI)V");
        removeGeofenceMethod_ = env->GetMethodID(cls, "onRemoveGeofence", "(Ljava/lang/String;)V");
        removeAllGeofencesMethod_ = env->GetMethodID(cls, "onRemoveAllGeofences", "()V");
        startScheduleTimerMethod_ = env->GetMethodID(cls, "onStartScheduleTimer", "(I)V");
        cancelScheduleTimerMethod_ = env->GetMethodID(cls, "onCancelScheduleTimer", "()V");
        sendHTTPRequestMethod_ = env->GetMethodID(cls, "onSendHTTPRequest", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
        startSyncRetryTimerMethod_ = env->GetMethodID(cls, "onStartSyncRetryTimer", "(I)V");
        cancelSyncRetryTimerMethod_ = env->GetMethodID(cls, "onCancelSyncRetryTimer", "()V");
    }

    ~AndroidPlatformBridge() {
        JNIEnv* env = getEnv();
        if (env && bridge_) {
            env->DeleteGlobalRef(bridge_);
        }
    }

    void startLocationUpdates(double desiredAccuracy, double distanceFilter) override {
        JNIEnv* env = getEnv();
        if (!env) return;
        env->CallVoidMethod(bridge_, startMethod_, desiredAccuracy, distanceFilter);
    }

    void stopLocationUpdates() override {
        JNIEnv* env = getEnv();
        if (!env) return;
        env->CallVoidMethod(bridge_, stopMethod_);
    }

    void dispatchEvent(const std::string& name, const std::string& json) override {
        JNIEnv* env = getEnv();
        if (!env) return;
        jstring jName = env->NewStringUTF(name.c_str());
        jstring jJson = env->NewStringUTF(json.c_str());
        env->CallVoidMethod(bridge_, dispatchMethod_, jName, jJson);
        env->DeleteLocalRef(jName);
        env->DeleteLocalRef(jJson);
    }

    std::string getDatabasePath() override {
        JNIEnv* env = getEnv();
        if (!env) return "";
        auto jPath = (jstring)env->CallObjectMethod(bridge_, getDbPathMethod_);
        const char* path = env->GetStringUTFChars(jPath, nullptr);
        std::string result(path);
        env->ReleaseStringUTFChars(jPath, path);
        env->DeleteLocalRef(jPath);
        return result;
    }

    void startMotionActivity() override {
        JNIEnv* env = getEnv();
        if (!env) return;
        env->CallVoidMethod(bridge_, startMotionMethod_);
    }

    void stopMotionActivity() override {
        JNIEnv* env = getEnv();
        if (!env) return;
        env->CallVoidMethod(bridge_, stopMotionMethod_);
    }

    void startStationaryGeofence(double lat, double lon, double radius) override {
        JNIEnv* env = getEnv();
        if (!env) return;
        env->CallVoidMethod(bridge_, startGeofenceMethod_, lat, lon, radius);
    }

    void stopStationaryGeofence() override {
        JNIEnv* env = getEnv();
        if (!env) return;
        env->CallVoidMethod(bridge_, stopGeofenceMethod_);
    }

    void startStopTimer(int seconds) override {
        JNIEnv* env = getEnv();
        if (!env) return;
        env->CallVoidMethod(bridge_, startTimerMethod_, (jint)seconds);
    }

    void cancelStopTimer() override {
        JNIEnv* env = getEnv();
        if (!env) return;
        env->CallVoidMethod(bridge_, cancelTimerMethod_);
    }

    void addGeofence(const std::string& identifier, double lat, double lon,
        double radius, bool notifyOnEntry, bool notifyOnExit,
        bool notifyOnDwell, int loiteringDelay) override {
        JNIEnv* env = getEnv();
        if (!env) return;
        jstring jId = env->NewStringUTF(identifier.c_str());
        env->CallVoidMethod(bridge_, addGeofenceMethod_, jId, lat, lon, radius,
            (jboolean)notifyOnEntry, (jboolean)notifyOnExit,
            (jboolean)notifyOnDwell, (jint)loiteringDelay);
        env->DeleteLocalRef(jId);
    }

    void removeGeofence(const std::string& identifier) override {
        JNIEnv* env = getEnv();
        if (!env) return;
        jstring jId = env->NewStringUTF(identifier.c_str());
        env->CallVoidMethod(bridge_, removeGeofenceMethod_, jId);
        env->DeleteLocalRef(jId);
    }

    void removeAllGeofences() override {
        JNIEnv* env = getEnv();
        if (!env) return;
        env->CallVoidMethod(bridge_, removeAllGeofencesMethod_);
    }

    void startScheduleTimer(int delaySeconds) override {
        JNIEnv* env = getEnv();
        if (!env) return;
        env->CallVoidMethod(bridge_, startScheduleTimerMethod_, (jint)delaySeconds);
    }

    void cancelScheduleTimer() override {
        JNIEnv* env = getEnv();
        if (!env) return;
        env->CallVoidMethod(bridge_, cancelScheduleTimerMethod_);
    }

    void sendHTTPRequest(const std::string& url,
                         const std::string& jsonPayload,
                         const std::string& headersJson,
                         int requestId) override {
        JNIEnv* env = getEnv();
        if (!env) return;
        jstring jUrl = env->NewStringUTF(url.c_str());
        jstring jPayload = env->NewStringUTF(jsonPayload.c_str());
        jstring jHeaders = env->NewStringUTF(headersJson.c_str());
        env->CallVoidMethod(bridge_, sendHTTPRequestMethod_, jUrl, jPayload, jHeaders, (jint)requestId);
        env->DeleteLocalRef(jUrl);
        env->DeleteLocalRef(jPayload);
        env->DeleteLocalRef(jHeaders);
    }

    void startSyncRetryTimer(int delaySeconds) override {
        JNIEnv* env = getEnv();
        if (!env) return;
        env->CallVoidMethod(bridge_, startSyncRetryTimerMethod_, (jint)delaySeconds);
    }

    void cancelSyncRetryTimer() override {
        JNIEnv* env = getEnv();
        if (!env) return;
        env->CallVoidMethod(bridge_, cancelSyncRetryTimerMethod_);
    }

private:
    JNIEnv* getEnv() {
        JNIEnv* env = nullptr;
        if (gJavaVM) {
            int status = gJavaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
            if (status == JNI_EDETACHED) {
                gJavaVM->AttachCurrentThread(&env, nullptr);
            }
        }
        return env;
    }

    jobject bridge_;
    jmethodID startMethod_;
    jmethodID stopMethod_;
    jmethodID dispatchMethod_;
    jmethodID getDbPathMethod_;
    jmethodID startMotionMethod_;
    jmethodID stopMotionMethod_;
    jmethodID startGeofenceMethod_;
    jmethodID stopGeofenceMethod_;
    jmethodID startTimerMethod_;
    jmethodID cancelTimerMethod_;
    jmethodID addGeofenceMethod_;
    jmethodID removeGeofenceMethod_;
    jmethodID removeAllGeofencesMethod_;
    jmethodID startScheduleTimerMethod_;
    jmethodID cancelScheduleTimerMethod_;
    jmethodID sendHTTPRequestMethod_;
    jmethodID startSyncRetryTimerMethod_;
    jmethodID cancelSyncRetryTimerMethod_;
};

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void*) {
    gJavaVM = vm;
    return JNI_VERSION_1_6;
}

JNIEXPORT jlong JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeCreate(JNIEnv* env, jobject thiz) {
    auto bridge = std::make_shared<AndroidPlatformBridge>(env, thiz);
    auto core = new geomony::GeomonyCore(bridge);

    geomony::Logger::instance().setHandler([](geomony::LogLevel level, const std::string& message) {
        JNIEnv* env = nullptr;
        if (gJavaVM) {
            gJavaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        }
        if (!env) return;

        const char* tag = "Geomony";
        jclass logClass = env->FindClass("android/util/Log");
        jmethodID logMethod;
        switch (level) {
            case geomony::LogLevel::Debug:
                logMethod = env->GetStaticMethodID(logClass, "d", "(Ljava/lang/String;Ljava/lang/String;)I");
                break;
            case geomony::LogLevel::Warning:
                logMethod = env->GetStaticMethodID(logClass, "w", "(Ljava/lang/String;Ljava/lang/String;)I");
                break;
            case geomony::LogLevel::Error:
                logMethod = env->GetStaticMethodID(logClass, "e", "(Ljava/lang/String;Ljava/lang/String;)I");
                break;
            default:
                logMethod = env->GetStaticMethodID(logClass, "i", "(Ljava/lang/String;Ljava/lang/String;)I");
                break;
        }
        jstring jTag = env->NewStringUTF(tag);
        jstring jMsg = env->NewStringUTF(message.c_str());
        env->CallStaticIntMethod(logClass, logMethod, jTag, jMsg);
        env->DeleteLocalRef(jTag);
        env->DeleteLocalRef(jMsg);
        env->DeleteLocalRef(logClass);
    });

    return reinterpret_cast<jlong>(core);
}

JNIEXPORT void JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeDestroy(JNIEnv*, jobject, jlong ptr) {
    delete reinterpret_cast<geomony::GeomonyCore*>(ptr);
}

JNIEXPORT void JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeConfigure(JNIEnv* env, jobject, jlong ptr, jstring config) {
    auto core = reinterpret_cast<geomony::GeomonyCore*>(ptr);
    const char* str = env->GetStringUTFChars(config, nullptr);
    core->configure(str);
    env->ReleaseStringUTFChars(config, str);
}

JNIEXPORT void JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeStart(JNIEnv*, jobject, jlong ptr) {
    reinterpret_cast<geomony::GeomonyCore*>(ptr)->start();
}

JNIEXPORT void JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeStop(JNIEnv*, jobject, jlong ptr) {
    reinterpret_cast<geomony::GeomonyCore*>(ptr)->stop();
}

JNIEXPORT jstring JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeGetState(JNIEnv* env, jobject, jlong ptr) {
    auto state = reinterpret_cast<geomony::GeomonyCore*>(ptr)->getState();
    return env->NewStringUTF(state.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeGetLocations(JNIEnv* env, jobject, jlong ptr) {
    auto locations = reinterpret_cast<geomony::GeomonyCore*>(ptr)->getLocations();
    return env->NewStringUTF(locations.c_str());
}

JNIEXPORT jint JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeGetCount(JNIEnv*, jobject, jlong ptr) {
    return reinterpret_cast<geomony::GeomonyCore*>(ptr)->getCount();
}

JNIEXPORT jboolean JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeDestroyLocations(JNIEnv*, jobject, jlong ptr) {
    return reinterpret_cast<geomony::GeomonyCore*>(ptr)->destroyLocations();
}

JNIEXPORT void JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeOnLocationReceived(
    JNIEnv* env, jobject, jlong ptr,
    jdouble lat, jdouble lng, jdouble alt,
    jdouble speed, jdouble heading, jdouble accuracy,
    jdouble speedAccuracy, jdouble headingAccuracy, jdouble altitudeAccuracy,
    jstring timestamp, jstring uuid) {

    auto core = reinterpret_cast<geomony::GeomonyCore*>(ptr);
    geomony::Location loc;
    loc.latitude = lat;
    loc.longitude = lng;
    loc.altitude = alt;
    loc.speed = speed;
    loc.heading = heading;
    loc.accuracy = accuracy;
    loc.speedAccuracy = speedAccuracy;
    loc.headingAccuracy = headingAccuracy;
    loc.altitudeAccuracy = altitudeAccuracy;
    // isMoving is set by the core state machine

    const char* ts = env->GetStringUTFChars(timestamp, nullptr);
    loc.timestamp = ts;
    env->ReleaseStringUTFChars(timestamp, ts);

    const char* id = env->GetStringUTFChars(uuid, nullptr);
    loc.uuid = id;
    env->ReleaseStringUTFChars(uuid, id);

    // Use current time for createdAt
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", std::gmtime(&time_t));
    loc.createdAt = std::string(buf) + "Z";

    core->onLocationReceived(loc);
}

// --- Callbacks from Kotlin to C++ ---

JNIEXPORT void JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeOnMotionDetected(JNIEnv*, jobject, jlong ptr, jint activityType, jint confidence) {
    reinterpret_cast<geomony::GeomonyCore*>(ptr)->onMotionDetected(activityType, confidence);
}

JNIEXPORT void JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeOnStopTimerFired(JNIEnv*, jobject, jlong ptr) {
    reinterpret_cast<geomony::GeomonyCore*>(ptr)->onStopTimerFired();
}

JNIEXPORT void JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeOnGeofenceExit(JNIEnv*, jobject, jlong ptr) {
    reinterpret_cast<geomony::GeomonyCore*>(ptr)->onGeofenceExit();
}

JNIEXPORT void JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeOnGeofenceEvent(JNIEnv* env, jobject, jlong ptr,
    jstring identifier, jstring action) {
    auto core = reinterpret_cast<geomony::GeomonyCore*>(ptr);
    const char* id = env->GetStringUTFChars(identifier, nullptr);
    const char* act = env->GetStringUTFChars(action, nullptr);
    core->onGeofenceEvent(id, act);
    env->ReleaseStringUTFChars(identifier, id);
    env->ReleaseStringUTFChars(action, act);
}

JNIEXPORT jboolean JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeAddGeofence(JNIEnv* env, jobject, jlong ptr, jstring json) {
    auto core = reinterpret_cast<geomony::GeomonyCore*>(ptr);
    const char* str = env->GetStringUTFChars(json, nullptr);
    bool result = core->addGeofence(str);
    env->ReleaseStringUTFChars(json, str);
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeRemoveGeofence(JNIEnv* env, jobject, jlong ptr, jstring identifier) {
    auto core = reinterpret_cast<geomony::GeomonyCore*>(ptr);
    const char* id = env->GetStringUTFChars(identifier, nullptr);
    bool result = core->removeGeofence(id);
    env->ReleaseStringUTFChars(identifier, id);
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeRemoveAllGeofences(JNIEnv*, jobject, jlong ptr) {
    return reinterpret_cast<geomony::GeomonyCore*>(ptr)->removeAllGeofences();
}

JNIEXPORT jstring JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeGetGeofences(JNIEnv* env, jobject, jlong ptr) {
    auto geofences = reinterpret_cast<geomony::GeomonyCore*>(ptr)->getGeofences();
    return env->NewStringUTF(geofences.c_str());
}

// --- Schedule ---

JNIEXPORT void JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeStartSchedule(JNIEnv*, jobject, jlong ptr) {
    reinterpret_cast<geomony::GeomonyCore*>(ptr)->startSchedule();
}

JNIEXPORT void JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeStopSchedule(JNIEnv*, jobject, jlong ptr) {
    reinterpret_cast<geomony::GeomonyCore*>(ptr)->stopSchedule();
}

JNIEXPORT void JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeOnScheduleTimerFired(
    JNIEnv*, jobject, jlong ptr,
    jint year, jint month, jint day, jint dayOfWeek,
    jint hour, jint minute, jint second) {
    reinterpret_cast<geomony::GeomonyCore*>(ptr)->onScheduleTimerFired(
        year, month, day, dayOfWeek, hour, minute, second);
}

JNIEXPORT jboolean JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeGetStopOnTerminate(JNIEnv*, jobject, jlong ptr) {
    return reinterpret_cast<geomony::GeomonyCore*>(ptr)->getStopOnTerminate();
}

JNIEXPORT void JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeOnSyncComplete(JNIEnv* env, jobject, jlong ptr,
    jint requestId, jboolean success, jint httpStatus, jstring responseText) {
    auto core = reinterpret_cast<geomony::GeomonyCore*>(ptr);
    const char* resp = env->GetStringUTFChars(responseText, nullptr);
    core->onSyncComplete(requestId, success, httpStatus, resp);
    env->ReleaseStringUTFChars(responseText, resp);
}

JNIEXPORT void JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeOnSyncRetryTimerFired(JNIEnv*, jobject, jlong ptr) {
    reinterpret_cast<geomony::GeomonyCore*>(ptr)->onSyncRetryTimerFired();
}

JNIEXPORT void JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeUpdateAuthorizationHeaders(JNIEnv* env, jobject, jlong ptr, jstring headersJson) {
    auto core = reinterpret_cast<geomony::GeomonyCore*>(ptr);
    const char* str = env->GetStringUTFChars(headersJson, nullptr);
    core->updateAuthorizationHeaders(str);
    env->ReleaseStringUTFChars(headersJson, str);
}

JNIEXPORT jstring JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeGetGeofenceEvents(JNIEnv* env, jobject, jlong ptr) {
    auto events = reinterpret_cast<geomony::GeomonyCore*>(ptr)->getGeofenceEvents();
    return env->NewStringUTF(events.c_str());
}

JNIEXPORT jint JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeGetGeofenceEventCount(JNIEnv*, jobject, jlong ptr) {
    return reinterpret_cast<geomony::GeomonyCore*>(ptr)->getGeofenceEventCount();
}

JNIEXPORT jboolean JNICALL
Java_com_geomony_GeomonyPlatformBridge_nativeDestroyGeofenceEvents(JNIEnv*, jobject, jlong ptr) {
    return reinterpret_cast<geomony::GeomonyCore*>(ptr)->destroyGeofenceEvents();
}

} // extern "C"
