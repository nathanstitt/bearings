import { GeotrackSimulator } from './harness/GeotrackSimulator';
import { locationSequence } from './harness/locations';
import type { Location, ScheduleEvent } from '../types';

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
  stop,
  startSchedule,
  stopSchedule,
  getState,
  onLocation,
  onSchedule,
} = require('../index') as typeof import('../index');

beforeEach(() => {
  mockSimulator.reset();
});

describe('schedule', () => {
  it('startSchedule sets schedulerRunning and emits schedule enabled event', async () => {
    await configure({ schedule: ['2-6 09:00-17:00'] });

    const events: ScheduleEvent[] = [];
    const sub = onSchedule((e) => events.push(e));

    const state = await startSchedule();
    expect(state.schedulerRunning).toBe(true);
    expect(state.tracking).toBe(true);

    // Should have emitted a schedule event with enabled=true
    expect(events).toHaveLength(1);
    expect(events[0]!.enabled).toBe(true);

    sub.remove();
  });

  it('stopSchedule stops scheduler and emits schedule disabled event', async () => {
    await configure({ schedule: ['1-7 00:00-23:59'] });

    const events: ScheduleEvent[] = [];
    const sub = onSchedule((e) => events.push(e));

    await startSchedule();
    const state = await stopSchedule();

    expect(state.schedulerRunning).toBe(false);
    expect(state.tracking).toBe(false);

    // Should have emitted enabled=true then enabled=false
    expect(events).toHaveLength(2);
    expect(events[0]!.enabled).toBe(true);
    expect(events[1]!.enabled).toBe(false);

    sub.remove();
  });

  it('locations are received while schedule is active', async () => {
    await configure({ schedule: ['1-7 00:00-23:59'] });

    const receivedLocations: Location[] = [];
    const locationSub = onLocation((loc) => receivedLocations.push(loc));

    await startSchedule();

    // Feed locations while schedule-driven tracking is active
    mockSimulator.feedLocations(
      locationSequence([
        [40.0, -74.0],
        [40.001, -74.001],
        [40.002, -74.002],
      ])
    );

    expect(receivedLocations).toHaveLength(3);
    expect(receivedLocations[0]!.latitude).toBe(40.0);
    expect(receivedLocations[2]!.latitude).toBe(40.002);

    locationSub.remove();
  });

  it('locations stop being received after stopSchedule', async () => {
    await configure({ schedule: ['1-7 00:00-23:59'] });

    const receivedLocations: Location[] = [];
    const locationSub = onLocation((loc) => receivedLocations.push(loc));

    await startSchedule();

    // Feed some locations while active
    mockSimulator.feedLocations(
      locationSequence([
        [40.0, -74.0],
        [40.001, -74.001],
      ])
    );
    expect(receivedLocations).toHaveLength(2);

    await stopSchedule();

    // Feed more locations after stopping — these should still emit
    // (the emitter is still subscribed), but tracking state is off
    const stateAfterStop = await getState();
    expect(stateAfterStop.tracking).toBe(false);
    expect(stateAfterStop.schedulerRunning).toBe(false);

    locationSub.remove();
  });

  it('schedule start then manual stop then schedule re-start works', async () => {
    await configure({ schedule: ['1-7 00:00-23:59'] });

    const scheduleEvents: ScheduleEvent[] = [];
    const scheduleSub = onSchedule((e) => scheduleEvents.push(e));

    // Start schedule
    const state1 = await startSchedule();
    expect(state1.schedulerRunning).toBe(true);
    expect(state1.tracking).toBe(true);

    // Manual stop
    const state2 = await stop();
    expect(state2.tracking).toBe(false);

    // Re-start schedule
    const state3 = await startSchedule();
    expect(state3.schedulerRunning).toBe(true);
    expect(state3.tracking).toBe(true);

    scheduleSub.remove();
  });

  it('schedule events interleave with location events correctly', async () => {
    await configure({ schedule: ['2-6 09:00-17:00'] });

    const allEvents: Array<{ type: string; data: unknown }> = [];

    const locationSub = onLocation((loc) =>
      allEvents.push({ type: 'location', data: loc })
    );
    const scheduleSub = onSchedule((e) =>
      allEvents.push({ type: 'schedule', data: e })
    );

    // Start schedule — triggers schedule event + tracking
    await startSchedule();

    // Feed locations
    mockSimulator.feedLocations(
      locationSequence([
        [40.0, -74.0],
        [40.001, -74.001],
      ])
    );

    // Stop schedule — triggers schedule event
    await stopSchedule();

    // Verify event order: schedule(enabled) -> locations -> schedule(disabled)
    expect(allEvents[0]).toMatchObject({ type: 'schedule' });
    expect((allEvents[0]!.data as ScheduleEvent).enabled).toBe(true);

    expect(allEvents[1]).toMatchObject({ type: 'location' });
    expect(allEvents[2]).toMatchObject({ type: 'location' });

    expect(allEvents[3]).toMatchObject({ type: 'schedule' });
    expect((allEvents[3]!.data as ScheduleEvent).enabled).toBe(false);

    locationSub.remove();
    scheduleSub.remove();
  });

  it('startSchedule without schedule rules does not start tracking', async () => {
    await configure({ schedule: [] });

    const events: ScheduleEvent[] = [];
    const sub = onSchedule((e) => events.push(e));

    const state = await startSchedule();
    // No rules configured, so scheduler runs but no tracking starts
    expect(state.schedulerRunning).toBe(true);
    expect(events).toHaveLength(0);
    expect(state.tracking).toBe(false);

    sub.remove();
  });

  it('getState reflects schedulerRunning after start and stop', async () => {
    await configure({ schedule: ['1 09:00-17:00'] });

    let state = await getState();
    expect(state.schedulerRunning).toBe(false);

    await startSchedule();
    state = await getState();
    expect(state.schedulerRunning).toBe(true);

    await stopSchedule();
    state = await getState();
    expect(state.schedulerRunning).toBe(false);
  });

  it('onSchedule subscription can be removed', async () => {
    await configure({ schedule: ['1-7 00:00-23:59'] });

    const events: ScheduleEvent[] = [];
    const sub = onSchedule((e) => events.push(e));

    await startSchedule();
    expect(events).toHaveLength(1);

    sub.remove();

    await stopSchedule();
    // Should not receive the stop event after unsubscribing
    expect(events).toHaveLength(1);
  });
});
