package com.afunx.ble.utils;

import java.util.List;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.util.Log;

public class WifiUtils {
	
	private static final String TAG = "WifiUtils";
	
	/**
	 * Callback of scan wifi
	 */
	public interface Callback {
		void onWifiScan(List<ScanResult> scanResultList);
	}
	
	private static volatile boolean sIsStop = true;
	private static volatile Thread sThread = null;
	
	private static void startWifiScanInternal(final Context context,
			final Callback callback, final long interval) {
		WifiManager wifiManager = (WifiManager) context
				.getSystemService(Context.WIFI_SERVICE);
		long startTimestamp;
		long sleepTimestamp;
		while (!sIsStop) {
			startTimestamp = System.currentTimeMillis();
			List<ScanResult> scanResultList = wifiManager.getScanResults();
			if (scanResultList != null && !scanResultList.isEmpty()) {
				callback.onWifiScan(scanResultList);
			}
			wifiManager.startScan();
			sleepTimestamp = interval - (System.currentTimeMillis() - startTimestamp);
			if (sleepTimestamp > 0 && scanResultList != null
					&& !scanResultList.isEmpty()) {
				try {
					Thread.sleep(sleepTimestamp);
				} catch (InterruptedException ignore) {
				}
			}
		}
	}
	
	private static void startWifiScanOnceInternal(final Context context,
			final Callback callback, final long interval) {
		WifiManager wifiManager = (WifiManager) context
				.getSystemService(Context.WIFI_SERVICE);
		long startTimestamp = System.currentTimeMillis();
		wifiManager.startScan();
		
		final long nap = 50;
		do {
			try {
				Thread.sleep(nap);
			} catch (InterruptedException ignore) {
			}
			List<ScanResult> scanResultList = wifiManager.getScanResults();
			if (scanResultList != null && !scanResultList.isEmpty()) {
				callback.onWifiScan(scanResultList);
			}
		} while (System.currentTimeMillis() - startTimestamp < interval);
	}
	
	/**
	 * start wifi Scan async once
	 * 
	 * @param context
	 *            the context
	 * @param callback
	 *            the callback
	 * @param interval
	 *            the interval in milliseconds
	 */
	public static void startWifiScanOnce(final Context context,
			final Callback callback, final long interval) {
		new Thread() {
			public void run() {
				startWifiScanOnceInternal(context, callback, interval);
			}
		}.start();
	}
	
	/**
	 * start Wifi Scan async
	 * 
	 * @param context
	 *            the context
	 * @param callback
	 *            the callback
	 * @param interval
	 *            the interval in milliseconds
	 */
	public static void startWifiScan(final Context context,
			final Callback callback, final long interval) {
		if (sIsStop) {
			sIsStop = false;
			sThread = new Thread() {
				public void run() {
					startWifiScanInternal(context, callback, interval);
				}
			};
			sThread.start();
		} else {
			Log.e(TAG, "startWifiScan() has started already");
		}
	}
	
	/**
	 * stop Wifi Scan
	 */
	public static void stopWifiScan() {
		sIsStop = true;
		if (sThread != null) {
			sThread.interrupt();
			sThread = null;
		} else {
			Log.e(TAG, "stopWifiScan()");
		}
	}
	
	/**
	 * get connected wifi ssid or null(if not connect)
	 * 
	 * @return connected wifi ssid or null(if not connect)
	 */
	public static String getConnectedWifiSsid(final Context context) {
		// check whether wifi is connected
		boolean isWifiConnected = false;
		ConnectivityManager connectivityManager = (ConnectivityManager) context
				.getSystemService(Context.CONNECTIVITY_SERVICE);
		NetworkInfo wiFiNetworkInfo = connectivityManager
				.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
		if (wiFiNetworkInfo != null) {
			isWifiConnected = wiFiNetworkInfo.isConnected();
		}

		if (!isWifiConnected) {
			return null;
		}
		// get wifi connected ssid
		WifiManager wifiManager = (WifiManager) context
				.getSystemService(Context.WIFI_SERVICE);
		WifiInfo wifiInfo = wifiManager.getConnectionInfo();

		String ssid = null;
		if (wifiInfo != null && wifiInfo.getSSID() != null) {
			int len = wifiInfo.getSSID().length();
			// mWifiInfo.getBSSID() = "\"" + ssid + "\"";
			if (wifiInfo.getSSID().startsWith("\"")
					&& wifiInfo.getSSID().endsWith("\"")) {
				ssid = wifiInfo.getSSID().substring(1, len - 1);
			} else {
				ssid = wifiInfo.getSSID();
			}

		}
		return ssid;
	}
	
	
	/**
	 * Open wifi brutally without user's mind
	 */
	public static void openWifiBrutally(Context context) {
		WifiManager wifiManager = (WifiManager)context.getSystemService(Context.WIFI_SERVICE);
        boolean isWifiEnabled = wifiManager.isWifiEnabled();
		if (!isWifiEnabled) {
			wifiManager.setWifiEnabled(true);
		}
	}
}
