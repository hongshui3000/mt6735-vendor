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
 * MediaTek Inc. (C) 2014. All rights reserved.
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

package com.mediatek.voicecommand.mgr;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.FileUtils;

import com.mediatek.common.voicecommand.VoiceCommandListener;
import com.mediatek.voicecommand.cfg.ConfigurationXml;
import com.mediatek.voicecommand.cfg.VoiceCustomization;
import com.mediatek.voicecommand.cfg.VoiceKeyWordInfo;
import com.mediatek.voicecommand.cfg.VoiceLanguageInfo;
import com.mediatek.voicecommand.cfg.VoiceProcessInfo;
import com.mediatek.voicecommand.cfg.VoiceWakeupInfo;
import com.mediatek.voicecommand.util.Log;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Locale;
import java.util.Map;
import java.util.Map.Entry;

public class ConfigurationManager {
    private static final String TAG = "ConfigurationManager";

    private volatile static ConfigurationManager sCfgMgr = null;
    // private static byte[] sInstanceLock = new byte[0];
    private final Context mContext;
    private int mCurrentLanguageIndex = -1;

    private final HashMap<String, String> mPaths = new HashMap<String, String>();
    private final HashMap<String, VoiceProcessInfo> mVoiceProcessInfos = new HashMap<String, VoiceProcessInfo>();
    private final HashMap<String, VoiceKeyWordInfo> mVoiceKeyWordInfos = new HashMap<String, VoiceKeyWordInfo>();

    private ArrayList<VoiceLanguageInfo> mLanguageList = new ArrayList<VoiceLanguageInfo>();
    private ArrayList<String> mVoiceUiFeatureNames = new ArrayList<String>();
    private ConfigurationXml mConfigurationXml;

    private String mUiPatternPath = "UIPattern";
    private String mUbmFilePath = "UBMFile";
    private String mModeFilePath = "ModeFile";
    private String mContactsdbFilePath = "ContactsdbFile";
    private String mContactsModeFilePath = "ContactsModeFile";

    private final String mUnlockTrainingPath = "/training/unlock/";
    private final String mAnyoneTrainingPath = "/training/anyone/";
    private final String mCommandTrainingPath = "/training/command/";
    private final String mWakeupInfoPath = "/wakeupinfo/";

    private final String mRecogPath = "recogpattern/";
    private final String mFeaturePath = "featurefile/";
    private final String mPasswordPath = "passwordfile/";

    private String mUnlockRecogPatternPath = "UnlockRecogPattern";
    private String mUnlockFeatureFilePath = "UnlockFeatureFile";
    private String mUnlockPswdFilePath = "UnlockPswdFile";

    private String mAnyoneRecogPatternPath = "AnyoneRecogPattern";
    private String mAnyoneFeatureFilePath = "AnyoneFeatureFile";
    private String mAnyonePswdFilePath = "AnyonePswdFile";

    private String mCommandRecogPatternPath = "CommandRecogPattern";
    private String mCommandFeatureFilePath = "CommandFeatureFile";
    private String mCommandPswdFilePath = "CommandPswdFile";

    private String mWakeupInfoFilePath = "WakeupinfoFile";
    private final String mWakeupAnyonePath = "wakeup/anyone.xml";
    private final String mWakeupCommandPath = "wakeup/command.xml";

    private ArrayList<VoiceWakeupInfo> mWakeupAnyoneList = new ArrayList<VoiceWakeupInfo>();
    private ArrayList<VoiceWakeupInfo> mWakeupCommandList = new ArrayList<VoiceWakeupInfo>();

    private int mWakeupMode = 1;
    private int mWakeupStatus = 0;

    private final String mContactsdbPath = "/contacts/";
    private boolean mIsCfgPrepared = true;

    private String mVoiceUiCacheFile = "com.mediatek.voicecommand_preferences";
    private String mVoiceLanguageCacheFile = "Voice_Language";
    private String mCurLanguageKey = "CurLanguageIndex";
    private String mIsFirstBoot = "IsFirstBoot";
    private String mCurSystemLanguageIndex = "CurSystemLanguageIndex";

    private boolean mIsSystemLanguage = false;
    private VoiceCustomization mVoiceCustomization = new VoiceCustomization();

    /*
     * ConfigurationManager for control configuration
     * 
     * @param context
     */
    private ConfigurationManager(Context context) {
        Log.i(TAG, "[ConfigurationManager]new ...");
        mContext = context.getApplicationContext();
        mConfigurationXml = new ConfigurationXml(context);
        mConfigurationXml.readVoiceProcessInfoFromXml(mVoiceProcessInfos, mVoiceUiFeatureNames);
        mConfigurationXml.readVoiceCommandPathFromXml(mPaths);
        mCurrentLanguageIndex = mConfigurationXml.readVoiceLanguangeFromXml(mLanguageList);
        mConfigurationXml.readVoiceWakeupFromXml(mWakeupAnyoneList, mWakeupAnyonePath);
        mConfigurationXml.readVoiceWakeupFromXml(mWakeupCommandList, mWakeupCommandPath);
        String dataDir = context.getApplicationInfo().dataDir;
        mPaths.put(mContactsdbFilePath, dataDir + mContactsdbPath);
        mPaths.put(mUnlockRecogPatternPath, dataDir + mUnlockTrainingPath + mRecogPath);
        mPaths.put(mUnlockFeatureFilePath, dataDir + mUnlockTrainingPath + mFeaturePath);
        mPaths.put(mUnlockPswdFilePath, dataDir + mUnlockTrainingPath + mPasswordPath);
        mPaths.put(mAnyoneRecogPatternPath, dataDir + mAnyoneTrainingPath + mRecogPath);
        mPaths.put(mAnyoneFeatureFilePath, dataDir + mAnyoneTrainingPath + mFeaturePath);
        mPaths.put(mAnyonePswdFilePath, dataDir + mAnyoneTrainingPath + mPasswordPath);
        mPaths.put(mCommandRecogPatternPath, dataDir + mCommandTrainingPath + mRecogPath);
        mPaths.put(mCommandFeatureFilePath, dataDir + mCommandTrainingPath + mFeaturePath);
        mPaths.put(mCommandPswdFilePath, dataDir + mCommandTrainingPath + mPasswordPath);
        mPaths.put(mWakeupInfoFilePath, dataDir + mWakeupInfoPath);

        mConfigurationXml.readVoiceCustomizationFromXml(mVoiceCustomization);

        mIsSystemLanguage = mVoiceCustomization.mIsSystemLanguage;
        if (mIsSystemLanguage) {
            Log.i(TAG, "[ConfigurationManager]new,isSystemLanguage.");
            // Register Local change receiver.
            IntentFilter filter = new IntentFilter();
            filter.addAction(Intent.ACTION_LOCALE_CHANGED);
            mContext.registerReceiver(mSystemLanguageReceiver, filter);
        }

        if (!makeDirForPath(mPaths.get(mUnlockRecogPatternPath))
                || !makeDirForPath(mPaths.get(mUnlockFeatureFilePath))
                || !makeDirForPath(mPaths.get(mUnlockPswdFilePath))
                || !makeDirForPath(mPaths.get(mAnyoneRecogPatternPath))
                || !makeDirForPath(mPaths.get(mAnyoneFeatureFilePath))
                || !makeDirForPath(mPaths.get(mAnyonePswdFilePath))
                || !makeDirForPath(mPaths.get(mCommandRecogPatternPath))
                || !makeDirForPath(mPaths.get(mCommandFeatureFilePath))
                || !makeDirForPath(mPaths.get(mCommandPswdFilePath))
                || !makeDirForPath(mPaths.get(mWakeupInfoFilePath))
                || !makeDirForPath(mPaths.get(mContactsdbFilePath))) {
            mIsCfgPrepared = false;
        }
        if (mIsCfgPrepared) {
            Log.i(TAG, "[ConfigurationManager]new,mIsCfgPrepared is true.");
            checkVoiceCachePref();
            if (mCurrentLanguageIndex >= 0) {
                mConfigurationXml.readKeyWordFromXml(mVoiceKeyWordInfos,
                        mLanguageList.get(mCurrentLanguageIndex).mFilePath);
            }
        }
    }

    public static ConfigurationManager getInstance(Context context) {
        if (null == sCfgMgr) {
            synchronized (ConfigurationManager.class) {
                if (null == sCfgMgr) {
                    sCfgMgr = new ConfigurationManager(context);
                }
            }

        }
        return sCfgMgr;
    }

    /**
     * Handle the case of using the system language.
     * 
     */
    public void useSystemLanguage() {
        mCurrentLanguageIndex = -1;
        String systemLanguage = Locale.getDefault().getLanguage() + "-"
                + Locale.getDefault().getCountry();

        SharedPreferences languagePref = mContext.getSharedPreferences(mVoiceLanguageCacheFile,
                Context.MODE_PRIVATE);
        boolean isFirstBoot = languagePref.getBoolean(mIsFirstBoot, true);
        Log.i(TAG, "[useSystemLanguage]isFirstBoot = " + isFirstBoot + ",systemLanguage = "
                + systemLanguage);
        updateCurLanguageIndex(systemLanguage);
        if (mCurrentLanguageIndex < 0) {
            if (isFirstBoot) {
                updateCurLanguageIndex(mVoiceCustomization.mDefaultLanguage);
            } else {
                mCurrentLanguageIndex = languagePref.getInt(mCurSystemLanguageIndex,
                        mCurrentLanguageIndex);
            }
        }
        if (isFirstBoot) {
            languagePref.edit().putBoolean(mIsFirstBoot, false).apply();
        }
    }

    /**
     * Update mCurrentLanguageIndex.
     * 
     * @param language
     *            local language string
     */
    public void updateCurLanguageIndex(String language) {
        Log.w(TAG, "[updateCurLanguageIndex]language = " + language);
        if (language == null) {
            Log.w(TAG, "[updateCurLanguageIndex]language is null,return.");
            return;
        }
        for (int i = 0; i < mLanguageList.size(); i++) {
            if (language.equals(mLanguageList.get(i).mLanguageCode)) {
                mCurrentLanguageIndex = i;
                SharedPreferences languagePref = mContext.getSharedPreferences(
                        mVoiceLanguageCacheFile, Context.MODE_PRIVATE);
                languagePref.edit().putInt(mCurSystemLanguageIndex, mCurrentLanguageIndex).apply();
                break;
            }
        }
    }

    /**
     * Check whether configuration file is prepared.
     */
    public boolean isCfgPrepared() {
        return mIsCfgPrepared;
    }

    public boolean getIsSystemLanguage() {
        return mIsSystemLanguage;
    }

    /**
     * Check whether process is Allowed to register or not.
     * 
     * @param processname
     *            process name
     * @return result
     */
    public int isAllowProcessRegister(String processname) {
        return mVoiceProcessInfos.containsKey(processname) ? VoiceCommandListener.VOICE_NO_ERROR
                : VoiceCommandListener.VOICE_ERROR_COMMON_ILLEGAL_PROCESS;
    }

    /**
     * Get the numbers of UiFeature.
     * 
     * @return the numbers of UiFeature
     */
    public int getUiFeatureNumber() {
        return mVoiceUiFeatureNames.size();
    }

    /**
     * Check whether process has permission to do action or not.
     * 
     * @param featurename
     *            feature or package name of application
     * @param mainaction
     *            main action
     * @return true if has operation permission
     */
    public boolean hasOperationPermission(String featurename, int mainaction) {
        Log.i(TAG, "[hasOperationPermission]featurename = " + featurename);
        VoiceProcessInfo info = mVoiceProcessInfos.get(featurename);
        if (info == null) {
            Log.i(TAG, "[hasOperationPermission]no this permission,return false.");
            return false;
        }
        return info.mPermissionIDList.contains(mainaction);
    }

    /**
     * Check whether process has permission to do actions in array or not.
     * 
     * @param featurename
     *            feature or package name of application
     * @param mainaction
     *            main action
     * @return true if has operation permission
     */
    public boolean containOperationPermission(String featurename, int[] mainaction) {
        Log.i(TAG, "[containOperationPermission]featurename = " + featurename);
        if (mainaction == null || mainaction.length == 0) {
            Log.w(TAG, "[containOperationPermission]mainaction = " + mainaction);
            return false;
        }
        VoiceProcessInfo info = mVoiceProcessInfos.get(featurename);
        if (info == null) {
            Log.w(TAG, "[containOperationPermission]info is null.");
            return false;
        }
        for (int i = 0; i < mainaction.length; i++) {
            if (info.mPermissionIDList.contains(mainaction[i])) {
                return true;
            }
        }
        return false;
    }

    /**
     * Get the feature name list and the process enable list.
     * 
     * @return the feature name list
     */
    public String[] getFeatureNameList() {
        int size = mVoiceUiFeatureNames.size();
        if (size == 0) {
            Log.w(TAG, "[getFeatureNameList]size is 0.");
            return null;
        }
        return mVoiceUiFeatureNames.toArray(new String[size]);
    }

    /**
     * Get the feature enable list.
     * 
     * @return the feature enable list
     */
    public int[] getFeatureEnableArray() {
        int size = mVoiceUiFeatureNames.size();
        if (size == 0) {
            Log.w(TAG, "[getFeatureEnableArray]size is 0.");
            return null;
        }
        int[] isEnableArray = new int[size];

        for (int i = 0; i < size; i++) {
            VoiceProcessInfo info = mVoiceProcessInfos.get(mVoiceUiFeatureNames.get(i));
            if (info != null) {
                isEnableArray[i] = info.mIsVoiceEnable ? 1 : 0;
            }
        }
        return isEnableArray;
    }

    /**
     * Update Feature enable in mVoiceProcessInfos.
     * 
     * @param featurename
     *            application feature name
     * @param enable
     *            true if want to enable
     * @return true if update success
     */
    public boolean updateFeatureEnable(String featurename, boolean enable) {
        Log.i(TAG, "[updateFeatureEnable]featurename = " + featurename + ",enable = " + enable);
        VoiceProcessInfo info = mVoiceProcessInfos.get(featurename);
        if (info == null) {
            Log.w(TAG, "[updateFeatureEnable]info is null.");
            return false;
        }

        info.mIsVoiceEnable = enable;
        if (info.mRelationProcessName != null
                && !info.mRelationProcessName.equals(ConfigurationXml.sPublicFeatureName)) {
            updateFeatureEnable(info.mRelationProcessName, enable);
        }
        updateFeatureListEnableToPref();

        return true;
    }

    /**
     * Get language list that this feature support.
     * 
     * @return language list
     */
    public String[] getLanguageList() {
        int size = mLanguageList.size();
        if (size == 0) {
            Log.w(TAG, "[getLanguageList]size is 0.");
            return null;
        }
        String[] language = new String[size];
        for (int i = 0; i < size; i++) {
            language[i] = mLanguageList.get(i).mName;
        }

        return language;
    }

    /**
     * Get Current Language.
     * 
     * @return the current Language
     */
    public int getCurrentLanguage() {
        return mCurrentLanguageIndex;
    }

    /**
     * Get Current Language ID defined in voicelanguage.xml.
     * 
     * @return the current Language ID
     */
    public int getCurrentLanguageID() {
        return mCurrentLanguageIndex < 0 ? mCurrentLanguageIndex : mLanguageList
                .get(mCurrentLanguageIndex).mLanguageID;
    }

    /**
     * Get Current Wakeup Info defined in voiceweakup.xml.
     * 
     * @param mode
     *            wakeup mode
     * @return wakeup information
     */
    public VoiceWakeupInfo[] getCurrentWakeupInfo(int mode) {
        if (VoiceCommandListener.VOICE_WAKEUP_MODE_SPEAKER_INDEPENDENT == mode) {
            if (mWakeupAnyoneList == null) {
                return null;
            }
            return mWakeupAnyoneList.toArray(new VoiceWakeupInfo[mWakeupAnyoneList.size()]);
        }
        if (VoiceCommandListener.VOICE_WAKEUP_MODE_SPEAKER_DEPENDENT == mode) {
            if (mWakeupCommandList == null) {
                return null;
            }
            return mWakeupCommandList.toArray(new VoiceWakeupInfo[mWakeupCommandList.size()]);
        }
        return null;
    }

    /**
     * Get APP Label from package name and class name.
     * 
     * @param packageName
     *            package name
     * @param className
     *            class name
     * @return application label
     */
    public String getAppLabel(String packageName, String className) {

        ComponentName component = new ComponentName(packageName, className);
        ActivityInfo info;
        try {
            info = mContext.getPackageManager().getActivityInfo(component, 0);
        } catch (NameNotFoundException e) {
            Log.e(TAG, "[getAppLabel] not found packageName: " + packageName);
            return null;
        }
        CharSequence name = info.loadLabel(mContext.getPackageManager());
        String appLabel = name.toString();
        Log.i(TAG, "[getAppLabel] appLabel:" + appLabel);

        return appLabel;
    }

    public void updateCurLanguageKeyword() {
        synchronized (mVoiceKeyWordInfos) {
            mVoiceKeyWordInfos.clear();
            if (mCurrentLanguageIndex >= 0) {
                mConfigurationXml.readKeyWordFromXml(mVoiceKeyWordInfos,
                        mLanguageList.get(mCurrentLanguageIndex).mFilePath);
            }
        }
    }

    /**
     * Set Current Language.
     * 
     * @param languageIndex
     *            the current language index
     */
    public void setCurrentLanguage(int languageIndex) {
        mCurrentLanguageIndex = languageIndex;
        updateCurLanguageToPref();
    }

    /**
     * Get process ID defined in voiceprocessinfo.xml.
     * 
     * @param featurename
     *            application feature name
     * 
     * @return process ID
     */
    public int getProcessID(String featurename) {
        VoiceProcessInfo processinfo = mVoiceProcessInfos.get(featurename);
        if (processinfo == null) {
            return -1;
        }

        return processinfo.mID;
    }

    /**
     * Check process is enable or not.
     * 
     * @param featurename
     *            application feature name
     * @return true if the feature name is enable
     */
    public boolean isProcessEnable(String featurename) {
        VoiceProcessInfo processinfo = mVoiceProcessInfos.get(featurename);
        if (processinfo == null) {
            return false;
        }
        return processinfo.mIsVoiceEnable;
    }

    /**
     * Get VoiceProcessInfo from processname.
     * 
     * @param processname
     *            process name
     * @return VoiceProcessInfo
     */
    public VoiceProcessInfo getProcessInfo(String processname) {
        return mVoiceProcessInfos.get(processname);
    }

    /**
     * Get Voice Recognition Pattern File Path from data/data.
     * 
     * @param mode
     *            Voice Wakeup mode
     * @return data/data/com.mediatek.voicecommand/training/***
     */
    public String getVoiceRecognitionPatternFilePath(int mode) {
        if (VoiceCommandListener.VOICE_WAKEUP_MODE_SPEAKER_INDEPENDENT == mode) {
            return mPaths.get(mAnyoneRecogPatternPath);
        }
        if (VoiceCommandListener.VOICE_WAKEUP_MODE_SPEAKER_DEPENDENT == mode) {
            return mPaths.get(mCommandRecogPatternPath);
        }
        return mPaths.get(mUnlockRecogPatternPath);
    }

    /**
     * Get Feature File Path from data/data.
     * 
     * @param mode
     *            Voice Wakeup mode
     * @return data/data/com.mediatek.voicecommand/training/***
     */
    public String getFeatureFilePath(int mode) {
        if (VoiceCommandListener.VOICE_WAKEUP_MODE_SPEAKER_INDEPENDENT == mode) {
            return mPaths.get(mAnyoneFeatureFilePath);
        }
        if (VoiceCommandListener.VOICE_WAKEUP_MODE_SPEAKER_DEPENDENT == mode) {
            return mPaths.get(mCommandFeatureFilePath);
        }
        return mPaths.get(mUnlockFeatureFilePath);
    }

    /**
     * Get Password File Path from data/data.
     * 
     * @param mode
     *            Voice Wakeup mode
     * @return data/data/com.mediatek.voicecommand/training/***
     */
    public String getPasswordFilePath(int mode) {

        if (VoiceCommandListener.VOICE_WAKEUP_MODE_SPEAKER_INDEPENDENT == mode) {
            return mPaths.get(mAnyonePswdFilePath);
        }
        if (VoiceCommandListener.VOICE_WAKEUP_MODE_SPEAKER_DEPENDENT == mode) {
            return mPaths.get(mCommandPswdFilePath);
        }
        return mPaths.get(mUnlockPswdFilePath);
    }

    /**
     * Get Voice Wakeup info Path from data/data/com.mediatek.voicecommand.
     * 
     * @return Voice Wakeup info Path
     */
    public String getWakeupInfoPath() {
        return mPaths.get(mWakeupInfoFilePath);
    }

    /**
     * Get Voice Wakeup mode from setting provider.
     * 
     * @return Voice Wakeup mode
     */
    public int getWakeupMode() {
        return mWakeupMode;
    }

    /**
     * Set Voice Wakeup mode to setting provider.
     * 
     * @param mode
     *            Voice Wakeup mode
     */
    public void setWakeupMode(int mode) {
        mWakeupMode = mode;
    }

    /**
     * Get Voice Wakeup Status from setting provider.
     * 
     * @return Voice Wakeup Status
     */
    public int getWakeupCmdStatus() {
        return mWakeupStatus;
    }

    /**
     * Set Voice Wakeup Status to setting provider.
     * 
     * @param wakeupStatus
     *            Voice Wakeup Status
     */
    public void setWakeupStatus(int wakeupStatus) {
        mWakeupStatus = wakeupStatus;
    }

    /**
     * Get VoiceUI Pattern Path from voicecommandpath.xml.
     * 
     * @return VoiceUI Pattern Path
     */
    public String getVoiceUIPatternPath() {
        return mPaths.get(mUiPatternPath);
    }

    /**
     * Get Contacts db File Path from voicecommandpath.xml.
     * 
     * @return Contacts db File Path
     */
    public String getContactsdbFilePath() {
        return mPaths.get(mContactsdbFilePath);
    }

    /**
     * Get UBM File Path from voicecommandpath.xml.
     * 
     * @return UBM File Path
     */
    public String getUbmFilePath() {
        return mPaths.get(mUbmFilePath);
    }

    /**
     * Get Model File Path from voicecommandpath.xml.
     * 
     * @return Model File Path
     */
    public String getModelFile() {
        return mPaths.get(mModeFilePath);
    }

    /**
     * Get Contacts Model File Path from voicecommandpath.xml.
     * 
     * @return Contacts Model File Path
     */
    public String getContactsModelFile() {
        return mPaths.get(mContactsModeFilePath);
    }

    /**
     * Get KeyWord refer to processName defined in keyword.xml.
     * 
     * @param processName
     *            feature name
     * @return KeyWord list
     */
    public String[] getKeyWord(String processName) {
        synchronized (mVoiceKeyWordInfos) {
            // return mVoiceKeyWordInfos.get(processName).mKeyWordArray;
            VoiceKeyWordInfo info = mVoiceKeyWordInfos.get(processName);
            if (info == null) {
                Log.w(TAG, "[getKeyWord]info is null.1");
                return null;
            }
            return info.mKeyWordArray;
        }
    }

    /**
     * Get command path refer to processName and id defined in keyword.xml.
     * 
     * @param processName
     *            feature name
     * 
     * @return command path
     */
    public String getCommandPath(String processName) {
        synchronized (mVoiceKeyWordInfos) {
            // return mVoiceKeyWordInfos.get(processName).mKeyWordPath;
            VoiceKeyWordInfo info = mVoiceKeyWordInfos.get(processName);
            if (info == null) {
                Log.w(TAG, "[getCommandPath]info is null.1");
                return null;
            }
            return info.mKeyWordPath;
        }
    }

    /**
     * Get KeyWord refer to processName defined in keyword.xml.
     * 
     * @param processName
     *            feature name
     * @return KeyWord list
     */
    public String[] getKeyWordForSettings(String processName) {
        synchronized (mVoiceKeyWordInfos) {
            VoiceProcessInfo info = mVoiceProcessInfos.get(processName);
            if (info != null && info.mRelationProcessName != null) {
                ArrayList<String> keywordList = new ArrayList<String>();
                // String[] keyword =
                // mVoiceKeyWordInfos.get(processName).mKeyWordArray;
                VoiceKeyWordInfo keyWordInfo = mVoiceKeyWordInfos.get(processName);
                if (keyWordInfo == null) {
                    Log.w(TAG, "[getKeyWordForSettings]keyWordInfo is null.1");
                    return null;
                }
                String[] keyword = keyWordInfo.mKeyWordArray;
                if (keyword != null) {
                    for (int i = 0; i < keyword.length; i++) {
                        keywordList.add(keyword[i]);
                    }
                }
                // String[] keywordRelation =
                // mVoiceKeyWordInfos.get(info.mRelationProcessName).mKeyWordArray;
                keyWordInfo = mVoiceKeyWordInfos.get(info.mRelationProcessName);
                if (keyWordInfo == null) {
                    Log.w(TAG, "[getKeyWordForSettings]keyWordInfo is null.2");
                    return null;
                }
                String[] keywordRelation = keyWordInfo.mKeyWordArray;
                if (keywordRelation != null) {
                    for (int i = 0; i < keywordRelation.length; i++) {
                        if (!keywordList.contains(keywordRelation[i])) {
                            keywordList.add(keywordRelation[i]);
                        }
                    }
                }

                return keywordList.toArray(new String[keywordList.size()]);
            } else {
                // return mVoiceKeyWordInfos.get(processName).mKeyWordArray;
                VoiceKeyWordInfo keyWordInfo = mVoiceKeyWordInfos.get(processName);
                if (keyWordInfo == null) {
                    Log.w(TAG, "[getKeyWordForSettings]keyWordInfo is null.3");
                    return null;
                }
                return keyWordInfo.mKeyWordArray;
            }
        }
    }

    /**
     * Get Process Name list according to featureName.
     * 
     * @param featureName
     *            feature name
     * @return process name list
     */
    public ArrayList<String> getProcessName(String featureName) {
        VoiceProcessInfo processinfo = mVoiceProcessInfos.get(featureName);
        if (processinfo == null) {
            return null;
        }
        return processinfo.mProcessNameList;
    }

    /**
     * ConfigurationManager release when VoiceCommandManagerService destroy.
     */
    public void release() {
        Log.i(TAG, "[release]mIsSystemLanguage : " + mIsSystemLanguage);
        if (mIsSystemLanguage) {
            mContext.unregisterReceiver(mSystemLanguageReceiver);
            sCfgMgr = null;
        }
    }

    /**
     * Create dir and file.
     * 
     * @param path
     *            file path
     * @return true if make dir success
     */
    public static boolean makeDirForPath(String path) {
        if (path == null) {
            Log.w(TAG, "[makeDirForPath]path is null");
            return false;
        }
        try {
            File dir = new File(path);
            if (!dir.exists()) {
                dir.mkdirs();
                FileUtils.setPermissions(dir.getPath(), 0775, -1, -1); // dwxrwxr-x
            }
        } catch (NullPointerException ex) {
            Log.e(TAG, "[makeDirForPath]NullPointerException.");
            return false;
        }
        return true;
    }

    /**
     * Create dir and file.
     * 
     * @param file
     *            file which want to be made dir
     * @return true if make dir success
     */
    public static boolean makeDirForFile(String file) {
        if (file == null) {
            Log.w(TAG, "[makeDirForFile]file is null");
            return false;
        }
        try {
            File f = new File(file);
            File dir = f.getParentFile();
            if (!dir.exists()) {
                dir.mkdirs();
                FileUtils.setPermissions(dir.getPath(), 0775, -1, -1); // dwxrwxr-x
            }
            FileUtils.setPermissions(f.getPath(), 0666, -1, -1); // -rw-rw-rw-
        } catch (NullPointerException ex) {
            Log.e(TAG, "[makeDirForFile]NullPointerException.");
            return false;
        }

        return true;
    }

    /**
     * Read Pref in mVoiceProcessInfos and mCurrentLanguageIndex.
     */
    private void checkVoiceCachePref() {
        SharedPreferences processPref = mContext.getSharedPreferences(mVoiceUiCacheFile,
                Context.MODE_PRIVATE);
        Map<String, Boolean> enableMap;
        try {
            enableMap = (Map<String, Boolean>) processPref.getAll();
        } catch (NullPointerException ex) {
            Log.e(TAG, "[checkVoiceCachePref] NullPointerException.");
            enableMap = null;
        }

        if (enableMap != null) {
            Iterator iter = enableMap.entrySet().iterator();
            while (iter.hasNext()) {
                Entry entry = (Entry) iter.next();
                String name = (String) entry.getKey();
                VoiceProcessInfo info = mVoiceProcessInfos.get(name);
                if (info != null) {
                    info.mIsVoiceEnable = (Boolean) entry.getValue();
                    if (info.mRelationProcessName != null) {
                        VoiceProcessInfo relationInfo = mVoiceProcessInfos
                                .get(info.mRelationProcessName);
                        if (relationInfo != null) {
                            relationInfo.mIsVoiceEnable = info.mIsVoiceEnable;
                        }
                    }
                }
            }
        }

        if (mIsSystemLanguage) {
            useSystemLanguage();
        } else {
            processPref = mContext.getSharedPreferences(mVoiceLanguageCacheFile,
                    Context.MODE_PRIVATE);
            mCurrentLanguageIndex = processPref.getInt(mCurLanguageKey, mCurrentLanguageIndex);
        }
        Log.d(TAG, "[checkVoiceCachePref]mCurrentLanguageIndex = " + mCurrentLanguageIndex);
    }

    /**
     * Receive Local change broadcast.
     */
    private BroadcastReceiver mSystemLanguageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.i(TAG, "[onReceive]...");
            if ((Intent.ACTION_LOCALE_CHANGED).equals(intent.getAction())) {
                String systemLanguage = Locale.getDefault().getLanguage() + "-"
                        + Locale.getDefault().getCountry();
                Log.d(TAG, "[onReceive]mSystemLanguageReceiver systemLanguage : " + systemLanguage);
                updateCurLanguageIndex(systemLanguage);
                updateCurLanguageKeyword();
            }
        }
    };

    /**
     * Save Current Language to Shared Preferences.
     */
    private void updateCurLanguageToPref() {
        Log.d(TAG, "[updateCurLanguageToPref]...");
        synchronized (mLanguageList) {
            SharedPreferences processPref = mContext.getSharedPreferences(mVoiceLanguageCacheFile,
                    Context.MODE_PRIVATE);
            processPref.edit().putInt(mCurLanguageKey, mCurrentLanguageIndex).apply();
        }
        updateCurLanguageKeyword();
    }

    /**
     * Save Feature enable to Shared Preferences.
     */
    private void updateFeatureListEnableToPref() {
        Log.d(TAG, "[updateFeatureListEnableToPref]...");
        synchronized (mVoiceProcessInfos) {
            SharedPreferences processPref = mContext.getSharedPreferences(mVoiceUiCacheFile,
                    Context.MODE_PRIVATE);
            for (String featurename : mVoiceUiFeatureNames) {
                VoiceProcessInfo info = mVoiceProcessInfos.get(featurename);
                if (info != null) {
                    processPref.edit().putBoolean(featurename, info.mIsVoiceEnable).apply();
                }
            }
        }
    }
}