import { NativeEventEmitter } from 'react-native';
import NativeGeomony from './NativeGeomony';
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

const emitter = new NativeEventEmitter(NativeGeomony);

export async function configure(config: Partial<Config>): Promise<State> {
  const json = JSON.stringify(config);
  const result = await NativeGeomony.configure(json);
  return JSON.parse(result) as State;
}

export async function start(): Promise<State> {
  const result = await NativeGeomony.start();
  return JSON.parse(result) as State;
}

export async function stop(): Promise<State> {
  const result = await NativeGeomony.stop();
  return JSON.parse(result) as State;
}

export async function getState(): Promise<State> {
  const result = await NativeGeomony.getState();
  return JSON.parse(result) as State;
}

export async function getLocations(): Promise<Location[]> {
  const result = await NativeGeomony.getLocations();
  return JSON.parse(result) as Location[];
}

export async function getCount(): Promise<number> {
  return NativeGeomony.getCount();
}

export async function destroyLocations(): Promise<boolean> {
  return NativeGeomony.destroyLocations();
}

export async function addGeofence(geofence: Geofence): Promise<boolean> {
  const json = JSON.stringify(geofence);
  return NativeGeomony.addGeofence(json);
}

export async function removeGeofence(identifier: string): Promise<boolean> {
  return NativeGeomony.removeGeofence(identifier);
}

export async function removeGeofences(): Promise<boolean> {
  return NativeGeomony.removeGeofences();
}

export async function getGeofences(): Promise<Geofence[]> {
  const result = await NativeGeomony.getGeofences();
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
  const result = await NativeGeomony.startSchedule();
  return JSON.parse(result) as State;
}

export async function stopSchedule(): Promise<State> {
  const result = await NativeGeomony.stopSchedule();
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
