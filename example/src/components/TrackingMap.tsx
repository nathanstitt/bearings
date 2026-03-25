import {
  useState,
  useRef,
  useEffect,
  useCallback,
  forwardRef,
  useImperativeHandle,
} from 'react';
import { View, TouchableOpacity, StyleSheet } from 'react-native';
import MapView, {
  Marker,
  Polyline,
  Circle,
  type LongPressEvent,
} from 'react-native-maps';
import { Ionicons } from '@expo/vector-icons';
import type { Location, Geofence, GeofenceEvent } from 'geomony';
import { COLORS, GEOFENCE_COLORS } from '../lib/config';

export interface TrackingMapRef {
  centerOn: (latitude: number, longitude: number) => void;
}

interface Props {
  locations: Location[];
  stationaryRadius: number;
  geofences?: Geofence[];
  geofenceEvents?: GeofenceEvent[];
  onLongPress?: (latitude: number, longitude: number) => void;
}

const ARROW_ANCHOR = { x: 0.5, y: 0.5 };

const TrackingMap = forwardRef<TrackingMapRef, Props>(
  (
    {
      locations,
      stationaryRadius,
      geofences = [],
      geofenceEvents = [],
      onLongPress,
    },
    ref
  ) => {
    const mapRef = useRef<MapView>(null);
    const [following, setFollowing] = useState(true);
    const [showRecenter, setShowRecenter] = useState(false);
    const userPanned = useRef(false);
    const hasInitialLocation = useRef(false);

    const latestLocation = locations[0];

    useImperativeHandle(ref, () => ({
      centerOn: (latitude: number, longitude: number) => {
        mapRef.current?.animateToRegion(
          {
            latitude,
            longitude,
            latitudeDelta: 0.005,
            longitudeDelta: 0.005,
          },
          500
        );
        setFollowing(true);
        setShowRecenter(false);
        userPanned.current = false;
      },
    }));

    useEffect(() => {
      if (latestLocation && mapRef.current && following) {
        const isFirst = !hasInitialLocation.current;
        hasInitialLocation.current = true;
        mapRef.current.animateToRegion(
          {
            latitude: latestLocation.latitude,
            longitude: latestLocation.longitude,
            latitudeDelta: 0.005,
            longitudeDelta: 0.005,
          },
          isFirst ? 500 : 300
        );
      }
    }, [following, latestLocation]);

    const handlePanDrag = useCallback(() => {
      if (!userPanned.current) {
        userPanned.current = true;
        setFollowing(false);
        setShowRecenter(true);
      }
    }, []);

    const handleRecenter = useCallback(() => {
      if (latestLocation) {
        mapRef.current?.animateToRegion(
          {
            latitude: latestLocation.latitude,
            longitude: latestLocation.longitude,
            latitudeDelta: 0.005,
            longitudeDelta: 0.005,
          },
          500
        );
      }
      setFollowing(true);
      setShowRecenter(false);
      userPanned.current = false;
    }, [latestLocation]);

    // Build polyline coordinates (oldest→newest for drawing order)
    const polylineCoords = [];
    for (let i = locations.length - 1; i >= 0; i--) {
      const loc = locations[i]!;
      polylineCoords.push({ latitude: loc.latitude, longitude: loc.longitude });
    }

    // Stationary locations
    const stationaryLocations = locations.filter((l) => !l.is_moving);

    const handleLongPress = useCallback(
      (e: LongPressEvent) => {
        const { latitude, longitude } = e.nativeEvent.coordinate;
        onLongPress?.(latitude, longitude);
      },
      [onLongPress]
    );

    // Determine geofence color based on latest event
    const getGeofenceColors = (identifier: string) => {
      const latestEvent = geofenceEvents.find(
        (e) => e.identifier === identifier
      );
      if (!latestEvent) {
        return {
          fill: GEOFENCE_COLORS.default,
          stroke: GEOFENCE_COLORS.defaultStroke,
        };
      }
      switch (latestEvent.action) {
        case 'ENTER':
          return {
            fill: GEOFENCE_COLORS.enter,
            stroke: GEOFENCE_COLORS.enterStroke,
          };
        case 'EXIT':
          return {
            fill: GEOFENCE_COLORS.exit,
            stroke: GEOFENCE_COLORS.exitStroke,
          };
        case 'DWELL':
          return {
            fill: GEOFENCE_COLORS.dwell,
            stroke: GEOFENCE_COLORS.dwellStroke,
          };
        default:
          return {
            fill: GEOFENCE_COLORS.default,
            stroke: GEOFENCE_COLORS.defaultStroke,
          };
      }
    };

    return (
      <View style={styles.container}>
        <MapView
          ref={mapRef}
          style={StyleSheet.absoluteFill}
          showsUserLocation={true}
          showsMyLocationButton={false}
          onPanDrag={handlePanDrag}
          onLongPress={handleLongPress}
        >
          {/* Polyline */}
          {polylineCoords.length > 1 && (
            <Polyline
              coordinates={polylineCoords}
              strokeColor={COLORS.polyline}
              strokeWidth={6}
            />
          )}

          {/* Stationary zones */}
          {stationaryLocations.map((loc) => (
            <Circle
              key={`stationary-${loc.uuid}`}
              center={{ latitude: loc.latitude, longitude: loc.longitude }}
              radius={stationaryRadius}
              fillColor={COLORS.stationary}
              strokeColor="rgba(200,0,0,0.4)"
              strokeWidth={1}
            />
          ))}

          {/* Current position accuracy circle */}
          {latestLocation && (
            <Circle
              center={{
                latitude: latestLocation.latitude,
                longitude: latestLocation.longitude,
              }}
              radius={latestLocation.accuracy}
              fillColor={COLORS.accuracy}
              strokeColor="rgba(33,150,243,0.3)"
              strokeWidth={1}
            />
          )}

          {/* Location markers with heading arrows */}
          {locations.map((loc) => (
            <Marker
              key={loc.uuid}
              coordinate={{
                latitude: loc.latitude,
                longitude: loc.longitude,
              }}
              anchor={ARROW_ANCHOR}
              flat
              rotation={loc.heading}
              tracksViewChanges={false}
            >
              <View
                style={[
                  styles.markerDot,
                  loc.is_moving ? styles.movingDot : styles.stationaryDot,
                ]}
              >
                {loc.is_moving && loc.heading > 0 && (
                  <Ionicons name="navigate" size={10} color={COLORS.white} />
                )}
              </View>
            </Marker>
          ))}

          {/* Geofence circles */}
          {geofences.map((g) => {
            const colors = getGeofenceColors(g.identifier);
            return (
              <Circle
                key={`geofence-${g.identifier}`}
                center={{ latitude: g.latitude, longitude: g.longitude }}
                radius={g.radius}
                fillColor={colors.fill}
                strokeColor={colors.stroke}
                strokeWidth={2}
              />
            );
          })}

          {/* Geofence center markers */}
          {geofences.map((g) => (
            <Marker
              key={`geofence-marker-${g.identifier}`}
              coordinate={{ latitude: g.latitude, longitude: g.longitude }}
              tracksViewChanges={false}
            >
              <View style={styles.geofenceMarker}>
                <Ionicons name="location" size={14} color={COLORS.white} />
              </View>
            </Marker>
          ))}
        </MapView>

        {showRecenter && (
          <TouchableOpacity
            style={styles.recenterButton}
            onPress={handleRecenter}
          >
            <Ionicons name="navigate-circle" size={32} color={COLORS.primary} />
          </TouchableOpacity>
        )}
      </View>
    );
  }
);

export default TrackingMap;

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  markerDot: {
    width: 16,
    height: 16,
    borderRadius: 8,
    alignItems: 'center',
    justifyContent: 'center',
    borderWidth: 2,
    borderColor: COLORS.white,
  },
  movingDot: {
    backgroundColor: COLORS.primary,
  },
  stationaryDot: {
    backgroundColor: COLORS.red,
  },
  geofenceMarker: {
    width: 24,
    height: 24,
    borderRadius: 12,
    backgroundColor: COLORS.green,
    alignItems: 'center',
    justifyContent: 'center',
    borderWidth: 2,
    borderColor: COLORS.white,
  },
  recenterButton: {
    position: 'absolute',
    top: 16,
    right: 16,
    width: 44,
    height: 44,
    borderRadius: 22,
    backgroundColor: COLORS.white,
    alignItems: 'center',
    justifyContent: 'center',
    elevation: 4,
    shadowColor: COLORS.black,
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.25,
    shadowRadius: 3,
  },
});
