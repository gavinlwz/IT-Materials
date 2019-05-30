package com.afunx.blewifidemo;

import java.util.List;

import com.afunx.ble.adapter.SsidSpinnerAdapter;
import com.afunx.ble.utils.WifiUtils;
import com.afunx.ble.widget.SpinnerButton;

import android.app.Fragment;
import android.content.Context;
import android.net.wifi.ScanResult;
import android.os.Bundle;
import android.os.Handler;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

public class SsidSpinnerFragment extends Fragment {

	private SpinnerButton mSpinner2;
	private SsidSpinnerAdapter mAdapter;
	private final long mInterval = 500;
	private Handler mHandler;
	private WifiUtils.Callback mCallback = new WifiUtils.Callback() {
		@Override
		public void onWifiScan(final List<ScanResult> scanResultList) {
			mHandler.post(new Runnable() {
				public void run() {
					// check whether the onWifiScan() callback is called first time
					boolean firstTime = mSpinner2.getSelectedItemPosition() == -1;
					// keep selected ssid immutable when spinner items changed
					String selectedSsid = mAdapter.getSelectedSsid(mSpinner2
							.getSelectedItemPosition());
					mAdapter.addOrUpdateScanResultList(scanResultList);
					int selectedPosition = mAdapter
							.getSelectedPosition(selectedSsid);
					mSpinner2.setSelection(selectedPosition);
					// when first time, set selected item according to the wifi connected
					if (firstTime) {
						String ssidWifiConnected = WifiUtils
								.getConnectedWifiSsid(getActivity());
						if (!TextUtils.isEmpty(ssidWifiConnected)) {
							int wifiSsidPosition = mAdapter
									.getSelectedPosition(ssidWifiConnected);
							mSpinner2.setSelection(wifiSsidPosition);
							mSpinner2.setText(ssidWifiConnected);
						}
					}
				}
			});
		}
	};

	public String getSsid() {
		int position = mSpinner2.getSelectedItemPosition();
		return mAdapter.getSelectedSsid(position);
	}
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mHandler = new Handler();
	}
	
	@Override
	public void onResume() {
		super.onResume();
		Context context = getActivity().getApplicationContext();
		WifiUtils.startWifiScanOnce(context, mCallback, mInterval);
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		View view = inflater.inflate(R.layout.fragment_ssid_spinner, container);
		Context context = getActivity();
		mAdapter = new SsidSpinnerAdapter(context);
		mSpinner2 = (SpinnerButton) view.findViewById(R.id.spn_ssid);
		mSpinner2.setAdapter(mAdapter);
		mSpinner2.setCallback(mCallback);
		return view;
	}
}
