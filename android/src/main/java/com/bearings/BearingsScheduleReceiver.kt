package com.bearings

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent

class BearingsScheduleReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        BearingsPlatformBridge.instance?.locationService?.evaluateScheduleNow()
    }
}
