import type { Spec } from '../../NativeGeotrack';
import type {
  ActivityType,
  Config,
  Geofence,
  Location,
  State,
} from '../../types';
import type { GeofenceTransition, SimulatedLocation } from './types';
import { isInsideGeofence } from './geo';

type EventCallback = (data: string) => void;

interface MockSubscription {
  remove: () => void;
}

const DEFAULT_CONFIG: Config = {
  desiredAccuracy: -1,
  distanceFilter: 10,
  stationaryRadius: 25,
  stopTimeout: 5,
  debug: false,
  stopOnTerminate: true,
  startOnBoot: false,
  url: '',
  enabled: false,
};

export class GeotrackSimulator {
  private config: Config = { ...DEFAULT_CONFIG };
  private enabled = false;
  private tracking = false;
  private isMoving = false;
  private schedulerRunning = false;
  private schedulerTracking = false;
  private currentActivityType: ActivityType = 'unknown';
  private currentActivityConfidence = 0;
  private locations: Location[] = [];
  private geofences: Map<string, Geofence> = new Map();
  private locationIdCounter = 0;

  /**
   * Per-geofence tracking of whether the last known position was inside.
   * `undefined` means no prior position (first location triggers ENTER if inside).
   */
  private geofenceState: Map<string, boolean | undefined> = new Map();

  /** Event listeners keyed by event name. */
  private listeners: Map<string, Set<EventCallback>> = new Map();

  /** Mock NativeEventEmitter instance returned to `new NativeEventEmitter(...)`. */
  readonly emitter = {
    addListener: (
      eventName: string,
      callback: EventCallback
    ): MockSubscription => {
      if (!this.listeners.has(eventName)) {
        this.listeners.set(eventName, new Set());
      }
      this.listeners.get(eventName)!.add(callback);
      return {
        remove: () => {
          this.listeners.get(eventName)?.delete(callback);
        },
      };
    },
    removeAllListeners: (_eventName: string) => {},
    listenerCount: (_eventName: string) => 0,
    removeSubscription: (_subscription: MockSubscription) => {},
    emit: (_eventName: string) => {},
  };

  /** Mock TurboModule satisfying the Spec interface. */
  readonly nativeModule: Spec = {
    configure: async (configJson: string): Promise<string> => {
      const partial = JSON.parse(configJson) as Partial<Config>;
      this.config = { ...this.config, ...partial };
      return JSON.stringify(this.getStateSnapshot());
    },

    start: async (): Promise<string> => {
      this.enabled = true;
      this.tracking = true;
      this.isMoving = true;
      return JSON.stringify(this.getStateSnapshot());
    },

    stop: async (): Promise<string> => {
      this.tracking = false;
      this.isMoving = false;
      return JSON.stringify(this.getStateSnapshot());
    },

    getState: async (): Promise<string> => {
      return JSON.stringify(this.getStateSnapshot());
    },

    getLocations: async (): Promise<string> => {
      return JSON.stringify(this.locations);
    },

    getCount: async (): Promise<number> => {
      return this.locations.length;
    },

    destroyLocations: async (): Promise<boolean> => {
      this.locations = [];
      this.locationIdCounter = 0;
      return true;
    },

    addGeofence: async (geofenceJson: string): Promise<boolean> => {
      const geofence = JSON.parse(geofenceJson) as Geofence;
      this.geofences.set(geofence.identifier, geofence);
      // Initialize geofence state — undefined means "first location decides"
      this.geofenceState.set(geofence.identifier, undefined);
      return true;
    },

    removeGeofence: async (identifier: string): Promise<boolean> => {
      this.geofences.delete(identifier);
      this.geofenceState.delete(identifier);
      return true;
    },

    removeGeofences: async (): Promise<boolean> => {
      this.geofences.clear();
      this.geofenceState.clear();
      return true;
    },

    getGeofences: async (): Promise<string> => {
      return JSON.stringify(Array.from(this.geofences.values()));
    },

    startSchedule: async (): Promise<string> => {
      this.schedulerRunning = true;
      // Simulate immediate evaluation: start tracking if schedule has rules
      if (
        this.config.schedule &&
        this.config.schedule.length > 0 &&
        this.config.schedule.some((r) => r.length > 0)
      ) {
        this.enabled = true;
        this.tracking = true;
        this.isMoving = true;
        this.schedulerTracking = true;
        this.emit('schedule', JSON.stringify({ enabled: true }));
      }
      return JSON.stringify(this.getStateSnapshot());
    },

    stopSchedule: async (): Promise<string> => {
      if (this.schedulerTracking) {
        this.tracking = false;
        this.isMoving = false;
        this.schedulerTracking = false;
        this.emit('schedule', JSON.stringify({ enabled: false }));
      }
      this.schedulerRunning = false;
      return JSON.stringify(this.getStateSnapshot());
    },

    addListener: (_eventName: string) => {},
    removeListeners: (_count: number) => {},
  };

  /**
   * Feed a sequence of locations through the simulator.
   * Returns all geofence transitions that occurred.
   */
  feedLocations(simLocations: SimulatedLocation[]): GeofenceTransition[] {
    const transitions: GeofenceTransition[] = [];

    simLocations.forEach((simLoc, index) => {
      const location = this.buildLocation(simLoc);
      this.locations.push(location);

      // Emit location event
      this.emit('location', JSON.stringify(location));

      // Check geofence transitions
      for (const [identifier, geofence] of this.geofences) {
        const inside = isInsideGeofence(
          { latitude: location.latitude, longitude: location.longitude },
          geofence
        );
        const wasInside = this.geofenceState.get(identifier);

        if (wasInside === undefined) {
          // First location — fire ENTER if inside
          if (inside && geofence.notifyOnEntry) {
            const transition: GeofenceTransition = {
              identifier,
              action: 'ENTER',
              locationIndex: index,
            };
            transitions.push(transition);
            this.emit(
              'geofence',
              JSON.stringify({
                identifier,
                action: 'ENTER',
                location,
              })
            );
          }
        } else if (!wasInside && inside) {
          // Crossed boundary: outside → inside
          if (geofence.notifyOnEntry) {
            const transition: GeofenceTransition = {
              identifier,
              action: 'ENTER',
              locationIndex: index,
            };
            transitions.push(transition);
            this.emit(
              'geofence',
              JSON.stringify({
                identifier,
                action: 'ENTER',
                location,
              })
            );
          }
        } else if (wasInside && !inside) {
          // Crossed boundary: inside → outside
          if (geofence.notifyOnExit) {
            const transition: GeofenceTransition = {
              identifier,
              action: 'EXIT',
              locationIndex: index,
            };
            transitions.push(transition);
            this.emit(
              'geofence',
              JSON.stringify({
                identifier,
                action: 'EXIT',
                location,
              })
            );
          }
        }

        this.geofenceState.set(identifier, inside);
      }
    });

    return transitions;
  }

  /** Reset all state between tests. */
  reset(): void {
    this.config = { ...DEFAULT_CONFIG };
    this.enabled = false;
    this.tracking = false;
    this.isMoving = false;
    this.schedulerRunning = false;
    this.schedulerTracking = false;
    this.currentActivityType = 'unknown';
    this.currentActivityConfidence = 0;
    this.locations = [];
    this.geofences.clear();
    this.geofenceState.clear();
    this.locationIdCounter = 0;
    this.listeners.clear();
  }

  private getStateSnapshot(): State {
    return {
      enabled: this.enabled,
      tracking: this.tracking,
      isMoving: this.isMoving,
      schedulerRunning: this.schedulerRunning,
      activity: {
        type: this.currentActivityType,
        confidence: this.currentActivityConfidence,
      },
      config: { ...this.config },
    };
  }

  private buildLocation(sim: SimulatedLocation): Location {
    this.locationIdCounter++;
    const timestamp = sim.timestamp ?? new Date(Date.now()).toISOString();
    return {
      id: this.locationIdCounter,
      uuid: `sim-${this.locationIdCounter}`,
      timestamp,
      latitude: sim.latitude,
      longitude: sim.longitude,
      altitude: sim.altitude ?? 0,
      speed: sim.speed ?? 0,
      heading: sim.heading ?? -1,
      accuracy: sim.accuracy ?? 5.0,
      speed_accuracy: -1,
      heading_accuracy: -1,
      altitude_accuracy: -1,
      is_moving: this.isMoving,
      activity: {
        type: this.currentActivityType,
        confidence: this.currentActivityConfidence,
      },
      event: 'location',
      extras: '',
      synced: false,
      created_at: timestamp,
    };
  }

  /** Set the current activity type and emit an activitychange event. */
  setActivity(type: ActivityType, confidence: number): void {
    this.currentActivityType = type;
    this.currentActivityConfidence = confidence;
    this.emit(
      'activitychange',
      JSON.stringify({ activity: { type, confidence } })
    );
  }

  /** Emit an arbitrary event (for testing event channels like motionchange). */
  emitEvent(eventName: string, data: string): void {
    this.emit(eventName, data);
  }

  private emit(eventName: string, data: string): void {
    const callbacks = this.listeners.get(eventName);
    if (callbacks) {
      for (const cb of callbacks) {
        cb(data);
      }
    }
  }
}
