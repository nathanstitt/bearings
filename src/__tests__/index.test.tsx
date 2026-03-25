import { GeotrackSimulator } from './harness/GeotrackSimulator';
import type {
  Location,
  GeofenceEvent,
  MotionChangeEvent,
  ActivityChangeEvent,
} from '../types';

const mockSimulator = new GeotrackSimulator();

jest.mock('../NativeGeotrack', () => ({
  __esModule: true,
  default: mockSimulator.nativeModule,
}));

jest.mock('react-native', () => ({
  NativeEventEmitter: jest.fn(() => mockSimulator.emitter),
  TurboModuleRegistry: {
    getEnforcing: jest.fn(() => mockSimulator.nativeModule),
  },
}));

const {
  configure,
  start,
  stop,
  getState,
  addGeofence,
  removeGeofence,
  removeGeofences,
  getGeofences,
  getLocations,
  getCount,
  destroyLocations,
  onLocation,
  onGeofence,
  onMotionChange,
  onActivityChange,
} = require('../index') as typeof import('../index');

beforeEach(() => {
  mockSimulator.reset();
});

describe('configure', () => {
  it('stringifies config and parses the response', async () => {
    const state = await configure({ distanceFilter: 50, debug: true });
    expect(state.config.distanceFilter).toBe(50);
    expect(state.config.debug).toBe(true);
    expect(state.enabled).toBe(false);
  });

  it('merges partial config with defaults', async () => {
    const state = await configure({ distanceFilter: 20 });
    expect(state.config.distanceFilter).toBe(20);
    expect(state.config.stationaryRadius).toBe(25); // default
  });
});

describe('start / stop', () => {
  it('start returns enabled and tracking state', async () => {
    const state = await start();
    expect(state.enabled).toBe(true);
    expect(state.tracking).toBe(true);
    expect(state.isMoving).toBe(true);
  });

  it('stop returns non-tracking state', async () => {
    await start();
    const state = await stop();
    expect(state.tracking).toBe(false);
    expect(state.isMoving).toBe(false);
  });
});

describe('getState', () => {
  it('returns current state', async () => {
    await configure({ debug: true });
    await start();
    const state = await getState();
    expect(state.enabled).toBe(true);
    expect(state.config.debug).toBe(true);
  });
});

describe('geofence management', () => {
  const testGeofence = {
    identifier: 'test',
    latitude: 40.0,
    longitude: -74.0,
    radius: 100,
    notifyOnEntry: true,
    notifyOnExit: true,
    notifyOnDwell: false,
    loiteringDelay: 0,
  };

  it('addGeofence serializes geofence to JSON', async () => {
    const result = await addGeofence(testGeofence);
    expect(result).toBe(true);

    const geofences = await getGeofences();
    expect(geofences).toHaveLength(1);
    expect(geofences[0]!.identifier).toBe('test');
  });

  it('removeGeofence removes by identifier', async () => {
    await addGeofence(testGeofence);
    await removeGeofence('test');
    const geofences = await getGeofences();
    expect(geofences).toHaveLength(0);
  });

  it('removeGeofences clears all', async () => {
    await addGeofence(testGeofence);
    await addGeofence({ ...testGeofence, identifier: 'test2' });
    await removeGeofences();
    const geofences = await getGeofences();
    expect(geofences).toHaveLength(0);
  });
});

describe('location storage', () => {
  it('getLocations returns empty array initially', async () => {
    const locs = await getLocations();
    expect(locs).toHaveLength(0);
  });

  it('getCount returns 0 initially', async () => {
    const count = await getCount();
    expect(count).toBe(0);
  });

  it('destroyLocations returns true', async () => {
    const result = await destroyLocations();
    expect(result).toBe(true);
  });
});

describe('onLocation', () => {
  it('parses JSON event and invokes callback with Location', () => {
    const received: Location[] = [];
    const sub = onLocation((loc) => received.push(loc));

    mockSimulator.feedLocations([{ latitude: 40.0, longitude: -74.0 }]);

    expect(received).toHaveLength(1);
    expect(received[0]!.latitude).toBe(40.0);
    expect(received[0]!.longitude).toBe(-74.0);

    sub.remove();
  });

  it('remove() unsubscribes the callback', () => {
    const received: Location[] = [];
    const sub = onLocation((loc) => received.push(loc));
    sub.remove();

    mockSimulator.feedLocations([{ latitude: 40.0, longitude: -74.0 }]);
    expect(received).toHaveLength(0);
  });
});

describe('onGeofence', () => {
  it('parses JSON event and invokes callback with GeofenceEvent', async () => {
    const received: GeofenceEvent[] = [];
    onGeofence((event) => received.push(event));

    await addGeofence({
      identifier: 'zone',
      latitude: 40.0,
      longitude: -74.0,
      radius: 1000,
      notifyOnEntry: true,
      notifyOnExit: false,
      notifyOnDwell: false,
      loiteringDelay: 0,
    });

    mockSimulator.feedLocations([{ latitude: 40.0, longitude: -74.0 }]);

    expect(received).toHaveLength(1);
    expect(received[0]!.identifier).toBe('zone');
    expect(received[0]!.action).toBe('ENTER');
  });
});

describe('onMotionChange', () => {
  it('parses motionchange event and provides isMoving', () => {
    const received: MotionChangeEvent[] = [];
    const sub = onMotionChange((event) => received.push(event));

    mockSimulator.emitEvent(
      'motionchange',
      JSON.stringify({
        id: 1,
        uuid: 'test',
        timestamp: '2026-01-01T00:00:00Z',
        latitude: 40.0,
        longitude: -74.0,
        altitude: 0,
        speed: 0,
        heading: -1,
        accuracy: 5,
        speed_accuracy: -1,
        heading_accuracy: -1,
        altitude_accuracy: -1,
        is_moving: true,
        event: 'motionchange',
        extras: '',
        synced: false,
        created_at: '2026-01-01T00:00:00Z',
      })
    );

    expect(received).toHaveLength(1);
    expect(received[0]!.isMoving).toBe(true);
    expect(received[0]!.location.latitude).toBe(40.0);

    sub.remove();
  });

  it('subscription remove unsubscribes', () => {
    const received: MotionChangeEvent[] = [];
    const sub = onMotionChange((event) => received.push(event));
    sub.remove();

    mockSimulator.emitEvent(
      'motionchange',
      JSON.stringify({
        id: 1,
        uuid: 'test',
        timestamp: '2026-01-01T00:00:00Z',
        latitude: 40.0,
        longitude: -74.0,
        altitude: 0,
        speed: 0,
        heading: -1,
        accuracy: 5,
        speed_accuracy: -1,
        heading_accuracy: -1,
        altitude_accuracy: -1,
        is_moving: false,
        event: 'motionchange',
        extras: '',
        synced: false,
        created_at: '2026-01-01T00:00:00Z',
      })
    );

    expect(received).toHaveLength(0);
  });
});

describe('onActivityChange', () => {
  it('parses activitychange event and provides activity type and confidence', () => {
    const received: ActivityChangeEvent[] = [];
    const sub = onActivityChange((event) => received.push(event));

    mockSimulator.setActivity('walking', 100);

    expect(received).toHaveLength(1);
    expect(received[0]!.activity.type).toBe('walking');
    expect(received[0]!.activity.confidence).toBe(100);

    sub.remove();
  });

  it('fires for each activity type change', () => {
    const received: ActivityChangeEvent[] = [];
    const sub = onActivityChange((event) => received.push(event));

    mockSimulator.setActivity('walking', 100);
    mockSimulator.setActivity('running', 75);
    mockSimulator.setActivity('automotive', 50);

    expect(received).toHaveLength(3);
    expect(received[0]!.activity.type).toBe('walking');
    expect(received[1]!.activity.type).toBe('running');
    expect(received[2]!.activity.type).toBe('automotive');

    sub.remove();
  });

  it('subscription remove unsubscribes', () => {
    const received: ActivityChangeEvent[] = [];
    const sub = onActivityChange((event) => received.push(event));
    sub.remove();

    mockSimulator.setActivity('walking', 100);
    expect(received).toHaveLength(0);
  });
});

describe('location activity', () => {
  it('locations include activity data', () => {
    const received: Location[] = [];
    const sub = onLocation((loc) => received.push(loc));

    mockSimulator.setActivity('cycling', 80);
    mockSimulator.feedLocations([{ latitude: 40.0, longitude: -74.0 }]);

    expect(received).toHaveLength(1);
    expect(received[0]!.activity).toBeDefined();
    expect(received[0]!.activity!.type).toBe('cycling');
    expect(received[0]!.activity!.confidence).toBe(80);

    sub.remove();
  });
});
