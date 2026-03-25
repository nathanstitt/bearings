import { useState, useEffect, useRef, useCallback } from 'react';
import {
  ScrollView,
  TouchableOpacity,
  Text,
  View,
  TextInput,
  Platform,
  StyleSheet,
  Alert,
} from 'react-native';
import type { Config } from 'bearings';
import { configure } from 'bearings';
import type { NativeStackScreenProps } from '@react-navigation/native-stack';
import SettingsSection from '../components/SettingsSection';
import SettingsRow from '../components/SettingsRow';
import { COLORS, ACCURACY_OPTIONS, DEFAULT_CONFIG } from '../lib/config';
import { loadSettings, saveSettings } from '../lib/settingsStorage';
import { useBearings } from '../App';

type RootStackParamList = {
  Home: undefined;
  Settings: undefined;
  AddGeofence: { latitude: number; longitude: number };
};

type Props = NativeStackScreenProps<RootStackParamList, 'Settings'>;

export default function SettingsScreen({ navigation }: Props) {
  const { schedulerRunning, startSchedule, stopSchedule } = useBearings();
  const [settings, setSettings] = useState<Partial<Config>>({
    ...DEFAULT_CONFIG,
  });
  const [loaded, setLoaded] = useState(false);
  const debounceTimer = useRef<ReturnType<typeof setTimeout> | null>(null);

  useEffect(() => {
    loadSettings().then((s) => {
      setSettings(s);
      setLoaded(true);
    });
  }, []);

  const update = useCallback(
    (key: keyof Config, value: Config[keyof Config]) => {
      setSettings((prev) => {
        const next = { ...prev, [key]: value };
        // Debounce text field saves
        if (debounceTimer.current) {
          clearTimeout(debounceTimer.current);
        }
        debounceTimer.current = setTimeout(() => {
          saveSettings(next);
        }, 500);
        return next;
      });
    },
    []
  );

  const handleApply = async () => {
    try {
      await saveSettings(settings);
      await configure(settings);
      navigation.goBack();
    } catch (err) {
      Alert.alert('Error', String(err));
    }
  };

  if (!loaded) {
    return null;
  }

  return (
    <ScrollView style={styles.container}>
      <SettingsSection title="Geolocation">
        <SettingsRow
          type="picker"
          label="Desired Accuracy"
          value={settings.desiredAccuracy ?? DEFAULT_CONFIG.desiredAccuracy}
          options={ACCURACY_OPTIONS}
          onValueChange={(v) => update('desiredAccuracy', v)}
        />
        <SettingsRow
          type="text"
          label="Distance Filter (m)"
          value={String(
            settings.distanceFilter ?? DEFAULT_CONFIG.distanceFilter
          )}
          onValueChange={(v) => {
            const n = parseInt(v, 10);
            if (!isNaN(n)) {
              update('distanceFilter', n);
            }
          }}
        />
        <SettingsRow
          type="text"
          label="Stationary Radius (m)"
          value={String(
            settings.stationaryRadius ?? DEFAULT_CONFIG.stationaryRadius
          )}
          onValueChange={(v) => {
            const n = parseInt(v, 10);
            if (!isNaN(n)) {
              update('stationaryRadius', n);
            }
          }}
        />
        <SettingsRow
          type="text"
          label="Stop Timeout (min)"
          value={String(settings.stopTimeout ?? DEFAULT_CONFIG.stopTimeout)}
          onValueChange={(v) => {
            const n = parseInt(v, 10);
            if (!isNaN(n)) {
              update('stopTimeout', n);
            }
          }}
        />
      </SettingsSection>

      <SettingsSection title="Application">
        <SettingsRow
          type="switch"
          label="Stop on Terminate"
          value={settings.stopOnTerminate ?? DEFAULT_CONFIG.stopOnTerminate}
          onValueChange={(v) => update('stopOnTerminate', v)}
        />
        <SettingsRow
          type="switch"
          label="Start on Boot"
          value={settings.startOnBoot ?? DEFAULT_CONFIG.startOnBoot}
          onValueChange={(v) => update('startOnBoot', v)}
        />
      </SettingsSection>

      <SettingsSection title="HTTP">
        <SettingsRow
          type="text"
          label="URL"
          value={settings.url ?? DEFAULT_CONFIG.url}
          placeholder="https://example.com/locations"
          onValueChange={(v) => update('url', v)}
        />
      </SettingsSection>

      <SettingsSection title="Debug">
        <SettingsRow
          type="switch"
          label="Debug Mode"
          value={settings.debug ?? DEFAULT_CONFIG.debug}
          onValueChange={(v) => update('debug', v)}
        />
      </SettingsSection>

      <SettingsSection title="Schedule">
        {(settings.schedule ?? []).map((rule, index) => (
          <View key={index} style={styles.scheduleRow}>
            <TextInput
              style={styles.scheduleInput}
              value={rule}
              placeholder='e.g. "2-6 09:00-17:00"'
              onChangeText={(text) => {
                const rules = [...(settings.schedule ?? [])];
                rules[index] = text;
                update('schedule', rules);
              }}
            />
            <TouchableOpacity
              style={styles.scheduleRemove}
              onPress={() => {
                const rules = [...(settings.schedule ?? [])];
                rules.splice(index, 1);
                update('schedule', rules);
              }}
            >
              <Text style={styles.scheduleRemoveText}>X</Text>
            </TouchableOpacity>
          </View>
        ))}
        <TouchableOpacity
          style={styles.addRuleButton}
          onPress={() => {
            const rules = [...(settings.schedule ?? []), ''];
            update('schedule', rules);
          }}
        >
          <Text style={styles.addRuleText}>+ Add Rule</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[
            styles.scheduleButton,
            schedulerRunning && styles.scheduleButtonActive,
          ]}
          onPress={() => {
            if (schedulerRunning) {
              stopSchedule();
            } else {
              startSchedule();
            }
          }}
        >
          <Text style={styles.scheduleButtonText}>
            {schedulerRunning ? 'Stop Schedule' : 'Start Schedule'}
          </Text>
        </TouchableOpacity>
        {Platform.OS === 'android' && (
          <SettingsRow
            type="switch"
            label="Use Alarm Manager"
            value={
              settings.scheduleUseAlarmManager ??
              DEFAULT_CONFIG.scheduleUseAlarmManager
            }
            onValueChange={(v) => update('scheduleUseAlarmManager', v)}
          />
        )}
      </SettingsSection>

      <TouchableOpacity style={styles.applyButton} onPress={handleApply}>
        <Text style={styles.applyButtonText}>Apply &amp; Restart</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f2f2f7',
    paddingTop: 16,
  },
  applyButton: {
    backgroundColor: COLORS.primary,
    marginHorizontal: 16,
    marginVertical: 24,
    paddingVertical: 14,
    borderRadius: 10,
    alignItems: 'center',
  },
  applyButtonText: {
    color: COLORS.white,
    fontSize: 17,
    fontWeight: '600',
  },
  scheduleRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 16,
    paddingVertical: 8,
    backgroundColor: COLORS.white,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: '#ccc',
  },
  scheduleInput: {
    flex: 1,
    fontSize: 15,
    paddingVertical: 6,
    paddingHorizontal: 8,
    backgroundColor: '#f0f0f0',
    borderRadius: 6,
  },
  scheduleRemove: {
    marginLeft: 8,
    width: 30,
    height: 30,
    borderRadius: 15,
    backgroundColor: COLORS.red,
    alignItems: 'center',
    justifyContent: 'center',
  },
  scheduleRemoveText: {
    color: COLORS.white,
    fontSize: 14,
    fontWeight: '700',
  },
  addRuleButton: {
    paddingHorizontal: 16,
    paddingVertical: 12,
    backgroundColor: COLORS.white,
  },
  addRuleText: {
    color: COLORS.primary,
    fontSize: 15,
    fontWeight: '600',
  },
  scheduleButton: {
    backgroundColor: COLORS.green,
    marginHorizontal: 16,
    marginTop: 8,
    marginBottom: 4,
    paddingVertical: 10,
    borderRadius: 8,
    alignItems: 'center',
  },
  scheduleButtonActive: {
    backgroundColor: COLORS.red,
  },
  scheduleButtonText: {
    color: COLORS.white,
    fontSize: 15,
    fontWeight: '600',
  },
});
