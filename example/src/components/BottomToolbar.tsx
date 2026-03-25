import { View, Text, TouchableOpacity, StyleSheet } from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { COLORS } from '../lib/config';

interface Props {
  tracking: boolean;
  locationCount: number;
  distance: number;
  onToggleTracking: () => void;
  onRecenter: () => void;
}

function formatDistance(meters: number): string {
  if (meters < 1000) {
    return `${Math.round(meters)} m`;
  }
  return `${(meters / 1000).toFixed(1)} km`;
}

export default function BottomToolbar({
  tracking,
  locationCount,
  distance,
  onToggleTracking,
  onRecenter,
}: Props) {
  return (
    <View style={styles.container}>
      <TouchableOpacity style={styles.iconButton} onPress={onRecenter}>
        <Ionicons name="locate" size={24} color={COLORS.black} />
      </TouchableOpacity>

      <View style={styles.info}>
        <Text style={styles.infoText}>
          {locationCount} pts {'\u00B7'} {formatDistance(distance)}
        </Text>
      </View>

      <TouchableOpacity style={styles.iconButton} onPress={onToggleTracking}>
        <Ionicons
          name={tracking ? 'pause' : 'play'}
          size={24}
          color={tracking ? COLORS.red : COLORS.green}
        />
      </TouchableOpacity>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: COLORS.gold,
    paddingHorizontal: 12,
    paddingVertical: 10,
    borderTopLeftRadius: 0,
    borderTopRightRadius: 0,
  },
  iconButton: {
    width: 44,
    height: 44,
    borderRadius: 22,
    backgroundColor: 'rgba(255,255,255,0.85)',
    alignItems: 'center',
    justifyContent: 'center',
  },
  info: {
    flex: 1,
    alignItems: 'center',
  },
  infoText: {
    fontSize: 15,
    fontWeight: '600',
    color: COLORS.black,
  },
});
