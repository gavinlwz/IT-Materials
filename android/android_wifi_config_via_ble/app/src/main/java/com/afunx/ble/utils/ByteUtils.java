package com.afunx.ble.utils;

public class ByteUtils {
	/**
	 * get pretty format of byte array
	 * 
	 * @param array
	 *            byte array
	 * @return pretty format of byte array
	 */
	public static String prettyFormat(byte[] array) {
		StringBuilder sb = new StringBuilder();
		sb.append("[");
		for (int index = 0; index < array.length; index++) {
			sb.append("0x");
			int value = 0xff & array[index];
			String hex = Integer.toHexString(value);
			if (hex.length() == 1) {
				sb.append("0");
			}
			sb.append(hex);
			if (index != array.length - 1) {
				sb.append(" ");
			}
		}
		sb.append("]");
		return sb.toString();
	}
}
