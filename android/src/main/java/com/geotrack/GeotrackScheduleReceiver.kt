package com.geotrack

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent

class GeotrackScheduleReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        GeotrackPlatformBridge.instance?.locationService?.evaluateScheduleNow()
    }
}
