/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.mediatek.connectivity;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.NetworkInfo;
import android.net.wifi.p2p.fastconnect.WifiP2pFastConnectInfo;
import android.net.wifi.p2p.WifiP2pConfig;
import android.net.wifi.p2p.WifiP2pInfo;
import android.net.wifi.p2p.WifiP2pDevice;
import android.net.wifi.p2p.WifiP2pDeviceList;
import android.net.wifi.p2p.WifiP2pGroup;
import android.net.wifi.p2p.WifiP2pGroupList;
import android.net.wifi.p2p.WifiP2pManager;
import android.net.wifi.WpsInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.OutputStream;
import java.io.PrintStream;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.Selector;
import java.nio.channels.SelectionKey;
import java.nio.channels.SocketChannel;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Set;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.regex.Matcher;
import java.util.regex.Pattern;


/**
 * Show the current status details of Wifi P2p related fields
 */
public class CdsWifiP2pInfoActivity extends Activity {

    private static final String CDS_WIFIP2P_VERSION = "0.8";
    private static final String TAG = "CDSINFO/WifiP2pInfo";

    // about wifi p2p
    private Context mContext;
    private WifiP2pManager mWifiP2pManager;
    private WifiP2pManager.Channel mWifiP2pChannel;
    private IntentFilter mWifiP2pFilter;
    private boolean mWifiP2pEnabled;
    private boolean mWifiP2pSearching;
    private WifiP2pDevice mThisDevice;
    private WifiP2pGroup mConnectedGroup;
    private WifiP2pDeviceList mPeers = new WifiP2pDeviceList();
    private WifiP2pDevice mConnectedDevice;
    private CdsWifiP2pLinkInfo mLinkInfoThread;
    private boolean mLinkInfoThreadRun;
    private CdsWifiP2pStabilityTest mStabilityTestThread;
    private boolean mStabilityTestRun;
    private String mStabilityTestPeerMAC;
    private int mStabilityTestConnectingCounter;
    private int mStabilityTestConnectedCounter;
    private boolean mIsScanSucceed;
    private long mNIC_TxStat;
    private long mNIC_RxStat;

    // about resource
    private TextView mTVP2pEMVersion;
    private TextView mTVP2pState;
    private TextView mTVDeviceName;
    private TextView mTVP2pMAC;
    private TextView mTVP2pDetailedInfo;
    private TextView mTVP2pNetworkState;
    private TextView mTVRole;
    private TextView mTVChannelConnected;
    private TextView mTVIPMyself;
    private TextView mTVPeerIPMAC;
    //private TextView mTVScanState;
    private TextView mTVScanResult;
    private TextView mTVPersistGroup;
    private TextView mTVLinkInfo;
    private CheckBox mCBLinkInfo;
    private CheckBox mCBListenMode;
    private CheckBox mCBAutoGO;
    private RadioGroup mRGWFDWPS;
    private RadioButton mRBWFDWPSDefault;
    private RadioButton mRBWFDWPSPBC;
    private RadioButton mRBWFDWPSKeypad;
    private RadioButton mRBWFDWPSDisplay;
    private RadioGroup mRGSettingWPS;
    private RadioButton mRBSettingWPSDefault;
    private RadioButton mRBSettingWPSPBC;
    private RadioButton mRBSettingWPSKeypad;
    private RadioButton mRBSettingWPSLabel;
    private RadioButton mRBSettingWPSDisplay;
    private RadioGroup mRGListenChannel;
    private RadioButton mRBListenAuto;
    private RadioButton mRBListenCH1;
    private RadioButton mRBListenCH6;
    private RadioButton mRBListenCH11;
    private RadioGroup mRGOperationChannel;
    private RadioButton mRBOperateAuto;
    private RadioButton mRBOperateCH1;
    private RadioButton mRBOperateCH6;
    private RadioButton mRBOperateCH11;
    private RadioButton mRBOperateCH36;
    private RadioGroup mRGFastConnStep0;
    private TextView mTVGOChoosen;
    private TextView mTVGCChoosen;
    private EditText mETGOWlanIP;
    private Button mBFastConnDo;
    private TextView mTVFastConnResult;
    private Button mBFastConnDisonnect;
    private EditText mETStabilityTestMac;
    private Button mBStabilityTestDo;
    private TextView mTVSTStatistics;
    private TextView mTVNICStatisticsValue;

    private static boolean mLinkInfo;
    private boolean mListen;
    private boolean mAutoGO;
    private int mWfdWpsConfig = WpsInfo.INVALID;
    private int mSettingWpsConfig = WpsInfo.INVALID;
    private int mListenChannel;
    private int mOperatingChannel;
    private int mFastRole;
    private InetAddress mGOWlanInetAddr;  // for GC use
    private static boolean mDoingFastConn;
    private String mHistory01 = ".";
    private String mHistory02 = "..";
    private String mHistory03 = "...";
    private String mHistory04 = "....";
    private String mHistory05 = ".....";
    private String mTotalInfo = "";

    // definition
    private static final int MSG_UPDATE_LINK_INFO_UI = 0x3301;
    private static final int MSG_DUMP_ERROR_UI = 0x3302;
    private static final int MSG_DUMP_CONNECTINFO_UI = 0x3303;
    private static final int MSG_SHOW_FC_DISCONNECT_BTN = 0x3304;
    private static final int MSG_SHOW_FC_RESULT_UI = 0x3305;
    private static final int MSG_MONITOR_GC_THREAD = 0x3306;
    private static final int MSG_UPDATE_PEER_GO_IP_UI = 0x3307;
    private static final int MSG_UPDATE_PEER_GC_IP_UI = 0x3308;
    private static final int MSG_UPDATE_STABILITY_TEST_UI = 0x3309;
    private static final int MSG_UPDATE_STABILITY_TEST_RESET = 0x330A;
    private static final int MSG_UPDATE_NIC_STATISTICS = 0x330B;

    private static final int LINK_INFO_SAMPLE_RATE = 2000;  //unit: ms
    private static final int FAST_ROLE_GO = 1;
    private static final int FAST_ROLE_GC = 2;
    private static final String STR_CONTROL_PLANE_TOKEN = "Wi-Fi Beam GC MAC=";
    private static final int CONTROL_PATH_PORT = 55688;
    private static final int DATA_PATH_PORT_BASE = 5566;
    private static final String DATA_PATH_PKT_PATTERN = "12345678";
    private static final int DATA_PATH_PKT_LEN = 1448;
    private static final int CHECK_P2P_CONN_DURATION = 333;
    private static final String FAST_CONN_GO_IP = "192.168.49.1";
    private static final String FAST_CONN_GC_IP = "192.168.49.2";
    private static final int FAST_CONN_SAMPLE_RATE = 5000;  //unit: ms
    private static final int WIFI_MAX_PING_TIMES = 15;
    private static final int FAST_CONN_GC_CTRL_RETRY_DURATION = 2000;
    private static final int FAST_CONN_SETUP_THRESHOLD = 20000;
    private static final int GET_PEER_IP_TIMEOUT = 60;    //unit: second
    private static final int GET_PEER_IP_SAMPLE_RATE = 1000;  //unit: ms
    private static final int STABILITY_TEST_PEER_FOUND_SAMPLE_RATE = 3000;  //unit: ms
    private static final int STABILITY_TEST_SCAN_MAX_TRY = 20;
    private static final int STABILITY_TEST_CONNECTED_INTERVAL = 20000;  //unit: ms
    private static final int STABILITY_TEST_DISCONNECTED_INTERVAL = 10000;  //unit: ms
    private static final String NET_DEV_FILE = "/proc/net/dev";
    private static final String NETWORK_INTERFACE = "p2p0";
    private static final int NIC_STATISTICS_SAMPLE_RATE = 5000;  //unit: ms

    // about fast connect, socket
    private GO_SocketServer mSocketServerThread;
    private boolean mSocketServerRun;
    private GC_SocketClient mSocketClientThread;
    private static int mDataPathPort = DATA_PATH_PORT_BASE;
    private static final int SOCKET_BUFFER_SIZE = 1460;


    //============================
    // Activity lifecycle
    //============================

    private final BroadcastReceiver mWifiP2pReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            /// Log.d(TAG, "receive action: " + action);

            if (WifiP2pManager.WIFI_P2P_STATE_CHANGED_ACTION.equals(action)) {
                mWifiP2pEnabled = intent.getIntExtra(WifiP2pManager.EXTRA_WIFI_STATE,
                                                     WifiP2pManager.WIFI_P2P_STATE_DISABLED) == WifiP2pManager.WIFI_P2P_STATE_ENABLED;
                handleP2pStateChanged();

            } else if (WifiP2pManager.WIFI_P2P_THIS_DEVICE_CHANGED_ACTION.equals(action)) {
                mThisDevice = (WifiP2pDevice) intent.getParcelableExtra(
                                  WifiP2pManager.EXTRA_WIFI_P2P_DEVICE);
                //Log.d(TAG, "Update device info: " + mThisDevice);
                handleThisDevChanged();

            } else if (WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION.equals(action)) {
                if (mWifiP2pManager == null) return;

                NetworkInfo networkInfo = (NetworkInfo) intent.getParcelableExtra(
                                              WifiP2pManager.EXTRA_NETWORK_INFO);
                WifiP2pInfo wifip2pinfo = (WifiP2pInfo) intent.getParcelableExtra(
                                              WifiP2pManager.EXTRA_WIFI_P2P_INFO);
                WifiP2pGroup wifiP2pGroup = (WifiP2pGroup) intent.getParcelableExtra(
                                                WifiP2pManager.EXTRA_WIFI_P2P_GROUP);
                handleP2pConnChanged(networkInfo, wifip2pinfo, wifiP2pGroup);

            } else if (WifiP2pManager.WIFI_P2P_DISCOVERY_CHANGED_ACTION.equals(action)) {
                int discoveryState = intent.getIntExtra(WifiP2pManager.EXTRA_DISCOVERY_STATE,
                                                        WifiP2pManager.WIFI_P2P_DISCOVERY_STOPPED);
                //Log.d(TAG, "Discovery state changed: " + discoveryState);
                handleDiscoveryChanged(discoveryState);

            } else if (WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION.equals(action)) {
                mPeers = (WifiP2pDeviceList) intent.getParcelableExtra(
                             WifiP2pManager.EXTRA_P2P_DEVICE_LIST);
                handlePeersChanged();

            } else if (WifiP2pManager.WIFI_P2P_PERSISTENT_GROUPS_CHANGED_ACTION.equals(action)) {
                handlePersistGroupChanged();

            }

        }

    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Log.d(TAG, "version: " + CDS_WIFIP2P_VERSION);

        setContentView(R.layout.wifi_p2p_status_test);
        mContext = this.getBaseContext();

        mWifiP2pManager = (WifiP2pManager) getSystemService(Context.WIFI_P2P_SERVICE);

        if (null == mWifiP2pManager) {
            Log.e(TAG, "WifiP2pManager is null");
            throw new RuntimeException("WifiP2pManager is null");
        }

        mWifiP2pChannel = mWifiP2pManager.initialize(mContext, mContext.getMainLooper(), null);

        mWifiP2pFilter = new IntentFilter();
        mWifiP2pFilter.addAction(WifiP2pManager.WIFI_P2P_STATE_CHANGED_ACTION);
        mWifiP2pFilter.addAction(WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION);
        mWifiP2pFilter.addAction(WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION);
        mWifiP2pFilter.addAction(WifiP2pManager.WIFI_P2P_THIS_DEVICE_CHANGED_ACTION);
        mWifiP2pFilter.addAction(WifiP2pManager.WIFI_P2P_DISCOVERY_CHANGED_ACTION);
        mWifiP2pFilter.addAction(WifiP2pManager.WIFI_P2P_PERSISTENT_GROUPS_CHANGED_ACTION);

        // UI
        // wifi p2p EM version
        mTVP2pEMVersion = (TextView) findViewById(R.id.wifi_p2p_EM_version_value);
        // wifi p2p state
        mTVP2pState = (TextView) findViewById(R.id.wifi_p2p_state_value);
        // device name
        mTVDeviceName = (TextView) findViewById(R.id.wifi_p2p_device_name_value);
        // wifi p2p mac
        mTVP2pMAC = (TextView) findViewById(R.id.wifi_p2p_mac_value);
        // detailed information
        mTVP2pDetailedInfo = (TextView) findViewById(R.id.wifi_p2p_detail_info_value);
        // wifi p2p network state
        mTVP2pNetworkState = (TextView) findViewById(R.id.wifi_p2p_network_state_value);
        // role
        mTVRole = (TextView) findViewById(R.id.wifi_p2p_role_value);
        // channel connected
        mTVChannelConnected = (TextView) findViewById(R.id.wifi_p2p_channel_connected_value);
        // IP myself
        mTVIPMyself = (TextView) findViewById(R.id.wifi_p2p_myself_IP_value);
        // MAC peer
        mTVPeerIPMAC = (TextView) findViewById(R.id.wifi_p2p_peer_ip_mac_value);
        // scan state
        //mTVScanState = (TextView) findViewById(R.id.wifi_p2p_scan_state_value);
        // scan result
        mTVScanResult = (TextView) findViewById(R.id.wifi_p2p_scan_result_value);
        // persistent group list
        mTVPersistGroup = (TextView) findViewById(R.id.wifi_p2p_persistent_group_value);
        // link info.
        mTVLinkInfo = (TextView) findViewById(R.id.wifi_p2p_link_info_value);
        mCBLinkInfo = (CheckBox) findViewById(R.id.wifi_p2p_link_info);

        // fast connect
        // step 0
        mRGFastConnStep0 = (RadioGroup) findViewById(R.id.wifi_p2p_FC_step0_group);
        mTVGOChoosen = (TextView) findViewById(R.id.wifi_p2p_FC_GO);
        mTVGCChoosen = (TextView) findViewById(R.id.wifi_p2p_FC_GC);
        // configuration gc side
        mETGOWlanIP = (EditText) findViewById(R.id.wifi_p2p_FC_GO_wlan_ip);
        mBFastConnDo = (Button) findViewById(R.id.wifi_p2p_FC_do);
        // step 1
        mTVFastConnResult = (TextView) findViewById(R.id.wifi_p2p_FC_result);
        // step 2
        mBFastConnDisonnect = (Button) findViewById(R.id.wifi_p2p_FC_disconnect);

        // listen mode
        mCBListenMode = (CheckBox) findViewById(R.id.wifi_p2p_listen_mode);
        // auto GO
        mCBAutoGO = (CheckBox) findViewById(R.id.wifi_p2p_autonomous_go);
        // WPS configuration for WFD
        mRGWFDWPS = (RadioGroup) findViewById(R.id.wifi_p2p_wfd_wps_radio_group);
        // WPS configuration for wifip2psetting
        mRGSettingWPS = (RadioGroup) findViewById(R.id.wifi_p2p_setting_wps_radio_group);
        // listen channel
        mRGListenChannel = (RadioGroup) findViewById(R.id.wifi_p2p_listen_channel_radio_group);
        mRBListenAuto = (RadioButton) findViewById(R.id.wifi_p2p_listen_channel_auto);
        // operation channel
        mRGOperationChannel = (RadioGroup) findViewById(R.id.wifi_p2p_operation_channel_radio_group);
        mRBOperateAuto = (RadioButton) findViewById(R.id.wifi_p2p_operation_channel_auto);

        // stability test
        mETStabilityTestMac = (EditText) findViewById(R.id.wifi_p2p_ST_mac);
        mBStabilityTestDo = (Button) findViewById(R.id.wifi_p2p_ST_do);
        mTVSTStatistics = (TextView) findViewById(R.id.wifi_p2p_ST_statistics);

        // stability thread
        mStabilityTestRun = false;

        // CdsWifiP2pLinkInfo thread
        mLinkInfoThreadRun = true;
        mLinkInfoThread = new CdsWifiP2pLinkInfo();
        mLinkInfoThread.setName("CdsWifiP2pLinkInfo");
        mLinkInfoThread.start();

        // NIC statistics
        mTVNICStatisticsValue = (TextView) findViewById(R.id.wifi_NIC_statistics_value);

    }

    @Override
    protected void onResume() {
        super.onResume();
        registerReceiver(mWifiP2pReceiver, mWifiP2pFilter);

        // wifi p2p EM version
        mTVP2pEMVersion.setText(CDS_WIFIP2P_VERSION);
        // wifi p2p state
        mTVP2pState.setText("N/A");
        // device name
        mTVDeviceName.setText("N/A");
        // wifi p2p mac
        mTVP2pMAC.setText("N/A");
        // detailed information
        mTVP2pDetailedInfo.setText("N/A");
        // wifi p2p network state
        mTVP2pNetworkState.setText("N/A");
        // role
        mTVRole.setText("N/A");
        // channel connected
        mTVChannelConnected.setText("N/A");
        // IP myself
        mTVIPMyself.setText("N/A");
        // MAC peer
        mTVPeerIPMAC.setText("N/A");
        // scan state
        //mTVScanState.setText("N/A");
        // scan result
        mTVScanResult.setText("N/A");
        // persistent group list
        mTVPersistGroup.setText("N/A");
        // link info.
        mTVLinkInfo.setText("N/A");
        mCBLinkInfo.setVisibility(View.VISIBLE);

        //restore last time UI
        Log.d(TAG, "mLinkInfo=" + mLinkInfo);
        mCBLinkInfo.setChecked(mLinkInfo);
        linkInfo(mLinkInfo);
        mCBLinkInfo.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Log.d(TAG, "mCBLinkInfo.onClick : " + mCBLinkInfo.isChecked());
                mLinkInfo = !mLinkInfo;
                linkInfo(mLinkInfo);
                mCBLinkInfo.setChecked(mLinkInfo);
            }
        });

        // fast connect
        // step 0
        mRGFastConnStep0.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            public void onCheckedChanged(RadioGroup group, final int checkedId) {
                switch (checkedId) {
                case R.id.wifi_p2p_FC_GO:
                    mFastRole = FAST_ROLE_GO;
                    mTVGOChoosen.setText(getString(R.string.wifi_p2p_FC_GO_choosen) + getInterfaceAddress("wlan0"));
                    mTVGCChoosen.setText(R.string.wifi_p2p_FC_GC);
                    break;
                case R.id.wifi_p2p_FC_GC:
                    mFastRole = FAST_ROLE_GC;
                    mTVGOChoosen.setText(R.string.wifi_p2p_FC_GO);
                    mTVGCChoosen.setText(R.string.wifi_p2p_FC_GC_choosen);
                    break;
                default:
                    return;
                }

                Log.d(TAG, "mRGFastConnStep0.onCheckedChanged : mFastRole=" + mFastRole);

                fastConnect_step0_GC_refresh();

                if (FAST_ROLE_GO == mFastRole) {
                    WiFiBeamPlus_GO();
                }

            }//switch
        });

        // configuration gc side
        mBFastConnDo.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Log.d(TAG, "mBFastConnDo.onClick");

                String go_ip = mETGOWlanIP.getText().toString();

                if (null != go_ip && false == mDoingFastConn) {
                    try {
                        if (!InetAddress.isNumeric(go_ip)) {
                            Log.e(TAG, "GO ip user input is wrong format, go_ip=" + go_ip);
                            return;
                        }

                        mGOWlanInetAddr = InetAddress.getByName(go_ip);

                    } catch (UnknownHostException e) {
                        Log.e(TAG, "GO ip user input got error, go_ip=" + go_ip);
                        return;

                    }

                    Log.d(TAG, "GO ip user input: " + mGOWlanInetAddr);
                    WiFiBeamPlus_GC();

                } //if

            }//onClick()
        });
        fastConnect_step0_GC_refresh();

        // step 1
        mTVFastConnResult.setText("N/A");

        // step 2
        mBFastConnDisonnect.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Log.d(TAG, "mBFastConnDisonnect.onClick");

                // disconnect fast connect
                if (mWifiP2pManager != null) {
                    mWifiP2pManager.removeGroup(mWifiP2pChannel, new WifiP2pManager.ActionListener() {
                        public void onSuccess() {
                            Log.d(TAG, " remove group success");
                        }
                        public void onFailure(int reason) {
                            Log.d(TAG, " remove group fail " + reason);
                        }
                    });
                }

                reset_all_fast_conn_setp_UI();

            }//onClick()
        });
        
        mBFastConnDisonnect.setVisibility(View.GONE);

        // listen mode
        mCBListenMode.setVisibility(View.VISIBLE);
        /*
        //restore last time UI
        Log.d(TAG, "mListen=" + mListen);
        mCBListenMode.setChecked(mListen);
        setListenMode(mListen);
        */
        mCBListenMode.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Log.d(TAG, "mCbListenMode.onClick : " + mCBListenMode.isChecked());
                mListen = !mListen;
                setListenMode(mListen);
                mCBListenMode.setChecked(mListen);
            }
        });

        // auto GO
        mCBAutoGO.setVisibility(View.VISIBLE);
        /*
        //restore last time UI
        Log.d(TAG, "mAutoGO=" + mAutoGO);
        mCBAutoGO.setChecked(mAutoGO);
        if (mAutoGO) {
            startAutoGO();
        } else {
            stopAutoGO();
        }
        */
        mCBAutoGO.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Log.d(TAG, "mCbAutoGO.onClick : " + mCBAutoGO.isChecked());
                mAutoGO = !mAutoGO;

                if (mAutoGO) {
                    startAutoGO();
                } else {
                    stopAutoGO();
                }

                mCBAutoGO.setChecked(mAutoGO);
            }
        });

        // WPS configuration for WFD
        mRGWFDWPS.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            public void onCheckedChanged(RadioGroup group, final int checkedId) {
                switch (checkedId) {
                case R.id.wifi_p2p_wfd_wps_default:
                    mWfdWpsConfig = WpsInfo.INVALID;
                    break;
                case R.id.wifi_p2p_wfd_wps_pbc:
                    mWfdWpsConfig = WpsInfo.PBC;
                    break;
                case R.id.wifi_p2p_wfd_wps_display:
                    mWfdWpsConfig = WpsInfo.DISPLAY;
                    break;
                case R.id.wifi_p2p_wfd_wps_keypad:
                    mWfdWpsConfig = WpsInfo.KEYPAD;
                    break;
                default:
                    mWfdWpsConfig = WpsInfo.INVALID;
                    break;
                }

                Log.d(TAG, "mRGWFDWPS.onCheckedChanged : mWfdWpsConfig=" + mWfdWpsConfig);
                Settings.Global.putInt(mContext.getContentResolver(),
                                       Settings.Global.WIFI_DISPLAY_WPS_CONFIG, mWfdWpsConfig);
            }
        });
        mWfdWpsConfig = Settings.Global.getInt(mContext.getContentResolver(),
                                               Settings.Global.WIFI_DISPLAY_WPS_CONFIG, WpsInfo.INVALID);
        //restore last time setting
        Log.d(TAG, "mWfdWpsConfig=" + mWfdWpsConfig);

        switch (mWfdWpsConfig) {
        case WpsInfo.INVALID:
            mRBWFDWPSDefault = (RadioButton) findViewById(R.id.wifi_p2p_wfd_wps_default);
            mRBWFDWPSDefault.setChecked(true);
            break;
        case WpsInfo.PBC:
            mRBWFDWPSPBC = (RadioButton) findViewById(R.id.wifi_p2p_wfd_wps_pbc);
            mRBWFDWPSPBC.setChecked(true);
            break;
        case WpsInfo.DISPLAY:
            mRBWFDWPSDisplay = (RadioButton) findViewById(R.id.wifi_p2p_wfd_wps_display);
            mRBWFDWPSDisplay.setChecked(true);
            break;
        case WpsInfo.KEYPAD:
            mRBWFDWPSKeypad = (RadioButton) findViewById(R.id.wifi_p2p_wfd_wps_keypad);
            mRBWFDWPSKeypad.setChecked(true);
            break;
        default:
            mWfdWpsConfig = WpsInfo.INVALID;
            mRBWFDWPSDefault = (RadioButton) findViewById(R.id.wifi_p2p_wfd_wps_default);
            mRBWFDWPSDefault.setChecked(true);
        }

        // WPS configuration for wifip2psetting
        mRGSettingWPS.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            public void onCheckedChanged(RadioGroup group, final int checkedId) {
                switch (checkedId) {
                case R.id.wifi_p2p_setting_wps_pbc:
                    mSettingWpsConfig = WpsInfo.PBC;
                    break;
                case R.id.wifi_p2p_setting_wps_display:
                    mSettingWpsConfig = WpsInfo.DISPLAY;
                    break;
                case R.id.wifi_p2p_setting_wps_keypad:
                    mSettingWpsConfig = WpsInfo.KEYPAD;
                    break;
                case R.id.wifi_p2p_setting_wps_label:
                    mSettingWpsConfig = WpsInfo.LABEL;
                    break;
                case R.id.wifi_p2p_wfd_wps_default:
                default:
                    mSettingWpsConfig = WpsInfo.INVALID;
                    break;
                }

                Log.d(TAG, "mRGSettingWPS.onCheckedChanged : mSettingWpsConfig=" + mSettingWpsConfig);
                SystemProperties.set("wifidirect.wps", Integer.toString(mSettingWpsConfig));
                //AOSP specific
                String toastStr = "[AOSP] please use--> adb shell setprop wifidirect.wps " + Integer.toString(mSettingWpsConfig);
                Toast.makeText(mContext, toastStr, Toast.LENGTH_SHORT).show();
            }
        });
        mSettingWpsConfig = SystemProperties.getInt("wifidirect.wps", -1);
        //restore last time setting
        Log.d(TAG, "mSettingWpsConfig=" + mSettingWpsConfig);

        switch (mSettingWpsConfig) {
        case WpsInfo.PBC:
            mRBSettingWPSPBC = (RadioButton) findViewById(R.id.wifi_p2p_setting_wps_pbc);
            mRBSettingWPSPBC.setChecked(true);
            break;
        case WpsInfo.DISPLAY:
            mRBSettingWPSDisplay = (RadioButton) findViewById(R.id.wifi_p2p_setting_wps_display);
            mRBSettingWPSDisplay.setChecked(true);
            break;
        case WpsInfo.KEYPAD:
            mRBSettingWPSKeypad = (RadioButton) findViewById(R.id.wifi_p2p_setting_wps_keypad);
            mRBSettingWPSKeypad.setChecked(true);
            break;
        case WpsInfo.LABEL:
            mRBSettingWPSLabel = (RadioButton) findViewById(R.id.wifi_p2p_setting_wps_label);
            mRBSettingWPSLabel.setChecked(true);
            break;
        case WpsInfo.INVALID:
        default:
            mSettingWpsConfig = WpsInfo.INVALID;
            mRBSettingWPSDefault = (RadioButton) findViewById(R.id.wifi_p2p_setting_wps_default);
            mRBSettingWPSDefault.setChecked(true);
            break;
        }

        // listen channel
        mRGListenChannel.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            public void onCheckedChanged(RadioGroup group, final int checkedId) {
                switch (checkedId) {
                case R.id.wifi_p2p_listen_channel_auto:
                    mListenChannel = 0;
                    break;
                case R.id.wifi_p2p_listen_channel_1:
                    mListenChannel = 1;
                    break;
                case R.id.wifi_p2p_listen_channel_6:
                    mListenChannel = 6;
                    break;
                case R.id.wifi_p2p_listen_channel_11:
                    mListenChannel = 11;
                    break;
                default:
                    mListenChannel = 0;
                    break;
                }

                Log.d(TAG, "mRGListenChannel.onCheckedChanged : mListenChannel=" + mListenChannel);
                setWifiP2pChannels(mListenChannel, mOperatingChannel);
            }
        });
        /*
        //restore last time UI
        Log.d(TAG, "mListenChannel=" + mListenChannel);
        switch (mListenChannel) {
            case 0:
                mRBListenAuto = (RadioButton) findViewById(R.id.wifi_p2p_listen_channel_auto);
                mRBListenAuto.setChecked(true);
                break;
            case 1:
                mRBListenCH1 = (RadioButton) findViewById(R.id.wifi_p2p_listen_channel_1);
                mRBListenCH1.setChecked(true);
                break;
            case 6:
                mRBListenCH6 = (RadioButton) findViewById(R.id.wifi_p2p_listen_channel_6);
                mRBListenCH6.setChecked(true);
                break;
            case 11:
                mRBListenCH11 = (RadioButton) findViewById(R.id.wifi_p2p_listen_channel_11);
                mRBListenCH11.setChecked(true);
                break;
            default:
                mListenChannel = 0;
                mRBListenAuto = (RadioButton) findViewById(R.id.wifi_p2p_listen_channel_auto);
                mRBListenAuto.setChecked(true);
        }
        */

        // operation channel
        mRGOperationChannel.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            public void onCheckedChanged(RadioGroup group, final int checkedId) {
                switch (checkedId) {
                case R.id.wifi_p2p_operation_channel_auto:
                    mOperatingChannel = 0;
                    break;
                case R.id.wifi_p2p_operation_channel_1:
                    mOperatingChannel = 1;
                    break;
                case R.id.wifi_p2p_operation_channel_6:
                    mOperatingChannel = 6;
                    break;
                case R.id.wifi_p2p_operation_channel_11:
                    mOperatingChannel = 11;
                    break;
                case R.id.wifi_p2p_operation_channel_36:
                    mOperatingChannel = 36;
                    break;
                default:
                    mOperatingChannel = 0;
                    break;
                }

                Log.d(TAG, "mRGOperationChannel.onCheckedChanged : mOperatingChannel=" + mOperatingChannel);
                setWifiP2pChannels(mListenChannel, mOperatingChannel);
            }
        });
        /*
        //restore last time UI
        Log.d(TAG, "mOperatingChannel=" + mOperatingChannel);
        switch (mOperatingChannel) {
            case 0:
                mRBOperateAuto = (RadioButton) findViewById(R.id.wifi_p2p_operation_channel_auto);
                mRBOperateAuto.setChecked(true);
                break;
            case 1:
                mRBOperateCH1 = (RadioButton) findViewById(R.id.wifi_p2p_operation_channel_1);
                mRBOperateCH1.setChecked(true);
                break;
            case 6:
                mRBOperateCH6 = (RadioButton) findViewById(R.id.wifi_p2p_operation_channel_6);
                mRBOperateCH6.setChecked(true);
                break;
            case 11:
                mRBOperateCH11 = (RadioButton) findViewById(R.id.wifi_p2p_operation_channel_11);
                mRBOperateCH11.setChecked(true);
                break;
            case 36:
                mRBOperateCH36 = (RadioButton) findViewById(R.id.wifi_p2p_operation_channel_36);
                mRBOperateCH36.setChecked(true);
                break;
            default:
                mOperatingChannel = 0;
                mRBOperateAuto = (RadioButton) findViewById(R.id.wifi_p2p_operation_channel_auto);
                mRBOperateAuto.setChecked(true);
        }
        */
        /*
        //restore last time UI functinoality
        setWifiP2pChannels(mListenChannel, mOperatingChannel);
        */

        // stability test button
        mBStabilityTestDo.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mStabilityTestPeerMAC = mETStabilityTestMac.getText().toString();
                Log.d(TAG, "mBStabilityTestDo.onClick, StabilityTestPeerMAC=" + mStabilityTestPeerMAC);

                if (null == mStabilityTestPeerMAC && false == mDoingFastConn) {
                    Log.d(TAG, "peer_mac is null, invalid");
                    return;
                }

                mStabilityTestRun = !mStabilityTestRun;

                try {
                    stabilityTest_button_refresh();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

            }//onClick()
        });

        try {
            stabilityTest_button_refresh();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        // NIC statistic
        mTVNICStatisticsValue.setText("N/A");
        mNIC_TxStat = mNIC_RxStat = 0;
        // trigger it auto update
        Message msg = mHandler.obtainMessage(MSG_UPDATE_NIC_STATISTICS);
        mHandler.sendMessage(msg);

    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(mWifiP2pReceiver);

    }

    @Override
    public void onDestroy() {
        mWifiP2pManager.deinitialize(mWifiP2pChannel);

        super.onDestroy();
        // CdsWifiP2pLinkInfo thread
        //mLinkInfoThread.stop();
        mLinkInfoThreadRun = false;
        // GO_SocketServer thread
        mSocketServerRun = false;

    }


    //============================
    // Local function
    //============================

    private void setListenMode(final boolean enable) {
        Log.d(TAG, "Setting listen mode to: " + enable);

        mWifiP2pManager.listen(mWifiP2pChannel, enable, new WifiP2pManager.ActionListener() {
            @Override
            public void onSuccess() {
                Log.d(TAG, "Successfully " + (enable ? "entered" : "exited")
                      + " listen mode.");
            }

            @Override
            public void onFailure(int reason) {
                Log.e(TAG, "Failed to " + (enable ? "entered" : "exited")
                      + " listen mode with reason " + reason + ".");
            }
        });
    }

    private void startAutoGO() {
        Log.d(TAG, "Starting Autonomous GO...");

        mWifiP2pManager.createGroup(mWifiP2pChannel, new WifiP2pManager.ActionListener() {
            @Override
            public void onSuccess() {
                Log.d(TAG, "Successfully started AutoGO.");
            }

            @Override
            public void onFailure(int reason) {
                Log.e(TAG, "Failed to start AutoGO with reason " + reason + ".");
            }
        });
    }

    private void stopAutoGO() {
        Log.d(TAG, "Stopping Autonomous GO...");

        mWifiP2pManager.removeGroup(mWifiP2pChannel, new WifiP2pManager.ActionListener() {
            @Override
            public void onSuccess() {
                Log.d(TAG, "Successfully stopped AutoGO.");
            }

            @Override
            public void onFailure(int reason) {
                Log.e(TAG, "Failed to stop AutoGO with reason " + reason + ".");
            }
        });
    }

    private void setWifiP2pChannels(final int lc, final int oc) {
        Log.d(TAG, "Setting wifi p2p channel: lc=" + lc + ", oc=" + oc);

        mWifiP2pManager.setWifiP2pChannels(mWifiP2pChannel,
        lc, oc, new WifiP2pManager.ActionListener() {
            @Override
            public void onSuccess() {
                Log.d(TAG, "Successfully set wifi p2p channels.");
            }

            @Override
            public void onFailure(int reason) {
                Log.e(TAG, "Failed to set wifi p2p channels with reason " + reason + ".");
            }
        });
    }

    private void handleP2pStateChanged() {
        if (true == mWifiP2pEnabled) {
            mTVP2pState.setText("Enabled");

        } else {
            mTVP2pState.setText("Disabled");
            mTVP2pDetailedInfo.setText("N/A");

        }
    }

    private void handleThisDevChanged() {
        if (null != mThisDevice && true == mWifiP2pEnabled) {
            mTVDeviceName.setText(mThisDevice.deviceName);
            mTVP2pMAC.setText(mThisDevice.deviceAddress);
            mTVP2pDetailedInfo.setText(mThisDevice.toString());

        } else {
            mTVDeviceName.setText("N/A");
            mTVP2pMAC.setText("N/A");
            mTVP2pDetailedInfo.setText("N/A");

        }
    }

    private void startSearch() {
        if (mWifiP2pManager != null && !mWifiP2pSearching) {
            mIsScanSucceed = false;
            mWifiP2pManager.discoverPeers(mWifiP2pChannel, new WifiP2pManager.ActionListener() {
                public void onSuccess() {
                    mIsScanSucceed = true;
                }
                public void onFailure(int reason) {
                    Log.e(TAG, " discover fail " + reason);
                    mIsScanSucceed = false;
                }
            });
        }
    }

    private void handleP2pConnChanged(NetworkInfo networkInfo, WifiP2pInfo wifip2pinfo, WifiP2pGroup wifiP2pGroup) {
        if (null != mWifiP2pManager && networkInfo.isConnected()) {
            mWifiP2pManager.requestGroupInfo(mWifiP2pChannel, new WifiP2pManager.GroupInfoListener() {
                public void onGroupInfoAvailable(WifiP2pGroup group) {
                    //Log.d(TAG, " group " + group);
                    mConnectedGroup = group;

                    // myself is GO
                    if (mConnectedGroup.getOwner().deviceAddress.equals(mThisDevice.deviceAddress)) {
                        String GCDevNameList = "Connected with: ";
                        String GCMacList = "";

                        for (WifiP2pDevice c : mConnectedGroup.getClientList()) {
                            GCDevNameList = GCDevNameList + c.deviceName + " ";
                            GCMacList = GCMacList + c.deviceAddress + "\n";
                        }

                        mTVP2pNetworkState.setText(GCDevNameList);
                        mTVPeerIPMAC.setText(GCMacList);
                        mTVRole.setText("Group Owner, psk: " + mConnectedGroup.getPassphrase());

                        //get peer IP
                        new Thread(new Runnable() {
                            public void run() {
                                int count = 0;
                                String peerGCsIP = null;
                                String totalResult = "";
                                boolean allDone = false;

                                for (WifiP2pDevice c : mConnectedGroup.getClientList()) {
                                    while (GET_PEER_IP_TIMEOUT >= count) {
                                        count++;
                                        SystemClock.sleep(GET_PEER_IP_SAMPLE_RATE);

                                        if (null == mConnectedGroup) {
                                            Log.e(TAG, "peer is GC(s), mConnectedGroup is null");
                                            return;
                                        }

                                        allDone = false;
                                        peerGCsIP = mWifiP2pManager.getPeerIpAddress(c.deviceAddress);

                                        // invalid case
                                        if (null == peerGCsIP) {
                                            Log.d(TAG, "peer is GC(s), getPeerIP, the " + count + " times");
                                            continue;
                                        }

                                        // IP duplicated case
                                        boolean duplicatedIP = false;

                                        for (WifiP2pDevice d : mConnectedGroup.getClientList()) {
                                            if (!d.deviceAddress.equals(c.deviceAddress) &&
                                                    null != d.deviceIP &&
                                            d.deviceIP.equals(peerGCsIP)) {
                                                Log.e(TAG, "peer is GC(s), getPeerIP, meet IP duplicated case");
                                                duplicatedIP = true;
                                                break;
                                            }
                                        }

                                        if (true == duplicatedIP) {
                                            continue;
                                        }

                                        // all checked pass
                                        c.deviceIP = peerGCsIP;
                                        totalResult = totalResult + c.deviceIP + " / " + c.deviceAddress + "\n";
                                        allDone = true;
                                        break;

                                    } //while

                                } //for

                                if (null != totalResult && true == allDone) {
                                    Message msg = mHandler.obtainMessage(MSG_UPDATE_PEER_GC_IP_UI, totalResult.toString());
                                    mHandler.sendMessage(msg);
                                    return;
                                }

                                Log.e(TAG, "peer is GC(s), getPeerIP timeout!");

                            }
                        }, "CdsWifiP2pGOGetPeerIP").start();

                        // myself is GC
                    } else {
                        String GODevNameList = "Connected with: ";
                        GODevNameList = GODevNameList + mConnectedGroup.getOwner().deviceName;
                        mTVP2pNetworkState.setText(GODevNameList);
                        mTVPeerIPMAC.setText(mConnectedGroup.getOwner().deviceAddress);
                        mTVRole.setText("Group Client");

                        //get peer IP
                        new Thread(new Runnable() {
                            public void run() {
                                int count = 0;
                                String peerGOIP = null;

                                while (GET_PEER_IP_TIMEOUT > count) {
                                    peerGOIP = mWifiP2pManager.getPeerIpAddress(mConnectedGroup.getOwner().deviceAddress);

                                    if (null != peerGOIP) {
                                        Message msg = mHandler.obtainMessage(MSG_UPDATE_PEER_GO_IP_UI, peerGOIP.toString());
                                        mHandler.sendMessage(msg);
                                        return;
                                    }

                                    count++;
                                    SystemClock.sleep(GET_PEER_IP_SAMPLE_RATE);

                                    if (null == mConnectedGroup) {
                                        Log.e(TAG, "peer is GO, mConnectedGroup is null");
                                        return;
                                    }

                                    Log.d(TAG, "peer is GO, getPeerIP, the " + count + " times");
                                } //while

                                Log.e(TAG, "peer is GO, getPeerIP timeout!");

                            }
                        }, "CdsWifiP2pGCGetPeerIP").start();
                    }

                    Inet4Address addr = getInterfaceAddress(mConnectedGroup.getInterface());

                    if (null != addr) {
                        mTVIPMyself.setText(addr.getHostAddress());
                    }

                }
            });
        }

        if (networkInfo.isConnected()) {
            mStabilityTestConnectedCounter++;

            if (true == mStabilityTestRun) {
                Log.d(TAG, "StabilityTest: Connected, StabilityTestConnectedCounter=" + mStabilityTestConnectedCounter);
            } else {
                Log.d(TAG, "Connected");
            }

            //mTVP2pNetworkState.setText("Connected with");
            mTVChannelConnected.setText("" + wifiP2pGroup.getFrequency());

        } else {
            mTVP2pNetworkState.setText("DisConnected");
            mTVChannelConnected.setText("N/A");
            mConnectedGroup = null;
            mConnectedDevice = null;
            mTVRole.setText("N/A");
            mTVIPMyself.setText("N/A");
            mTVPeerIPMAC.setText("N/A");

            reset_all_fast_conn_setp_UI();

            //start a search when we are disconnected
            //but not on group removed broadcast event
            startSearch();

        }

    }

    private void handleDiscoveryChanged(int discoveryState) {
        if (discoveryState == WifiP2pManager.WIFI_P2P_DISCOVERY_STARTED) {
            //mTVScanState.setText("Started");
            mWifiP2pSearching = true;

        } else {
            //mTVScanState.setText("Stoppedd");
            mWifiP2pSearching = false;

        }

    }

    private void handlePeersChanged() {
        String PeerList = "";

        //Log.d(TAG, "List of available peers");
        for (WifiP2pDevice peer: mPeers.getDeviceList()) {
            //Log.d(TAG, "-> " + peer);

            String status = "";

            switch (peer.status) {
            case WifiP2pDevice.CONNECTED:
                status = "CONNECTED";
                mConnectedDevice = peer;
                break;
            case WifiP2pDevice.INVITED:
                status = "INVITED";
                break;
            case WifiP2pDevice.FAILED:
                status = "FAILED";
                break;
            case WifiP2pDevice.AVAILABLE:
                status = "AVAILABLE";
                break;
            case WifiP2pDevice.UNAVAILABLE:
                status = "UNAVAILABLE";
                break;
            }

            PeerList = PeerList +
                       peer.deviceName + "    " +
                       peer.deviceAddress + "    " +
                       status + "\n!==========!==========!==========!==========!\n";
        }

        mTVScanResult.setText(PeerList);

    }

    private void handlePersistGroupChanged() {
        if (mWifiP2pManager != null) {
            mWifiP2pManager.requestPersistentGroupInfo(mWifiP2pChannel, new WifiP2pManager.PersistentGroupInfoListener() {
                public void onPersistentGroupInfoAvailable(WifiP2pGroupList groups) {

                    String PersistGroupList = "";

                    for (WifiP2pGroup group: groups.getGroupList()) {
                        //Log.d(TAG, " group " + group);
                        PersistGroupList = PersistGroupList +
                                           group.getNetworkId() + "    " +
                                           group.getNetworkName() + "\n";
                    }

                    mTVPersistGroup.setText(PersistGroupList);
                }

            });

        }

    }

    private static Inet4Address getInterfaceAddress(String networkInterface) {
        NetworkInterface iface;

        try {
            iface = NetworkInterface.getByName(networkInterface);

            if (null == iface) {
                Log.e(TAG, "Could not obtain network interface on : "
                      + networkInterface);
                return null;
            }
        } catch (SocketException ex) {
            Log.e(TAG, "Could not obtain address of network interface "
                  + networkInterface, ex);
            return null;
        }

        Enumeration<InetAddress> addrs = iface.getInetAddresses();

        while (addrs.hasMoreElements()) {
            InetAddress addr = addrs.nextElement();

            if (addr instanceof Inet4Address) {
                return (Inet4Address) addr;
            }
        }

        Log.d(TAG, "Could not obtain address of network interface "
              + networkInterface + " because it had no IPv4 addresses.");
        return null;
    }

    private void linkInfo(final boolean enable) {
        // note: need refer to CdsWifiP2pLinkInfo thread!

        if (true == enable && null == mConnectedDevice) {
            mTVLinkInfo.setText("N/A");

        } else if (false == enable) {
            mTVLinkInfo.setText("N/A");

        }

    }

    public class CdsWifiP2pLinkInfo extends Thread {

        public void run() {
            //Log.d(TAG, "CdsWifiP2pLinkInfo(): start thread");

            while (true == mLinkInfoThreadRun) {

                if (true == mLinkInfo && null != mConnectedDevice && null != mConnectedGroup) {
                    // GO case
                    if (true == mConnectedGroup.isGroupOwner()) {
                        for (WifiP2pDevice c : mConnectedGroup.getClientList()) {
                            /*
                            mWifiP2pManager.requestWifiP2pLinkInfo(mWifiP2pChannel, c.deviceAddress, new WifiP2pManager.WifiP2pLinkInfoListener() {
                                @Override
                                public void onLinkInfoAvailable(WifiP2pLinkInfo status) {
                                    if (null != status && null != status.linkInfo) {
                                        mTotalInfo = mTotalInfo +
                                                     status.toString() + "\n";
                                    } else {
                                        Log.e(TAG, "onLinkInfoAvailable() parameter is null on GO case");

                                    }
                                }//onLinkInfoAvailable()
                            });
                            */
                            mTotalInfo = "disable this utility due to LEGO API check\n";
                        } // for

                        if (null != mTotalInfo) {
                            Message msg = mHandler.obtainMessage(MSG_UPDATE_LINK_INFO_UI, mTotalInfo.toString());
                            mHandler.sendMessage(msg);
                            mTotalInfo = "";
                        }

                        // GC case
                    } else {
                        /*
                        mWifiP2pManager.requestWifiP2pLinkInfo(mWifiP2pChannel, mConnectedDevice.deviceAddress, new WifiP2pManager.WifiP2pLinkInfoListener() {
                            @Override
                            public void onLinkInfoAvailable(WifiP2pLinkInfo status) {
                                if (null != status && null != status.linkInfo) {
                                    Message msg = mHandler.obtainMessage(MSG_UPDATE_LINK_INFO_UI, status.toString());
                                    mHandler.sendMessage(msg);

                                } else {
                                    Log.e(TAG, "onLinkInfoAvailable() parameter is null on GC case");

                                }
                            }//onLinkInfoAvailable()
                        });
                        */
                        mTotalInfo = "disable this utility due to LEGO API check\n";
                    }

                } else {
                    Message msg = mHandler.obtainMessage(MSG_UPDATE_LINK_INFO_UI, "N/A");
                    mHandler.sendMessage(msg);

                }

                SystemClock.sleep(LINK_INFO_SAMPLE_RATE);

            } //while

            //Log.d(TAG, "CdsWifiP2pLinkInfo(): leave thread");
        }// run

    }

    private final Handler mHandler = new Handler() {

        public void handleMessage(Message msg) {
            SimpleDateFormat formatter = new SimpleDateFormat("dd MMM yyyy  hh:mm:ss a");
            String now = null;

            switch (msg.what) {
            case MSG_UPDATE_LINK_INFO_UI:
                now = formatter.format(new Date());
                mTVLinkInfo.setText("[ " + now + " ]\n\n" + (String) msg.obj);
                break;
            case MSG_DUMP_ERROR_UI:
                mTVFastConnResult.setText((String) msg.obj);
                break;
            case MSG_DUMP_CONNECTINFO_UI:
                mHistory01 = mHistory02;
                mHistory02 = mHistory03;
                mHistory03 = mHistory04;
                mHistory04 = mHistory05;
                mHistory05 = msg.obj.toString();
                String strShow =
                    mHistory01 + "\n!==========!==========!==========!==========!\n" +
                    mHistory02 + "\n!==========!==========!==========!==========!\n" +
                    mHistory03 + "\n!==========!==========!==========!==========!\n" +
                    mHistory04 + "\n!==========!==========!==========!==========!\n" +
                    mHistory05;
                mTVFastConnResult.setText(strShow);
                // to fix screen off->on, mBFastConnDisonnect button is gone
                //break;
            case MSG_SHOW_FC_DISCONNECT_BTN:
                mBFastConnDisonnect.setVisibility(View.VISIBLE);
                mTVFastConnResult.setVisibility(View.VISIBLE);
                break;
            case MSG_SHOW_FC_RESULT_UI:
                mTVFastConnResult.setVisibility(View.VISIBLE);
                break;
            case MSG_MONITOR_GC_THREAD:
                Log.e(TAG, "[TIMEOUT] fast connect still not connected after " + FAST_CONN_SETUP_THRESHOLD + " ms,\n" +
                      " mConnectedDevice=" + mConnectedDevice + "\n" +
                      " mConnectedGroup=" + mConnectedGroup + "\n");
                // ToDo: more retry or error handle
                break;
            case MSG_UPDATE_PEER_GO_IP_UI:
                mTVPeerIPMAC.setText((String) msg.obj + " / " + mConnectedGroup.getOwner().deviceAddress);
                break;
            case MSG_UPDATE_PEER_GC_IP_UI:
                mTVPeerIPMAC.setText((String) msg.obj);
                break;
            case MSG_UPDATE_STABILITY_TEST_UI:
                mTVSTStatistics.setText((String) msg.obj + "\n\n\n\n\n\n\n\n");
                break;
            case MSG_UPDATE_STABILITY_TEST_RESET:
                stabilityTest_reset();
                break;
            case MSG_UPDATE_NIC_STATISTICS:
                update_nic_statistics();
                break;

            } //switch

        }

    };

    private void WiFiBeamPlus_GO() {
        if (false == mSocketServerRun) {
            // GO_SocketServer thread
            mSocketServerRun = true;
            mSocketServerThread = new GO_SocketServer();
            mSocketServerThread.setName("CdsWifiP2pGOReceiver");
            mSocketServerThread.start();

        }

    }

    public class GO_SocketServer extends Thread {

        public void run() {
            Log.d(TAG, "[GO] GO_SocketServer(): start thread");
            ServerSocket serverSocketCtrl = null;
            ServerSocket serverSocketData = null;
            Selector selector = null;
            boolean dataPathWork = false;
            long lastSampleTime, thisSampleTime, totalTime;  //unit: ms
            long lastSampleDataRecv, thisSampleDataRecv, totalDataRecv;

            // init
            try {
                ServerSocketChannel serverChannelCrtl;
                InetSocketAddress netAddress;

                serverChannelCrtl = ServerSocketChannel.open();
                serverSocketCtrl = serverChannelCrtl.socket();
                netAddress = new InetSocketAddress((InetAddress) getInterfaceAddress("wlan0"), CONTROL_PATH_PORT);
                serverSocketCtrl.bind(netAddress);
                Log.d(TAG, "[GO] {control plane} Bind OK!");

                serverChannelCrtl.configureBlocking(false);
                selector = Selector.open();

                if (null != selector) {
                    serverChannelCrtl.register(selector, SelectionKey.OP_ACCEPT);
                    Log.d(TAG, "[GO] {control plane} Register OK!");
                } else {
                    Log.e(TAG, "[GO] selector is null");
                    Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GO] selector is null");
                    mHandler.sendMessage(msg);
                }

            } catch (IOException e) {
                Log.e(TAG, "[GO] {control plane} Initialize Exception " + e);
                Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GO] {control plane} Initialize Exception");
                mHandler.sendMessage(msg);

            }

            Log.d(TAG, "[GO] {control plane} Initialzed done");
            lastSampleTime = thisSampleTime = totalTime = lastSampleDataRecv = thisSampleDataRecv = totalDataRecv = 0;

            while (true == mSocketServerRun) {
                //Log.d(TAG, "[GO] Socket Server is waiting ...");

                if (true == dataPathWork) {
                    while (null == mConnectedDevice) {
                        //Log.d(TAG, "[GO] polling p2p interface per " + CHECK_P2P_CONN_DURATION + " ms");
                        SystemClock.sleep(CHECK_P2P_CONN_DURATION);
                    }

                    Message msgX = mHandler.obtainMessage(MSG_SHOW_FC_DISCONNECT_BTN);
                    mHandler.sendMessage(msgX);

                    // init
                    try {
                        ServerSocketChannel serverChannelData;
                        InetSocketAddress netAddress;

                        serverChannelData = ServerSocketChannel.open();
                        serverSocketData = serverChannelData.socket();
                        netAddress = new InetSocketAddress((InetAddress) getInterfaceAddress("p2p0"), mDataPathPort);
                        serverSocketData.bind(netAddress);
                        Log.d(TAG, "[GO] {data plane} Bind OK!");

                        serverChannelData.configureBlocking(false);
                        //selector = Selector.open();
                        serverChannelData.register(selector, SelectionKey.OP_ACCEPT);
                        Log.d(TAG, "[GO] {data plane} Register OK!");
                        lastSampleTime = thisSampleTime = totalTime = totalDataRecv = 0;

                    } catch (IOException e) {
                        Log.e(TAG, "[GO] {data plane} Initialize Exception " + e);
                        Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GO] {data plane} Initialize Exception");
                        mHandler.sendMessage(msg);

                    }

                    mDataPathPort = mDataPathPort + 1;
                    dataPathWork = false;
                }

                // select
                try {
                    selector.select();

                } catch (IOException e) {
                    Log.e(TAG, "[GO] Select Exception " + e);
                    Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GO] Select Exception");
                    mHandler.sendMessage(msg);

                }

                //Log.d(TAG, "[GO] select() trigger");

                ByteBuffer ReadBuffer;
                int readByteSize = -1;
                byte[] Array;

                // readable or writeable
                Set readyKeys = selector.selectedKeys();
                Iterator iterator = readyKeys.iterator();

                while (iterator.hasNext()) {
                    SelectionKey key = (SelectionKey) iterator.next();
                    iterator.remove();

                    try {
                        if (key.isAcceptable()) {
                            Log.d(TAG, "[GO] Received a connection request.");
                            ServerSocketChannel server = (ServerSocketChannel) key.channel();
                            SocketChannel client = server.accept();
                            client.configureBlocking(false);

                            SelectionKey key2 = client.register(selector, SelectionKey.OP_READ);
                            ByteBuffer buffer = ByteBuffer.allocate(SOCKET_BUFFER_SIZE);
                            key2.attach(buffer);

                        } //isAcceptable()

                        if (key.isReadable()) {
                            //Log.d(TAG, "[GO] Received a packet.");
                            SocketChannel client = (SocketChannel) key.channel();
                            ReadBuffer = (ByteBuffer) key.attachment();
                            ReadBuffer.clear();
                            //Log.d(TAG, " Read mReadBuffer.capacity:" + mReadBuffer.capacity()
                            //        + " mReadBuffer.position():" + mReadBuffer.position()
                            //        + " mReadBuffer.limit():" + mReadBuffer.limit());

                            readByteSize = client.read(ReadBuffer);

                            if (readByteSize <= 0) {
                                Log.e(TAG, "[GO] readByteSize is" + readByteSize + ", abnormal!");
                                key.cancel();
                                key.channel().close();
                                client.close();
                                continue;
                            }

                            ReadBuffer.flip();
                            Array = ReadBuffer.array();
                            String p = new String(Array);
                            //Log.d(TAG, "[GO] Received packet info: " + p);

                            String CommandString = new String(Array, 0, readByteSize, "UTF-8");
                            String WriteString = parseControlPlaneCommand(CommandString);
                            ReadBuffer.clear();

                            if (WriteString != null) {
                                ReadBuffer.limit(SOCKET_BUFFER_SIZE);
                                //Log.d(TAG, "Before write ReadBuffer.capacity:" + ReadBuffer.capacity()
                                //        + " ReadBuffer.position():" + ReadBuffer.position()
                                //        + " ReadBuffer.limit():" + ReadBuffer.limit());
                                ReadBuffer.put(WriteString.getBytes("utf-8"));
                                ReadBuffer.flip();
                                int n = -1;
                                n = client.write(ReadBuffer);
                                dataPathWork = true;
                                Log.d(TAG, "[GO] Send reply string length:" + n + ", content:" + WriteString);

                            } else {
                                //Log.d(TAG, "[GO] The data path packet, don't send reply packet!");
                                thisSampleTime = System.currentTimeMillis();
                                totalDataRecv = totalDataRecv + readByteSize;

                                if (thisSampleTime - lastSampleTime >= FAST_CONN_SAMPLE_RATE) {
                                    // no count the 1st time since thisSampleTime=0
                                    if (0 != lastSampleTime) {
                                        totalTime = totalTime + (thisSampleTime - lastSampleTime);
                                        thisSampleDataRecv = totalDataRecv;

                                        String strShow =
                                            "[GO][Accmulated] time= " + totalTime + " ms, " +
                                            "data= " + totalDataRecv + " bytes, => " +
                                            (((thisSampleDataRecv - lastSampleDataRecv) * 8) / (((thisSampleTime - lastSampleTime) / 1000) * 1024 * 1024)) +
                                            " Mbps";
                                        Message msg = mHandler.obtainMessage(MSG_DUMP_CONNECTINFO_UI, strShow);
                                        mHandler.sendMessage(msg);

                                    }

                                    lastSampleTime = thisSampleTime;
                                    lastSampleDataRecv = thisSampleDataRecv;

                                }

                                continue;

                            }

                            key.cancel();
                            //key.channel().close();

                        } //isReadable()

                    } catch (IOException e) {
                        key.cancel();
                        Log.e(TAG, "[GO] Key Exception " + e);
                        Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GO] Key Exception");
                        mHandler.sendMessage(msg);

                        try {
                            key.channel().close();
                        } catch (IOException ce) {
                            Log.e(TAG, "[GO] Close Key Exception " + ce);
                        }

                    }

                } //while

                readyKeys.clear();

            } //while

            try {
                if (null != serverSocketCtrl) {
                    serverSocketCtrl.close();
                }

                if (null != serverSocketData) {
                    serverSocketData.close();
                }
            } catch (IOException e1) {
                Log.e(TAG, "[GO] Select close Exception " + e1);
            }

        }// run

    }

    private static final Pattern controlPlanePattern = Pattern.compile(
                STR_CONTROL_PLANE_TOKEN + "(.*)");

    private String parseControlPlaneCommand(String commandStr) {
        if (null != commandStr) {
            Matcher matcher = controlPlanePattern.matcher(commandStr);

            if (!matcher.find()) {
                //Log.e(TAG, "[GO] commandStr is Malformed");
                //Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GO] commandStr is Malformed");
                //mHandler.sendMessage(msg);
                return null;
            }

            String gc_mac = matcher.group(1);

            if (null == gc_mac) {
                Log.e(TAG, "[GO] parse GC MAC failed");
                Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GO] parse GC MAC failed");
                mHandler.sendMessage(msg);
                return null;
            }

            Log.d(TAG, "[GO] get GC MAC=" + gc_mac + ", data path port=" + mDataPathPort);

            WifiP2pFastConnectInfo param = new WifiP2pFastConnectInfo();
            param.deviceAddress = gc_mac;
            WifiP2pFastConnectInfo fastConnGOParam = mWifiP2pManager.fastConnectAsGo(param);

            if (null == fastConnGOParam) {
                Log.e(TAG, "[GO] fastConnGOParam is null");
                Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GO] fastConnGOParam is null");
                mHandler.sendMessage(msg);
                return null;
            }

            Message msgX = mHandler.obtainMessage(MSG_SHOW_FC_RESULT_UI);
            mHandler.sendMessage(msgX);
            Log.d(TAG, "[GO] fastConnect GO mode, fastConnGOParam: " + fastConnGOParam);
            Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GO] fastConnGOParam:" + fastConnGOParam);
            mHandler.sendMessage(msg);

            return "ssid=" + fastConnGOParam.ssid +
                   " psk=" + fastConnGOParam.psk +
                   " mac=" + mThisDevice.deviceAddress +
                   " port=" + mDataPathPort +
                   "\n";

        }

        Log.e(TAG, "[GO] commandStr is null");
        Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GO] commandStr is null");
        mHandler.sendMessage(msg);
        return null;

    }

    private void fastConnect_step0_GC_refresh() {
        if (FAST_ROLE_GC == mFastRole) {
            mETGOWlanIP.setVisibility(View.VISIBLE);
            mBFastConnDo.setVisibility(View.VISIBLE);

        } else {
            mETGOWlanIP.setVisibility(View.GONE);
            mBFastConnDo.setVisibility(View.GONE);
            mGOWlanInetAddr = null;

        }

    }

    private void WiFiBeamPlus_GC() {
        mBFastConnDo.setText(R.string.wifi_p2p_FC_doing);
        mTVFastConnResult.setVisibility(View.VISIBLE);
        mDoingFastConn = true;

        // GC_SocketClient thread
        mSocketClientThread = new GC_SocketClient();
        mSocketClientThread.setName("CdsWifiP2pGCClient");
        mSocketClientThread.start();

        Message msg = mHandler.obtainMessage(MSG_MONITOR_GC_THREAD);
        mHandler.sendMessageDelayed(msg, FAST_CONN_SETUP_THRESHOLD);

    }

    public class GC_SocketClient extends Thread {

        public void run() {
            Log.d(TAG, "[GC] GC_SocketClient(): start thread");
            Socket gc_control_path_socket = null;
            String data_plane_command = null;

            try {
                // control plane
                do {
                    gc_control_path_socket = new Socket(mGOWlanInetAddr, CONTROL_PATH_PORT);

                    if (null == gc_control_path_socket) {
                        Log.e(TAG, "[GC] gc_control_path_socket is null");
                        Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GC] gc_control_path_socket is null");
                        mHandler.sendMessage(msg);
                        break;
                    }

                    // send
                    OutputStream control_path_out_stream = gc_control_path_socket.getOutputStream();

                    if (null == control_path_out_stream) {
                        Log.e(TAG, "[GC] control_path_out_stream is null");
                        Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GC] control_path_out_stream is null");
                        mHandler.sendMessage(msg);
                        break;
                    }

                    PrintStream pout = new PrintStream(control_path_out_stream);

                    if (null == pout) {
                        Log.e(TAG, "[GC] pout is null");
                        Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GC] pout is null");
                        mHandler.sendMessage(msg);
                        break;
                    }

                    pout.println(STR_CONTROL_PLANE_TOKEN + mThisDevice.deviceAddress + "\n");
                    Log.d(TAG, "[GC] control path: " + STR_CONTROL_PLANE_TOKEN + mThisDevice.deviceAddress);

                    // receive
                    InputStream control_path_in_stream = gc_control_path_socket.getInputStream();

                    if (null == control_path_in_stream) {
                        Log.e(TAG, "[GC] control_path_in_stream is null");
                        Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GC] control_path_in_stream is null");
                        mHandler.sendMessage(msg);
                        break;
                    }

                    DataInputStream din = new DataInputStream(control_path_in_stream);

                    if (null == din) {
                        Log.e(TAG, "[GC] din is null");
                        Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GC] din is null");
                        mHandler.sendMessage(msg);
                        break;
                    }

                    data_plane_command = din.readLine();
                    Log.d(TAG, "[GC] control path get response:" + data_plane_command);

                } while (false);

            } catch (IOException e) {
                Log.e(TAG, "[GC] WiFiBeamPlus_GC control socket exception:" + e);
                Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GC] WiFiBeamPlus_GC control socket exception");
                mHandler.sendMessage(msg);

            } finally {
                try {
                    if (null != gc_control_path_socket) {
                        gc_control_path_socket.close();
                    }
                } catch (IOException e) {
                    Log.e(TAG, "[GC] control close socket():" + e);
                }
            }

            // data plane: parsing
            int GO_port = parseDatePlaneCommand(data_plane_command);

            if (0 >= GO_port) {
                Log.e(TAG, "[GC] GO port is invilid, setup socket again!");
                Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GC] GO port is invilid, setup socket again!");
                mHandler.sendMessage(msg);
                // sleep then try again
                SystemClock.sleep(FAST_CONN_GC_CTRL_RETRY_DURATION);
                run();
            }

            // data plane: waiting
            while (null == mConnectedDevice) {
                //Log.d(TAG, "[GC] polling p2p interface per " + CHECK_P2P_CONN_DURATION + " ms");
                SystemClock.sleep(CHECK_P2P_CONN_DURATION);
            }

            Message msgX = mHandler.obtainMessage(MSG_SHOW_FC_DISCONNECT_BTN);
            mHandler.sendMessage(msgX);

            // data plane packet
            String packet = DATA_PATH_PKT_PATTERN;

            while (DATA_PATH_PKT_LEN > packet.length()) {
                packet = packet + DATA_PATH_PKT_PATTERN;
            }

            //Log.d(TAG, "[GC] default packet length=" + packet.length());

            // data plane
            while (null != mConnectedDevice) {
                Socket gc_data_path_socket = null;
                long lastSampleTime, thisSampleTime, totalTime;  //unit: ms
                long lastSampleDataSend, thisSampleDataSend, totalDataSend;
                lastSampleTime = thisSampleTime = totalTime = lastSampleDataSend = thisSampleDataSend = totalDataSend = 0;

                try {

                    do {
                        // setup socket
                        gc_data_path_socket = new Socket(InetAddress.getByName(FAST_CONN_GO_IP), GO_port);

                        if (null == gc_data_path_socket) {
                            Log.e(TAG, "[GC] gc_control_path_socket is null");
                            Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GC] gc_control_path_socket is null");
                            mHandler.sendMessage(msg);
                            break;
                        }

                        OutputStream data_path_out_stream = gc_data_path_socket.getOutputStream();

                        if (null == data_path_out_stream) {
                            Log.e(TAG, "[GC] data_path_out_stream is null");
                            Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GC] data_path_out_stream is null");
                            mHandler.sendMessage(msg);
                            break;
                        }

                        PrintStream pout = new PrintStream(data_path_out_stream);

                        if (null == pout) {
                            Log.e(TAG, "[GC] pout is null");
                            Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GC] pout is null");
                            mHandler.sendMessage(msg);
                            break;
                        }

                        // send
                        Log.d(TAG, "[GC] data path sent start");

                        while (null != mConnectedDevice) {
                            pout.println(packet);
                            //Log.d(TAG, "[GC] data path sent --> ");
                            thisSampleTime = System.currentTimeMillis();
                            totalDataSend = totalDataSend + packet.length();

                            if (thisSampleTime - lastSampleTime >= FAST_CONN_SAMPLE_RATE) {
                                // no count the 1st time since thisSampleTime=0
                                if (0 != lastSampleTime) {
                                    totalTime = totalTime + (thisSampleTime - lastSampleTime);
                                    thisSampleDataSend = totalDataSend;

                                    String strShow =
                                        "[GC][Accmulated] time= " + totalTime + " ms, " +
                                        "data= " + totalDataSend + " bytes, => " +
                                        (((thisSampleDataSend - lastSampleDataSend) * 8) / (((thisSampleTime - lastSampleTime) / 1000) * 1024 * 1024)) +
                                        " Mbps";
                                    Message msg = mHandler.obtainMessage(MSG_DUMP_CONNECTINFO_UI, strShow);
                                    mHandler.sendMessage(msg);

                                }

                                lastSampleTime = thisSampleTime;
                                lastSampleDataSend = thisSampleDataSend;

                            }

                        } // while()

                        Log.d(TAG, "[GC] data path sent done");

                    } while (false);

                } catch (IOException e) {
                    Log.e(TAG, "[GC] WiFiBeamPlus_GC data socket exception:" + e);
                    Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GC] WiFiBeamPlus_GC data socket exception");
                    mHandler.sendMessage(msg);

                } finally {
                    try {
                        if (null != gc_data_path_socket) {
                            gc_data_path_socket.close();
                        }

                    } catch (IOException e) {
                        Log.e(TAG, "[GC] data close socket():" + e);

                    } finally {
                        continue; // go to while (null != mConnectedDevice)

                    }

                }

            } //while (null != mConnectedDevice)


        }//run

    }

    private static final Pattern dataPlanePattern = Pattern.compile(
                "ssid=(.*) " +
                "psk=(.*) " +
                "mac=(.*) " +
                "port=(.*)");

    private int parseDatePlaneCommand(String commandStr) {
        if (null != commandStr) {
            Matcher matcher = dataPlanePattern.matcher(commandStr);

            if (!matcher.find()) {
                Log.e(TAG, "[GC] commandStr is Malformed");
                Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GC] commandStr is Malformed");
                mHandler.sendMessage(msg);
                return -1;
            }

            String GO_ssid = matcher.group(1);
            String GO_psk = matcher.group(2);
            String GO_mac = matcher.group(3);
            String GO_port = matcher.group(4);

            if (null == GO_ssid || null == GO_psk || null == GO_mac || null == GO_port) {
                Log.e(TAG, "[GC] parse GO cred. failed");
                Message msg = mHandler.obtainMessage(MSG_DUMP_ERROR_UI, "[GC] parse GO cred. failed");
                mHandler.sendMessage(msg);
                return -1;
            }

            Log.d(TAG, "[GC] parse GO cred.:" +
                  "ssid=" + GO_ssid +
                  " psk=" + GO_psk +
                  " mac=" + GO_mac +
                  " port=" + GO_port);

            // do fast connect
            WifiP2pFastConnectInfo param = new WifiP2pFastConnectInfo();
            param.networkId = 33;
            param.deviceAddress = GO_mac;
            param.ssid = GO_ssid;
            param.authType = "0x0020";
            param.encrType = "0x0008";
            param.psk = GO_psk;
            param.gcIpAddress = FAST_CONN_GC_IP;
            Log.d(TAG, "[GC] fastConnect GC mode, param: " + param);
            mWifiP2pManager.fastConnectAsGc(mWifiP2pChannel, param, null);

            return Integer.valueOf(GO_port);

        }

        return 0;
    }

    private void reset_all_fast_conn_setp_UI() {
        mBFastConnDisonnect.setVisibility(View.GONE);
        mTVFastConnResult.setText("N/A");
        mTVFastConnResult.setVisibility(View.GONE);
        mTVGOChoosen.setText(R.string.wifi_p2p_FC_GO);
        mTVGCChoosen.setText(R.string.wifi_p2p_FC_GC);
        mETGOWlanIP.setVisibility(View.GONE);
        mBFastConnDo.setText(R.string.wifi_p2p_FC_do);
        mBFastConnDo.setVisibility(View.GONE);
        mGOWlanInetAddr = null;
        mDoingFastConn = false;
        mRGFastConnStep0.clearCheck();

        mHistory01 = ".";
        mHistory02 = "..";
        mHistory03 = "...";
        mHistory04 = "....";
        mHistory05 = ".....";

    }

    public class CdsWifiP2pStabilityTest extends Thread {

        public void run() {

            mWifiP2pManager.removeGroup(mWifiP2pChannel, null);
            mWifiP2pManager.cancelConnect(mWifiP2pChannel, null);

            while (true == mStabilityTestRun) {

                //Log.d(TAG, "StabilityTest: do scan");
                //scan
                startSearch();

                //check scan result
                boolean peerFound = false;
                int counter = 0;

                while (false == peerFound) {
                    for (WifiP2pDevice peer: mPeers.getDeviceList()) {
                        if (peer.deviceAddress.equalsIgnoreCase(mStabilityTestPeerMAC)) {
                            peerFound = true;
                        }

                    } // for

                    if (false == peerFound) {
                        if (false == stabilityTest_sleep(STABILITY_TEST_PEER_FOUND_SAMPLE_RATE))    return;

                        counter++;
                        //Log.d(TAG, "StabilityTest: scan then waiting " + counter*STABILITY_TEST_PEER_FOUND_SAMPLE_RATE/1000 + " seconds");

                        if (false == mIsScanSucceed) {
                            startSearch();
                        }

                        if (STABILITY_TEST_SCAN_MAX_TRY <= counter) {
                            Log.e(TAG, "StabilityTest: can't scan peer device!");

                            Message msg2 = mHandler.obtainMessage(MSG_UPDATE_STABILITY_TEST_RESET);
                            mHandler.sendMessage(msg2);

                            String stabilityStatic = "do connect " + mStabilityTestConnectingCounter + " times, " +
                                                     "connected " + mStabilityTestConnectedCounter + " times\n" +
                                                     "Can't find peer device over " + 20 * STABILITY_TEST_PEER_FOUND_SAMPLE_RATE / 1000 + " seconds this round!" ;
                            Message msg1 = mHandler.obtainMessage(MSG_UPDATE_STABILITY_TEST_UI, stabilityStatic);
                            mHandler.sendMessage(msg1);

                            return;
                        }
                    }

                }  //while

                //connect
                WifiP2pConfig config = new WifiP2pConfig();
                config.deviceAddress = mStabilityTestPeerMAC.toLowerCase();
                config.wps.setup = WpsInfo.PBC;
                mWifiP2pManager.connect(mWifiP2pChannel, config, null);

                mStabilityTestConnectingCounter ++;
                Log.d(TAG, "StabilityTest: do connect, StabilityTestConnectingCounter=" + mStabilityTestConnectingCounter);

                //sleep
                if (false == stabilityTest_sleep(STABILITY_TEST_CONNECTED_INTERVAL))    return;

                //static
                String stabilityStatic = "do connect " + mStabilityTestConnectingCounter + " times, " +
                                             "connected " + mStabilityTestConnectedCounter + " times" ;
                Message msg = mHandler.obtainMessage(MSG_UPDATE_STABILITY_TEST_UI, stabilityStatic);
                mHandler.sendMessage(msg);

                //disconnnect
                //Log.d(TAG, "StabilityTest: do disconnect");
                mWifiP2pManager.removeGroup(mWifiP2pChannel, null);
                mWifiP2pManager.cancelConnect(mWifiP2pChannel, null);

                //sleep
                if (false == stabilityTest_sleep(STABILITY_TEST_DISCONNECTED_INTERVAL))    return;

            } //whie

            Log.d(TAG, "StabilityTest: end of thread");

        } //run

    }

    private void stabilityTest_button_refresh() throws InterruptedException {
        Log.d(TAG, "stabilityTest_button_refresh(): mStabilityTestRun=" + mStabilityTestRun);

        if (true == mStabilityTestRun) {
            mBStabilityTestDo.setText(mContext.getResources().getString(R.string.wifi_p2p_ST_doing));
            mStabilityTestThread = new CdsWifiP2pStabilityTest();
            mStabilityTestThread.setName("CdsWifiP2pStabilityTest");
            mStabilityTestThread.start();

        } else {
            mBStabilityTestDo.setText(mContext.getResources().getString(R.string.wifi_p2p_ST_do));
            if (null != mStabilityTestThread) {
                //mStabilityTestThread.stop();
                mStabilityTestThread.interrupt();
            }
            mStabilityTestConnectingCounter = 0;
            mStabilityTestConnectedCounter = 0;
            mTVSTStatistics.setText("N/A");

        }

    }

    private void stabilityTest_reset() {
        mStabilityTestRun = false;

        try {
            stabilityTest_button_refresh();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

    }

    private boolean stabilityTest_sleep(int duatrionMS) {
        int remainMS = duatrionMS;

        while (0 < remainMS) {
            SystemClock.sleep(500);
            remainMS = remainMS - 500;

            if (false == mStabilityTestRun) {
                Log.e(TAG, "StabilityTest: thread should be terminated!");
                return false;
            }

        } //while

        return true;
    }

    //Layer 2 Tx/Rx
    private void update_nic_statistics() {
        FileInputStream fileStream = null;
        boolean isP2pEnabled = false;

        try {
            fileStream =  new FileInputStream(NET_DEV_FILE);
            DataInputStream in = new DataInputStream(fileStream);
            BufferedReader br = new BufferedReader(new InputStreamReader(in));
            String str = br.readLine();

            while ((str != null) && (str.length() != 0)) {
                //Log.d(TAG, "update_nic_statistics(): read a line = " + str);
                if (str.contains(NETWORK_INTERFACE)) {
                    //Log.d(TAG, "update_nic_statistics(): ==>" + str + "<==");
                    isP2pEnabled = true;
                    parse_nic_statistics(str);
                }

                str = br.readLine();

            } //while()

        } catch (IOException e) {
            Log.e(TAG, "update_nic_statistics(): Exception: " + e);

        } finally {
            if (fileStream != null) {
                try {
                    fileStream.close();
                    //Log.e(TAG, "update_nic_statistics(): close file OK!");
                } catch (IOException e) {
                    Log.e(TAG, "update_nic_statistics(): fileStream.close exception: " + e);

                }
            }

        }

        if (false == isP2pEnabled) {
            mTVNICStatisticsValue.setText("N/A");
            mNIC_TxStat = mNIC_RxStat = 0;
        }

        Message msg = mHandler.obtainMessage(MSG_UPDATE_NIC_STATISTICS);
        mHandler.sendMessageDelayed(msg, NIC_STATISTICS_SAMPLE_RATE);

    }

    /* format:
     *    face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
     *    p2p0:       0       0    0    0    0     0          0         0      468       6    0    0    0     0       0          0
     */
    private void parse_nic_statistics(String statistics) {
        String temp = "";
        SimpleDateFormat formatter = new SimpleDateFormat("dd MMM yyyy  hh:mm:ss a");
        String showOnUI = "[ " + formatter.format(new Date()) + " ]\n\n" + "RX:\n\tbytes=";
        int round = 1;
        long NowTxStat = 0;
        long NowRxStat = 0;

        // pass the pre 7 bytes: "  p2p0:"
        for (int index = 7; index < statistics.length(); index++) {
            if (statistics.substring(index + 0, index + 1).equals(" ")) {
                if (!temp.equals("")) {
                    //Log.d(TAG, "parse_nic_statistics(): <1> ->|" + temp + "|<-");
                    switch (round) {
                    case 1:
                        NowRxStat = Long.valueOf(temp);
                        showOnUI = showOnUI + temp + "\n\tpackets=";
                        break;
                    case 2:
                        showOnUI = showOnUI + temp + "\n\terrs=";
                        break;
                    case 3:
                        showOnUI = showOnUI + temp + "\n\tdrop=";
                        break;
                    case 4:
                        showOnUI = showOnUI + temp + "\n\tfifo=";
                        break;
                    case 5:
                        showOnUI = showOnUI + temp + "\n\tframe=";
                        break;
                    case 6:
                        showOnUI = showOnUI + temp + "\n\tcompressed=";
                        break;
                    case 7:
                        showOnUI = showOnUI + temp + "\n\tmulticast=";
                        break;
                    case 8:
                        showOnUI = showOnUI + temp + "\nTX:\n\tbytes=";
                        break;
                    case 9:
                        NowTxStat = Long.valueOf(temp);
                        showOnUI = showOnUI + temp + "\n\tpackets=";
                        break;
                    case 10:
                        showOnUI = showOnUI + temp + "\n\terrs=";
                        break;
                    case 11:
                        showOnUI = showOnUI + temp + "\n\tdrop=";
                        break;
                    case 12:
                        showOnUI = showOnUI + temp + "\n\tfifo=";
                        break;
                    case 13:
                        showOnUI = showOnUI + temp + "\n\tcolls=";
                        break;
                    case 14:
                        showOnUI = showOnUI + temp + "\n\tcarrier=";
                        break;
                    case 15:
                        showOnUI = showOnUI + temp + "\n\tcompressed=";
                        break;

                    } //switch

                    round ++ ;

                }

                temp = "";
                continue;

            }

            temp = temp + statistics.substring(index + 0, index + 1);

        } //for

        if (null != temp) {
            //Log.d(TAG, "parse_nic_statistics(): <2> ->|" + temp + "|<-");
            showOnUI = showOnUI + temp;
        }

        /*
        Log.d(TAG, "parse_nic_statistics():" +
            " NowRxStat=" + NowRxStat +
            " mNIC_RxStat=" + mNIC_RxStat +
            " NowTxStat=" + NowTxStat +
            " mNIC_TxStat=" + mNIC_TxStat );
        */
        long showRxInt = ((NowRxStat - mNIC_RxStat) * 8) / (1024 * 1024 * (NIC_STATISTICS_SAMPLE_RATE / 1000));
        long showTxInt = ((NowTxStat - mNIC_TxStat) * 8) / (1024 * 1024 * (NIC_STATISTICS_SAMPLE_RATE / 1000));
        long showRxPoint = ((NowRxStat - mNIC_RxStat) * 8) % (1024 * 1024 * (NIC_STATISTICS_SAMPLE_RATE / 1000));
        long showTxPoint = ((NowTxStat - mNIC_TxStat) * 8) % (1024 * 1024 * (NIC_STATISTICS_SAMPLE_RATE / 1000));

        // show statistics when Tx/Rx throughput is larger than 1 Mbps
        if (showRxInt > 1 || showTxInt > 1) {
            showOnUI = showOnUI +
                       "\n\nRx throughput= " +
                       Long.valueOf(showRxInt) + "." + Long.valueOf(showRxPoint) +
                       " Mbps" +
                       "\nTx throughput= " +
                       Long.valueOf(showTxInt) + "." + Long.valueOf(showTxPoint) +
                       " Mbps";
        }

        mNIC_RxStat = NowRxStat;
        mNIC_TxStat = NowTxStat;

        //Log.d(TAG, "parse_nic_statistics(): showOnUI ->|\n" + showOnUI + "|<-");
        mTVNICStatisticsValue.setText(showOnUI);

    }

}
