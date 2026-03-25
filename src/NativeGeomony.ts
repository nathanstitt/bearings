import type { TurboModule } from 'react-native';
import { TurboModuleRegistry } from 'react-native';

export interface Spec extends TurboModule {
  configure(config: string): Promise<string>;
  start(): Promise<string>;
  stop(): Promise<string>;
  getState(): Promise<string>;
  getLocations(): Promise<string>;
  getCount(): Promise<number>;
  destroyLocations(): Promise<boolean>;
  addGeofence(geofenceJson: string): Promise<boolean>;
  removeGeofence(identifier: string): Promise<boolean>;
  removeGeofences(): Promise<boolean>;
  getGeofences(): Promise<string>;
  startSchedule(): Promise<string>;
  stopSchedule(): Promise<string>;
  addListener(eventName: string): void;
  removeListeners(count: number): void;
}

export default TurboModuleRegistry.getEnforcing<Spec>('Geomony');
