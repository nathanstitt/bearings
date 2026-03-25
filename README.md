# Bearings

Battery-conscious background location tracking & geofencing SDK for React Native.

Built as a [Turbo Module](https://reactnative.dev/docs/the-new-architecture/pillars-turbomodules) with shared C++17 core logic and thin platform bridges for iOS and Android.

## Features

- Background location tracking with configurable accuracy and distance filters
- Offline-aware HTTP sync with batching, exponential backoff, and priority geofence sync
- Geofencing with ENTER, EXIT, and DWELL events
- Schedule-based tracking windows
- SQLite-backed persistent location storage
- Motion-change detection (MOVING / STATIONARY state machine)
- Foreground service support on Android

## Installation

```sh
yarn add bearings
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
  onGeofence,
} from 'bearings';

// Configure
await configure({
  desiredAccuracy: -1,
  distanceFilter: 10,
  stopOnTerminate: false,
  startOnBoot: true,
  debug: false,
  url: 'https://my-server.com/locations',
  syncThreshold: 5,
  maxBatchSize: 100,
});

// Subscribe to location updates
const locationSub = onLocation((location) => {
  console.log('[location]', location.latitude, location.longitude);
});

// Subscribe to motion changes
const motionSub = onMotionChange((event) => {
  console.log('[motion]', event.isMoving ? 'MOVING' : 'STATIONARY');
});

// Start tracking
await start();

// Add a geofence
await addGeofence({
  identifier: 'home',
  latitude: 40.785091,
  longitude: -73.968285,
  radius: 200,
  notifyOnEntry: true,
  notifyOnExit: true,
  notifyOnDwell: false,
  loiteringDelay: 0,
});

// Subscribe to geofence events
const geofenceSub = onGeofence((event) => {
  console.log('[geofence]', event.identifier, event.action);
});

// Stop tracking
await stop();

// Clean up subscriptions
locationSub.remove();
motionSub.remove();
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
| `removeGeofence(identifier)` | `Promise<boolean>` | Remove a geofence by identifier. |
| `removeGeofences()` | `Promise<boolean>` | Remove all geofences. |
| `getGeofences()` | `Promise<Geofence[]>` | Get all registered geofences. |

### Scheduling

| Function | Returns | Description |
|----------|---------|-------------|
| `startSchedule()` | `Promise<State>` | Start schedule-based tracking. |
| `stopSchedule()` | `Promise<State>` | Stop schedule-based tracking. |

### Events

| Function | Event Data | Description |
|----------|------------|-------------|
| `onLocation(callback)` | `Location` | Fired on each new location. |
| `onGeofence(callback)` | `GeofenceEvent` | Fired on geofence ENTER, EXIT, or DWELL. |
| `onMotionChange(callback)` | `MotionChangeEvent` | Fired when motion state changes. |
| `onSchedule(callback)` | `ScheduleEvent` | Fired when a schedule window starts or stops. |

All event subscribers return a `Subscription` with a `remove()` method.

### HTTP Sync

When a `url` is configured, Bearings automatically POSTs stored locations to your server. Sync is offline-aware — locations accumulate in SQLite while offline and flush when connectivity is restored.

**Behavior:**
- Sync triggers when unsynced location count reaches `syncThreshold`.
- Geofence ENTER/EXIT events trigger an immediate sync regardless of threshold.
- On HTTP failure, the device is treated as offline and retries with exponential backoff (base `syncRetryBaseSeconds`, capped at 300s).
- When connectivity is restored, the backoff resets and pending locations sync immediately.
- Locations are never marked as synced until the server responds with success.
- Only one sync request is in flight at a time; remaining locations flush in subsequent batches.

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
  ]
}
```

The `getState()` response includes a `sync` object:

| Field | Type | Description |
|-------|------|-------------|
| `sync.enabled` | `boolean` | `true` when `url` is configured. |
| `sync.connected` | `boolean` | Current connectivity status. |
| `sync.syncInFlight` | `boolean` | Whether a sync request is currently in progress. |
| `sync.unsyncedCount` | `number` | Number of locations awaiting sync. |

### Config

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `desiredAccuracy` | `number` | `-1` | GPS accuracy (platform-specific). |
| `distanceFilter` | `number` | `10` | Minimum distance (meters) between updates. |
| `stationaryRadius` | `number` | `25` | Radius (meters) for stationary detection. |
| `stopTimeout` | `number` | `5` | Minutes without movement before entering STATIONARY. |
| `debug` | `boolean` | `false` | Enable debug logging and sound effects. |
| `stopOnTerminate` | `boolean` | `true` | Stop tracking when the app is terminated. |
| `startOnBoot` | `boolean` | `false` | Resume tracking after device reboot. |
| `url` | `string` | `''` | URL for automatic location sync (POST endpoint). |
| `syncThreshold` | `number` | `5` | Number of unsynced locations before a sync is triggered. |
| `maxBatchSize` | `number` | `100` | Maximum locations per HTTP POST request. |
| `syncRetryBaseSeconds` | `number` | `10` | Base delay (seconds) for exponential backoff on sync failure. |
| `schedule` | `string[]` | — | Schedule windows for time-based tracking. |
| `scheduleUseAlarmManager` | `boolean` | — | Use AlarmManager for schedule triggers (Android). |

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
