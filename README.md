# Geomony

Battery-conscious background location tracking & geofencing SDK for React Native.

Built as a [Turbo Module](https://reactnative.dev/docs/the-new-architecture/pillars-turbomodules) with shared C++17 core logic and thin platform bridges for iOS and Android.

## Features

- Background location tracking with configurable accuracy and distance filters
- Offline-aware HTTP sync with batching, exponential backoff, and priority geofence sync
- Custom HTTP headers for authenticated sync (e.g. `Authorization: Bearer <token>`)
- HTTP event callbacks for detecting 401s and handling token refresh
- Geofencing with ENTER, EXIT, and DWELL events
- Geofence event persistence and sync alongside locations
- Schedule-based tracking windows
- SQLite-backed persistent location storage
- Motion-change detection (MOVING / STATIONARY state machine)
- Foreground service support on Android

## Installation

```sh
yarn add geomony
```

### iOS

```sh
cd ios && pod install
```

### Android

No additional steps — the native module is auto-linked.

## Usage

```typescript
import {
  configure,
  start,
  stop,
  onLocation,
  onMotionChange,
  addGeofence,
  addGeofences,
  onGeofence,
  onAuthorizationRefresh,
} from 'geomony';

// Configure with auth headers
await configure({
  desiredAccuracy: -1,
  distanceFilter: 10,
  stopOnTerminate: false,
  startOnBoot: true,
  debug: false,
  url: 'https://my-server.com/locations',
  syncThreshold: 5,
  maxBatchSize: 100,
  headers: {
    Authorization: 'Bearer my-jwt-token',
  },
});

// Subscribe to location updates
const locationSub = onLocation((location) => {
  console.log('[location]', location.latitude, location.longitude);
});

// Subscribe to motion changes
const motionSub = onMotionChange((event) => {
  console.log('[motion]', event.isMoving ? 'MOVING' : 'STATIONARY');
});

// Handle auth failures — geomony calls this on 401/403, retries automatically
const authSub = onAuthorizationRefresh(async () => {
  const newToken = await refreshMyToken();
  return { Authorization: `Bearer ${newToken}` };
});

// Start tracking
await start();

// Add a geofence with structured extras
await addGeofence({
  identifier: 'home',
  latitude: 40.785091,
  longitude: -73.968285,
  radius: 200,
  notifyOnEntry: true,
  notifyOnExit: true,
  notifyOnDwell: false,
  loiteringDelay: 0,
  extras: { jobId: '123', orgId: '456' },
});

// Or add multiple geofences at once
await addGeofences([
  {
    identifier: 'office',
    latitude: 40.7128,
    longitude: -74.006,
    radius: 150,
    notifyOnEntry: true,
    notifyOnExit: true,
    notifyOnDwell: false,
    loiteringDelay: 0,
  },
]);

// Subscribe to geofence events
const geofenceSub = onGeofence((event) => {
  console.log('[geofence]', event.identifier, event.action, event.extras);
});

// Stop tracking
await stop();

// Clean up subscriptions
locationSub.remove();
motionSub.remove();
authSub.remove();
geofenceSub.remove();
```

## API

### Tracking

| Function | Returns | Description |
|----------|---------|-------------|
| `configure(config)` | `Promise<State>` | Set configuration options. Partial configs are merged with defaults. |
| `start()` | `Promise<State>` | Start location tracking. |
| `stop()` | `Promise<State>` | Stop location tracking. |
| `getState()` | `Promise<State>` | Get current tracking state. |

### Location Storage

| Function | Returns | Description |
|----------|---------|-------------|
| `getLocations()` | `Promise<Location[]>` | Retrieve all stored locations. |
| `getCount()` | `Promise<number>` | Get count of stored locations. |
| `destroyLocations()` | `Promise<boolean>` | Delete all stored locations. |

### Geofencing

| Function | Returns | Description |
|----------|---------|-------------|
| `addGeofence(geofence)` | `Promise<boolean>` | Register a geofence. |
| `addGeofences(geofences)` | `Promise<boolean>` | Register multiple geofences. Returns `false` if any fails. |
| `removeGeofence(identifier)` | `Promise<boolean>` | Remove a geofence by identifier. |
| `removeGeofences()` | `Promise<boolean>` | Remove all geofences. |
| `getGeofences()` | `Promise<Geofence[]>` | Get all registered geofences. |
| `getGeofenceEvents()` | `Promise<PersistedGeofenceEvent[]>` | Get all persisted geofence events. |
| `getGeofenceEventCount()` | `Promise<number>` | Get count of persisted geofence events. |
| `destroyGeofenceEvents()` | `Promise<boolean>` | Delete all persisted geofence events. |

### Scheduling

| Function | Returns | Description |
|----------|---------|-------------|
| `startSchedule()` | `Promise<State>` | Start schedule-based tracking. |
| `stopSchedule()` | `Promise<State>` | Stop schedule-based tracking. |

Schedule-based tracking automatically starts and stops tracking during configured time windows. Outside those windows, all location monitoring is completely off — no GPS, no geofences, no motion activity.

Configure schedule rules as an array of strings in the `schedule` config option. Each rule has the format `"<days> <startTime>-<endTime>"`.

**Day-of-week rules** use 1=Sunday through 7=Saturday. Days can be individual values or ranges, comma-separated:

```typescript
await configure({
  schedule: [
    '2-6 09:00-17:00',       // Mon–Fri, 9am–5pm
    '7 10:00-14:00',         // Saturday, 10am–2pm
    '1 00:00-00:00',         // Sunday, all day (midnight to midnight)
    '2,4,6 18:00-22:00',     // Mon, Wed, Fri evenings
    '5 22:00-06:00',         // Thursday night into Friday morning (crosses midnight)
  ],
});
await startSchedule();
```

**Literal date rules** use `YYYY-MM-DD` format for one-off windows:

```typescript
await configure({
  schedule: [
    '2026-04-15 08:00-20:00',  // April 15th only, 8am–8pm
  ],
});
```

When multiple rules overlap, tracking is active if *any* rule matches. The scheduler evaluates rules at each transition boundary and sets a timer for the next one — it does not poll on an interval. An `onSchedule` event fires when tracking starts or stops due to the schedule.

**Platform note:** On Android, schedule timers use `AlarmManager.setExactAndAllowWhileIdle`, which reliably wakes the app from the background. On iOS, schedule evaluation piggybacks on incoming location events — `dispatch_source` timers do not fire while the app is suspended. This means schedule transitions on iOS may be delayed until the next location update or geofence event wakes the app.

### Events

| Function | Event Data | Description |
|----------|------------|-------------|
| `onLocation(callback)` | `Location` | Fired on each new location. |
| `onGeofence(callback)` | `GeofenceEvent` | Fired on geofence ENTER, EXIT, or DWELL. |
| `onMotionChange(callback)` | `MotionChangeEvent` | Fired when motion state changes. |
| `onActivityChange(callback)` | `ActivityChangeEvent` | Fired when detected activity changes (walking, driving, etc). |
| `onSchedule(callback)` | `ScheduleEvent` | Fired when a schedule window starts or stops. |
| `onHttp(callback)` | `HttpEvent` | Fired after each sync HTTP request with status, response text, and success flag. |
| `onAuthorizationRefresh(callback)` | `AuthorizationRefreshEvent` | Fired on 401/403 sync failure. Callback returns fresh headers; sync retries immediately. |

All event subscribers return a `Subscription` with a `remove()` method.

### HTTP Sync

When a `url` is configured, Geomony automatically POSTs stored locations and geofence events to your server. Sync is offline-aware — records accumulate in SQLite while offline and flush when connectivity is restored.

**Behavior:**
- Sync triggers when the total unsynced count (locations + geofence events) reaches `syncThreshold`.
- Geofence ENTER/EXIT events trigger an immediate sync regardless of threshold.
- Custom HTTP headers (e.g. `Authorization`) are sent with every sync request via the `headers` config option.
- The `onHttp` event fires after each sync HTTP request with `{ status, responseText, success }`.
- On 401/403 responses, geomony dispatches an `authorizationRefresh` event instead of entering backoff. The `onAuthorizationRefresh` callback returns fresh headers and sync retries immediately.
- On other HTTP failures, the device is treated as offline and retries with exponential backoff (base `syncRetryBaseSeconds`, capped at 300s).
- When connectivity is restored, the backoff resets and pending records sync immediately.
- Records are never marked as synced until the server responds with a 2xx status.
- Only one sync request is in flight at a time; remaining records flush in subsequent batches.

**POST payload format:**
```json
{
  "location": [
    {
      "uuid": "...",
      "timestamp": "2026-01-01T00:00:00Z",
      "latitude": 40.7128,
      "longitude": -74.006,
      "altitude": 10.0,
      "speed": 1.5,
      "heading": 180.0,
      "accuracy": 5.0,
      "speed_accuracy": 1.0,
      "heading_accuracy": 10.0,
      "altitude_accuracy": 3.0,
      "is_moving": true,
      "activity": { "type": "walking", "confidence": 85 },
      "event": "",
      "extras": ""
    }
  ],
  "geofence": [
    {
      "identifier": "home",
      "action": "ENTER",
      "latitude": 40.785,
      "longitude": -73.968,
      "accuracy": 5.0,
      "extras": { "jobId": "123" },
      "timestamp": "2026-01-01T00:00:00Z"
    }
  ]
}
```

The `location` and `geofence` keys are only present when there are unsynced records of that type.

**Auth headers example:**

```typescript
// Set initial headers
await configure({
  url: 'https://api.example.com/locations',
  headers: { Authorization: `Bearer ${token}` },
});

// Handle 401/403 — refresh token and retry automatically
onAuthorizationRefresh(async () => {
  const newToken = await refreshToken();
  return { Authorization: `Bearer ${newToken}` };
});
```

When the sync endpoint returns 401 or 403, geomony pauses sync (no backoff timer) and fires the `onAuthorizationRefresh` callback. The callback should return a `Record<string, string>` with updated headers, or `null` to skip the retry. On receiving new headers, geomony updates the config and immediately retries the sync flush.

The `getState()` response includes a `sync` object:

| Field | Type | Description |
|-------|------|-------------|
| `sync.enabled` | `boolean` | `true` when `url` is configured. |
| `sync.connected` | `boolean` | Current connectivity status. |
| `sync.syncInFlight` | `boolean` | Whether a sync request is currently in progress. |
| `sync.unsyncedCount` | `number` | Number of locations awaiting sync. |
| `sync.geofenceEventUnsyncedCount` | `number` | Number of geofence events awaiting sync. |

### Config

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `desiredAccuracy` | `number` | `-1` | GPS accuracy (platform-specific). |
| `distanceFilter` | `number` | `10` | Minimum distance (meters) between updates. |
| `stationaryRadius` | `number` | `25` | Radius (meters) for stationary detection. |
| `stopTimeout` | `number` | `5` | Minutes without movement before entering STATIONARY. |
| `debug` | `boolean` | `false` | Enable debug logging and sound effects. |
| `stopOnTerminate` | `boolean` | `true` | Stop tracking when the app is terminated (see below). |
| `startOnBoot` | `boolean` | `false` | Resume tracking after device reboot. |
| `url` | `string` | `''` | URL for automatic location sync (POST endpoint). |
| `syncThreshold` | `number` | `5` | Number of unsynced locations before a sync is triggered. |
| `maxBatchSize` | `number` | `100` | Maximum locations per HTTP POST request. |
| `syncRetryBaseSeconds` | `number` | `10` | Base delay (seconds) for exponential backoff on sync failure. |
| `headers` | `Record<string, string>` | `{}` | Custom HTTP headers sent with every sync request (e.g. `{ Authorization: 'Bearer ...' }`). |
| `enabled` | `boolean` | `false` | Whether tracking is currently enabled. |
| `schedule` | `string[]` | — | Schedule windows for time-based tracking. |
| `scheduleUseAlarmManager` | `boolean` | — | Use AlarmManager for schedule triggers (Android). |

### `stopOnTerminate`

Controls what happens when the user swipes the app away or the OS terminates it.

**`stopOnTerminate: true` (default)** — All tracking stops. Location updates, motion activity, geofences, timers, and sync are fully cleaned up.

**`stopOnTerminate: false`** — Tracking survives app termination:

- **iOS**: A stationary geofence is left at the last known position. When the user moves beyond it, iOS relaunches the app in the background. Your app must call `configure()` on every launch to reconnect the C++ core — the JS layer is responsible for restoring state.
- **Android**: The foreground service continues running headlessly via `START_STICKY`. The `onTaskRemoved` handler keeps the service alive instead of calling `stopSelf()`.

On termination with `stopOnTerminate: false`, timers and motion activity monitoring are cancelled (they won't survive process death), but the stationary geofence is preserved. If the device was in MOVING state, a new stationary geofence is placed at the last known position. If already STATIONARY, the existing geofence is left in place.

## Battery

Geomony is designed around minimizing GPS usage — the single largest battery drain in location tracking apps.

### Motion state machine

The core optimization. The device is always in one of two states:

- **MOVING** — GPS is active, receiving location updates at the configured `distanceFilter`.
- **STATIONARY** — GPS is completely off. A low-power geofence (`stationaryRadius`, default 25m) monitors for movement instead. Geofence monitoring uses cell/Wi-Fi radios and the motion coprocessor, consuming roughly 100x less power than GPS.

Transitions are debounced by `stopTimeout` (default 5 minutes). When the motion activity sensor reports "still", a timer starts. If movement resumes before the timer fires, it's cancelled — this prevents rapid state oscillations at traffic lights or brief stops. Only after the full `stopTimeout` elapses does the device transition to STATIONARY and turn off GPS.

### Motion activity detection

Both platforms provide hardware-level activity classification (still, walking, running, cycling, driving) via dedicated low-power coprocessors (Apple M-series motion coprocessor, Android Activity Recognition API). These sensors run continuously at negligible power cost and drive the state machine without requiring GPS.

Low-confidence readings are filtered out on iOS to avoid spurious state transitions.

### Distance filter

Locations closer than `distanceFilter` meters to the last dispatched location are silently dropped — they aren't stored, dispatched, or synced. This is measured from the last *dispatched* position, not the last *received* position, so the baseline only advances on meaningful movement. Reduces GPS wake-up processing by 50-90% in typical use.

### Accuracy levels

The `desiredAccuracy` config trades accuracy for power:

| Value | iOS | Android | Power |
|-------|-----|---------|-------|
| `-1` | Best | `PRIORITY_HIGH_ACCURACY` | Highest (GPS) |
| `-2` | 10m | `PRIORITY_BALANCED_POWER_ACCURACY` | Medium (GPS + network) |
| `-3` | 100m | `PRIORITY_LOW_POWER` | Lowest (network/Wi-Fi only) |

At `-3`, GPS hardware is never activated.

### Schedule-based tracking

When `schedule` rules are configured, tracking automatically starts and stops at defined time windows (e.g., weekday business hours only). Outside those windows, all location monitoring is completely off — no GPS, no geofences, no motion activity. The schedule timer fires only at transition boundaries, not on a poll interval.

### Sync batching

Locations accumulate in SQLite and are POSTed in batches of up to `maxBatchSize` when the unsynced count reaches `syncThreshold`. This amortizes the cost of radio wake-up — one request with 100 locations costs barely more battery than one request with 1. On sync failure, exponential backoff (10s base, 300s cap) prevents the radio from thrashing while offline. Locations are never lost; they sync when connectivity returns.

## Architecture

```
┌─────────────────────────────────────────────┐
│  TypeScript API  (src/)                     │
│  configure · start · stop · onLocation · …  │
└──────────────────┬──────────────────────────┘
                   │ JSON strings
┌──────────────────┴──────────────────────────┐
│  C++ Core  (cpp/)                           │
│  State machine · SQLite store · Config      │
├─────────────────┬───────────────────────────┤
│  iOS Bridge     │  Android Bridge           │
│  (Obj-C++)      │  (Kotlin + JNI)           │
│  CLLocationMgr  │  FusedLocationProvider    │
│                 │  ForegroundService        │
└─────────────────┴───────────────────────────┘
```

- **C++ core** (`cpp/`) — Platform-agnostic business logic: state machine (UNKNOWN -> MOVING <-> STATIONARY), SQLite location storage, config management, geofence tracking, scheduling, and offline-aware HTTP sync orchestration.
- **iOS bridge** (`ios/`) — `CLLocationManager`-based location delegate, Turbo Module entry point.
- **Android bridge** (`android/`) — `FusedLocationProviderClient`, foreground service for background tracking, JNI layer to C++ core.
- **TypeScript layer** (`src/`) — Thin wrapper over the Turbo Module. All native methods exchange JSON strings across the JS-Native boundary.

## Development

```sh
yarn                    # Install dependencies
yarn typecheck          # Type-check with TypeScript
yarn lint               # Lint with ESLint
yarn test               # Run Jest tests
yarn example start      # Start Metro bundler for example app
yarn example ios        # Build & run example on iOS
yarn example android    # Build & run example on Android
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for the full development workflow.

## License

MIT
