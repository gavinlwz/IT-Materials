package com.afunx.ble.task;

import java.util.List;
import java.util.UUID;
import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;

import com.afunx.ble.constants.BleKeys;
import com.afunx.ble.utils.BleUtils;
import com.afunx.ble.utils.ByteUtils;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.util.Log;

public class WifiConnectTask implements Runnable {

	private static final String TAG = "WifiConnectTask";
	private final Context mContext;
	private final String mBleAddress;
	private final Object mLockConnect = new Object();
	private volatile boolean mIsConnectSuc = false;
	private final Object mLockDiscover = new Object();
	private volatile boolean mIsDiscoverSuc = false;
	private final Object mLockFinish = new Object();
	private volatile boolean mIsFinishSuc = false;
	private BluetoothGatt mBluetoothGatt;
	private String mSsid;
	private String mPwd;
	private Runnable mCallbackFinish;
	private Runnable mCallbackTimeout;
	private Runnable mCallbackSuc;
	private Runnable mCallbackFail;
	private BleUtils.Callback mCallbackBle;
	private BluetoothGattCharacteristic mGattCharacteristic;
	
	private static final long INTERVAL_WRITE_GATT_CHAR = 800;
	private static final long TIMEOUT_SERVICE_DISCOVER = 1000;
	private static final long TIMEOUT_BLE_CONNECT = 8000;
	private static final long TIMEOUT_BLE_CONNECT_WIFI = 20000;
	private List<BluetoothGattService> mServiceList;

	public void setSsid(String ssid) {
		mSsid = ssid;
	}

	public void setPassword(String password) {
		mPwd = password;
	}

	public void setFinishCallback(Runnable callbackFinish) {
		mCallbackFinish = callbackFinish;
	}
	
	public void setTimeoutCallback(Runnable callbackTimeout) {
		mCallbackTimeout = callbackTimeout;
	}
	
	public void setSucCallback(Runnable callbackSuc) {
		mCallbackSuc = callbackSuc;
	}
	
	public void setFailCallback(Runnable callbackFail) {
		mCallbackFail = callbackFail;
	}
	
	public void setBleCallback(BleUtils.Callback callbackBle) {
		mCallbackBle = callbackBle;
	}
	
	private void notifyLockConnect() {
		mIsConnectSuc = true;
		synchronized (mLockConnect) {
			mLockConnect.notify();
		}
	}
	
	private void notifyLockDiscover() {
		mIsDiscoverSuc = true;
		synchronized (mLockDiscover) {
			mLockDiscover.notify();
		}
	}
	
	private void notifyLockFinish() {
		mIsFinishSuc = true;
		synchronized (mLockFinish) {
			mLockFinish.notify();
		}
	}
	
	private BluetoothGattCallback mBluetoothGattCallback = new BluetoothGattCallback() {
		@Override
		public void onConnectionStateChange(BluetoothGatt gatt, int status,
				int newState) {
			Log.i(TAG, "onConnectionStateChange() status:" + status
					+ ",newState:" + newState);
			switch(newState){
			case BluetoothProfile.STATE_CONNECTED:
				notifyLockConnect();
				break;
			case BluetoothProfile.STATE_DISCONNECTED:
				connectAsync();
				break;
			}
		}

		@Override
		public void onCharacteristicRead(BluetoothGatt gatt,
				BluetoothGattCharacteristic characteristic, int status) {
			Log.i(TAG, "onCharacteristicRead() characteristic:"
					+ characteristic + ",status:" + status);
		}

		@Override
		public void onCharacteristicWrite(BluetoothGatt gatt,
				BluetoothGattCharacteristic characteristic, int status) {
			Log.i(TAG, "onCharacteristicWrite() characteristic:"
					+ characteristic + ",status:" + status);
		}

		@Override
		public void onDescriptorRead(BluetoothGatt gatt,
				BluetoothGattDescriptor descriptor, int status) {
			Log.i(TAG, "onDescriptorRead() descriptor:" + descriptor
					+ ",status:" + status);
		}

		@Override
		public void onDescriptorWrite(BluetoothGatt gatt,
				BluetoothGattDescriptor descriptor, int status) {
			Log.i(TAG, "onDescriptorRead() descriptor:" + descriptor
					+ ",status:" + status);
		}

		@Override
		public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status) {
			Log.i(TAG, "onReadRemoteRssi() rssi:" + rssi + ",status:" + status);
		}

		@Override
		public void onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
			Log.i(TAG, "onMtuChanged() mtu:" + mtu + ",status:" + status);
		}

		@Override
		public void onServicesDiscovered(BluetoothGatt gatt, int status) {
			Log.i(TAG, "onServicesDiscovered() status:" + status);
			if(status==BluetoothGatt.GATT_SUCCESS) {
				notifyLockDiscover();
			}
		}
		
		@Override
		public void onCharacteristicChanged(BluetoothGatt gatt,
				BluetoothGattCharacteristic characteristic) {
			UUID uuid = characteristic.getUuid();
			byte[] value = characteristic.getValue();
			Log.i(TAG, "onCharacteristicChanged uuid:" + uuid + ", value:"
					+ ByteUtils.prettyFormat(value));
			if (mCallbackBle != null) {
				mCallbackBle.onCharacteristicChanged(gatt, characteristic);
			}
			notifyLockFinish();
		}
		
		@Override
		public void onReliableWriteCompleted(BluetoothGatt gatt, int status) {
			Log.i(TAG, "onReliableWriteCompleted() status:" + status);
		}
	};
	
	private WifiConnectTask(Context context, String bleAddress) {
		mContext = context;
		mBleAddress = bleAddress;
	}
	
	public static WifiConnectTask createInstance(Context context,
			String bleAddress) {
		Log.i(TAG, "bleAddress:" + bleAddress);
		return new WifiConnectTask(context, bleAddress);
	}

	private static BluetoothAdapter getAdapter() {
		return BluetoothAdapter.getDefaultAdapter();
	}
	
	private boolean connectAsync() {
		BluetoothAdapter adapter = getAdapter();
		if (adapter == null) {
			Log.w(TAG, "BluetoothAdapter is null");
			return false;
		}
		if (mBluetoothGatt == null) {
			final BluetoothDevice device = adapter.getRemoteDevice(mBleAddress);
			if (device == null) {
				Log.w(TAG, "Device not found");
				return false;
			}
			Log.d(TAG, "connectAsync() connectGatt");
			mBluetoothGatt = device.connectGatt(mContext, false,
					mBluetoothGattCallback);
			return true;
		} else {
			Log.d(TAG, "connectAsync() connect");
			return mBluetoothGatt.connect();
		}
	}
	
	private void sendMsg(BluetoothGattCharacteristic gattChar,String msg) {
		Log.d(TAG,"send msg: "+msg);
		gattChar.setValue(msg);
		boolean ret = mBluetoothGatt.writeCharacteristic(gattChar);
		try {
			Thread.sleep(INTERVAL_WRITE_GATT_CHAR);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		Log.d(TAG,"sendMsg ret="+ret);
	}
	public static String substring(String str, int f, int t) {
		if (f > str.length())
			return null;
		if (t > str.length()) {
			return str.substring(f, str.length());
		} else {
			return str.substring(f, t);
		}
	}
	private void sendWifiSsidPwd(BluetoothGattCharacteristic gattChar) {
       //three package
		String msg;
		sendMsg(gattChar, "size:3");
		try {
			JSONObject root = new JSONObject();

			root.put("SSID", mSsid);
			root.put("password", mPwd);
			root.put("encrypt", "WPAPSK");
			root.put("channel", "0");

			JSONObject gwinfo = new JSONObject();
			gwinfo.put("user_key", "unisound");
			gwinfo.put("gwAddress2", "https://112.13.96.199:20443");


			root.put("CGW",gwinfo);
			msg = root.toString();
			int size = msg.length();
			int sizeI = (size +2)/3;
			String msg1 = substring(msg,0,sizeI);
			String msg2 = substring(msg,sizeI,sizeI*2);
			String msg3 = substring(msg,sizeI*2,sizeI*3);

			Log.d(TAG,msg);
			sendMsg(gattChar, msg1);
			sendMsg(gattChar, msg2);
			sendMsg(gattChar, msg3);
		}catch (JSONException e){
			e.printStackTrace();
		}


	}
	

	private void doSucCallback() {
		if (mCallbackSuc != null) {
			mCallbackSuc.run();
		}
	}
	
	private void doFailCallback() {
		if (mCallbackFail != null) {
			mCallbackFail.run();
		}
	}
	
	private void enableNotification() {
		// enable notification
		if(mGattCharacteristic!=null) {
			mBluetoothGatt.setCharacteristicNotification(mGattCharacteristic, true);
		} else {
			Log.w(TAG, "enableNotification() mGattCharacteristic is null");
		}
	}
	
	private void disableNotification() {
		// disable notification
		if (mBluetoothGatt != null && mGattCharacteristic != null) {
			mBluetoothGatt.setCharacteristicNotification(mGattCharacteristic,
					false);
		} else {
			Log.w(TAG, "disableNotification() mBluetoothGatt or mGattCharacteristic is null");
		}
	}
	
	private void close() {
		if (mBluetoothGatt != null) {
			mBluetoothGatt.disconnect();
			mBluetoothGatt.close();
			mBluetoothGatt = null;
		}
	}

	private void showCharactersOfService(BluetoothGattService service) {
		List<BluetoothGattCharacteristic> cList = service.getCharacteristics();
		for (BluetoothGattCharacteristic c : cList) {
			Log.d(TAG,c.getUuid().toString());
		}
	}
	
	@Override
	public void run() {
		Log.i(TAG, "Thread:" + Thread.currentThread());
		// clear state
		mIsConnectSuc = false;
		mIsDiscoverSuc = false;
		mIsFinishSuc = false;
		
		synchronized (mLockDiscover) {
			synchronized (mLockConnect) {
				// connect async
				if (connectAsync()) {
					Log.i(TAG, "connectAsync() suc");
					// wait connect result
					try {
						mLockConnect.wait(TIMEOUT_BLE_CONNECT);
					} catch (InterruptedException ignore) {
					}
					if (!mIsConnectSuc) {
						Log.i(TAG, "fail to connect ble: " + mBleAddress);
						doFailCallback();
						return;
					}

					mBluetoothGatt.discoverServices();
					try {
						mLockDiscover.wait(TIMEOUT_SERVICE_DISCOVER);
					} catch (InterruptedException ignore) {
					}
					if (!mIsDiscoverSuc) {
						Log.i(TAG, "fail to discover services");
						doFailCallback();
						return;
					}
					BluetoothGattCharacteristic gattCharacteristic;
					//get unisound characteristic
					final UUID serviceUuid1 = BleKeys.UUID_WIFI_SERVICE_UNIONE;
					final UUID characteristic1 = BleKeys.UUID_CONFIGURE_CHARACTERISTIC_UNIONE;
					final BluetoothGattService gattService1 = mBluetoothGatt
							.getService(serviceUuid1);
					if (gattService1 != null) {
						Log.w(TAG, "get service: " + serviceUuid1
								+ ", gatt:" + mBluetoothGatt);
						gattCharacteristic = gattService1
								.getCharacteristic(characteristic1);
						if (gattCharacteristic == null) {
							Log.w(TAG, "fail to get characteristic: "
									+ characteristic1);
							showCharactersOfService(gattService1);
							doFailCallback();
							return;
						}

					} else {
						// get characteristic
						final UUID serviceUuid = BleKeys.UUID_WIFI_SERVICE;
						final UUID characteristic = BleKeys.UUID_CONFIGURE_CHARACTERISTIC;
						final BluetoothGattService gattService = mBluetoothGatt
								.getService(serviceUuid);
						if (gattService == null) {
							Log.w(TAG, "fail to get service: " + serviceUuid
									+ ", gatt:" + mBluetoothGatt);
							doFailCallback();
							return;
						}
						gattCharacteristic = gattService
								.getCharacteristic(characteristic);
						if (gattCharacteristic == null) {
							Log.w(TAG, "fail to get characteristic: "
									+ characteristic);
							showCharactersOfService(gattService);
							doFailCallback();
							return;
						}
					}
					// send wifi ssid and pwd
					sendWifiSsidPwd(gattCharacteristic);
					
					mGattCharacteristic = gattCharacteristic;
					
					// enable notification
					enableNotification();
					
					synchronized (mLockFinish) {
						try {
							mLockFinish.wait(TIMEOUT_BLE_CONNECT_WIFI);
						} catch (InterruptedException ignore) {
						} finally {
							if (mIsFinishSuc) {
								if (mCallbackFinish != null) {
									mCallbackFinish.run();
								}
							} else {
								if (mCallbackTimeout != null) {
									mCallbackTimeout.run();
								}
							}
							disableNotification();
							close();
						}
					}
				}

			}
		}
		Log.i(TAG, "WifiConnectTask run() exit");
	}

}
