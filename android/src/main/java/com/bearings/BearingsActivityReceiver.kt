package com.bearings

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import com.google.android.gms.location.ActivityTransitionResult
import com.google.android.gms.location.DetectedActivity

class BearingsActivityReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        if (!ActivityTransitionResult.hasResult(intent)) return
        val result = ActivityTransitionResult.extractResult(intent) ?: return

        for (event in result.transitionEvents) {
            // Map to activity type codes: 0=unknown, 1=stationary, 2=walking, 3=running, 4=cycling, 5=automotive
            val activityType = when (event.activityType) {
                DetectedActivity.STILL -> 1
                DetectedActivity.WALKING -> 2
                DetectedActivity.RUNNING -> 3
                DetectedActivity.ON_BICYCLE -> 4
                DetectedActivity.IN_VEHICLE -> 5
                else -> continue
            }
            // Activity transitions are high-confidence events
            BearingsPlatformBridge.instance?.notifyMotionDetected(activityType, 100)
        }
    }
}
