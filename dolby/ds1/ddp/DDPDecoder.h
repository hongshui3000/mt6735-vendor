/******************************************************************************
 *  This program is protected under international and U.S. copyright laws as
 *  an unpublished work. This program is confidential and proprietary to the
 *  copyright owners. Reproduction or disclosure, in whole or in part, or the
 *  production of derivative works therefrom without the express permission of
 *  the copyright owners is prohibited.
 *
 *                 Copyright (C) 2011-2013 by Dolby Laboratories,
 *                             All rights reserved.
 ******************************************************************************/
#ifndef DDP_DECODER_H
#define DDP_DECODER_H

#include "MediaSource.h"
#include "MetaData.h"
#include "SimpleSoftOMXComponent.h"

/* ENUMARTED DOWNMIX CONFIGURATIONS
 * We have 3 modes:
 * 1 - Internal DDP Downmixer - audo detect downmix
 * 2 - Internal DDP Downmixer - surround compatible downmix
 * 3 - Internal DDP Downmixer - stereo downmix
 * 4 - No up or downmix is performed.
 */
typedef enum ddp_downmix_config_t {
    DDP_OUTMIX_AUTO,
    DDP_OUTMIX_LTRT,
    DDP_OUTMIX_LORO,
    DDP_OUTMIX_STREAM
} ddp_downmix_config_t;

struct tDdpDecoderExternal;

namespace android {

struct MediaBufferGroup;

#define DS_UDC_MODE_CHANGE_FIX

struct DDPDecoder : public SimpleSoftOMXComponent {
    DDPDecoder(const char *name,const OMX_CALLBACKTYPE *callbacks,
            OMX_PTR appData,
            OMX_COMPONENTTYPE **component);

    virtual status_t start();
    virtual status_t stop();

    virtual status_t SetDownmixConfig(ddp_downmix_config_t config);

    virtual void setMultiChannelPCMOutDownmix();

protected:
    virtual ~DDPDecoder();

    virtual OMX_ERRORTYPE internalGetParameter(
            OMX_INDEXTYPE index, OMX_PTR params);

    virtual OMX_ERRORTYPE internalSetParameter(
            OMX_INDEXTYPE index, const OMX_PTR params);

    virtual void onQueueFilled(OMX_U32 portIndex);
    virtual void onPortFlushCompleted(OMX_U32 portIndex);
    virtual void onPortEnableCompleted(OMX_U32 portIndex, bool enabled);
    virtual void onReset();

private:
    enum {
        kNumInputBuffers = 4,
        kNumOutputBuffers = 4,
    };

    tDdpDecoderExternal *mConfig;

    void *mDecoder;

    int64_t mAnchorTimeUs;
    int64_t mNumFramesOutput;
    int mLastChannelCount;
    int mLastSamplingRate;
    int mLastDRCmode;
    bool mPrepareOutputPortChange;
    bool mIsEC3;
    bool mSignalledError;
    bool mStarted;
    bool mFadeIn;

    enum {
        NONE,
        AWAITING_DISABLED,
        AWAITING_ENABLED
    } mOutputPortSettingsChange;

	int64_t mLastMediaTimeUs;
	int64_t mLastAdjustedTimeUs;

    void init(const char *name);
    void initPorts();

    DISALLOW_EVIL_CONSTRUCTORS(DDPDecoder);
    
    void fadeInBuffer(int16_t* data, int nrChans, int frameLengthSamplesPerChannel);
    
};

}  // namespace android

#endif  /* DDP_DECODER_H */
