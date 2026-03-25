import { useRef, useCallback } from 'react';
import { View, StyleSheet } from 'react-native';
import type { NativeStackScreenProps } from '@react-navigation/native-stack';
import TrackingMap, { type TrackingMapRef } from '../components/TrackingMap';
import BottomToolbar from '../components/BottomToolbar';
import FABMenu from '../components/FABMenu';
import { useGeomony } from '../App';

type RootStackParamList = {
  Home: undefined;
  Settings: undefined;
  AddGeofence: { latitude: number; longitude: number };
};

type Props = NativeStackScreenProps<RootStackParamList, 'Home'>;

export default function HomeScreen({ navigation }: Props) {
  const mapRef = useRef<TrackingMapRef>(null);
  const {
    tracking,
    locations,
    distance,
    stationaryRadius,
    geofences,
    geofenceEvents,
    startTracking,
    stopTracking,
    clearLocations,
    resetDistance,
  } = useGeomony();

  const handleToggleTracking = useCallback(() => {
    if (tracking) {
      stopTracking();
    } else {
      startTracking();
    }
  }, [tracking, startTracking, stopTracking]);

  const handleMapLongPress = useCallback(
    (latitude: number, longitude: number) => {
      navigation.navigate('AddGeofence', { latitude, longitude });
    },
    [navigation]
  );

  const handleRecenter = useCallback(() => {
    const latest = locations[0];
    if (latest) {
      mapRef.current?.centerOn(latest.latitude, latest.longitude);
    }
  }, [locations]);

  return (
    <View style={styles.container}>
      <TrackingMap
        ref={mapRef}
        locations={locations}
        stationaryRadius={stationaryRadius}
        geofences={geofences}
        geofenceEvents={geofenceEvents}
        onLongPress={handleMapLongPress}
      />
      <FABMenu
        navigation={navigation}
        onDestroyLocations={clearLocations}
        onResetDistance={resetDistance}
      />
      <BottomToolbar
        tracking={tracking}
        locationCount={locations.length}
        distance={distance}
        onToggleTracking={handleToggleTracking}
        onRecenter={handleRecenter}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
});
