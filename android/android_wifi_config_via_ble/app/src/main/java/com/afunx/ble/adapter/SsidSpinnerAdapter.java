package com.afunx.ble.adapter;

import java.text.Collator;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;

import com.afunx.blewifidemo.R;

import android.content.Context;
import android.net.wifi.ScanResult;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

public class SsidSpinnerAdapter extends BaseAdapter {

	private final Context mContext;
	private final List<ScanResult> mScanResultList;
	private Comparator<ScanResult> mSsidComparator = new Comparator<ScanResult>() {
		@Override
		public int compare(ScanResult lScanResult, ScanResult rScanResult) {
			int result;

			String lSsid = lScanResult.SSID;
			String rSsid = rScanResult.SSID;
			if (lSsid.equals(rSsid)) {
				result = 0;
			} else {
				// order scanResult by its ssid
				List<String> lrSsidList = new ArrayList<String>();
				lrSsidList.add(lSsid);
				lrSsidList.add(rSsid);
				// for Chinese can't be compared by its ssid directly,
				// but Chinese can be sorted by its ssid directly
				Collections.sort(lrSsidList,
						Collator.getInstance(Locale.getDefault()));
				if (lrSsidList.get(0).equals(lSsid)) {
					result = -1;
				} else {
					result = 1;
				}
			}
			return result;
		}
	};

	public SsidSpinnerAdapter(Context context) {
		mContext = context;
		mScanResultList = new ArrayList<ScanResult>();
	}
	
	/**
	 * get scan result from mScanResultList or null(if not exist)
	 * 
	 * @param scanResult
	 *            scan result
	 * @return scan result from mScanResultList or null(if not exist)
	 */
	private ScanResult getScanResult(ScanResult scanResult) {
		for (ScanResult scanResultInList : mScanResultList) {
			if (scanResult.SSID.equals(scanResultInList.SSID)) {
				return scanResultInList;
			}
		}
		return null;
	}
	
	/**
	 * clear all ScanResult in mScanResultList
	 */
	public void clear() {
		mScanResultList.clear();
		notifyDataSetChanged();
	}
	
	/**
	 * get selected ssid by selected position
	 * 
	 * @param position
	 *            the selected position
	 * @return selected ssid by selected position
	 */
	public String getSelectedSsid(int position) {
		if (position == -1) {
			return null;
		}
		ScanResult scanResult = mScanResultList.get(position);
		return scanResult.SSID;
	}

	/**
	 * get selected position by selected ssid
	 * 
	 * @param ssid
	 *            the selected ssid
	 * @return selected position by selected ssid
	 */
	public int getSelectedPosition(String ssid) {
		if (ssid == null) {
			return -1;
		}
		for (int position = 0; position < mScanResultList.size(); position++) {
			ScanResult scanResult = mScanResultList.get(position);
			if (ssid.equals(scanResult.SSID)) {
				return position;
			}
		}
		return -1;
	}
	
	/**
	 * add ScanResult list
	 * 
	 * @param scanResult
	 *            the scanResult List to be added or updated
	 */
	public void addOrUpdateScanResultList(List<ScanResult> scanResultList) {
		// add or update
		for (ScanResult scanResult : scanResultList) {
			ScanResult scanResultInList = getScanResult(scanResult);
			if (scanResultInList != null) {
				// update
				scanResultInList.level = scanResult.level;
			} else {
				// add
				if (!TextUtils.isEmpty(scanResult.SSID)) {
					mScanResultList.add(scanResult);
				}
			}
		}
		// sort
		Collections.sort(mScanResultList, mSsidComparator);
		// notify data changed
		notifyDataSetChanged();
	}
	
	private static class ViewHolder {
		TextView tvSsid;
		TextView tvRssi;
	}

	@Override
	public int getCount() {
		return mScanResultList.size();
	}

	@Override
	public Object getItem(int position) {
		return mScanResultList.get(position);
	}

	@Override
	public long getItemId(int position) {
		return 0;
	}

	@Override
	public View getView(int position, View convertView, ViewGroup parent) {
		// get view holder
		ViewHolder viewHolder = null;
		if (convertView == null) {
			convertView = View.inflate(mContext, R.layout.item_ssid, null);
			viewHolder = new ViewHolder();
			viewHolder.tvSsid = (TextView) convertView
					.findViewById(R.id.tv_ssid);
			viewHolder.tvRssi = (TextView) convertView
					.findViewById(R.id.tv_rssi);
			convertView.setTag(viewHolder);
		} else {
			viewHolder = (ViewHolder) convertView.getTag();
		}
		// set view content
		ScanResult scanResult = mScanResultList.get(position);
		viewHolder.tvSsid.setText(scanResult.SSID);
		viewHolder.tvRssi.setText(String.valueOf(scanResult.level));
		return convertView;
	}
}
