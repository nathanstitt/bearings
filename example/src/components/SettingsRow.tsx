import { useState } from 'react';
import {
  View,
  Text,
  Switch,
  TextInput,
  TouchableOpacity,
  Modal,
  FlatList,
  StyleSheet,
} from 'react-native';
import { COLORS } from '../lib/config';

interface BaseProps {
  label: string;
}

interface SwitchRowProps extends BaseProps {
  type: 'switch';
  value: boolean;
  onValueChange: (value: boolean) => void;
}

interface PickerOption {
  label: string;
  value: number;
}

interface PickerRowProps extends BaseProps {
  type: 'picker';
  value: number;
  options: PickerOption[];
  onValueChange: (value: number) => void;
}

interface TextRowProps extends BaseProps {
  type: 'text';
  value: string;
  placeholder?: string;
  onValueChange: (value: string) => void;
}

type Props = SwitchRowProps | PickerRowProps | TextRowProps;

export default function SettingsRow(props: Props) {
  const [pickerVisible, setPickerVisible] = useState(false);

  if (props.type === 'switch') {
    return (
      <View style={styles.row}>
        <Text style={styles.label}>{props.label}</Text>
        <Switch
          value={props.value}
          onValueChange={props.onValueChange}
          trackColor={{ false: '#ccc', true: COLORS.primary }}
        />
      </View>
    );
  }

  if (props.type === 'picker') {
    const selectedOption = props.options.find((o) => o.value === props.value);
    return (
      <>
        <TouchableOpacity
          style={styles.row}
          onPress={() => setPickerVisible(true)}
        >
          <Text style={styles.label}>{props.label}</Text>
          <Text style={styles.pickerValue}>
            {selectedOption?.label ?? String(props.value)}
          </Text>
        </TouchableOpacity>
        <Modal
          visible={pickerVisible}
          transparent
          animationType="fade"
          onRequestClose={() => setPickerVisible(false)}
        >
          <TouchableOpacity
            style={styles.modalOverlay}
            activeOpacity={1}
            onPress={() => setPickerVisible(false)}
          >
            <View style={styles.modalContent}>
              <Text style={styles.modalTitle}>{props.label}</Text>
              <FlatList
                data={props.options}
                keyExtractor={(item) => String(item.value)}
                renderItem={({ item }) => (
                  <TouchableOpacity
                    style={[
                      styles.modalOption,
                      item.value === props.value && styles.modalOptionSelected,
                    ]}
                    onPress={() => {
                      props.onValueChange(item.value);
                      setPickerVisible(false);
                    }}
                  >
                    <Text
                      style={[
                        styles.modalOptionText,
                        item.value === props.value &&
                          styles.modalOptionTextSelected,
                      ]}
                    >
                      {item.label}
                    </Text>
                  </TouchableOpacity>
                )}
              />
            </View>
          </TouchableOpacity>
        </Modal>
      </>
    );
  }

  // text
  return (
    <View style={styles.row}>
      <Text style={styles.label}>{props.label}</Text>
      <TextInput
        style={styles.textInput}
        value={props.value}
        placeholder={props.placeholder}
        onChangeText={props.onValueChange}
        autoCapitalize="none"
        autoCorrect={false}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingHorizontal: 16,
    paddingVertical: 12,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: '#eee',
    minHeight: 48,
  },
  label: {
    fontSize: 16,
    color: COLORS.black,
    flex: 1,
  },
  pickerValue: {
    fontSize: 16,
    color: COLORS.primary,
  },
  textInput: {
    fontSize: 16,
    color: COLORS.black,
    textAlign: 'right',
    flex: 1,
    marginLeft: 16,
    padding: 0,
  },
  modalOverlay: {
    flex: 1,
    backgroundColor: 'rgba(0,0,0,0.5)',
    justifyContent: 'center',
    alignItems: 'center',
  },
  modalContent: {
    backgroundColor: COLORS.white,
    borderRadius: 12,
    width: 280,
    maxHeight: 400,
    overflow: 'hidden',
  },
  modalTitle: {
    fontSize: 17,
    fontWeight: '600',
    padding: 16,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: '#eee',
    textAlign: 'center',
  },
  modalOption: {
    paddingHorizontal: 16,
    paddingVertical: 14,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: '#eee',
  },
  modalOptionSelected: {
    backgroundColor: 'rgba(33,150,243,0.08)',
  },
  modalOptionText: {
    fontSize: 16,
    color: COLORS.black,
  },
  modalOptionTextSelected: {
    color: COLORS.primary,
    fontWeight: '600',
  },
});
