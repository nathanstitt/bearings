import { GeotrackSimulator } from './harness/GeotrackSimulator';
import {
  walkPath,
  locationSequence,
  destinationPoint,
} from './harness/locations';
import type { GeofenceEvent } from '../types';

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

// Import the public API *after* mocks are set up (Jest hoists mocks automatically)
const {
  configure,
  start,
  addGeofence,
  removeGeofence,
  onGeofence,
  getLocations,
  getCount,
  destroyLocations,
} = require('../index') as typeof import('../index');

// Central Park, NYC as a reference point
const CENTER: [number, number] = [40.785091, -73.968285];
const RADIUS = 200; // meters

beforeEach(async () => {
  mockSimulator.reset();
  await configure({ distanceFilter: 10 });
  await start();
});

describe('geofence simulation', () => {
  it('fires ENTER when walking toward a geofence', async () => {
    await addGeofence({
      identifier: 'park',
      latitude: CENTER[0],
      longitude: CENTER[1],
      radius: RADIUS,
      notifyOnEntry: true,
      notifyOnExit: true,
      notifyOnDwell: false,
      loiteringDelay: 0,
    });

    // Start 500m north, walk toward center
    const far = destinationPoint(CENTER, 0, 500);
    const locations = walkPath(far, CENTER, 20);
    const transitions = mockSimulator.feedLocations(locations);

    expect(transitions.length).toBeGreaterThanOrEqual(1);
    expect(transitions[0]!.action).toBe('ENTER');
    expect(transitions[0]!.identifier).toBe('park');
  });

  it('fires ENTER then EXIT when walking through a geofence', async () => {
    await addGeofence({
      identifier: 'park',
      latitude: CENTER[0],
      longitude: CENTER[1],
      radius: RADIUS,
      notifyOnEntry: true,
      notifyOnExit: true,
      notifyOnDwell: false,
      loiteringDelay: 0,
    });

    // Walk from 500m north, through center, to 500m south
    const north = destinationPoint(CENTER, 0, 500);
    const south = destinationPoint(CENTER, 180, 500);
    const locations = walkPath(north, south, 40);
    const transitions = mockSimulator.feedLocations(locations);

    expect(transitions).toHaveLength(2);
    expect(transitions[0]!.action).toBe('ENTER');
    expect(transitions[1]!.action).toBe('EXIT');
    expect(transitions[0]!.locationIndex).toBeLessThan(
      transitions[1]!.locationIndex
    );
  });

  it('fires events for multiple geofences in sequence', async () => {
    const centerA: [number, number] = [40.785091, -73.968285];
    const centerB = destinationPoint(centerA, 180, 600);

    await addGeofence({
      identifier: 'A',
      latitude: centerA[0],
      longitude: centerA[1],
      radius: RADIUS,
      notifyOnEntry: true,
      notifyOnExit: true,
      notifyOnDwell: false,
      loiteringDelay: 0,
    });
    await addGeofence({
      identifier: 'B',
      latitude: centerB[0],
      longitude: centerB[1],
      radius: RADIUS,
      notifyOnEntry: true,
      notifyOnExit: true,
      notifyOnDwell: false,
      loiteringDelay: 0,
    });

    // Walk from 500m north of A, through A, through B, to 500m south of B
    const startPoint = destinationPoint(centerA, 0, 500);
    const endPoint = destinationPoint(centerB, 180, 500);
    const locations = walkPath(startPoint, endPoint, 80);
    const transitions = mockSimulator.feedLocations(locations);

    // Expect ENTER A, EXIT A, ENTER B, EXIT B
    expect(transitions).toHaveLength(4);
    expect(transitions[0]).toMatchObject({ identifier: 'A', action: 'ENTER' });
    expect(transitions[1]).toMatchObject({ identifier: 'A', action: 'EXIT' });
    expect(transitions[2]).toMatchObject({ identifier: 'B', action: 'ENTER' });
    expect(transitions[3]).toMatchObject({ identifier: 'B', action: 'EXIT' });
  });

  it('suppresses ENTER when notifyOnEntry is false', async () => {
    await addGeofence({
      identifier: 'park',
      latitude: CENTER[0],
      longitude: CENTER[1],
      radius: RADIUS,
      notifyOnEntry: false,
      notifyOnExit: true,
      notifyOnDwell: false,
      loiteringDelay: 0,
    });

    const north = destinationPoint(CENTER, 0, 500);
    const south = destinationPoint(CENTER, 180, 500);
    const locations = walkPath(north, south, 40);
    const transitions = mockSimulator.feedLocations(locations);

    expect(transitions).toHaveLength(1);
    expect(transitions[0]!.action).toBe('EXIT');
  });

  it('suppresses EXIT when notifyOnExit is false', async () => {
    await addGeofence({
      identifier: 'park',
      latitude: CENTER[0],
      longitude: CENTER[1],
      radius: RADIUS,
      notifyOnEntry: true,
      notifyOnExit: false,
      notifyOnDwell: false,
      loiteringDelay: 0,
    });

    const north = destinationPoint(CENTER, 0, 500);
    const south = destinationPoint(CENTER, 180, 500);
    const locations = walkPath(north, south, 40);
    const transitions = mockSimulator.feedLocations(locations);

    expect(transitions).toHaveLength(1);
    expect(transitions[0]!.action).toBe('ENTER');
  });

  it('fires ENTER when first location is already inside the geofence', async () => {
    await addGeofence({
      identifier: 'park',
      latitude: CENTER[0],
      longitude: CENTER[1],
      radius: RADIUS,
      notifyOnEntry: true,
      notifyOnExit: true,
      notifyOnDwell: false,
      loiteringDelay: 0,
    });

    // Start at the center itself
    const locations = locationSequence([CENTER]);
    const transitions = mockSimulator.feedLocations(locations);

    expect(transitions).toHaveLength(1);
    expect(transitions[0]).toMatchObject({
      identifier: 'park',
      action: 'ENTER',
    });
  });

  it('does not fire EXIT after geofence is removed mid-sequence', async () => {
    await addGeofence({
      identifier: 'park',
      latitude: CENTER[0],
      longitude: CENTER[1],
      radius: RADIUS,
      notifyOnEntry: true,
      notifyOnExit: true,
      notifyOnDwell: false,
      loiteringDelay: 0,
    });

    // First, feed a location inside
    const insideLoc = locationSequence([CENTER]);
    const t1 = mockSimulator.feedLocations(insideLoc);
    expect(t1).toHaveLength(1);
    expect(t1[0]!.action).toBe('ENTER');

    // Remove the geofence
    await removeGeofence('park');

    // Feed a location outside — should not fire EXIT
    const outside = destinationPoint(CENTER, 0, 500);
    const outsideLoc = locationSequence([outside]);
    const t2 = mockSimulator.feedLocations(outsideLoc);
    expect(t2).toHaveLength(0);
  });

  it('emits geofence events through the public onGeofence API', async () => {
    const events: GeofenceEvent[] = [];
    onGeofence((event) => events.push(event));

    await addGeofence({
      identifier: 'park',
      latitude: CENTER[0],
      longitude: CENTER[1],
      radius: RADIUS,
      notifyOnEntry: true,
      notifyOnExit: true,
      notifyOnDwell: false,
      loiteringDelay: 0,
    });

    const north = destinationPoint(CENTER, 0, 500);
    const south = destinationPoint(CENTER, 180, 500);
    const locations = walkPath(north, south, 40);
    mockSimulator.feedLocations(locations);

    expect(events).toHaveLength(2);
    expect(events[0]!.action).toBe('ENTER');
    expect(events[0]!.identifier).toBe('park');
    expect(events[1]!.action).toBe('EXIT');
    expect(events[1]!.location).toBeDefined();
  });

  it('stores locations and supports getLocations/getCount/destroyLocations', async () => {
    const locs = locationSequence([
      [40.0, -74.0],
      [40.1, -74.1],
      [40.2, -74.2],
    ]);
    mockSimulator.feedLocations(locs);

    const stored = await getLocations();
    expect(stored).toHaveLength(3);
    expect(stored[0]!.latitude).toBe(40.0);

    const count = await getCount();
    expect(count).toBe(3);

    const destroyed = await destroyLocations();
    expect(destroyed).toBe(true);

    const afterCount = await getCount();
    expect(afterCount).toBe(0);

    const afterLocs = await getLocations();
    expect(afterLocs).toHaveLength(0);
  });
});
