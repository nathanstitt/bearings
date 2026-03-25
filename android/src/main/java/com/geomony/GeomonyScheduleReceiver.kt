package com.geomony

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent

class GeomonyScheduleReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        GeomonyPlatformBridge.instance?.locationService?.evaluateScheduleNow()
    }
}
