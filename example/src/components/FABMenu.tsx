import { useState, useRef } from 'react';
import {
  View,
  TouchableOpacity,
  Animated,
  Alert,
  StyleSheet,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import type { NativeStackNavigationProp } from '@react-navigation/native-stack';
import { COLORS } from '../lib/config';
import { requestLocationPermissions } from '../lib/permissions';

type RootStackParamList = {
  Home: undefined;
  Settings: undefined;
  AddGeofence: { latitude: number; longitude: number };
};

interface Props {
  navigation: NativeStackNavigationProp<RootStackParamList, 'Home'>;
  onDestroyLocations: () => void;
  onResetDistance: () => void;
}

interface Action {
  icon: React.ComponentProps<typeof Ionicons>['name'];
  color: string;
  onPress: () => void;
}

export default function FABMenu({
  navigation,
  onDestroyLocations,
  onResetDistance,
}: Props) {
  const [open, setOpen] = useState(false);
  const animation = useRef(new Animated.Value(0)).current;

  const toggle = () => {
    const toValue = open ? 0 : 1;
    Animated.timing(animation, {
      toValue,
      duration: 250,
      useNativeDriver: true,
    }).start();
    setOpen(!open);
  };

  const close = () => {
    Animated.timing(animation, {
      toValue: 0,
      duration: 200,
      useNativeDriver: true,
    }).start();
    setOpen(false);
  };

  const handleDestroy = () => {
    close();
    Alert.alert(
      'Destroy Locations',
      'Delete all stored locations? This cannot be undone.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Delete',
          style: 'destructive',
          onPress: onDestroyLocations,
        },
      ]
    );
  };

  const handlePermissions = async () => {
    close();
    const result = await requestLocationPermissions();
    Alert.alert(
      'Permissions',
      `Foreground: ${result.foreground ? 'Granted' : 'Denied'}\nBackground: ${
        result.background ? 'Granted' : 'Denied'
      }`
    );
  };

  const actions: Action[] = [
    {
      icon: 'settings',
      color: COLORS.grey,
      onPress: () => {
        close();
        navigation.navigate('Settings' as never);
      },
    },
    {
      icon: 'refresh',
      color: COLORS.primary,
      onPress: () => {
        close();
        onResetDistance();
      },
    },
    {
      icon: 'trash',
      color: COLORS.red,
      onPress: handleDestroy,
    },
    {
      icon: 'shield-checkmark',
      color: COLORS.green,
      onPress: handlePermissions,
    },
  ];

  const overlayOpacity = animation.interpolate({
    inputRange: [0, 1],
    outputRange: [0, 1],
  });

  const rotation = animation.interpolate({
    inputRange: [0, 1],
    outputRange: ['0deg', '45deg'],
  });

  return (
    <>
      {open && (
        <Animated.View
          style={[styles.overlay, { opacity: overlayOpacity }]}
          pointerEvents={open ? 'auto' : 'none'}
        >
          <TouchableOpacity
            style={StyleSheet.absoluteFill}
            activeOpacity={1}
            onPress={close}
          />
        </Animated.View>
      )}
      <View style={styles.container} pointerEvents="box-none">
        {actions.map((action, index) => {
          const translateY = animation.interpolate({
            inputRange: [0, 1],
            outputRange: [0, -(index + 1) * 60],
          });
          const opacity = animation;
          return (
            <Animated.View
              key={action.icon}
              style={[
                styles.actionButton,
                { transform: [{ translateY }], opacity },
              ]}
            >
              <TouchableOpacity
                style={[
                  styles.actionTouchable,
                  { backgroundColor: action.color },
                ]}
                onPress={action.onPress}
              >
                <Ionicons name={action.icon} size={22} color={COLORS.white} />
              </TouchableOpacity>
            </Animated.View>
          );
        })}
        <TouchableOpacity style={styles.fab} onPress={toggle}>
          <Animated.View style={{ transform: [{ rotate: rotation }] }}>
            <Ionicons name="add" size={28} color={COLORS.white} />
          </Animated.View>
        </TouchableOpacity>
      </View>
    </>
  );
}

const styles = StyleSheet.create({
  overlay: {
    ...StyleSheet.absoluteFillObject,
    backgroundColor: 'rgba(0,0,0,0.3)',
  },
  container: {
    position: 'absolute',
    right: 16,
    bottom: 76,
    alignItems: 'center',
  },
  fab: {
    width: 56,
    height: 56,
    borderRadius: 28,
    backgroundColor: COLORS.primary,
    alignItems: 'center',
    justifyContent: 'center',
    elevation: 6,
    shadowColor: COLORS.black,
    shadowOffset: { width: 0, height: 3 },
    shadowOpacity: 0.3,
    shadowRadius: 4,
  },
  actionButton: {
    position: 'absolute',
    bottom: 0,
  },
  actionTouchable: {
    width: 44,
    height: 44,
    borderRadius: 22,
    alignItems: 'center',
    justifyContent: 'center',
    elevation: 4,
    shadowColor: COLORS.black,
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.25,
    shadowRadius: 3,
  },
});
