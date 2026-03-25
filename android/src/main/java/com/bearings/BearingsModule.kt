package com.bearings

import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactMethod

class BearingsModule(private val reactContext: ReactApplicationContext) :
    NativeBearingsSpec(reactContext) {

    private var platformBridge: BearingsPlatformBridge? = null

    private fun getBridge(): BearingsPlatformBridge {
        if (platformBridge == null) {
            platformBridge = BearingsPlatformBridge(reactContext).also { bridge ->
                bridge.locationService = BearingsLocationService(reactContext, bridge)
            }
        }
        return platformBridge!!
    }

    override fun configure(config: String, promise: Promise) {
        try {
            val bridge = getBridge()
            bridge.configure(config)
            promise.resolve(bridge.getState())
        } catch (e: Exception) {
            promise.reject("BEARINGS_ERROR", e.message, e)
        }
    }

    override fun start(promise: Promise) {
        try {
            val bridge = getBridge()
            BearingsForegroundService.start(reactContext)
            bridge.start()
            promise.resolve(bridge.getState())
        } catch (e: Exception) {
            promise.reject("BEARINGS_ERROR", e.message, e)
        }
    }

    override fun stop(promise: Promise) {
        try {
            val bridge = getBridge()
            bridge.stop()
            BearingsForegroundService.stop(reactContext)
            promise.resolve(bridge.getState())
        } catch (e: Exception) {
            promise.reject("BEARINGS_ERROR", e.message, e)
        }
    }

    override fun getState(promise: Promise) {
        try {
            promise.resolve(getBridge().getState())
        } catch (e: Exception) {
            promise.reject("BEARINGS_ERROR", e.message, e)
        }
    }

    override fun getLocations(promise: Promise) {
        try {
            promise.resolve(getBridge().getLocations())
        } catch (e: Exception) {
            promise.reject("BEARINGS_ERROR", e.message, e)
        }
    }

    override fun getCount(promise: Promise) {
        try {
            promise.resolve(getBridge().getCount().toDouble())
        } catch (e: Exception) {
            promise.reject("BEARINGS_ERROR", e.message, e)
        }
    }

    override fun destroyLocations(promise: Promise) {
        try {
            promise.resolve(getBridge().destroyLocations())
        } catch (e: Exception) {
            promise.reject("BEARINGS_ERROR", e.message, e)
        }
    }

    override fun addGeofence(geofenceJson: String, promise: Promise) {
        try {
            promise.resolve(getBridge().addGeofence(geofenceJson))
        } catch (e: Exception) {
            promise.reject("BEARINGS_ERROR", e.message, e)
        }
    }

    override fun removeGeofence(identifier: String, promise: Promise) {
        try {
            promise.resolve(getBridge().removeGeofence(identifier))
        } catch (e: Exception) {
            promise.reject("BEARINGS_ERROR", e.message, e)
        }
    }

    override fun removeGeofences(promise: Promise) {
        try {
            promise.resolve(getBridge().removeAllGeofences())
        } catch (e: Exception) {
            promise.reject("BEARINGS_ERROR", e.message, e)
        }
    }

    override fun getGeofences(promise: Promise) {
        try {
            promise.resolve(getBridge().getGeofences())
        } catch (e: Exception) {
            promise.reject("BEARINGS_ERROR", e.message, e)
        }
    }

    override fun startSchedule(promise: Promise) {
        try {
            val bridge = getBridge()
            bridge.startSchedule()
            promise.resolve(bridge.getState())
        } catch (e: Exception) {
            promise.reject("BEARINGS_ERROR", e.message, e)
        }
    }

    override fun stopSchedule(promise: Promise) {
        try {
            val bridge = getBridge()
            bridge.stopSchedule()
            promise.resolve(bridge.getState())
        } catch (e: Exception) {
            promise.reject("BEARINGS_ERROR", e.message, e)
        }
    }

    override fun addListener(eventName: String) {
        // Required for event emitter
    }

    override fun removeListeners(count: Double) {
        // Required for event emitter
    }

    override fun invalidate() {
        platformBridge?.let {
            it.locationService?.stop()
            it.destroy()
        }
        platformBridge = null
        super.invalidate()
    }

    companion object {
        const val NAME = NativeBearingsSpec.NAME
    }
}
