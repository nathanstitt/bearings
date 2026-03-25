# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Bearings is a React Native Turbo Module for battery-conscious background location tracking and geofencing. The core logic is implemented in shared C++17, with thin platform bridges for iOS (Objective-C++) and Android (Kotlin + JNI).

## Commands

```sh
yarn                    # Install dependencies (must use Yarn, not npm)
yarn typecheck          # Type-check with TypeScript
yarn lint               # Lint with ESLint
yarn lint --fix         # Auto-fix lint issues
yarn test               # Run Jest unit tests
yarn build:ios          # Build iOS (xcodebuild, no simulator launch)
yarn example start      # Start Metro bundler for example app
yarn example ios        # Build & run example app on iOS
yarn example android    # Build & run example app on Android
```

## Architecture

### Three-layer design

1. **C++ core** (`cpp/`) — All business logic: state machine, location storage (SQLite), config management. Platform-agnostic.
   - `BearingsCore` — Main entry point. Owns the state machine (UNKNOWN → MOVING ↔ STATIONARY), config, and location store.
   - `PlatformBridge` — Abstract interface that platform layers implement to provide OS-specific capabilities (location updates, geofencing, motion activity, timers).
   - `LocationStore` — SQLite-backed persistent location storage.
   - Built via CMake. Android builds a shared library (with JNI); iOS builds a static library (sources compiled via CocoaPods).

2. **Platform bridges** — Implement `PlatformBridge` in native code:
   - **iOS** (`ios/`) — `BearingsPlatformBridge.mm` implements the C++ `PlatformBridge`. `BearingsLocationDelegate` wraps `CLLocationManager`. `Bearings.mm` is the Turbo Module entry point.
   - **Android** (`android/`) — `BearingsPlatformBridge.kt` calls C++ via JNI (`android/src/main/cpp/bearings_jni.cpp`). `BearingsLocationService.kt` uses FusedLocationProvider. `BearingsForegroundService.kt` keeps tracking alive in background. `BearingsModule.kt` is the Turbo Module entry point.

3. **TypeScript layer** (`src/`) — Thin wrapper over the Turbo Module. All native methods exchange JSON strings which are parsed/serialized at the JS boundary.
   - `NativeBearings.ts` — TurboModule spec (codegen input).
   - `index.tsx` — Public API: `configure`, `start`, `stop`, `getState`, `getLocations`, `getCount`, `destroyLocations`, `onLocation`, `onMotionChange`.
   - `types.ts` — TypeScript interfaces for `Location`, `Config`, `State`, `MotionChangeEvent`.

### Key patterns

- **JSON string bridge**: Native methods accept/return JSON strings (not typed objects) across the JS↔Native boundary. The TypeScript layer handles `JSON.stringify`/`JSON.parse`.
- **Event emission**: Native events (`location`, `motionchange`, `error`) are dispatched via `NativeEventEmitter`. The C++ core calls `PlatformBridge::dispatchEvent()` which routes through each platform's event system.
- **Vendored C dependencies** in `cpp/vendor/`: SQLite (`sqlite3/`) and nlohmann/json (`nlohmann/json.hpp`). Both are compiled/included with specific flags (see `CMakeLists.txt` and `Bearings.podspec`). Use nlohmann/json for all JSON construction and parsing in C++ code.

## Commit Convention

Follow [Conventional Commits](https://www.conventionalcommits.org/): `fix:`, `feat:`, `refactor:`, `docs:`, `test:`, `chore:`. Pre-commit hooks enforce this via commitlint.

## Monorepo Structure

Yarn workspaces with two packages: the library (root) and the example app (`example/`). The example app uses Expo with `expo run:ios`/`expo run:android` (dev client, not Expo Go). Always refer to CONTRIBUTING.md for detailed development workflow.
