import AsyncStorage from '@react-native-async-storage/async-storage';
import type { Config } from 'geomony';
import { DEFAULT_CONFIG } from './config';

const STORAGE_KEY = '@geomony/settings';

export async function loadSettings(): Promise<Partial<Config>> {
  try {
    const raw = await AsyncStorage.getItem(STORAGE_KEY);
    if (raw) {
      return JSON.parse(raw) as Partial<Config>;
    }
  } catch {
    // ignore read errors, return defaults
  }
  return { ...DEFAULT_CONFIG };
}

export async function saveSettings(config: Partial<Config>): Promise<void> {
  await AsyncStorage.setItem(STORAGE_KEY, JSON.stringify(config));
}
