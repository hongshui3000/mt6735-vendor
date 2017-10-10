package com.mediatek.op.telephony;

import android.provider.Settings;
import android.telephony.Rlog;

/**
 * TelephonyExt OP09 plugin.
 *
 */
public class TelephonyExtOP09 extends TelephonyExt {
    private static final String TAG = "TelephonyExtOP09";
    private static final int APN_AUTO_MODE = 0;
    private static final int APN_MANUAL_MODE = 1;

    @Override
    public boolean isManualApnMode() {
      boolean manualApnMode = (Settings.Global.getInt(mContext.getContentResolver(),
                  Settings.Global.CDMA_APN_MANUAL_MODE, APN_AUTO_MODE) == APN_MANUAL_MODE);
      Rlog.d(TAG, "manualApnMode = " + manualApnMode);
        return manualApnMode;
    }
}
