package com.afunx.ble.utils;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothAdapter.LeScanCallback;

public class BleUtils {
	
	/**
	 * Callback of ble
	 */
	public interface Callback {
		public void onCharacteristicChanged(BluetoothGatt gatt,
				BluetoothGattCharacteristic characteristic);
	}
	
	private static BluetoothAdapter getAdapter() {
		return BluetoothAdapter.getDefaultAdapter();
	}

	/**
	 * Open ble brutally without user's mind
	 */
	public static void openBleBrutally() {
		BluetoothAdapter adapter = getAdapter();
		if (!adapter.isEnabled()) {
			BluetoothAdapter.getDefaultAdapter().enable();
		}
	}
	
	/**
	 * Starts a scan for Bluetooth LE devices.
	 * 
	 * Results of the scan are reported using the
	 * {@link LeScanCallback#onLeScan} callback.
	 * 
	 * {@link BluetoothAdapter#startLeScan(LeScanCallback)}
	 * 
	 * @param callback
	 *            callback the callback LE scan results are delivered
	 * @return true, if the scan was started successfully
	 */
	public static boolean startLeScan(LeScanCallback callback) {
		BluetoothAdapter adapter = getAdapter();
		return adapter.startLeScan(callback);
	}

	/**
	 * Stops an ongoing Bluetooth LE device scan.
	 * 
	 * @param callback
	 *            used to identify which scan to stop must be the same handle
	 *            used to start the scan
	 *            {@link BluetoothAdapter#stopLeScan(LeScanCallback)}
	 */
	public static void stopLeScan(LeScanCallback callback) {
		BluetoothAdapter adapter = getAdapter();
		adapter.stopLeScan(callback);
	}
}
