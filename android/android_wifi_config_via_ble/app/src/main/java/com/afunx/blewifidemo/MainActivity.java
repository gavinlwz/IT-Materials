package com.afunx.blewifidemo;

import com.afunx.ble.adapter.BleDeviceAdapter;
import com.afunx.ble.constants.BleKeys;
import com.afunx.ble.device.BleDevice;
import com.afunx.ble.utils.BleDeviceUtils;
import com.afunx.ble.utils.BleUtils;
import com.afunx.ble.utils.ScanRecordUtil;
import com.afunx.ble.utils.WifiUtils;

import android.app.Activity;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothAdapter.LeScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.ParcelUuid;
import android.support.v4.widget.SwipeRefreshLayout;
import android.support.v4.widget.SwipeRefreshLayout.OnRefreshListener;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Toast;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ListView;

public class MainActivity extends Activity {
	
	private static final String TAG = "MainActivity";
	
	private SwipeRefreshLayout mSwipeRefreshLayout;
	
	private ListView mListView;
	private BleDeviceAdapter mBleDeviceAdapter;
	private LeScanCallback mLeScanCallback;
	private Handler mHandler;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mHandler = new Handler();
		// it is brutally sometimes
		BleUtils.openBleBrutally();
		WifiUtils.openWifiBrutally(getApplicationContext());
		init();
		doRefresh();
	}
	
	@Override
	protected void onResume() {
		super.onResume();
	}
	
	@Override
	protected void onPause() {
		super.onPause();
	}
	
	private void init() {
		setContentView(R.layout.activity_main);
		// swipe refresh layout and listview
		mSwipeRefreshLayout = (SwipeRefreshLayout) findViewById(R.id.swipeRefreshLayout);
		mListView = (ListView) findViewById(R.id.lv_devices);
//		mListView.setEmptyView(findViewById(R.id.pb_empty));
		mSwipeRefreshLayout.setOnRefreshListener(new OnRefreshListener() {
			@Override
			public void onRefresh() {
				doRefresh();
			}
		});
		
		// ble device adapter
		mBleDeviceAdapter = new BleDeviceAdapter(this);
		mListView.setAdapter(mBleDeviceAdapter);
		// listview OnItemClickListener
		mListView.setOnItemClickListener(new OnItemClickListener(){

			@Override
			public void onItemClick(AdapterView<?> parent, View view,
					int position, long id) {
				Log.w(TAG, "item " + position + " is selected");
				Intent intent = new Intent(MainActivity.this, ConnectWifiActivity.class);
				BleDevice bleDevice = (BleDevice) mBleDeviceAdapter.getItem(position);
				intent.putExtra(BleKeys.BLE_ADDRESS, bleDevice.getBluetoothDevice().getAddress());
				startActivity(intent);
			}
			
		});
		
		// LeScanCallback
		mLeScanCallback = new LeScanCallback() {
			@Override
			public void onLeScan(BluetoothDevice device, int rssi, byte[] scanRecord) {
				Log.w(TAG, "onLeScan");

				if(BleDeviceUtils.isEspBluetoothDevice(device)) {
					BleDevice bleDevice = new BleDevice();
					bleDevice.setBluetoothDevice(device);
					bleDevice.setRssi(rssi);
					bleDevice.setScanRecord(scanRecord);
					final ScanRecordUtil scanRecordUtil = ScanRecordUtil.parseFromBytes(scanRecord);
					Log.d(TAG, "MonitorUtil onLeScan " + ScanRecordUtil.parseFromBytes(scanRecord).toString());
					boolean b = scanRecordUtil.getServiceData().containsKey(ParcelUuid.fromString(BleKeys.UUID_STR));
					if(b) {
						Log.d(TAG,"contain:"+BleKeys.UUID_STR);
					}
					b = scanRecordUtil.getServiceData().containsKey(ParcelUuid.fromString(BleKeys.UUID_STR2));
					if(b) {
						Log.d(TAG,"contain:"+BleKeys.UUID_STR2);
					}
					mBleDeviceAdapter.addOrUpdateDevice(bleDevice);
					mBleDeviceAdapter.notifyDataSetChanged();
				}
			}
		};
		
	}

	private void doRefresh() {
		final long interval = 5000;
		mSwipeRefreshLayout.setRefreshing(true);
		Toast.makeText(this, R.string.scaning, Toast.LENGTH_LONG).show();
		// clear ble devices in UI
		mBleDeviceAdapter.clear();
		new Thread() {
			public void run() {
				BleUtils.startLeScan(mLeScanCallback);
				try {
					Thread.sleep(interval);
				} catch (InterruptedException ignore) {
				}
				BleUtils.stopLeScan(mLeScanCallback);
				mHandler.post(new Runnable() {
					@Override
					public void run() {
						// stop swipe refresh refreshing
						mSwipeRefreshLayout.setRefreshing(false);
					}
				});
			}
		}.start();
	}
	
}
