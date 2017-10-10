/*
 * Copyright (C) 2011-2014 MediaTek Inc.
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

package com.mediatek.widget;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.RadioButton;

import com.mediatek.widget.AccountItemView;
import com.mediatek.widget.CustomAccountRemoteViews.AccountInfo;
import com.mediatek.internal.R;

import java.util.List;

public class DefaultAccountPickerAdapter extends BaseAdapter {

    private Context mContext;
    private List<AccountInfo> mData;

    public DefaultAccountPickerAdapter(Context context) {
        mContext = context;
    }

    /**
     * set the data of the adapter.
     */
    void setItemData(List<AccountInfo> data) {
        if (mData != null) {
            mData.clear();
        }
        mData = data;
    }

    /**
     * set active status
     */
    void setActiveStatus(int position) {
        for(AccountInfo accountInfo : mData) {
            accountInfo.setActiveStatus(false);
        }
        mData.get(position).setActiveStatus(true);
    }

    @Override
    public int getCount() {
        return mData.size();
    }

    @Override
    public AccountInfo getItem(int position) {
        return mData.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder viewHolder;
        if (convertView == null) {
            LayoutInflater inflator = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            convertView = inflator.inflate(R.layout.default_account_picker_item, null);

            viewHolder = new ViewHolder();
            viewHolder.accountItem = (AccountItemView) convertView.findViewById(R.id.account_item);
            viewHolder.radioButton = (RadioButton) convertView.findViewById(R.id.account_radio);
            convertView.setTag(viewHolder);
        } else {
            viewHolder = (ViewHolder) convertView.getTag();
        }

        AccountInfo accountInfo = getItem(position);
        // set account info
        viewHolder.accountItem.setAccountIcon(accountInfo.getIconId());
        viewHolder.accountItem.setAccountName(accountInfo.getLabel());
        viewHolder.accountItem.setAccountNumber(accountInfo.getNumber());

        viewHolder.radioButton.setChecked(accountInfo.isActive());

        return convertView;
    }

    private class ViewHolder {
        AccountItemView accountItem;
        RadioButton radioButton;
    }
}