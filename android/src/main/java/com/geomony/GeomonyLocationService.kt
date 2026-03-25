package com.geomony

import android.Manifest
import android.app.AlarmManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.location.Location
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import androidx.core.app.ActivityCompat
import com.google.android.gms.location.*
import java.text.SimpleDateFormat
import java.util.*

class GeomonyLocationService(
    private val context: Context,
    private val bridge: GeomonyPlatformBridge
) {
    private val fusedClient: FusedLocationProviderClient =
        LocationServices.getFusedLocationProviderClient(context)

    private var locationCallback: LocationCallback? = null

    // Motion activity
    private val activityClient: ActivityRecognitionClient =
        ActivityRecognition.getClient(context)
    private var activityPendingIntent: PendingIntent? = null

    // Stationary geofence
    private val geofencingClient: GeofencingClient =
        LocationServices.getGeofencingClient(context)
    private var geofencePendingIntent: PendingIntent? = null

    // User geofences
    private var userGeofencePendingIntent: PendingIntent? = null
    private val userGeofenceIds = mutableSetOf<String>()

    // Stop timer
    private val mainHandler = Handler(Looper.getMainLooper())
    private var stopTimerRunnable: Runnable? = null

    // Schedule timer
    private val alarmManager: AlarmManager =
        context.getSystemService(Context.ALARM_SERVICE) as AlarmManager
    private var schedulePendingIntent: PendingIntent? = null

    fun start(desiredAccuracy: Double, distanceFilter: Double) {
        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            return
        }

        val priority = when {
            desiredAccuracy == -1.0 -> Priority.PRIORITY_HIGH_ACCURACY
            desiredAccuracy == -2.0 -> Priority.PRIORITY_BALANCED_POWER_ACCURACY
            desiredAccuracy == -3.0 -> Priority.PRIORITY_LOW_POWER
            else -> Priority.PRIORITY_HIGH_ACCURACY
        }

        val request = LocationRequest.Builder(priority, 1000L)
            .setMinUpdateDistanceMeters(distanceFilter.toFloat())
            .setWaitForAccurateLocation(false)
            .build()

        locationCallback = object : LocationCallback() {
            override fun onLocationResult(result: LocationResult) {
                for (location in result.locations) {
                    onLocationReceived(location)
                }
            }
        }

        fusedClient.requestLocationUpdates(request, locationCallback!!, Looper.getMainLooper())
    }

    fun stop() {
        locationCallback?.let {
            fusedClient.removeLocationUpdates(it)
            locationCallback = null
        }
    }

    // --- Motion Activity ---

    fun startMotionActivity() {
        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.ACTIVITY_RECOGNITION) != PackageManager.PERMISSION_GRANTED) {
            return
        }

        val transitions = listOf(
            ActivityTransition.Builder()
                .setActivityType(DetectedActivity.STILL)
                .setActivityTransition(ActivityTransition.ACTIVITY_TRANSITION_ENTER)
                .build(),
            ActivityTransition.Builder()
                .setActivityType(DetectedActivity.WALKING)
                .setActivityTransition(ActivityTransition.ACTIVITY_TRANSITION_ENTER)
                .build(),
            ActivityTransition.Builder()
                .setActivityType(DetectedActivity.RUNNING)
                .setActivityTransition(ActivityTransition.ACTIVITY_TRANSITION_ENTER)
                .build(),
            ActivityTransition.Builder()
                .setActivityType(DetectedActivity.ON_BICYCLE)
                .setActivityTransition(ActivityTransition.ACTIVITY_TRANSITION_ENTER)
                .build(),
            ActivityTransition.Builder()
                .setActivityType(DetectedActivity.IN_VEHICLE)
                .setActivityTransition(ActivityTransition.ACTIVITY_TRANSITION_ENTER)
                .build()
        )

        val request = ActivityTransitionRequest(transitions)
        val intent = Intent(context, GeomonyActivityReceiver::class.java)
        activityPendingIntent = PendingIntent.getBroadcast(
            context, 100, intent,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_MUTABLE
        )

        activityClient.requestActivityTransitionUpdates(request, activityPendingIntent!!)
    }

    fun stopMotionActivity() {
        activityPendingIntent?.let {
            activityClient.removeActivityTransitionUpdates(it)
            activityPendingIntent = null
        }
    }

    // --- Stationary Geofence ---

    fun startStationaryGeofence(lat: Double, lon: Double, radius: Double) {
        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            return
        }

        val geofence = Geofence.Builder()
            .setRequestId("__geomony_stationary")
            .setCircularRegion(lat, lon, radius.toFloat())
            .setExpirationDuration(Geofence.NEVER_EXPIRE)
            .setTransitionTypes(Geofence.GEOFENCE_TRANSITION_EXIT)
            .build()

        val geofenceRequest = GeofencingRequest.Builder()
            .setInitialTrigger(0)
            .addGeofence(geofence)
            .build()

        val intent = Intent(context, GeomonyGeofenceReceiver::class.java)
        geofencePendingIntent = PendingIntent.getBroadcast(
            context, 101, intent,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_MUTABLE
        )

        geofencingClient.addGeofences(geofenceRequest, geofencePendingIntent!!)
    }

    fun stopStationaryGeofence() {
        geofencePendingIntent?.let {
            geofencingClient.removeGeofences(it)
            geofencePendingIntent = null
        }
    }

    // --- User Geofences ---

    private fun getUserGeofencePendingIntent(): PendingIntent {
        if (userGeofencePendingIntent == null) {
            val intent = Intent(context, GeomonyGeofenceReceiver::class.java)
            userGeofencePendingIntent = PendingIntent.getBroadcast(
                context, 102, intent,
                PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_MUTABLE
            )
        }
        return userGeofencePendingIntent!!
    }

    fun addUserGeofence(
        id: String, lat: Double, lon: Double, radius: Double,
        onEntry: Boolean, onExit: Boolean, onDwell: Boolean, loiteringDelay: Int
    ) {
        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            return
        }

        var transitionTypes = 0
        if (onEntry) transitionTypes = transitionTypes or Geofence.GEOFENCE_TRANSITION_ENTER
        if (onExit) transitionTypes = transitionTypes or Geofence.GEOFENCE_TRANSITION_EXIT
        if (onDwell) transitionTypes = transitionTypes or Geofence.GEOFENCE_TRANSITION_DWELL
        if (transitionTypes == 0) transitionTypes = Geofence.GEOFENCE_TRANSITION_ENTER or Geofence.GEOFENCE_TRANSITION_EXIT

        val builder = Geofence.Builder()
            .setRequestId(id)
            .setCircularRegion(lat, lon, radius.toFloat())
            .setExpirationDuration(Geofence.NEVER_EXPIRE)
            .setTransitionTypes(transitionTypes)

        if (onDwell) {
            builder.setLoiteringDelay(loiteringDelay)
        }

        val geofenceRequest = GeofencingRequest.Builder()
            .setInitialTrigger(GeofencingRequest.INITIAL_TRIGGER_ENTER)
            .addGeofence(builder.build())
            .build()

        geofencingClient.addGeofences(geofenceRequest, getUserGeofencePendingIntent())
        userGeofenceIds.add(id)
    }

    fun removeUserGeofence(id: String) {
        geofencingClient.removeGeofences(listOf(id))
        userGeofenceIds.remove(id)
    }

    fun removeAllUserGeofences() {
        if (userGeofenceIds.isNotEmpty()) {
            geofencingClient.removeGeofences(userGeofenceIds.toList())
            userGeofenceIds.clear()
        }
    }

    // --- Stop Timer ---

    fun startStopTimer(seconds: Int) {
        cancelStopTimer()
        stopTimerRunnable = Runnable {
            bridge.notifyStopTimerFired()
            stopTimerRunnable = null
        }
        mainHandler.postDelayed(stopTimerRunnable!!, seconds * 1000L)
    }

    fun cancelStopTimer() {
        stopTimerRunnable?.let {
            mainHandler.removeCallbacks(it)
            stopTimerRunnable = null
        }
    }

    // --- Schedule Timer ---

    fun startScheduleTimer(delaySeconds: Int) {
        cancelScheduleTimer()

        if (delaySeconds == 0) {
            evaluateScheduleNow()
            return
        }

        val intent = Intent(context, GeomonyScheduleReceiver::class.java)
        schedulePendingIntent = PendingIntent.getBroadcast(
            context, 103, intent,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        val triggerAtMillis = System.currentTimeMillis() + delaySeconds * 1000L

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (alarmManager.canScheduleExactAlarms()) {
                alarmManager.setExactAndAllowWhileIdle(
                    AlarmManager.RTC_WAKEUP, triggerAtMillis, schedulePendingIntent!!
                )
            } else {
                alarmManager.setAndAllowWhileIdle(
                    AlarmManager.RTC_WAKEUP, triggerAtMillis, schedulePendingIntent!!
                )
            }
        } else {
            alarmManager.setExactAndAllowWhileIdle(
                AlarmManager.RTC_WAKEUP, triggerAtMillis, schedulePendingIntent!!
            )
        }
    }

    fun cancelScheduleTimer() {
        schedulePendingIntent?.let {
            alarmManager.cancel(it)
            schedulePendingIntent = null
        }
    }

    fun evaluateScheduleNow() {
        val calendar = Calendar.getInstance()
        bridge.notifyScheduleTimerFired(
            calendar.get(Calendar.YEAR),
            calendar.get(Calendar.MONTH) + 1, // Calendar.MONTH is 0-based
            calendar.get(Calendar.DAY_OF_MONTH),
            calendar.get(Calendar.DAY_OF_WEEK), // 1=Sunday, matches our convention
            calendar.get(Calendar.HOUR_OF_DAY),
            calendar.get(Calendar.MINUTE),
            calendar.get(Calendar.SECOND)
        )
    }

    // --- Sync ---

    private var syncRetryRunnable: Runnable? = null

    fun sendHTTPRequest(url: String, jsonPayload: String, requestId: Int) {
        Thread {
            var success = false
            try {
                val connection = java.net.URL(url).openConnection() as java.net.HttpURLConnection
                connection.requestMethod = "POST"
                connection.setRequestProperty("Content-Type", "application/json")
                connection.doOutput = true
                connection.connectTimeout = 30_000
                connection.readTimeout = 30_000

                connection.outputStream.use { os ->
                    os.write(jsonPayload.toByteArray(Charsets.UTF_8))
                }

                val responseCode = connection.responseCode
                success = responseCode in 200..299
                connection.disconnect()
            } catch (_: Exception) {
                success = false
            }
            val result = success
            mainHandler.post {
                bridge.notifySyncComplete(requestId, result)
            }
        }.start()
    }

    fun startSyncRetryTimer(delaySeconds: Int) {
        cancelSyncRetryTimer()
        syncRetryRunnable = Runnable {
            bridge.notifySyncRetryTimerFired()
            syncRetryRunnable = null
        }
        mainHandler.postDelayed(syncRetryRunnable!!, delaySeconds * 1000L)
    }

    fun cancelSyncRetryTimer() {
        syncRetryRunnable?.let {
            mainHandler.removeCallbacks(it)
            syncRetryRunnable = null
        }
    }

    // --- Location received ---

    private fun onLocationReceived(location: Location) {
        val dateFormat = SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", Locale.US)
        dateFormat.timeZone = TimeZone.getTimeZone("UTC")
        val timestamp = dateFormat.format(Date(location.time))
        val uuid = UUID.randomUUID().toString()

        bridge.onLocationReceived(
            lat = location.latitude,
            lng = location.longitude,
            alt = location.altitude,
            speed = location.speed.toDouble(),
            heading = location.bearing.toDouble(),
            accuracy = location.accuracy.toDouble(),
            speedAccuracy = if (location.hasSpeedAccuracy()) location.speedAccuracyMetersPerSecond.toDouble() else -1.0,
            headingAccuracy = if (location.hasBearingAccuracy()) location.bearingAccuracyDegrees.toDouble() else -1.0,
            altitudeAccuracy = if (location.hasVerticalAccuracy()) location.verticalAccuracyMeters.toDouble() else -1.0,
            timestamp = timestamp,
            uuid = uuid
        )
    }
}
