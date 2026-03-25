import {
  createContext,
  useContext,
  useState,
  useEffect,
  useRef,
  useCallback,
  type ReactNode,
} from 'react';
import { Switch } from 'react-native';
import { NavigationContainer } from '@react-navigation/native';
import { createNativeStackNavigator } from '@react-navigation/native-stack';
import {
  configure,
  start,
  stop,
  onLocation,
  onMotionChange,
  onGeofence,
  onSchedule,
  startSchedule as sdkStartSchedule,
  stopSchedule as sdkStopSchedule,
  destroyLocations,
  addGeofence as sdkAddGeofence,
  removeGeofence as sdkRemoveGeofence,
  removeGeofences as sdkRemoveGeofences,
  getGeofences as sdkGetGeofences,
  type Location,
  type Geofence,
  type GeofenceEvent,
  type ScheduleEvent,
} from 'bearings';
import HomeScreen from './screens/HomeScreen';
import SettingsScreen from './screens/SettingsScreen';
import AddGeofenceScreen from './screens/AddGeofenceScreen';
import { COLORS, DEFAULT_CONFIG } from './lib/config';
import { loadSettings } from './lib/settingsStorage';
import { requestLocationPermissions } from './lib/permissions';
import { haversineDistance } from './lib/geo';

// --- Context ---

interface BearingsContextValue {
  tracking: boolean;
  isMoving: boolean;
  schedulerRunning: boolean;
  locations: Location[];
  distance: number;
  stationaryRadius: number;
  geofences: Geofence[];
  geofenceEvents: GeofenceEvent[];
  scheduleEvents: ScheduleEvent[];
  startTracking: () => void;
  stopTracking: () => void;
  startSchedule: () => Promise<void>;
  stopSchedule: () => Promise<void>;
  clearLocations: () => void;
  resetDistance: () => void;
  addGeofence: (geofence: Geofence) => Promise<void>;
  removeGeofence: (identifier: string) => Promise<void>;
  removeAllGeofences: () => Promise<void>;
}

const BearingsContext = createContext<BearingsContextValue | null>(null);

export function useBearings(): BearingsContextValue {
  const ctx = useContext(BearingsContext);
  if (!ctx) {
    throw new Error('useBearings must be used within a BearingsProvider');
  }
  return ctx;
}

// --- Provider ---

const MAX_LOCATIONS = 1000;

function BearingsProvider({ children }: { children: ReactNode }) {
  const [tracking, setTracking] = useState(false);
  const [isMoving, setIsMoving] = useState(false);
  const [locations, setLocations] = useState<Location[]>([]);
  const [distance, setDistance] = useState(0);
  const [stationaryRadius, setStationaryRadius] = useState(
    DEFAULT_CONFIG.stationaryRadius
  );
  const [geofences, setGeofences] = useState<Geofence[]>([]);
  const [geofenceEvents, setGeofenceEvents] = useState<GeofenceEvent[]>([]);
  const [schedulerRunning, setSchedulerRunning] = useState(false);
  const [scheduleEvents, setScheduleEvents] = useState<ScheduleEvent[]>([]);
  const configured = useRef(false);
  const lastLocation = useRef<Location | null>(null);

  useEffect(() => {
    async function init() {
      await requestLocationPermissions();
      const settings = await loadSettings();
      setStationaryRadius(
        settings.stationaryRadius ?? DEFAULT_CONFIG.stationaryRadius
      );
      await configure(settings);
      configured.current = true;

      // Load persisted geofences
      const savedGeofences = await sdkGetGeofences();
      setGeofences(savedGeofences);
    }
    init();
  }, []);

  useEffect(() => {
    const locationSub = onLocation((location) => {
      setLocations((prev) => {
        const next = [location, ...prev];
        return next.length > MAX_LOCATIONS
          ? next.slice(0, MAX_LOCATIONS)
          : next;
      });

      // Accumulate distance
      if (lastLocation.current) {
        const d = haversineDistance(
          lastLocation.current.latitude,
          lastLocation.current.longitude,
          location.latitude,
          location.longitude
        );
        setDistance((prev) => prev + d);
      }
      lastLocation.current = location;
    });

    const motionSub = onMotionChange((event) => {
      setIsMoving(event.isMoving);
      console.log(
        `[Bearings] Motion change: ${event.isMoving ? 'MOVING' : 'STATIONARY'}`
      );
    });

    const geofenceSub = onGeofence((event) => {
      setGeofenceEvents((prev) => [event, ...prev].slice(0, 100));
      console.log(`[Bearings] Geofence: ${event.action} ${event.identifier}`);
    });

    const scheduleSub = onSchedule((event) => {
      setScheduleEvents((prev) => [event, ...prev].slice(0, 100));
      setTracking(event.enabled);
      setIsMoving(event.enabled);
      console.log(
        `[Bearings] Schedule: tracking ${event.enabled ? 'started' : 'stopped'}`
      );
    });

    return () => {
      locationSub.remove();
      motionSub.remove();
      geofenceSub.remove();
      scheduleSub.remove();
    };
  }, []);

  const startTracking = useCallback(async () => {
    if (!configured.current) return;
    await start();
    setTracking(true);
    setIsMoving(true);
  }, []);

  const stopTracking = useCallback(async () => {
    await stop();
    setTracking(false);
    setIsMoving(false);
  }, []);

  const clearLocations = useCallback(async () => {
    await destroyLocations();
    setLocations([]);
    setDistance(0);
    lastLocation.current = null;
  }, []);

  const resetDistance = useCallback(() => {
    setDistance(0);
  }, []);

  const handleAddGeofence = useCallback(async (geofence: Geofence) => {
    await sdkAddGeofence(geofence);
    const updated = await sdkGetGeofences();
    setGeofences(updated);
  }, []);

  const handleRemoveGeofence = useCallback(async (identifier: string) => {
    await sdkRemoveGeofence(identifier);
    const updated = await sdkGetGeofences();
    setGeofences(updated);
  }, []);

  const handleStartSchedule = useCallback(async () => {
    if (!configured.current) return;
    const state = await sdkStartSchedule();
    setSchedulerRunning(state.schedulerRunning);
  }, []);

  const handleStopSchedule = useCallback(async () => {
    const state = await sdkStopSchedule();
    setSchedulerRunning(state.schedulerRunning);
    setTracking(false);
    setIsMoving(false);
  }, []);

  const handleRemoveAllGeofences = useCallback(async () => {
    await sdkRemoveGeofences();
    setGeofences([]);
    setGeofenceEvents([]);
  }, []);

  return (
    <BearingsContext.Provider
      value={{
        tracking,
        isMoving,
        schedulerRunning,
        locations,
        distance,
        stationaryRadius,
        geofences,
        geofenceEvents,
        scheduleEvents,
        startTracking,
        stopTracking,
        startSchedule: handleStartSchedule,
        stopSchedule: handleStopSchedule,
        clearLocations,
        resetDistance,
        addGeofence: handleAddGeofence,
        removeGeofence: handleRemoveGeofence,
        removeAllGeofences: handleRemoveAllGeofences,
      }}
    >
      {children}
    </BearingsContext.Provider>
  );
}

// --- Navigation ---

type RootStackParamList = {
  Home: undefined;
  Settings: undefined;
  AddGeofence: { latitude: number; longitude: number };
};

const Stack = createNativeStackNavigator<RootStackParamList>();

function HeaderSwitch() {
  const { tracking, startTracking, stopTracking } = useBearings();

  return (
    <Switch
      value={tracking}
      onValueChange={(value) => {
        if (value) {
          startTracking();
        } else {
          stopTracking();
        }
      }}
      trackColor={{ false: '#ccc', true: COLORS.green }}
    />
  );
}

export default function App() {
  return (
    <BearingsProvider>
      <NavigationContainer>
        <Stack.Navigator>
          <Stack.Screen
            name="Home"
            component={HomeScreen}
            options={{
              title: 'Bearings',
              headerRight: HeaderSwitch,
            }}
          />
          <Stack.Screen
            name="Settings"
            component={SettingsScreen}
            options={{ title: 'Settings' }}
          />
          <Stack.Screen
            name="AddGeofence"
            component={AddGeofenceScreen}
            options={{ title: 'Add Geofence' }}
          />
        </Stack.Navigator>
      </NavigationContainer>
    </BearingsProvider>
  );
}
