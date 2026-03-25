import type { SimulatedLocation } from './types';
import { getBearing } from './geo';

const DEG_TO_RAD = Math.PI / 180;
const RAD_TO_DEG = 180 / Math.PI;
const EARTH_RADIUS = 6371000;

/**
 * Convert bare coordinate pairs into a timestamped location sequence.
 */
export function locationSequence(
  points: [number, number][],
  opts?: { intervalMs?: number; startTime?: Date; accuracy?: number }
): SimulatedLocation[] {
  const intervalMs = opts?.intervalMs ?? 1000;
  const startTime = opts?.startTime ?? new Date('2026-01-01T00:00:00Z');
  const accuracy = opts?.accuracy ?? 5.0;

  return points.map(([latitude, longitude], i) => ({
    latitude,
    longitude,
    timestamp: new Date(startTime.getTime() + i * intervalMs).toISOString(),
    accuracy,
    speed: 0,
    heading: -1,
    altitude: 0,
  }));
}

/**
 * Generate evenly spaced points along a great-circle path from start to end.
 */
export function walkPath(
  start: [number, number],
  end: [number, number],
  steps: number,
  opts?: { intervalMs?: number; startTime?: Date }
): SimulatedLocation[] {
  const intervalMs = opts?.intervalMs ?? 1000;
  const startTime = opts?.startTime ?? new Date('2026-01-01T00:00:00Z');
  const bearing = getBearing(start[0], start[1], end[0], end[1]);

  const points: SimulatedLocation[] = [];
  for (let i = 0; i <= steps; i++) {
    const fraction = i / steps;
    const lat = start[0] + fraction * (end[0] - start[0]);
    const lon = start[1] + fraction * (end[1] - start[1]);
    points.push({
      latitude: lat,
      longitude: lon,
      timestamp: new Date(startTime.getTime() + i * intervalMs).toISOString(),
      accuracy: 5.0,
      speed: i > 0 ? 1.5 : 0,
      heading: bearing,
      altitude: 0,
    });
  }
  return points;
}

/**
 * Generate repeated locations at the same point (for dwell testing).
 */
export function stayAt(
  point: [number, number],
  durationMs: number,
  intervalMs: number,
  opts?: { startTime?: Date }
): SimulatedLocation[] {
  const startTime = opts?.startTime ?? new Date('2026-01-01T00:00:00Z');
  const count = Math.floor(durationMs / intervalMs) + 1;

  return Array.from({ length: count }, (_, i) => ({
    latitude: point[0],
    longitude: point[1],
    timestamp: new Date(startTime.getTime() + i * intervalMs).toISOString(),
    accuracy: 5.0,
    speed: 0,
    heading: -1,
    altitude: 0,
  }));
}

/**
 * Compute a destination point given a start, bearing (degrees), and distance (meters).
 */
export function destinationPoint(
  start: [number, number],
  bearingDeg: number,
  distanceMeters: number
): [number, number] {
  const lat1 = start[0] * DEG_TO_RAD;
  const lon1 = start[1] * DEG_TO_RAD;
  const brng = bearingDeg * DEG_TO_RAD;
  const d = distanceMeters / EARTH_RADIUS;

  const lat2 = Math.asin(
    Math.sin(lat1) * Math.cos(d) + Math.cos(lat1) * Math.sin(d) * Math.cos(brng)
  );
  const lon2 =
    lon1 +
    Math.atan2(
      Math.sin(brng) * Math.sin(d) * Math.cos(lat1),
      Math.cos(d) - Math.sin(lat1) * Math.sin(lat2)
    );

  return [lat2 * RAD_TO_DEG, lon2 * RAD_TO_DEG];
}
