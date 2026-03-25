package com.geotrack

import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.modules.core.DeviceEventManagerModule
import java.io.File

class GeotrackPlatformBridge(private val reactContext: ReactApplicationContext) {
    var nativePtr: Long = 0L
        private set

    var locationService: GeotrackLocationService? = null

    init {
        System.loadLibrary("geotrack_core")
        nativePtr = nativeCreate()
        instance = this
    }

    fun configure(configJson: String) {
        nativeConfigure(nativePtr, configJson)
    }

    fun start() {
        nativeStart(nativePtr)
    }

    fun stop() {
        nativeStop(nativePtr)
    }

    fun getState(): String {
        return nativeGetState(nativePtr)
    }

    fun getLocations(): String {
        return nativeGetLocations(nativePtr)
    }

    fun getCount(): Int {
        return nativeGetCount(nativePtr)
    }

    fun destroyLocations(): Boolean {
        return nativeDestroyLocations(nativePtr)
    }

    fun onLocationReceived(
        lat: Double, lng: Double, alt: Double,
        speed: Double, heading: Double, accuracy: Double,
        speedAccuracy: Double, headingAccuracy: Double, altitudeAccuracy: Double,
        timestamp: String, uuid: String
    ) {
        nativeOnLocationReceived(
            nativePtr, lat, lng, alt,
            speed, heading, accuracy,
            speedAccuracy, headingAccuracy, altitudeAccuracy,
            timestamp, uuid
        )
    }

    fun destroy() {
        if (nativePtr != 0L) {
            nativeDestroy(nativePtr)
            nativePtr = 0L
        }
        if (instance === this) {
            instance = null
        }
    }

    // --- Notify methods (called from broadcast receivers) ---

    fun notifyMotionDetected(activityType: Int, confidence: Int) {
        if (nativePtr != 0L) {
            nativeOnMotionDetected(nativePtr, activityType, confidence)
        }
    }

    fun notifyStopTimerFired() {
        if (nativePtr != 0L) {
            nativeOnStopTimerFired(nativePtr)
        }
    }

    fun notifyGeofenceExit() {
        if (nativePtr != 0L) {
            nativeOnGeofenceExit(nativePtr)
        }
    }

    fun notifyGeofenceEvent(identifier: String, action: String) {
        if (nativePtr != 0L) {
            nativeOnGeofenceEvent(nativePtr, identifier, action)
        }
    }

    fun startSchedule() {
        if (nativePtr != 0L) {
            nativeStartSchedule(nativePtr)
        }
    }

    fun stopSchedule() {
        if (nativePtr != 0L) {
            nativeStopSchedule(nativePtr)
        }
    }

    fun notifyScheduleTimerFired(year: Int, month: Int, day: Int, dayOfWeek: Int,
                                  hour: Int, minute: Int, second: Int) {
        if (nativePtr != 0L) {
            nativeOnScheduleTimerFired(nativePtr, year, month, day, dayOfWeek, hour, minute, second)
        }
    }

    fun addGeofence(json: String): Boolean {
        if (nativePtr == 0L) return false
        return nativeAddGeofence(nativePtr, json)
    }

    fun removeGeofence(identifier: String): Boolean {
        if (nativePtr == 0L) return false
        return nativeRemoveGeofence(nativePtr, identifier)
    }

    fun removeAllGeofences(): Boolean {
        if (nativePtr == 0L) return false
        return nativeRemoveAllGeofences(nativePtr)
    }

    fun getGeofences(): String {
        if (nativePtr == 0L) return "[]"
        return nativeGetGeofences(nativePtr)
    }

    // --- Callbacks from C++ via JNI ---

    @Suppress("unused")
    fun onStartLocationUpdates(desiredAccuracy: Double, distanceFilter: Double) {
        locationService?.start(desiredAccuracy, distanceFilter)
    }

    @Suppress("unused")
    fun onStopLocationUpdates() {
        locationService?.stop()
    }

    @Suppress("unused")
    fun onDispatchEvent(name: String, json: String) {
        reactContext.getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter::class.java)
            .emit(name, json)
    }

    @Suppress("unused")
    fun onGetDatabasePath(): String {
        val dbDir = reactContext.getDatabasePath("geotrack")?.parentFile
        if (dbDir != null && !dbDir.exists()) {
            dbDir.mkdirs()
        }
        return reactContext.getDatabasePath("geotrack.db")?.absolutePath ?: ""
    }

    @Suppress("unused")
    fun onStartMotionActivity() {
        locationService?.startMotionActivity()
    }

    @Suppress("unused")
    fun onStopMotionActivity() {
        locationService?.stopMotionActivity()
    }

    @Suppress("unused")
    fun onStartStationaryGeofence(lat: Double, lon: Double, radius: Double) {
        locationService?.startStationaryGeofence(lat, lon, radius)
    }

    @Suppress("unused")
    fun onStopStationaryGeofence() {
        locationService?.stopStationaryGeofence()
    }

    @Suppress("unused")
    fun onAddGeofence(
        identifier: String, lat: Double, lon: Double, radius: Double,
        notifyOnEntry: Boolean, notifyOnExit: Boolean,
        notifyOnDwell: Boolean, loiteringDelay: Int
    ) {
        locationService?.addUserGeofence(identifier, lat, lon, radius,
            notifyOnEntry, notifyOnExit, notifyOnDwell, loiteringDelay)
    }

    @Suppress("unused")
    fun onRemoveGeofence(identifier: String) {
        locationService?.removeUserGeofence(identifier)
    }

    @Suppress("unused")
    fun onRemoveAllGeofences() {
        locationService?.removeAllUserGeofences()
    }

    @Suppress("unused")
    fun onStartStopTimer(seconds: Int) {
        locationService?.startStopTimer(seconds)
    }

    @Suppress("unused")
    fun onCancelStopTimer() {
        locationService?.cancelStopTimer()
    }

    @Suppress("unused")
    fun onStartScheduleTimer(delaySeconds: Int) {
        locationService?.startScheduleTimer(delaySeconds)
    }

    @Suppress("unused")
    fun onCancelScheduleTimer() {
        locationService?.cancelScheduleTimer()
    }

    // Native methods
    private external fun nativeCreate(): Long
    private external fun nativeDestroy(ptr: Long)
    private external fun nativeConfigure(ptr: Long, config: String)
    private external fun nativeStart(ptr: Long)
    private external fun nativeStop(ptr: Long)
    private external fun nativeGetState(ptr: Long): String
    private external fun nativeGetLocations(ptr: Long): String
    private external fun nativeGetCount(ptr: Long): Int
    private external fun nativeDestroyLocations(ptr: Long): Boolean
    private external fun nativeOnLocationReceived(
        ptr: Long,
        lat: Double, lng: Double, alt: Double,
        speed: Double, heading: Double, accuracy: Double,
        speedAccuracy: Double, headingAccuracy: Double, altitudeAccuracy: Double,
        timestamp: String, uuid: String
    )
    private external fun nativeOnMotionDetected(ptr: Long, activityType: Int, confidence: Int)
    private external fun nativeOnStopTimerFired(ptr: Long)
    private external fun nativeOnGeofenceExit(ptr: Long)
    private external fun nativeOnGeofenceEvent(ptr: Long, identifier: String, action: String)
    private external fun nativeStartSchedule(ptr: Long)
    private external fun nativeStopSchedule(ptr: Long)
    private external fun nativeOnScheduleTimerFired(ptr: Long, year: Int, month: Int, day: Int,
                                                     dayOfWeek: Int, hour: Int, minute: Int, second: Int)
    private external fun nativeAddGeofence(ptr: Long, json: String): Boolean
    private external fun nativeRemoveGeofence(ptr: Long, identifier: String): Boolean
    private external fun nativeRemoveAllGeofences(ptr: Long): Boolean
    private external fun nativeGetGeofences(ptr: Long): String

    companion object {
        var instance: GeotrackPlatformBridge? = null
            private set
    }
}
