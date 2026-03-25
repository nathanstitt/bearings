export interface SimulatedLocation {
  latitude: number;
  longitude: number;
  timestamp?: string;
  accuracy?: number;
  speed?: number;
  heading?: number;
  altitude?: number;
}

export interface GeofenceTransition {
  identifier: string;
  action: 'ENTER' | 'EXIT';
  locationIndex: number;
}
