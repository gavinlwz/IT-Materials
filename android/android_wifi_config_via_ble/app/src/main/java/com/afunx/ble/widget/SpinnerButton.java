package com.afunx.ble.widget;

import com.afunx.ble.adapter.SsidSpinnerAdapter;
import com.afunx.ble.utils.WifiUtils;
import com.afunx.blewifidemo.R;

import android.app.AlertDialog;
import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class SpinnerButton extends Button implements OnItemClickListener {

	private SsidSpinnerAdapter mAdapter;
	private ListView mListView;
	private Button mBtnScan;
	private int mSelectedPosition = -1;
	private AlertDialog mDialog;
	private WifiUtils.Callback mCallback;

	public SpinnerButton(Context context, AttributeSet attrs) {
		super(context, attrs);
		this.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				onTapped();
			}
		});
	}

	private void onTapped() {
		
		final Context context = getContext();
		View view = View.inflate(context, R.layout.wifi_scan_spinner, null);
		
		mListView = (ListView) view.findViewById(R.id.lv_wifi_scan);
		mListView.setAdapter(mAdapter);
		mListView.setOnItemClickListener(this);
		mListView.setSelection(mSelectedPosition);
		
		mBtnScan = (Button) view.findViewById(R.id.btn_scan);
		mBtnScan.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				scanWifi();
			}
		});
		
		mDialog = new AlertDialog.Builder(context).setView(view).create();
		mDialog.show();
	}
	
	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position,
			long id) {
		// update position
		setSelection(position);
		// set text
		TextView tvSsid = (TextView) view.findViewById(R.id.tv_ssid);
		this.setText(tvSsid.getText());
		if (mDialog != null) {
			mDialog.dismiss();
			mDialog = null;
		}
	}

	private void scanWifi() {
		final long interval = 3500;
		WifiUtils.startWifiScanOnce(getContext().getApplicationContext(), mCallback, interval);
		Toast.makeText(getContext(), R.string.scaning, Toast.LENGTH_LONG).show();
	}
	
	public void setCallback(WifiUtils.Callback callback) {
		mCallback = callback;
	}
	
	public void setAdapter(SsidSpinnerAdapter adapter) {
		mAdapter = adapter;
	}

	public void setSelection(int position) {
		mSelectedPosition = position;
	}
	
	public SsidSpinnerAdapter getAdapter() {
		return mAdapter;
	}

	public int getSelectedItemPosition() {
		return mSelectedPosition;
	}

}
