import { NativeEventEmitter } from 'react-native';
import NativeBearings from './NativeBearings';
import type {
  ActivityChangeEvent,
  Config,
  Geofence,
  GeofenceEvent,
  Location,
  MotionChangeEvent,
  ScheduleEvent,
  State,
  Subscription,
} from './types';

export type {
  Activity,
  ActivityChangeEvent,
  ActivityType,
  Config,
  Geofence,
  GeofenceEvent,
  Location,
  MotionChangeEvent,
  ScheduleEvent,
  State,
  Subscription,
} from './types';

const emitter = new NativeEventEmitter(NativeBearings);

export async function configure(config: Partial<Config>): Promise<State> {
  const json = JSON.stringify(config);
  const result = await NativeBearings.configure(json);
  return JSON.parse(result) as State;
}

export async function start(): Promise<State> {
  const result = await NativeBearings.start();
  return JSON.parse(result) as State;
}

export async function stop(): Promise<State> {
  const result = await NativeBearings.stop();
  return JSON.parse(result) as State;
}

export async function getState(): Promise<State> {
  const result = await NativeBearings.getState();
  return JSON.parse(result) as State;
}

export async function getLocations(): Promise<Location[]> {
  const result = await NativeBearings.getLocations();
  return JSON.parse(result) as Location[];
}

export async function getCount(): Promise<number> {
  return NativeBearings.getCount();
}

export async function destroyLocations(): Promise<boolean> {
  return NativeBearings.destroyLocations();
}

export async function addGeofence(geofence: Geofence): Promise<boolean> {
  const json = JSON.stringify(geofence);
  return NativeBearings.addGeofence(json);
}

export async function removeGeofence(identifier: string): Promise<boolean> {
  return NativeBearings.removeGeofence(identifier);
}

export async function removeGeofences(): Promise<boolean> {
  return NativeBearings.removeGeofences();
}

export async function getGeofences(): Promise<Geofence[]> {
  const result = await NativeBearings.getGeofences();
  return JSON.parse(result) as Geofence[];
}

export function onGeofence(
  callback: (event: GeofenceEvent) => void
): Subscription {
  const subscription = emitter.addListener('geofence', (data) => {
    const json = data as string;
    const event = JSON.parse(json) as GeofenceEvent;
    callback(event);
  });
  return {
    remove: () => subscription.remove(),
  };
}

export function onLocation(
  callback: (location: Location) => void
): Subscription {
  const subscription = emitter.addListener('location', (data) => {
    const json = data as string;
    const location = JSON.parse(json) as Location;
    callback(location);
  });
  return {
    remove: () => subscription.remove(),
  };
}

export async function startSchedule(): Promise<State> {
  const result = await NativeBearings.startSchedule();
  return JSON.parse(result) as State;
}

export async function stopSchedule(): Promise<State> {
  const result = await NativeBearings.stopSchedule();
  return JSON.parse(result) as State;
}

export function onSchedule(
  callback: (event: ScheduleEvent) => void
): Subscription {
  const subscription = emitter.addListener('schedule', (data) => {
    const json = data as string;
    const event = JSON.parse(json) as ScheduleEvent;
    callback(event);
  });
  return {
    remove: () => subscription.remove(),
  };
}

export function onActivityChange(
  callback: (event: ActivityChangeEvent) => void
): Subscription {
  const subscription = emitter.addListener('activitychange', (data) => {
    const json = data as string;
    const event = JSON.parse(json) as ActivityChangeEvent;
    callback(event);
  });
  return {
    remove: () => subscription.remove(),
  };
}

export function onMotionChange(
  callback: (event: MotionChangeEvent) => void
): Subscription {
  const subscription = emitter.addListener('motionchange', (data) => {
    const json = data as string;
    const location = JSON.parse(json) as Location;
    callback({
      location,
      isMoving: location.is_moving,
    });
  });
  return {
    remove: () => subscription.remove(),
  };
}
