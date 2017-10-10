/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2012. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

package com.mediatek.mms.plugin;

import android.content.Context;
import android.content.res.Resources;
import android.os.Handler;
import android.os.Message;
import android.widget.Toast;

import com.mediatek.common.PluginImpl;
import com.mediatek.mms.ext.DefaultMmsFailedNotifyExt;
import com.mediatek.op09.plugin.R;
import com.mediatek.xlog.Xlog;

/**
 * M: OP09 Mms failed notify.
 */
@PluginImpl(interfaceName = "com.mediatek.mms.ext.IMmsFailedNotifyExt")
public class Op09MmsFailedNotifyExt extends DefaultMmsFailedNotifyExt {
    private static final String TAG = "Mms/Op09MmsFailedNotifyExt";
    private Context mContext = null;
    private boolean mIsShowTransientFailure;
    private Resources mResources = null;

    private Handler mToastHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            String str = null;

            switch(msg.what) {
            case REQUEST_RESPONSE_TEXT:
                str = (String) msg.obj;
                break;

            case DATA_OCCUPIED:
                str = mResources.getString(R.string.failed_data_occupied);
                break;

            case CONNECTION_FAILED:
                str = mResources.getString(R.string.failed_connection_failed);
                break;

            case GATEWAY_NO_RESPONSE:
                str = mResources.getString(R.string.failed_gateway_no_response);
                break;

            case HTTP_ABNORMAL:
                str = mResources.getString(R.string.failed_abnormal);
                break;

            case CANCEL_DOWNLOAD:
                str = mResources.getString(R.string.mms_cancel_download);
                break;

            case DISABLE_DELIVERY_REPORT:
                str = mResources.getString(R.string.disable_delivery_report);
                break;

            default:
                break;
            }

            if (str != null) {
                Toast.makeText(mContext, str, Toast.LENGTH_LONG).show();
            }
        }
    };

    /**
     * M: Constructor.
     * @param context the Context.
     */
    public Op09MmsFailedNotifyExt(Context context) {
        super(context);
        mContext = context;
        mIsShowTransientFailure = true;
        mResources = getResources();
    }

    /**
     * M: pop up toast.
     * @param reason the reason's id.
     * @param statusText the status text.
     */
    public void popupToast(int reason, String statusText) {
        Xlog.d(TAG, "MmsFailedNotifyExt.popupToast()");

        switch(reason) {
        case REQUEST_RESPONSE_TEXT:
            if (statusText == null) {
                Xlog.d(TAG, "popupToast() statusText == null!");
                return;
            }
            Xlog.d(TAG, "popupToast():REQUEST_RESPONSE_TEXT");
            Message msg = mToastHandler.obtainMessage(REQUEST_RESPONSE_TEXT);
            msg.obj = statusText;
            mToastHandler.sendMessage(msg);
            break;

        case DATA_OCCUPIED:
            Xlog.d(TAG, "popupToast():DATA_OCCUPIED");
            mToastHandler.sendEmptyMessage(DATA_OCCUPIED);
            break;

        case CONNECTION_FAILED:
            Xlog.d(TAG, "popupToast():DATA_CONNECT_FAILED");
            mToastHandler.sendEmptyMessage(CONNECTION_FAILED);
            break;

        case GATEWAY_NO_RESPONSE:
            Xlog.d(TAG, "popupToast():GATEWAY_NOT_RESPOND");
            mToastHandler.sendEmptyMessage(GATEWAY_NO_RESPONSE);
            break;

        case HTTP_ABNORMAL:
            Xlog.d(TAG, "popupToast():HTTP_ERROR");
            mToastHandler.sendEmptyMessage(HTTP_ABNORMAL);
            break;

        case CANCEL_DOWNLOAD:
            Xlog.d(TAG, "popupToast():CANCEL_DOWNLOAD");
            mToastHandler.sendEmptyMessage(CANCEL_DOWNLOAD);
            break;

        case DISABLE_DELIVERY_REPORT:
            Xlog.d(TAG, "popupToast():DISABLE_DELIVERY_REPORT");
            mToastHandler.sendEmptyMessage(DISABLE_DELIVERY_REPORT);
            break;

        default:
            Xlog.d(TAG, "popupToast():default");
            break;
        }
    }

    /**
     * M: get enable for failed notification.
     * @return true: notify failed notification's notice.
     */
    public boolean getFailedNotificationEnabled() {
        Xlog.d(TAG, "getFailedNotificationEnabled() sIsShowTransientFailed = "
            + mIsShowTransientFailure);
        return mIsShowTransientFailure;
    }

    /**
     * M: set failed notification switcher value.
     * @param isShow true:notify failed notification's notice.
     */
    public void setFailedNotificationEnabled(boolean isShow) {
        mIsShowTransientFailure = isShow;
        Xlog.d(TAG, "setShowFailedEnabled() sIsShowTransientFailed = " + mIsShowTransientFailure);
    }

}
