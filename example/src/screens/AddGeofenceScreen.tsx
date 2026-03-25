import { useState, useCallback } from 'react';
import {
  View,
  ScrollView,
  TouchableOpacity,
  Text,
  StyleSheet,
} from 'react-native';
import type { NativeStackScreenProps } from '@react-navigation/native-stack';
import SettingsSection from '../components/SettingsSection';
import SettingsRow from '../components/SettingsRow';
import { COLORS, RADIUS_OPTIONS } from '../lib/config';
import { useGeomony } from '../App';

type RootStackParamList = {
  Home: undefined;
  Settings: undefined;
  AddGeofence: { latitude: number; longitude: number };
};

type Props = NativeStackScreenProps<RootStackParamList, 'AddGeofence'>;

export default function AddGeofenceScreen({ route, navigation }: Props) {
  const { latitude, longitude } = route.params;
  const { addGeofence: ctxAddGeofence } = useGeomony();

  const [identifier, setIdentifier] = useState('');
  const [radius, setRadius] = useState(150);
  const [notifyOnEntry, setNotifyOnEntry] = useState(true);
  const [notifyOnExit, setNotifyOnExit] = useState(true);
  const [notifyOnDwell, setNotifyOnDwell] = useState(false);
  const [loiteringDelay, setLoiteringDelay] = useState('30000');

  const handleAdd = useCallback(async () => {
    if (!identifier.trim()) return;

    await ctxAddGeofence({
      identifier: identifier.trim(),
      latitude,
      longitude,
      radius,
      notifyOnEntry,
      notifyOnExit,
      notifyOnDwell,
      loiteringDelay: parseInt(loiteringDelay, 10) || 30000,
    });
    navigation.goBack();
  }, [
    identifier,
    latitude,
    longitude,
    radius,
    notifyOnEntry,
    notifyOnExit,
    notifyOnDwell,
    loiteringDelay,
    ctxAddGeofence,
    navigation,
  ]);

  return (
    <ScrollView style={styles.container}>
      <SettingsSection title="Location">
        <SettingsRow
          type="text"
          label="Latitude"
          value={latitude.toFixed(6)}
          onValueChange={() => {}}
        />
        <SettingsRow
          type="text"
          label="Longitude"
          value={longitude.toFixed(6)}
          onValueChange={() => {}}
        />
      </SettingsSection>

      <SettingsSection title="Geofence">
        <SettingsRow
          type="text"
          label="Identifier"
          value={identifier}
          placeholder="e.g. Home"
          onValueChange={setIdentifier}
        />
        <SettingsRow
          type="picker"
          label="Radius"
          value={radius}
          options={RADIUS_OPTIONS}
          onValueChange={setRadius}
        />
      </SettingsSection>

      <SettingsSection title="Notifications">
        <SettingsRow
          type="switch"
          label="Notify on Entry"
          value={notifyOnEntry}
          onValueChange={setNotifyOnEntry}
        />
        <SettingsRow
          type="switch"
          label="Notify on Exit"
          value={notifyOnExit}
          onValueChange={setNotifyOnExit}
        />
        <SettingsRow
          type="switch"
          label="Notify on Dwell"
          value={notifyOnDwell}
          onValueChange={setNotifyOnDwell}
        />
        {notifyOnDwell && (
          <SettingsRow
            type="text"
            label="Loitering Delay (ms)"
            value={loiteringDelay}
            placeholder="30000"
            onValueChange={setLoiteringDelay}
          />
        )}
      </SettingsSection>

      <View style={styles.buttonContainer}>
        <TouchableOpacity
          style={[styles.button, !identifier.trim() && styles.buttonDisabled]}
          onPress={handleAdd}
          disabled={!identifier.trim()}
        >
          <Text style={styles.buttonText}>Add Geofence</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
    paddingTop: 16,
  },
  buttonContainer: {
    paddingHorizontal: 16,
    paddingVertical: 24,
  },
  button: {
    backgroundColor: COLORS.primary,
    borderRadius: 8,
    paddingVertical: 14,
    alignItems: 'center',
  },
  buttonDisabled: {
    opacity: 0.5,
  },
  buttonText: {
    color: COLORS.white,
    fontSize: 17,
    fontWeight: '600',
  },
});
