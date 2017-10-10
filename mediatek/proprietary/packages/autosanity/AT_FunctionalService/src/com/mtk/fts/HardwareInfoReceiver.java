package com.mtk.fts;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.util.Log;

public class HardwareInfoReceiver extends BroadcastReceiver {
    static final String LOG_TAG_STRING = HardwareInfoReceiver.class.getName();
    static final String HDACTION_STRING = "com.mediatek.hardwareinfo";
    private static final String TAG = FTestService.TAG;

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.i(LOG_TAG_STRING, HDACTION_STRING);
        if (intent.getAction().equals(HDACTION_STRING)) {
            Log.i(LOG_TAG_STRING, "matched action: "+HDACTION_STRING);
            PackageManager pManager = context.getPackageManager();
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.append("has_camera = " + pManager.hasSystemFeature(PackageManager.FEATURE_CAMERA) + "\n");
            stringBuilder.append("has_front_camera = " + pManager.hasSystemFeature(PackageManager.FEATURE_CAMERA_FRONT)
                    + "\n");
            AudioManager audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
            stringBuilder.append("is_wired_headset_on = " + audioManager.isWiredHeadsetOn() + "\n");
            Log.i(LOG_TAG_STRING, stringBuilder.toString());
            Log.i(TAG, stringBuilder.toString());
        }
    }
}
