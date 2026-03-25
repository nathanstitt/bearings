package com.bearings

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import com.google.android.gms.location.Geofence
import com.google.android.gms.location.GeofencingEvent

class BearingsGeofenceReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        val event = GeofencingEvent.fromIntent(intent) ?: return
        if (event.hasError()) return

        val bridge = BearingsPlatformBridge.instance ?: return

        for (geofence in event.triggeringGeofences ?: emptyList()) {
            val id = geofence.requestId

            if (id.startsWith("__bearings_")) {
                // Internal stationary geofence
                if (event.geofenceTransition == Geofence.GEOFENCE_TRANSITION_EXIT) {
                    bridge.notifyGeofenceExit()
                }
            } else {
                // User geofence
                val action = when (event.geofenceTransition) {
                    Geofence.GEOFENCE_TRANSITION_ENTER -> "ENTER"
                    Geofence.GEOFENCE_TRANSITION_EXIT -> "EXIT"
                    Geofence.GEOFENCE_TRANSITION_DWELL -> "DWELL"
                    else -> continue
                }
                bridge.notifyGeofenceEvent(id, action)
            }
        }
    }
}
