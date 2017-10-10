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

#define LOG_TAG "DDPDecoder"
#include <utils/Log.h>

#include "DDPDecoder.h"
#include "ddpdec_client.h"
#include "udc_api.h"

#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>
#include <cutils/properties.h>

/* DDPDEC_OUT_BUF_SIZE is a fixed value defining the number of bytes of PCM
 * data that will be written on a successful frame decode, assuming a maximal
 * number of decoded channels
 *
 *IF this is a 2-channel only system, and multichannel PCM will never be
 *required, then the number of output chans to create a buffer for can be
 *reduced to 2 by replacing MAX_PCM_CHANS with 2. MAX_PCM_CHANS will
 *enumerate to 8.
 */
static const int DDPDEC_OUT_BUF_SIZE = GBL_BLKSIZE * GBL_MAXBLOCKS * DEC_MAXPCMCHANS * GBL_BYTESPERWRD;

/*
 * The MAXFRAMESINOUTPUTBUFFER value defines the maximum number of audio
 * frames that there will be room for in the output PCM buffer. This must be
 * bounded by the size of the AudioTrack's buffer.
 *
 * Where buffer->size() = DDPDEC_OUT_BUF_SIZE * MAXFRAMESINOUTPUTBUFFER
 */
static const int MAXFRAMESINOUTPUTBUFFER = 3;

/* FRAMERBUFFERSIZE is defined by assuming that the maximum amount of data that
 * could be carried over to the next buffer read cycle is 1 frame. It is set
 * to 2 frames for safety. This value is the
 * 2 * ( size of DD+ word (larger than DD) * num. bytes/word)
 */
static const int FRAMERBUFFERSIZE = 2* (GBL_MAXDDPFRMWRDS * GBL_BYTESPERWRD);

static char cEndpointProp[] = "dolby.audio.sink.info";
static char cDsStateProp[] = "dolby.ds.state";

// The endpoint name definition. The definition is same as the possible value of system property "dolby.audio.sink.info"
const char* endpoints[] = {
    "hdmi2",
    "hdmi6",
    "hdmi8",
    "headset",
    "speaker",
//    "bluetooth",//or other supported audio output device
    "invalid" //this is the default value we will get if not set
};

// The downmix configuration definition for each device.
const int EndpointConfig[][4] = {
    // downmix mode(Ds off)             downmix mode(Ds on)               DRC setting                                              max output channel
    {DDP_OUTMIX_LORO,           DDP_OUTMIX_LTRT,            DDPI_UDC_COMP_LINE,                                   2                    },     // hdmi2
    {DDP_OUTMIX_STREAM,      DDP_OUTMIX_STREAM,        DDPI_UDC_COMP_LINE,                               6                    },     // hdmi6
    {DDP_OUTMIX_STREAM,      DDP_OUTMIX_STREAM,        DDPI_UDC_COMP_LINE,                               8                    },     // hdmi8
    {DDP_OUTMIX_LORO,            DDP_OUTMIX_LTRT,            DDPI_UDC_COMP_PORTABLE_L14,             2                    },     // headset
#ifdef DOLBY_MONO_SPEAKER
    {DDP_OUTMIX_LORO,           DDP_OUTMIX_LORO,            DDPI_UDC_COMP_PORTABLE_L14,             2                    },     // speaker
#else
    {DDP_OUTMIX_LORO,            DDP_OUTMIX_LTRT,            DDPI_UDC_COMP_PORTABLE_L14,             2                    },     // speaker
#endif
//    {DDP_OUTMIX_LORO,            DDP_OUTMIX_LTRT,            DDPI_UDC_COMP_PORTABLE_L14,             2                    },     //bluetooth or other
    {DDP_OUTMIX_LORO,            DDP_OUTMIX_LTRT,            DDPI_UDC_COMP_PORTABLE_L14,             2                    },     //invalid/default
};

namespace android {

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

DDPDecoder::DDPDecoder(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component)
    : SimpleSoftOMXComponent(name, callbacks, appData, component),
      mConfig(new tDdpDecoderExternal),
      mDecoder(NULL),
      mAnchorTimeUs(0),
      mNumFramesOutput(0),
      mLastChannelCount(0),
      mLastSamplingRate(0),
      mLastDRCmode(DDPI_UDC_COMP_PORTABLE_L14),
      mPrepareOutputPortChange(false),
      mIsEC3(false),
      mSignalledError(false),
      mStarted(false),
      mFadeIn(false),
      mOutputPortSettingsChange(NONE),
      mLastMediaTimeUs(0),
      mLastAdjustedTimeUs(0){

      init(name);
      initPorts();
}

void DDPDecoder::init(const char *name) {
    ALOG1("-> init");

    if (!strcmp(name, "OMX.dolby.ec3.decoder")) {
        mIsEC3 = true;
    } else {
        CHECK(!strcmp(name, "OMX.dolby.ac3.decoder"));
    }

    memset(mConfig, 0, sizeof(tDdpDecoderExternal));
    mConfig->nrChans = -1;
    mConfig->nPCMOutMaxChannels = -1;

    mConfig->nActiveDsState = -1;
    mConfig->nActiveEndpoint = -1;

    ALOG1("<- init");
}

DDPDecoder::~DDPDecoder() {
    ALOG1("-> ~DDPDecoder");

    if (mStarted) {
        stop();
    }

    delete mConfig;
    mConfig = NULL;

    ALOG1("<- ~DDPDecoder");
}

void DDPDecoder::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    def.nPortIndex = 0;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumInputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = FRAMERBUFFERSIZE;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainAudio;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.audio.cMIMEType =
        const_cast<char *>(
                mIsEC3
                    ? MEDIA_MIMETYPE_AUDIO_EAC3
                    : MEDIA_MIMETYPE_AUDIO_AC3);

    def.format.audio.pNativeRender = NULL;
    def.format.audio.bFlagErrorConcealment = OMX_FALSE;
    def.format.audio.eEncoding = OMX_AUDIO_CodingDDP;

    addPort(def);

    def.nPortIndex = 1;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumOutputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;

    def.nBufferSize = DDPDEC_OUT_BUF_SIZE * MAXFRAMESINOUTPUTBUFFER;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainAudio;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 2;

    def.format.audio.cMIMEType = const_cast<char *>("audio/raw");
    def.format.audio.pNativeRender = NULL;
    def.format.audio.bFlagErrorConcealment = OMX_FALSE;
    def.format.audio.eEncoding = OMX_AUDIO_CodingPCM;

    addPort(def);
}

//Function to control the downmixer option in stagefright scope
status_t DDPDecoder::SetDownmixConfig(ddp_downmix_config_t config) {

    ALOG2(" SetDownmixConfig");

    switch (config)
    {
        case DDP_OUTMIX_AUTO:
        {
            ALOG2(" SetDownmixConfig->DDP_OUTMIX_AUTO");

            // In this case we want the internal downmixer and automatically selected
            // LoRo/LtRt/PLII downmix dependent on stream dmixmod parameter
            mConfig->downmixMode = DDPI_UDC_STEREOMODE_AUTO;
            mConfig->updatedChannelRouting = 1;
            break;
        }

        case DDP_OUTMIX_LTRT:
        {
            ALOG2(" SetDownmixConfig->DDP_OUTMIX_LTRT");

            // In this case we want the internal downmixer and always a
            // surround downmix
            mConfig->downmixMode = DDPI_UDC_STEREOMODE_SRND;
            mConfig->updatedChannelRouting = 1;
            break;
        }

        case DDP_OUTMIX_LORO:
        {
            ALOG2(" SetDownmixConfig->DDP_OUTMIX_LORO");

            // In this case we want the internal downmixer and always an LoRo downmix
            mConfig->downmixMode = DDPI_UDC_STEREOMODE_STEREO;
            mConfig->updatedChannelRouting = 1;
            break;
        }

        case DDP_OUTMIX_STREAM:
        {
            ALOG2(" SetDownmixConfig->DDP_OUTMIX_STREAM");

            // In this case we want N channel output and an external downmixer.
            // We don't care about the internal downmixer mode
            mConfig->downmixMode = DDPI_UDC_STEREOMODE_AUTO;
            mConfig->updatedChannelRouting = 1;
            //portable mode is not applied
            break;
        }

        default:
        {
            ALOGE("-> SetDownmixMode: INVALID DOWNMIX MODE OPTION **** ");
            //just to be safe, make sure channel routing doesn't get changed
            mConfig->updatedChannelRouting = 0;
            return UNKNOWN_ERROR;
        }
    }
    return OK;
}

void DDPDecoder::setMultiChannelPCMOutDownmix() {

    /* The endpoint system property and DS state property will tell us the active endpoint and DS on/off status.
     * Firstly, acquire the current endpoint and current DS state. */
    char cEndpoint[PROPERTY_VALUE_MAX];
    char cDsState[PROPERTY_VALUE_MAX];

    property_get(cEndpointProp, cEndpoint, "invalid");
    property_get(cDsStateProp, cDsState, "invalid");

    ALOG3("Endpoint System Property = %s", cEndpoint);
    ALOG3("DS on/off state Property = %s", cDsState);

    const int nEndpoints = sizeof endpoints / sizeof *endpoints;
    int activeEndpoint = (nEndpoints - 1); //default to invalid
    for (int i = 0; i < nEndpoints; i++){
        if (strcmp(cEndpoint, endpoints[i]) == 0) {
            activeEndpoint = i;
        }
    }

    int activeDsState = 0;
    activeDsState = (strcmp(cDsState, "on") == 0) ? 1 : 0;

    //Check if this has changed from the last value
    //If so, set downmix/drc values
    if ((activeEndpoint != mConfig->nActiveEndpoint) || (activeDsState != mConfig->nActiveDsState)) {

        if (activeEndpoint == (nEndpoints - 1)) {
            ALOG2("Active Endpoint not defined - using default");
        }

        //Update stored values
        if (activeEndpoint != mConfig->nActiveEndpoint) {
            ALOG1("Active Endpoint has changed from %d to %d", mConfig->nActiveEndpoint, activeEndpoint);
            mConfig->nActiveEndpoint = activeEndpoint;
        }
        if (activeDsState != mConfig->nActiveDsState) {
            ALOG1("DS state has changed from %d to %d", mConfig->nActiveDsState, activeDsState);
            mConfig->nActiveDsState = activeDsState;
        }

        // We are expecting Audio_Tear_Down event to take care of closing the Decoder and re-opening it.
        // As of now there are some instances in which this is not happening (for example when HDMI is plugged-in and
        // Headphone is plugged out) during play-back.
        // The below logic makes sure that output channels are configured only during init time but not in run time as 
        // sending OMX_EventPortSettingsChanged during run time causes crash in OMXCodec.cpp due to 
        // Audio_Tear_Down event.
        if (!mStarted)
        {
            //set max number of output channels
            mConfig->nPCMOutMaxChannels = EndpointConfig[activeEndpoint][3];
            ALOG2("Endpoint Switched - Setting Max Output Channels to %d", mConfig->nPCMOutMaxChannels);
        }

        //set drc mode
        mConfig->drcMode = EndpointConfig[activeEndpoint][2];
        ALOG2("Endpoint Switched - Setting DRC Mode to %d", mConfig->drcMode);

        //set desired downmix config
        SetDownmixConfig((ddp_downmix_config_t)((activeDsState == 1) ? EndpointConfig[activeEndpoint][1] : EndpointConfig[activeEndpoint][0]));
        ALOG2("Endpoint Switched - Setting Downmix Config to %d", mConfig->downmixMode);

    }
}

status_t DDPDecoder::start() {
    ALOG1("-> start");

    CHECK(!mStarted);

    mConfig->inputBufferCurrentLength = 0;
    mConfig->inputBufferUsedLength    = 0;

    mDecoder = ddpdec_open(mConfig);
    if (mDecoder == NULL) {
        ALOGE("ddpdec_open() FAIL");
        return UNKNOWN_ERROR;
    }

    //Set the decoder configuration to its default
    //This must be done after the decoder is started as it
    //will reininitialise these values
    setMultiChannelPCMOutDownmix();

    mLastDRCmode = mConfig->drcMode;
    mAnchorTimeUs = 0;
    mNumFramesOutput = 0;
    mStarted = true;

    ALOG2("mStarted = %d", mStarted);
    ALOG1("<- start");

    return OK;
}

status_t DDPDecoder::stop() {

    //shutdown cleanly
    ALOG1("-> stop");

    CHECK(mStarted);

    mStarted = false;

    ddpdec_close(mConfig, mDecoder);
    mDecoder = NULL;

    ALOG1("<- stop");

    return OK;
}

OMX_ERRORTYPE DDPDecoder::internalGetParameter(
        OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
        case OMX_IndexParamAudioPcm:
        {
            OMX_AUDIO_PARAM_PCMMODETYPE *pcmParams =
                (OMX_AUDIO_PARAM_PCMMODETYPE *)params;

            if (pcmParams->nPortIndex > 1) {
                return OMX_ErrorUndefined;
            }

            pcmParams->eNumData = OMX_NumericalDataSigned;
            pcmParams->eEndian = OMX_EndianBig;
            pcmParams->bInterleaved = OMX_TRUE;
            pcmParams->nBitPerSample = 16;
            pcmParams->ePCMMode = OMX_AUDIO_PCMModeLinear;

            pcmParams->eChannelMapping[0] = OMX_AUDIO_ChannelLF;
            pcmParams->eChannelMapping[1] = OMX_AUDIO_ChannelRF;
            pcmParams->eChannelMapping[2] = OMX_AUDIO_ChannelCF;
            pcmParams->eChannelMapping[3] = OMX_AUDIO_ChannelLFE;
            pcmParams->eChannelMapping[4] = OMX_AUDIO_ChannelLS;
            pcmParams->eChannelMapping[5] = OMX_AUDIO_ChannelRS;
            pcmParams->eChannelMapping[6] = OMX_AUDIO_ChannelLR;
            pcmParams->eChannelMapping[7] = OMX_AUDIO_ChannelRR;

            if (!mStarted) {
                pcmParams->nChannels = 2;
                pcmParams->nSamplingRate = 48000;
                //Also set the 'last' parameters to the same value so we don't immediatley ask to reconfigure
                //the port after startup
                mLastSamplingRate = pcmParams->nSamplingRate;
                mLastChannelCount = pcmParams->nChannels;

            } else {
                pcmParams->nChannels = mConfig->nrChans;
                pcmParams->nSamplingRate = mConfig->samplingRate;
            }

            return OMX_ErrorNone;
        }

        default:
            return SimpleSoftOMXComponent::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE DDPDecoder::internalSetParameter(
        OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
        case OMX_IndexParamStandardComponentRole:
        {
            const OMX_PARAM_COMPONENTROLETYPE *roleParams =
                (const OMX_PARAM_COMPONENTROLETYPE *)params;

            if (strncmp((const char *)roleParams->cRole,
                    mIsEC3 ? "audio_decoder.ec3" : "audio_decoder.ac3",
                    OMX_MAX_STRINGNAME_SIZE - 1)) {
                return OMX_ErrorUndefined;
            }

            return OMX_ErrorNone;
        }

        case OMX_IndexParamAudioPcm:
        {
            const OMX_AUDIO_PARAM_PCMMODETYPE *pcmParams =
                (OMX_AUDIO_PARAM_PCMMODETYPE *)params;

            if (pcmParams->nPortIndex != 1) {
                return OMX_ErrorUndefined;
            }

            if (pcmParams->nChannels < 1 || pcmParams->nChannels > 8) {
                return OMX_ErrorUndefined;
            }

            return OMX_ErrorNone;
        }

        default:
            return SimpleSoftOMXComponent::internalSetParameter(index, params);
    }
}

void DDPDecoder::onQueueFilled(OMX_U32 portIndex) {

    ALOG3("-> onQueueFilled (Port %d)", (uint32_t) portIndex);

    if (mSignalledError || mOutputPortSettingsChange != NONE) {
        ALOGW("<- onQueueFilled: mSignalledError=%d mOutputPortSettingsChange=%d", mSignalledError, mOutputPortSettingsChange);
        return;
    }

    List<BufferInfo *> &inQueue = getPortQueue(0);
    List<BufferInfo *> &outQueue = getPortQueue(1);

    ALOG3("inQueue: %d items, outQueue: %d items", inQueue.size(), outQueue.size());

    if (!mStarted) {
        status_t err = start();
        if (err != OK) {
            mSignalledError = true;
            notify(OMX_EventError, OMX_ErrorUndefined, err, NULL);
            return;
        }
    }

    while (!inQueue.empty() && !outQueue.empty()) {

        BufferInfo *inInfo = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;

        BufferInfo *outInfo = *outQueue.begin();
        OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;

        if (mPrepareOutputPortChange) {
            // Check if all the output buffers have been consumed.
            // If not, we'll hold onto the input buffer and will decode
            // it once the output port has been reconfigured.
            if (outQueue.size() == kNumOutputBuffers) {
                ALOG2("output parameter will be reconfigured");

                mPrepareOutputPortChange = false;
                notify(OMX_EventPortSettingsChanged, 1, 0, NULL);
                mOutputPortSettingsChange = AWAITING_DISABLED;
            }
            return;
        }

        if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {

            ALOG3("End of input stream");

            // END OF STREAM
            inQueue.erase(inQueue.begin());
            inInfo->mOwnedByUs = false;
            notifyEmptyBufferDone(inHeader);

            outHeader->nFilledLen = 0;
            outHeader->nFlags = OMX_BUFFERFLAG_EOS;

            outQueue.erase(outQueue.begin());
            outInfo->mOwnedByUs = false;
            notifyFillBufferDone(outHeader);
            return;
        }

        ALOG3("-> Decode loop");
        ALOG3("inHeader nOffset=%d nFilledLen=%d nTimeStamp=%lld    outHeader nOffset=%d nAllocLen=%d", (int) inHeader->nOffset, (int) inHeader->nFilledLen, inHeader->nTimeStamp, (int) outHeader->nOffset, (int) outHeader->nAllocLen);

        if (inHeader->nOffset == 0) {
            mAnchorTimeUs = inHeader->nTimeStamp;
            mNumFramesOutput = 0;
            outHeader->nTimeStamp = mAnchorTimeUs;
        }

        //wrap read data in MediaBuffer object
        MediaBuffer *decoderInputBuffer = new MediaBuffer(inHeader->pBuffer + inHeader->nOffset, inHeader->nFilledLen);
        if (decoderInputBuffer == NULL) {
            ALOG2("Invalid Read");

            // END OF STREAM
            inQueue.erase(inQueue.begin());
            inInfo->mOwnedByUs = false;
            notifyEmptyBufferDone(inHeader);

            outHeader->nFilledLen = 0;
            outHeader->nFlags = OMX_BUFFERFLAG_EOS;

            outQueue.erase(outQueue.begin());
            outInfo->mOwnedByUs = false;
            notifyFillBufferDone(outHeader);
            return;
        }

        ALOG3("Read into decoderInputBuffer");
        ALOG3("post read decoderInputBuffer->range_length() = %d",decoderInputBuffer->range_length() );
        ALOG3("post read decoderInputBuffer->range_offset() = %d",decoderInputBuffer->range_offset() );
        ALOG3("post read decoderInputBuffer->size() = %d",decoderInputBuffer->size() );

        //wrap the output buffer in MediaBuffer object
        MediaBuffer *decoderOutputBuffer = new MediaBuffer(outHeader->pBuffer + outHeader->nOffset, outHeader->nAllocLen);
        if (decoderOutputBuffer == NULL)
        {
            ALOG2("Invalid Write");
            decoderInputBuffer->release();
            decoderInputBuffer = NULL;
            return;
        }
        //make sure the buffer range is set empty
        decoderOutputBuffer->set_range(0, 0);

        //Main process/decode loop
        while ((decoderInputBuffer->range_length() != 0)
            && ((decoderOutputBuffer->size() - decoderOutputBuffer->range_length() - decoderOutputBuffer->range_offset()) >= (size_t) DDPDEC_OUT_BUF_SIZE))
        {

            ALOG3("-> Main decode loop");

            //First check to see if the output requirements for max number of channels
            //(or endpoint) has changed as this will necessitate a change in downmix mode and possible
            //output port reconfiguration
            setMultiChannelPCMOutDownmix();

            //(re)assign /update the input buffer to the decoder
            mConfig->pInputBuffer = (char *)decoderInputBuffer->data() + decoderInputBuffer->range_offset();
            mConfig->inputBufferCurrentLength = decoderInputBuffer->range_length();
            mConfig->inputBufferUsedLength = 0;
            //(re)assign the output buffer
            mConfig->pOutputBuffer = (char *)decoderOutputBuffer->data() + decoderOutputBuffer->range_offset() + decoderOutputBuffer->range_length();

            //Frame decoded here
            ERROR_CODE decoderErr = static_cast<ERROR_CODE>(ddpdec_process(mConfig, mDecoder));

            if (decoderErr == DDPAUDEC_SUCCESS) {

                // Check on the sampling rate and number of channels to see whether either have changed.
                if (mConfig->samplingRate != mLastSamplingRate) {
                    ALOGW("Sample rate changed - was %d Hz, is now is %d Hz", mLastSamplingRate, mConfig->samplingRate);
                    mLastSamplingRate = mConfig->samplingRate;
                    mPrepareOutputPortChange = true;
                }

                if (mConfig->nrChans != mLastChannelCount) {
                    ALOGW("Number of output channels changed -  was %d, is now %d", mLastChannelCount, mConfig->nrChans);
                    mLastChannelCount = mConfig->nrChans;
                    mPrepareOutputPortChange = true;
                }

                if (mFadeIn) {
                    fadeInBuffer((int16_t*)mConfig->pOutputBuffer, mConfig->nrChans, mConfig->frameLengthSamplesPerChannel);
                    mFadeIn = false;
                }
                if (mConfig->drcMode != mLastDRCmode){
                    ALOGW("DRC Mode changed -  was %d, is now %d", mLastDRCmode, mConfig->drcMode);
                    mLastDRCmode = mConfig->drcMode;
                    
                    // In our current implementation we are setting the new DRC value to decoder only after current frame set. Hence the 
                    // first frame after changing the DRC always contains the old value. We decided to do memset of zeros, as we dont want
                    // to see issue with A/V sync.
                    memset(mConfig->pOutputBuffer, 0, mConfig->nrChans * mConfig->frameLengthSamplesPerChannel * GBL_BYTESPERWRD);
                    mFadeIn = true;
                }

                if (mPrepareOutputPortChange) {
                    // if the size of outQueue is equal to the allocated output port buffer count,
                    // it means all the output buffers have been consumed and it is able to
                    // tirgger the port change event.
                    if (outQueue.size() == kNumOutputBuffers) {
                        ALOG2("output parameter will be reconfigured");

                        mPrepareOutputPortChange = false;
                        notify(OMX_EventPortSettingsChanged, 1, 0, NULL);
                        mOutputPortSettingsChange = AWAITING_DISABLED;

                        //remove reference to decoder input and out buffer
                        if (decoderInputBuffer) {
                            decoderInputBuffer->release();
                            decoderInputBuffer = NULL;
                        }
                        if (decoderOutputBuffer) {
                            decoderOutputBuffer->release();
                            decoderOutputBuffer = NULL;
                        }
                    }
                    return;
                }
            } else {
                mConfig->frameLengthSamplesPerChannel = 0;
                mConfig->samplingRate = mLastSamplingRate;
                mConfig->nrChans = mLastChannelCount;
            }

            int numOutBytes = mConfig->nrChans * mConfig->frameLengthSamplesPerChannel * GBL_BYTESPERWRD;

            ALOG3("mConfig->frameLengthSamplesPerChannel = %d",mConfig->frameLengthSamplesPerChannel);
            ALOG3("mConfig->sampling rate = %d", mConfig->samplingRate);
            ALOG3("mConfig->nrChans = %d ", mConfig->nrChans);
            ALOG3("numOutBytes: %d channels, %d samples/chan -> %d numOutBytes", mConfig->nrChans, mConfig->frameLengthSamplesPerChannel, numOutBytes);
            ALOG3("mConfig->inputBufferUsedLength = %d",  mConfig->inputBufferUsedLength);

            // update used space in input buffer
            decoderInputBuffer->set_range((decoderInputBuffer->range_offset() + mConfig->inputBufferUsedLength),(decoderInputBuffer->range_length() - mConfig->inputBufferUsedLength));
            // update used space in output buffer
            decoderOutputBuffer->set_range((decoderOutputBuffer->range_offset()), (decoderOutputBuffer->range_length() + numOutBytes));

            ALOG3("decoderInputBuffer->range_offset() = %d, decoderInputBuffer->range_length() = %d",decoderInputBuffer->range_offset(), decoderInputBuffer->range_length());
            ALOG3("decoderOutputBuffer->range_offset() = %d, decoderOutputBuffer->range_length() = %d",decoderOutputBuffer->range_offset(), decoderOutputBuffer->range_length());
            ALOG3("Space remaining in output buffer: %d", (decoderOutputBuffer->size() - decoderOutputBuffer->range_length()));

            mNumFramesOutput += (mConfig->frameLengthSamplesPerChannel * mConfig->nrChans);

            int64_t keytime = (mAnchorTimeUs + (mNumFramesOutput * 1000000) / (mConfig->samplingRate ? mConfig->samplingRate : 1));
            ALOG3("after meta data set");
            ALOG3("kKeyTime = %lld", keytime);
            ALOG3("manchortimeUs = %lld", mAnchorTimeUs);
            ALOG3("mNumFramesOutput = %lld", mNumFramesOutput);

            ALOG3("<- Main decode loop");
        } // end of while loop

        if (mLastMediaTimeUs != inHeader->nTimeStamp) {
            //reset adjustments
            mLastAdjustedTimeUs = 0;
            mLastMediaTimeUs = inHeader->nTimeStamp;
        } else {
            if (mConfig->samplingRate != 0) {
                int64_t delta = (mConfig->frameLengthSamplesPerChannel * 1000000) / mConfig->samplingRate;
                ALOG3("delta = %lld", delta);
                mLastAdjustedTimeUs += delta;
            } else {
                mLastAdjustedTimeUs = 0;
            }
        }
        ALOG3("mLastAdjustedTimeUs = %lld, mLastMediaTimeUs = %lld, wouldBeTime = %lld", mLastAdjustedTimeUs, mLastMediaTimeUs, (mLastMediaTimeUs + mLastAdjustedTimeUs) );

        //we used whole input buffer
        inHeader->nOffset += inHeader->nFilledLen;
        inHeader->nFilledLen = 0;
        inInfo->mOwnedByUs = false;
        inQueue.erase(inQueue.begin());
        inInfo = NULL;
        notifyEmptyBufferDone(inHeader);
        inHeader = NULL;

        //We're done with the input buffer, so release it
        if (decoderInputBuffer != NULL) {
            ALOG3("Release decoderInputBuffer");
            decoderInputBuffer->release();
            decoderInputBuffer = NULL;
        }

        if (decoderOutputBuffer != NULL) {
            if (decoderOutputBuffer->range_length() != 0) { 
                //update the output buffer header object
                outHeader->nFilledLen = decoderOutputBuffer->range_length();
                outHeader->nOffset = 0;
                outHeader->nFlags = 0;
                outHeader->nTimeStamp = mLastMediaTimeUs + mLastAdjustedTimeUs;
                outInfo->mOwnedByUs = false;
                outQueue.erase(outQueue.begin());
                outInfo = NULL;
                notifyFillBufferDone(outHeader);
                outHeader = NULL;
            }
            ALOG3("Release decoderOutputBuffer");
            decoderOutputBuffer->release();
            decoderOutputBuffer = NULL;
        }
    }

    ALOG3("<- onQueueFilled");
}

void DDPDecoder::onPortFlushCompleted(OMX_U32 portIndex) {
    ALOG3("onPortFlushCompleted(%d)", (int) portIndex);

    if (portIndex == 0) {
        //clear any existing input/remainder buffers
        mNumFramesOutput = 0;
    }
}

void DDPDecoder::onReset() {
    ALOG3("onReset() called");
}

void DDPDecoder::onPortEnableCompleted(OMX_U32 portIndex, bool enabled) {
    if (portIndex != 1) {
        return;
    }

    switch (mOutputPortSettingsChange) {
        case NONE:
            break;

        case AWAITING_DISABLED:
        {
            CHECK(!enabled);
            mOutputPortSettingsChange = AWAITING_ENABLED;
            break;
        }

        default:
        {
            CHECK_EQ((int)mOutputPortSettingsChange, (int)AWAITING_ENABLED);
            CHECK(enabled);
            mOutputPortSettingsChange = NONE;
            break;
        }
    }
}

void DDPDecoder::fadeInBuffer(int16_t* data, int nrChans, int frameLengthSamplesPerChannel)
{
    float fadeFactor = 0.0;
    float fadeStepSize = 1.0 / (mConfig->frameLengthSamplesPerChannel);
    int32_t index;
    ALOGV("Fade Implementation: Fading in buffer 0x%x of size = %d", data, nrChans * frameLengthSamplesPerChannel * GBL_BYTESPERWRD);
    for (int i = 0; i < mConfig->frameLengthSamplesPerChannel; i++)
    {
        for(int ch = 0; ch < mConfig->nrChans; ch++)
        {
            index = i * mConfig->nrChans + ch;
            data[index] = ((data[index] * fadeFactor) + 0.5);
        }
        fadeFactor += fadeStepSize;
    }
}
}  // namespace android

android::SoftOMXComponent *createSoftOMXComponent(
        const char *name, const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData, OMX_COMPONENTTYPE **component) {
    return new android::DDPDecoder(name, callbacks, appData, component);
}
