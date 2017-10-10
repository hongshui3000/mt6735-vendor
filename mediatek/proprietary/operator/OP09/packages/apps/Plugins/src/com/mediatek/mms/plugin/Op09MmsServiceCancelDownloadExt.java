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

import android.content.ContentValues;
import android.content.Context;
import android.database.sqlite.SqliteWrapper;
import android.provider.Telephony.Mms;
import android.text.TextUtils;
import android.util.Log;

import com.mediatek.common.PluginImpl;
import com.mediatek.mms.service.ext.DefaultMmsServiceCancelDownloadExt;
import com.mediatek.xlog.Xlog;

import org.apache.http.client.HttpClient;
import org.apache.http.conn.ClientConnectionManager;

import java.util.HashMap;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;

/**
 * M: op09 mms service cancel download implemention.
 */
@PluginImpl(interfaceName = "com.mediatek.mms.service.ext.IMmsServiceCancelDownloadExt")
public class Op09MmsServiceCancelDownloadExt extends DefaultMmsServiceCancelDownloadExt {
    private static final String TAG = "Mms/Op09MmsServiceCancelDownloadExt";
    private static final String STATUS_EXT = "st_ext";

    private Context mContext;
    private HashMap<String, HttpClient> mClientMap;

    /**
     * M: Constructor.
     * @param context the Context.
     */
    public Op09MmsServiceCancelDownloadExt(Context context) {
        super(context);
        mContext = context;
        mClientMap = new HashMap<String, HttpClient>();
    }

    @Override
    public void cancelDownload(String uri) {
        HttpClient client = (HttpClient) mClientMap.get(uri);
        Xlog.d(TAG, "cancelDownload uri:" + uri + ", client = " + client);
        if (client != null) {
            Xlog.d(TAG, "[cancelDownload]ClientConnectionManager.close()");
            ClientConnectionManager ccm = client.getConnectionManager();
            ccm.closeExpiredConnections();
            ccm.shutdown();
        }
    }

    @Override
    public void addHttpClient(String url, HttpClient client) {
        Xlog.d(TAG, "addHttpClient: url:" + url + " \t client:" + client);
        mClientMap.put(url, client);
    }

    @Override
    public void removeHttpClient(String url) {
        Xlog.d(TAG, "removeHttpClient, url:" + url);
        mClientMap.remove(url);
    }

    @Override
    public boolean isCancelDownloadEnable() {
        Xlog.d(TAG, "[isCancelDownloadEnable] OP09 value: true");
        return true;
    }

    ConcurrentLinkedQueue<String> mAcquireNetWorkUri = new ConcurrentLinkedQueue<String>();
    ConcurrentHashMap<String, Boolean> mNeedBeCanceledUri =
        new ConcurrentHashMap<String, Boolean>();
    ConcurrentHashMap<Integer, String> mCachedLocationUri =
        new ConcurrentHashMap<Integer, String>();
    @Override
    public void addAcquireNetworkUri(String uri) {
        Log.d(TAG, "[addAcquireNetworkUri], uri:" + uri);
        if (TextUtils.isEmpty(uri)) {
            return;
        }
        if (mAcquireNetWorkUri.contains(uri)) {
            mAcquireNetWorkUri.remove(uri);
        }
        mAcquireNetWorkUri.offer(uri);
    }

    @Override
    public void removeAcquireNetWorkUri(String uri) {
        Log.d(TAG, "[removeAcquireNetWorkUri], uri:" + uri);
        if (TextUtils.isEmpty(uri)) {
            return;
        }
        if (mAcquireNetWorkUri.contains(uri)) {
            mAcquireNetWorkUri.remove(uri);
        }
    }

    @Override
    public boolean canBeCanceled(String uri) {
        Log.d(TAG, "[canBeCanceled], uri:" + uri);
        if (TextUtils.isEmpty(uri)) {
            Log.d(TAG, "[canBeCanceled], uri:" + uri + ", false");
            return false;
        }
        String firstCachedUri = mAcquireNetWorkUri.peek();
        if (!TextUtils.isEmpty(firstCachedUri) && firstCachedUri.equals(uri)) {
            Log.d(TAG, "[canBeCanceled], uri:" + uri + ", true");
            return true;
        }
        if (!mNeedBeCanceledUri.contains(uri)) {
            Log.d(TAG, "[canBeCanceled], uri:" + uri + ", not cached, push in the queue.");
            mNeedBeCanceledUri.put(uri, true);
        }
        Log.d(TAG, "[canBeCanceled], uri:" + uri + ", false");
        return false;
    }

    @Override
    public boolean needBeCanceled(String uri) {
        String str = null;
        if (TextUtils.isEmpty(uri)) {
            str = mAcquireNetWorkUri.peek();
        } else {
            str = uri;
        }
        if (!TextUtils.isEmpty(str) && mNeedBeCanceledUri.containsKey(str)) {
            Log.d(TAG, "[needBeCanceled], true");
            return true;
        }
        Log.d(TAG, "[needBeCanceled], false");
        return false;
    }

    @Override
    public void removeCanceledUri(String uri) {
        if (!TextUtils.isEmpty(uri)) {
            mNeedBeCanceledUri.remove(uri);
        }
    }

    @Override
    public void cacheLocationUri(Integer key, String locationUri) {
        Log.d(TAG, "[cacheLocationuri], key:" + key + "\t uri:" + locationUri);
        if (TextUtils.isEmpty(locationUri)) {
            return;
        }
        mCachedLocationUri.put(key, locationUri);
    }

    @Override
    public String getCachedLocationUri(Integer key) {
        return mCachedLocationUri.get(key);
    }

    @Override
    public void removeCachedLocationUri(Integer key) {
        mCachedLocationUri.remove(key);
    }
}
