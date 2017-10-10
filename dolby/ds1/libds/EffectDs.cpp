/******************************************************************************
 *  This program is protected under international and U.S. copyright laws as
 *  an unpublished work. This program is confidential and proprietary to the
 *  copyright owners. Reproduction or disclosure, in whole or in part, or the
 *  production of derivative works therefrom without the express permission of
 *  the copyright owners is prohibited.
 *
 *               Copyright (C) 2011-2014 by Dolby Laboratories,
 *                             All rights reserved.
 ******************************************************************************/

#define LOG_TAG "EffectDs"

#if (DOLBY_DAP_LOG_LEVEL >= 1)
#define ALOG1(...) ALOGI(__VA_ARGS__)
#if (DOLBY_DAP_LOG_LEVEL >= 2)
#define ALOG2(...) ALOGD(__VA_ARGS__)
#if (DOLBY_DAP_LOG_LEVEL >= 3)
#define ALOG3(...) ALOGD(__VA_ARGS__)
#else
#define ALOG3(...)
#endif
#else
#define ALOG2(...)
#define ALOG3(...)
#endif
#else
#define ALOG1(...)
#define ALOG2(...)
#define ALOG3(...)
#endif

#include <cutils/log.h>
#ifdef DOLBY_DAP_OPENSLES_PREGAIN
#include <math.h>
#endif // DOLBY_DAP_OPENSLES_PREGAIN
#include <utils/String8.h>
#include <utils/KeyedVector.h>
#include "EffectDs.h"

//
// AK
//
#include "ak.h"
#include "Ds1ap.h"
#include "Ds1apBuffer.h"


#define AK_BLKSIZE             AK_BLOCK_SIZE  /**< Samples per block */
#define DEFAULT_EFFECT_SAMPLERATE      44100          /**< Initial sampling rate used for the DS effect.
                                                           This should match the native output sampling rate
                                                           being used by the device, to avoid an unnecessary
                                                           re-initialisation of the DS effect */
#define DEFAULT_NUM_IN_CHANNELS        2              /**< Number of input channels. */
#define DEFAULT_NUM_OUT_CHANNELS       2              /**< Number of output channels. */
#define NUM_MAX_CHANNELS               2
#define BYTES_PER_FRAME        (sizeof(IO_DATUM) * NUM_MAX_CHANNELS)
#define DS_GLOBAL_EFFECT_AUDIO_SESSIONID 0

//
// AudioEffect interface
//
#define CPU_LOAD_ARM9E  0                           // CPU load indication (in 0.1 MIPS, see EffectApi.h)
#define MEM_USAGE       0                           // Data Memory usage (in kB, see EffectApi.h)
#define EFFECT_NAME     "Ds"                        // human readable effect name, must have length < EFFECT_STRING_LEN_MAX
#define EFFECT_DEV      "Dolby Laboratories, Inc."  // human readable effect implementor name, must have length < EFFECT_STRING_LEN_MAX

const uint32_t uOne24 = 1 << 24;                    ///< @internal Value 1 in 8.24 fixed-point.

//
// The dB scale factor to convert dB value to 'preg' value.
//
const int DB_SCALE    = 16;

//#define AK_AUDIO_DUMP

#ifdef AK_AUDIO_DUMP
//
// Usage: Change the volume down to 0 to end the current audio dump session while music is playing.
// And then change the volume 2 clicks up to launch the new audio dump session during music playback.
// As a result, the previous dumped audio will be overwritten.
// The retrieved audio dump will be under /data folder, so first ensure /data folder is writable.
//
#define AUDIO_DUMP_VOL_MIN 0xC000
static FILE*    pFilePcmInputDump = NULL;
static FILE*    pFilePcmOutputDump = NULL;
static bool     bAudioDumpStopped = true;
static uint32_t curVolume = 0;
#endif // AK_AUDIO_DUMP

// This macro is for testing 32-bit DAP in a 16-bit audio system. 
// It converts 16-bit input audio to 32-bit so it can be processed by 32-bit DAP. 
// It then converts the 32-bit output audio from DAP to 16-bit for final output.
//#define DOLBY_DAP_32BIT_TEST_IN_16BIT_FRAMEWORK

///
/// The array length for those AK parameters that contains multiple elements.
///
static const int AK_PARAM_VER_ARRAY_LEN  = 4,
                 AK_PARAM_LCVD_ARRAY_LEN = 2;

///
/// The endpoints supported by AK.
///
static const int DS1_ENDP_INT_SPEAKERS = 0,
                 DS1_ENDP_EXT_SPEAKERS = 1,
                 DS1_ENDP_HEADPHONES   = 2,
                 DS1_ENDP_HDMI         = 3,
                 DS1_ENDP_SPDIF        = 4,
                 DS1_ENDP_DLNA         = 5,
                 DS1_ENDP_ANALOG       = 6;

///
/// The column indices used for parameters in DS1_PARAMS_PER_ENDP table
///
static const int DS1_PARAM_DVLO   = 0,
                 DS1_PARAM_DVLI   = 1,
                 DS1_PARAM_VMB    = 2,
                 DS1_PARAMS_TOTAL = 3;
///
/// Array of dap parameter values for every supported endpoint
///
static const int DS1_PARAMS_PER_ENDP[][DS1_PARAMS_TOTAL] = {
    // dvlo    //dvli    //vmb
    {-320,    -320,    144},  // DS1_ENDP_INT_SPEAKERS
    {-320,    -320,    144},  // DS1_ENDP_EXT_SPEAKERS
    {-320,    -320,    144},  // DS1_ENDP_HEADPHONES
        {-320,    -320,    144},  // DS1_ENDP_HDMI
    //{-496,    -496,    0},    // DS1_ENDP_HDMI
    {-320,    -320,    144},  // DS1_ENDP_SPDIF    -- Not Used
    {-320,    -320,    144},  // DS1_ENDP_DLNA     -- Not Used
    {-320     -320,    144},  // DS1_ENDP_ANALOG   -- Not Used
};

// effect_interface_t interface implementation for DS effect
extern "C" const struct effect_interface_s gDsInterface;

namespace android {
namespace {

///
/// Ds AudioEffect descriptor.
///
const effect_descriptor_t gDsDescriptor =
{
    //These UUIDs were generated randomely as per the OpenSL-ES specification
    EFFECT_SL_IID_DS_, ///< Type-UUID for Ds.
    EFFECT_UUID_DS,   ///< UUID for Ds.
    EFFECT_CONTROL_API_VERSION,
    EFFECT_FLAG_TYPE_INSERT |
    EFFECT_FLAG_INSERT_FIRST |  ///< If EFFECT_FLAG_INSERT_EXCLUSIVE is used, CTS may fail.
    EFFECT_FLAG_VOLUME_IND |    ///< Request volume indication.
    EFFECT_FLAG_DEVICE_IND |    ///< Request indication of output device.
    EFFECT_FLAG_AUDIO_MODE_IND, ///< Request indication of audio mode (phone state ringing, in a call, or idle).
    CPU_LOAD_ARM9E,
    MEM_USAGE,
    EFFECT_NAME,
    EFFECT_DEV
};

///
/// gDescriptors is an array of all defined effect descriptors in this library.
///
static const effect_descriptor_t *const gDescriptors[] =
{
    &gDsDescriptor,
};

///
/// Contains the state or context of an instance of this effect.
///
/// The first attribute is the interface pointer to this effect's implementation,
/// as specified in EffectAPI.h.
/// The rest of the struct contains other useful state information for this instance.
///
struct EffectContext
{
    const struct effect_interface_s *itfe;      ///< Interface specified in EffectAPI.h.
    effect_config_t                 config;     ///< @internal Config set by EFFECT_CMD_SET_CONFIG command.

    Ds1ap             *pDs1ap;                  ///< @internal AK instance for this effect instance.
#ifdef USE_AKBUFFER
    void              *pDs1apBuffer;            ///< @internal AK buffer instance for this effect instance.
#endif // USE_AKBUFFER

    int audioSessionId;                         ///< @internal Audio session id for this effect instance.
                                                ///  0 if global, otherwise track/lpa

    audio_mode_t      audioMode;                ///< @internal Current audio mode as reported by the system.
    int               activeDevice;             ///< @internal Current device reported by system as active.
    int               deviceUsed;               ///< @internal Device that is present in the cache whose settings are
                                                /// currently being used.

    bool     bEnabled;                          ///< @internal Base effect enabled/disabled flag. This will be set as
                                                /// disabled during the graceful-disable period.

    bool    bFormatSupported;                   ///< @internal True if input sample rate and channel configuration is supported by AK,
                                                /// false otherwise.

    int      nGracefulSampleCountdown;          ///< @internal Timer to facilitate graceful disable/enable.
    #define COUNTDOWN_SAMPLERATE_DIVISOR 8      ///< @internal Fraction of a second to spend gracefully disabling.
                                                ///The sampling rate is divided by this value to calculate
                                                /// the number of samples to countdown.

    #define PRE_CROSSFADE_SAMPLES       2048    ///< @internal Number of samples to drop from AK processed output
                                                /// before starting the cross-fade when enabling.
    int      nGracefulSampleCountdownStart;

    int32_t           nParams;                  ///< @internal Number of AK parameters.
    char              **paramNames;             ///< @internal AK parameter names.
    DefaultKeyedVector<String8, int> paramNameToIndex;   ///< @internal The map between the parameter name to its index.
    DefaultKeyedVector<int,int>      paramIndexToLength; ///< @internal The map between the parameter index to its array length.
    int32_t           nSettingsPerDevice;       ///< @internal Number of AK settings defined as settable.
    DSParameterOffset *settingsDefns;           ///< @internal Index of parameter-offset pairs defined as settable.
    int               nDevicesInCache;          ///< @internal Number of devices (with settings) currently in cache (rows).
    DSDevice          *deviceCache;             ///< @internal The device id for each row of settings in the cache.
    DSValue           *settingsCache;           ///< @internal Cache of settings values for each device specified (pseudo
                                                /// 2D array). The index of each value corresponds to the parameter-offset
                                                /// pair in the settingsDefns index.
    int     *akParamHandles;                    ///< @internal The handles for all AK parameters.
    bool    bHasProcessed;                      ///< @internal The new processing flag since the last visualizer data retrieval.
    int     nCurrentVisualizerBands;            ///< @internal The number of visualizer frequency bands.
    int     inputChannelCount;
    int     outputChannelCount;
#ifdef DOLBY_DAP_OPENSLES_LPA
    audio_buffer_t *lpaOutputBuffer;
    size_t   lpaOutputBufferSize;
#endif // DOLBY_DAP_OPENSLES_LPA
#ifdef DOLBY_DAP_BYPASS_SOUND_TYPES
    bool  bBypass;                              ///< @internal: effect is put in a bypass mode.
#endif //DOLBY_DAP_BYPASS_SOUND_TYPES

#ifndef FIXED_POINT
    float fLastVolumeL;                         ///< @internal Last volume (Left) used when setting AK with a volume.
    float fLastVolumeR;                         ///< @internal Last volume (Right) used when setting AK with a volume.
#else
    int32_t nLastVolumeL;                       ///< @internal Last volume (Left) used when setting AK with a volume (8.24 format).
    int32_t nLastVolumeR;                       ///< @internal Last volume (Right) used when setting AK with a volume (8.24 format).
#endif // FIXED_POINT

    IO_DATUM _akPrimerBuffer[AK_BLKSIZE * DEFAULT_NUM_OUT_CHANNELS];      ///< @internal Temporary buffer used during crossfades.

#if DOLBY_DAP_LOG_LEVEL >= 1
    unsigned int _nLogTimer;
#endif

    EffectContext() :
        pDs1ap(NULL),
#ifdef USE_AKBUFFER
        pDs1apBuffer(NULL),
#endif // USE_AKBUFFER
        audioSessionId(DS_GLOBAL_EFFECT_AUDIO_SESSIONID),
        activeDevice(AUDIO_DEVICE_OUT_SPEAKER),
        deviceUsed(0),
        bEnabled(false),
        bFormatSupported(false),
        nGracefulSampleCountdown(0),
        nGracefulSampleCountdownStart(0),
        nParams(0),
        paramNames(NULL),
        paramNameToIndex(-1),
        paramIndexToLength(0),
        nSettingsPerDevice(0),
        settingsDefns(NULL),
        nDevicesInCache(0),
        deviceCache(NULL),
        settingsCache(NULL),
        akParamHandles(NULL),
        bHasProcessed(false),
        nCurrentVisualizerBands(0),
        inputChannelCount(DEFAULT_NUM_IN_CHANNELS),
        outputChannelCount(DEFAULT_NUM_OUT_CHANNELS),
#ifdef DOLBY_DAP_OPENSLES_LPA
        lpaOutputBuffer(NULL),
        lpaOutputBufferSize(0),
#endif // DOLBY_DAP_OPENSLES_LPA
#ifdef DOLBY_DAP_BYPASS_SOUND_TYPES
        bBypass(false),
#endif // DOLBY_DAP_BYPASS_SOUND_TYPES
#ifndef FIXED_POINT
        fLastVolumeL(1.0f),
        fLastVolumeR(1.0f)
#else
        nLastVolumeL(uOne24),
        nLastVolumeR(uOne24)
#endif // FIXED_POINT
    {
        #if DOLBY_DAP_LOG_LEVEL >= 1
        _nLogTimer = 0;
        #endif
        itfe = &gDsInterface;
        paramNameToIndex.clear();
        paramIndexToLength.clear();
    }

    ~EffectContext()
    {
        for (int i = 0; i < nParams; i++)
        {
            delete [] paramNames[i];
        }
        delete [] paramNames;
        delete [] akParamHandles;
        delete [] settingsDefns;
        delete [] deviceCache;
        delete [] settingsCache;
#ifdef DOLBY_DAP_OPENSLES_LPA
        delete [] lpaOutputBuffer;
#endif // DOLBY_DAP_OPENSLES_LPA
    }
};

/// @internal
/// @param device The audio device whose corresponding AK endpoint is to be returned.
/// @return The endpoint needs to be set to the AK settings.
///
static int _mapDeviceToAkEndpoint(int device)
{
    switch (device)
    {
        case AUDIO_DEVICE_OUT_WIRED_HEADSET:
        case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
        case AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES:
        case AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET:
        case AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET:
            return DS1_ENDP_HEADPHONES;
        case AUDIO_DEVICE_OUT_SPEAKER:
            return DS1_ENDP_INT_SPEAKERS;
        case AUDIO_DEVICE_OUT_EARPIECE:
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO:
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET:
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT:
        case AUDIO_DEVICE_OUT_BLUETOOTH_A2DP:
        case AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER:
        case AUDIO_DEVICE_OUT_USB_ACCESSORY:
        case AUDIO_DEVICE_OUT_USB_DEVICE:
            return DS1_ENDP_EXT_SPEAKERS;
        case AUDIO_DEVICE_OUT_REMOTE_SUBMIX:
        case AUDIO_DEVICE_OUT_AUX_DIGITAL:
            return DS1_ENDP_HDMI;
        default:
            return DS1_ENDP_EXT_SPEAKERS;
    }
}

/// @internal
/// @param device The audio device whose fallback list is to be returned.
/// @return A static array of devices that may be used if the specified device does not have any settings
/// configured. The array is in decreasing priority order. Each array is terminated with a 0 element.
///
static const int *_getFallbackDevices(int device)
{
    switch (device)
    {
        case AUDIO_DEVICE_OUT_EARPIECE:                      // earpiece
        {
            static const int f[] = {AUDIO_DEVICE_OUT_SPEAKER, 0};
            return f;
        }
        case AUDIO_DEVICE_OUT_SPEAKER:                       // speaker
        {
            static const int f[] = {AUDIO_DEVICE_OUT_EARPIECE, 0};
            return f;
        }
        case AUDIO_DEVICE_OUT_WIRED_HEADSET:                 // wired headset, with microphone
        {
            static const int f[] = {AUDIO_DEVICE_OUT_WIRED_HEADPHONE, AUDIO_DEVICE_OUT_EARPIECE, 0};
            return f;
        }
        case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:               // wired headphone, without microphone
        {
            static const int f[] = {AUDIO_DEVICE_OUT_WIRED_HEADSET, AUDIO_DEVICE_OUT_EARPIECE, 0};
            return f;
        }
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO:                // generic bluetooth SCO
        {
            static const int f[] = {AUDIO_DEVICE_OUT_BLUETOOTH_A2DP, AUDIO_DEVICE_OUT_WIRED_HEADSET, AUDIO_DEVICE_OUT_WIRED_HEADPHONE, AUDIO_DEVICE_OUT_EARPIECE, 0};
            return f;
        }
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET:        // bluetooth SCO headset
        {
            static const int f[] = {AUDIO_DEVICE_OUT_BLUETOOTH_SCO, AUDIO_DEVICE_OUT_BLUETOOTH_A2DP, AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES, AUDIO_DEVICE_OUT_WIRED_HEADSET, AUDIO_DEVICE_OUT_WIRED_HEADPHONE, AUDIO_DEVICE_OUT_EARPIECE, 0};
            return f;
        }
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT:         // bluetooth SCO car kit
        {
            static const int f[] = {AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER, AUDIO_DEVICE_OUT_SPEAKER, 0};
            return f;
        }
        case AUDIO_DEVICE_OUT_BLUETOOTH_A2DP:               // generic bluetooth A2DP
        {
            static const int f[] = {AUDIO_DEVICE_OUT_BLUETOOTH_SCO, AUDIO_DEVICE_OUT_WIRED_HEADSET, AUDIO_DEVICE_OUT_WIRED_HEADPHONE, AUDIO_DEVICE_OUT_EARPIECE, 0};
            return f;
        }
        case AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES:   // bluetooth A2DP headphones
        {
            static const int f[] = {AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET, AUDIO_DEVICE_OUT_WIRED_HEADPHONE, AUDIO_DEVICE_OUT_WIRED_HEADSET, AUDIO_DEVICE_OUT_EARPIECE, 0};
            return f;
        }
        case AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER:      // bluetooth A2DP speakers
        {
            static const int f[] = {AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT, AUDIO_DEVICE_OUT_SPEAKER, 0};
            return f;
        }
        case AUDIO_DEVICE_OUT_AUX_DIGITAL:                 // digital output
        {
            ALOGW("_getFallbackDevices() DS effect being applied to AUDIO_DEVICE_OUT_AUX_DIGITAL device!");
            static const int f[] = {AUDIO_DEVICE_OUT_SPEAKER, 0};
            return f;
        }
        case AUDIO_DEVICE_OUT_REMOTE_SUBMIX:
        {
            static const int f[] = {AUDIO_DEVICE_OUT_SPEAKER, 0};
            return f;
        }
        case AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET:
        case AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET:
        case AUDIO_DEVICE_OUT_USB_ACCESSORY:
        case AUDIO_DEVICE_OUT_USB_DEVICE:
        {
            static const int f[] = {AUDIO_DEVICE_OUT_WIRED_HEADPHONE, AUDIO_DEVICE_OUT_WIRED_HEADSET, AUDIO_DEVICE_OUT_EARPIECE, 0};
            return f;
        }
    }
    static const int f[] = {0};        //Default to the first available device setting.
    return f;
}

/// @internal
/// Wraps calls to reset the AK user/device settings.
///
/// @param pDs1ap    AK Ds1ap instance pointer.
///
static inline void _akReset(Ds1ap *pDs1ap)
{
    if (pDs1ap == NULL)
        return;
    ALOG1("ak_reset");
    ak_instance *pAkInstance = pDs1ap->get_ak_hndl();
    ak_reset(pAkInstance, AK_ROOT, AK_RESET_USER);
}

/// @internal
/// Find the parameter index with the specified parameter name.
///
/// @param pContext  Effect engine context.
/// @param name      AK parameter name.
/// @return The parameter index on success, and -1 on failure.
///
static inline int _akFindParamIndex(EffectContext *pContext, const char *name)
{
    if (pContext->pDs1ap == NULL || name == NULL)
    {
        ALOGE("_akFindParamIndex: No instance or null name");
        return -1;
    }
    int paramIndex = -1;
    if (!pContext->paramNameToIndex.isEmpty())
    {
        paramIndex = pContext->paramNameToIndex.valueFor(String8(name));
        ALOG2("_akFindParamIndex: find index %d for parameter %s", paramIndex, name);
    }
    return paramIndex;
}

/// @internal
/// Wraps calls to ak_set with logging.
///
/// @param pContext  Effect engine context.
/// @param parameter AK parameter index.
/// @param offset    The element offset.
/// @param value     The element value.
///
static inline void _akSet(EffectContext *pContext, int parameter, int offset, int value)
{
    if (pContext->pDs1ap == NULL || pContext->paramNames == NULL)
    {
        ALOGE("_akSet: No instance or no parameter definitions");
        return;
    }
    else if (parameter < 0 || parameter >= pContext->nParams)
    {
        ALOGE("_akSet: Wrong parameter index %d", parameter);
        return;
    }
    ALOG1("ak_set(%i/%s, %i) = %i", parameter, pContext->paramNames[parameter], offset, value);
    ak_instance *pAkInstance = pContext->pDs1ap->get_ak_hndl();
    int hndl = 0;
    if (pContext->akParamHandles != NULL)
        hndl = pContext->akParamHandles[parameter];
    else
        hndl = ak_find(pAkInstance, AK_ROOT, AK_N_HELPER(pContext->paramNames[parameter]));
    ak_set(pAkInstance, hndl, offset, value);
}

/// @internal
/// Wraps calls to ak_set_bulk with logging.
///
/// @param pContext  Effect engine context.
/// @param parameter AK parameter index.
/// @param offset    The element offset.
/// @param count     Number of elements to set.
/// @param type      The data type in AK_DATATYPE_xxx describing the return type.
/// @param pData     The buffer containing parameter values to set.
///
static inline void _akSetBulk(EffectContext *pContext, int parameter, int offset, int count, int type, void *pData)
{
    if (pContext->pDs1ap == NULL || pContext->paramNames == NULL)
    {
        ALOGE("_akSetBulk: No instance or no parameter definitions");
        return;
    }
    else if (parameter < 0 || parameter >= pContext->nParams)
    {
        ALOGE("_akSetBulk: Wrong parameter index %d", parameter);
        return;
    }
#if (DOLBY_DAP_LOG_LEVEL >= 1)
    char *sz = new char[count * 7];
    int c = 0;
    for (int i = 0; i < count - 1; ++i)
    {
        c += sprintf(sz + c, "%i, ", *((int16_t *)pData + i));
    }
    c += sprintf(sz + c, "%i", *((int16_t *)pData + count - 1));
    ALOG1("ak_set_bulk(%i/%s, [%i-%i]) = [%s]", parameter, pContext->paramNames[parameter], offset, offset + count - 1, sz);
    delete [] sz;
#endif
    ak_instance *pAkInstance = pContext->pDs1ap->get_ak_hndl();
    int hndl = 0;
    if (pContext->akParamHandles != NULL)
        hndl = pContext->akParamHandles[parameter];
    else
        hndl = ak_find(pAkInstance, AK_ROOT, AK_N_HELPER(pContext->paramNames[parameter]));
    ak_set_bulk(pAkInstance, hndl, offset, count, type, pData);
}

/// @internal
/// Wraps calls to ak_get with logging.
///
/// @param pContext  Effect engine context.
/// @param parameter AK parameter index.
/// @param offset    The element offset.
/// @return The current value of the given element, and 0 if DS is not properly initialized or parameters are not defined yet.
///
static inline int _akGet(EffectContext *pContext, int parameter, int offset)
{
    if (pContext->pDs1ap == NULL || pContext->paramNames == NULL)
    {
        ALOGE("_akGet: No instance or no parameter definitions");
        return 0;
    }
    else if (parameter < 0 || parameter >= pContext->nParams)
    {
        ALOGE("_akGet: Wrong parameter index %d", parameter);
        return 0;
    }
    ALOG1("ak_get(%i/%s, %i)", parameter, pContext->paramNames[parameter], offset);
    ak_instance *pAkInstance = pContext->pDs1ap->get_ak_hndl();
    int hndl = 0;
    if (pContext->akParamHandles != NULL)
        hndl = pContext->akParamHandles[parameter];
    else
        hndl = ak_find(pAkInstance, AK_ROOT, AK_N_HELPER(pContext->paramNames[parameter]));
    return ak_get(pAkInstance, hndl, offset);
}

/// @internal
/// Wraps calls to ak_get_bulk with logging.
///
/// @param pContext  Effect engine context.
/// @param parameter AK parameter index.
/// @param offset    The element offset.
/// @param count     Number of elements to read.
/// @param type      The data type in AK_DATATYPE_xxx describing the return type.
/// @param pData     The buffer to receive parameter values.
/// @return AK_OK on success, and one of the AK_ERR_xxx on failure.
///
static inline int _akGetBulk(EffectContext *pContext, int parameter, int offset, int count, int type, void *pData)
{
    if (pContext->pDs1ap == NULL || pContext->paramNames == NULL)
    {
        ALOGE("_akGetBulk: No instance or no parameter definitions");
        return AK_ERR_ARGUMENT;
    }
    else if (parameter < 0 || parameter >= pContext->nParams)
    {
        ALOGE("_akGetBulk: Wrong parameter index %d", parameter);
        return AK_ERR_ARGUMENT;
    }
    ALOG2("ak_get_bulk(%i/%s, [%i-%i])", parameter, pContext->paramNames[parameter], offset, offset + count - 1);
    ak_instance *pAkInstance = pContext->pDs1ap->get_ak_hndl();
    int hndl = 0;
    if (pContext->akParamHandles != NULL)
        hndl = pContext->akParamHandles[parameter];
    else
        hndl = ak_find(pAkInstance, AK_ROOT, AK_N_HELPER(pContext->paramNames[parameter]));
    return ak_get_bulk(pAkInstance, hndl, offset, count, type, pData);
}

#ifdef DOLBY_DAP_LICENSE
/// @internal
/// Validate the license.
///
/// @param pContext Effect engine context.
/// @return true if the license is valid, and false otherwise.
///
static inline bool _akIsLicenseValid(EffectContext *pContext)
{
    if (pContext->pDs1ap == NULL)
        return false;
    int16_t lic_status[AK_PARAM_LCVD_ARRAY_LEN];
    int lcvdIndex = _akFindParamIndex(pContext, "lcvd");
    _akGetBulk(pContext, lcvdIndex, 0, AK_PARAM_LCVD_ARRAY_LEN, AK_DATATYPE_INT16, lic_status);
    ALOG2("_akIsLicenseValid() License type %i, License error code %i", lic_status[0], lic_status[1]);
    return (lic_status[0] != 0 && lic_status[1] == 0);
}
#endif // DOLBY_DAP_LICENSE

/// @internal
/// Wraps calls to ak_process.
///
/// @param pDs1apBuffer    AK Ds1ap buffer pointer.
/// @param pDs1apInstance  AK Ds1ap instance pointer.
/// @param pInputData      Input buffer for ak_process.
/// @param pOutputData     Output buffer for ak_process.
/// @param nSamples        Samples (frames) being processed by ak_process.
///
static inline void _akProcess(void *pDs1apBuffer, Ds1ap *pDs1apInstance, IO_DATUM *pInputData, IO_DATUM *pOutputData, int nSamples)
{
    if (pDs1apInstance == NULL || pDs1apBuffer == NULL)
        return;
    ALOG3("Ds1apBufferProcessSamples process %i samples", nSamples);

#ifdef AK_AUDIO_DUMP
    // Launch an audio dump session if it is stopped and the volume is big enough.
    if (bAudioDumpStopped && curVolume >= AUDIO_DUMP_VOL_MIN)
    {
        ALOG2("DOLBY_AUDIO_DUMP: Open the file for PCM input/output dump");
        // PCM data format
        // Encoding: Signed 16 bit PCM (pContext->config.inputCfg.format, pContext->config.outputCfg.format)
        // Byte order: Little-endian
        // Channels: 2 (pContext->config.inputCfg.channels, pContext->config.outputCfg.channels)
        // Sample Rate: DEFAULT_EFFECT_SAMPLERATE (pContext->config.inputCfg.samplingRate, pContext->config.outputCfg.samplingRate)
        pFilePcmInputDump  = fopen("/data/dsPcmInput.pcm", "wb");
        pFilePcmOutputDump = fopen("/data/dsPcmOutput.pcm", "wb");
        if (pFilePcmInputDump == NULL || pFilePcmOutputDump == NULL)
            ALOGE("DOLBY_AUDIO_DUMP: Failed to open audio dump file. /data/ file system writable?");
        else
            bAudioDumpStopped = false;
    }
    if (pFilePcmInputDump != NULL)
    {
        fwrite(pInputData, 1, nSamples * BYTES_PER_FRAME, pFilePcmInputDump);
    }
#endif // AK_AUDIO_DUMP

    Ds1apBufferProcessSamples(pDs1apBuffer, pDs1apInstance->get_ak_hndl(), pDs1apInstance->get_ak_scratch_mem(), pInputData, pOutputData, nSamples);

#ifdef AK_AUDIO_DUMP
    if (pFilePcmOutputDump != NULL)
    {
        fwrite(pOutputData, 1, nSamples * BYTES_PER_FRAME, pFilePcmOutputDump);
    }
#endif // AK_AUDIO_DUMP
}

/// @internal
/// Get the device index whose device settings are to be used for AK based on the currently active device.
///
/// If no settings have been provided for the current device, a fallback device is determined, and that device's settings are used.
/// If no appropriate device is found, then the first device specified in the cache will be used by default.
///
/// @param pContext Effect engine context.
/// @return The device index whose settings will be applied, and -1 indicates no devices in cache.
///
static int _getDeviceIndex(EffectContext *pContext)
{
    if (pContext->nDevicesInCache == 0)
    {
        ALOGW("_getDeviceIndex() Device settings cache is empty! No settings to apply.");
        return -1;
    }

#if (DOLBY_DAP_LOG_LEVEL >= 2)
    ALOG1("Device settings cache:");
    char *sz = new char[10 + pContext->nSettingsPerDevice * 13];
    for (int iDevice = 0; iDevice < pContext->nDevicesInCache; ++iDevice)
    {
        DSValue *v = &(pContext->settingsCache[iDevice * pContext->nSettingsPerDevice]);
        int c = 0;
        for (int s = 0; s < pContext->nSettingsPerDevice; ++s)
            c += sprintf(sz + c, "%i.%i=%i ", pContext->settingsDefns[s].parameter, pContext->settingsDefns[s].offset, v[s]);
        ALOG2("#%i device:%i settings:%s", iDevice, pContext->deviceCache[iDevice], sz);
    }
    delete [] sz;
#endif

    //
    // Resolve the current active device to one of the devices actually in the cache.
    //
    int deviceToTry = pContext->activeDevice;
    int deviceIndex = 0;
    int nRetries = 0;
    int const *fallback = _getFallbackDevices(pContext->activeDevice);
    do
    {
        ALOG1("Trying to find device %i in cache...", deviceToTry);
        int i;
        for (i = 0; i < pContext->nDevicesInCache; ++i)
        {
            if (pContext->deviceCache[i] == deviceToTry)
            {
                //Matched, store the index
                ALOG1("Device %i found at index %i", deviceToTry, i);
                deviceIndex = i;
                break;
            }
        }
        if (i < pContext->nDevicesInCache)
            break;
        ALOG1("Device %i not in cache. Falling back...", deviceToTry);

        deviceToTry = fallback[nRetries++];
    } while (deviceToTry != 0);

    //
    // Even if the no devices were matched, deviceIndex 0 will be used.
    //
    if (deviceToTry == 0)
    {
        deviceIndex = 0;
        ALOGW("_getDeviceIndex() No suitable device found. Using first entry in cache as default (device %i)", pContext->deviceCache[deviceIndex]);
    }

    return deviceIndex;
}

/// @internal
/// Checks which device settings are to be used for AK based on the currently active device, and applies those settings to AK.
///
/// @param pContext Effect engine context.
///
static void _checkDeviceAndApplySettingsToAk(EffectContext *pContext)
{
    ALOG2("_checkDeviceAndApplySettingsToAk() activeDevice:%i deviceUsed:%i bEnabled:%i nSettingsPerDevice:%i", pContext->activeDevice, pContext->deviceUsed, pContext->bEnabled ? 1 : 0, pContext->nSettingsPerDevice);

    int deviceIndex = _getDeviceIndex(pContext);
    if (deviceIndex < 0 || deviceIndex >= pContext->nDevicesInCache)
    {
        ALOGW("_checkDeviceAndApplySettingsToAk() No device settings found to apply.");
        return;
    }
    pContext->deviceUsed = pContext->deviceCache[deviceIndex];

    //
    // Apply settings to AK.
    //
    ALOG1("_checkDeviceAndApplySettingsToAk() Applying settings to AK for device %i (index %i)", pContext->deviceUsed, deviceIndex);

    DSValue *value = &(pContext->settingsCache[deviceIndex * pContext->nSettingsPerDevice]);
    for (int i = 0; i < pContext->nSettingsPerDevice; ++i)
    {
        DSParameterOffset *fp = &pContext->settingsDefns[i];
        int length = pContext->paramIndexToLength.valueFor(fp->parameter);
        ALOG3("parameter %s, length %i", pContext->paramNames[fp->parameter], length);
        if (fp->offset != 0)
        {
            ALOGE("Wrong start offset %i, the start offset must always be 0!", fp->offset);
            ALOGW("The subsequent settings are ignored!");
        }
        else
        {
            if (length <= 1)
            {
                ALOG3(" values[%i]: ak_set(%i/%s, %i) = %i", i, fp->parameter, pContext->paramNames[fp->parameter], fp->offset, value[i]);
                _akSet(pContext, fp->parameter, fp->offset, value[i]);
            }
            else
            {
                ALOG3(" values[%i-%i]: ak_set_bulk(%i/%s, [%i-%i]) = [...]", i, i + length - 1,
                      fp->parameter, pContext->paramNames[fp->parameter], fp->offset, fp->offset + length - 1);
                _akSetBulk(pContext, fp->parameter, fp->offset, length, AK_DATATYPE_INT16, &value[i]);
                i = i + length - 1;
            }
        }
    }

    // Set internal vcnb and vcbf parameters based on the current genb and gebf settings.
    int index = _akFindParamIndex(pContext, "genb");
    pContext->nCurrentVisualizerBands = _akGet(pContext, index, 0);
    index = _akFindParamIndex(pContext, "vcnb");
    _akSet(pContext, index, 0, pContext->nCurrentVisualizerBands);
    int16_t vcbf[pContext->nCurrentVisualizerBands];
    index = _akFindParamIndex(pContext, "gebf");
    _akGetBulk(pContext, index, 0, pContext->nCurrentVisualizerBands, AK_DATATYPE_INT16, vcbf);
    index = _akFindParamIndex(pContext, "vcbf");
    _akSetBulk(pContext, index, 0, pContext->nCurrentVisualizerBands, AK_DATATYPE_INT16, vcbf);
    index = _akFindParamIndex(pContext,"dvlo");
    _akSet(pContext, index, 0,DS1_PARAMS_PER_ENDP[_mapDeviceToAkEndpoint(pContext->activeDevice)][DS1_PARAM_DVLO]);
    index = _akFindParamIndex(pContext,"dvli");
    _akSet(pContext, index, 0,DS1_PARAMS_PER_ENDP[_mapDeviceToAkEndpoint(pContext->activeDevice)][DS1_PARAM_DVLI]);
    index = _akFindParamIndex(pContext,"vmb");
    _akSet(pContext, index, 0,DS1_PARAMS_PER_ENDP[_mapDeviceToAkEndpoint(pContext->activeDevice)][DS1_PARAM_VMB]);


    ALOG2("_checkDeviceAndApplySettingsToAk() end");
}

/// @internal
/// Scale each sample by the specified volume level scalar.
///
/// NOTE: This function should be optimised with a fixed-point algorithm.
///
/// @param pSamples Audio sample buffer (16 bit samples).
/// @param fVolume Volume scale in [0.0, 1.0] to be applied directly to each sample.
/// @param nSamples Number of samples in the sample buffer pSamples.
///
static inline void volumeScale(IO_DATUM *pSamples, float fVolume, int nSamples)
{
    for (; nSamples > 0; --nSamples, ++pSamples)
        *pSamples = (IO_DATUM)(((float)*pSamples) * fVolume);
}

#ifndef FIXED_POINT
/// @internal
/// Scale each left and right sample by the specified volume level scalars.
///
/// @param pSamples Audio sample buffer, stereo 16 bit samples.
/// @param fVolumeL Volume scale in [0.0, 1.0] to be applied directly to each left channel sample.
/// @param fVolumeR Volume scale in [0.0, 1.0] to be applied directly to each right channel sample.
/// @param nFrames Number of frames in the sample buffer pSamples.
///
/// NOTE: This function should be optimised with a fixed-point algorithm.
///
static inline void volumeScaleLR(IO_DATUM *pSamples, float fVolumeL, float fVolumeR, int nFrames)
{
    //assuming stereo buffer; Left/Right interleaved.
    //apply the relevant gain to each channel
    for (; nFrames > 0; --nFrames)
    {
        *pSamples = (IO_DATUM)(((float)*pSamples) * fVolumeL);
        ALOG3("fVolumeL = %f, pSamples = %x, nFrames = %d", fVolumeL, pSamples, nFrames);
        ++pSamples;

        *pSamples = (IO_DATUM)(((float)*pSamples) * fVolumeR);
        ALOG3("fVolumeR = %f, pSamples = %x, nFrames = %d", fVolumeR, pSamples, nFrames);
        ++pSamples;
    }
}

#else
/// This code is for demonstration only.
inline void volumeScaleLR_fixed (IO_DATUM *pSamples, int32_t nVolumeL, int32_t nVolumeR, int nFrames)
{
    IO_DATUM valL, valR;
    int Result32L, Result32R;

    for (;nFrames > 0; nFrames --)
    {
        //Load Channel Samples
        valL = *pSamples;
        valR = *(pSamples+1);

        //Apply Volume
        Result32L = ((int)(((int64_t)( nVolumeL) *(valL)) >> 16));
        Result32R = ((int)(((int64_t)( nVolumeR) *(valR)) >> 16));

        //Store back the volume applied samples
        *pSamples++ = (IO_DATUM)(Result32L >> 8);
        *pSamples++ = (IO_DATUM)(Result32R >> 8);
    }
}
#endif // FIXED_POINT

/// @internal
/// Clamps the sample to a valid 32-bit audio PCM sample range.
///
/// @param sample Sample to clamp to valid 32-bit PCM range.
/// @return The sample value, clamped to 32-bit range.
///
static inline int32_t clamp32(int64_t sample)
{
    if ((sample>>31) ^ (sample>>63))
        sample = 0x7FFFFFFF ^ (sample>>63);
    return sample;
}
/// @internal
/// Clamps the sample to a valid 16-bit audio PCM sample range.
///
/// @note This inline function should be optimised into CPU-specific assembly instructions
///  e.g.  asm ("ssat %[Rd], 16, %[Rm]" : [Rd] "=r" (sample) :[Rm] "r" (sample));
///
/// @param sample Sample to clamp to valid 16-bit PCM range.
/// @return The sample value, clamped to 16-bit range.
///
static inline int16_t clamp16(int32_t sample)
{
    if ((sample>>15) ^ (sample>>31))
        sample = 0x7FFF ^ (sample>>31);
    return sample;
}

/// @internal
/// Mix the input sample buffer into the contents of the output sample buffer.
///
/// @param inBuffer Input buffer.
/// @param outBuffer Output buffer to be mixed into.
/// @param nSamples Number of samples in the sample buffers.
///
static inline void accumulate(audio_buffer_t *inBuffer, audio_buffer_t *outBuffer, int nSamples)
{
    for (; nSamples > 0; --nSamples, ++inBuffer, ++outBuffer){
#ifdef DOLBY_DAP_32BIT
        *outBuffer->s32 = clamp32((int64_t)(*outBuffer->s32) + (int64_t)(*inBuffer->s32));
#else
        *outBuffer->s16 = clamp16((int32_t)(*outBuffer->s16) + (int32_t)(*inBuffer->s16));
#endif
    }
}

/// @internal
/// Mix the input sample buffer into the contents of the output sample buffer, applying the
/// specified volume level and an additional fade component. The fade component transitions between
/// the start and finish fade volume.
///
/// NOTE: This function should be optimised with a fixed-point algorithm.
///
/// This function is similar to copyWithVolumeFade except that mixing is performed.
///
/// @param inBuffer Input buffer. These samples will have the base volume
/// applied, and the additional fade volume which transitions from the start to finish level.
/// @param outBuffer Output buffer to be mixed into.
/// @param nFrames Number of frames in the sample buffers.
/// @param fVolumeL Base volume scale in [0.0, 1.0] to be applied to each left channel sample of pInSamples.
/// @param fVolumeR Base volume scale in [0.0, 1.0] to be applied to each right channel sample of pInSamples.
/// @param fStart Initial additional fade volume to apply to samples in pInSamples. The volume
/// transitions from this level to the finish level.
/// @param fFinish Final additional fade volume to apply to samples in pInSamples.
///
static inline void accumulateWithVolumeFade(audio_buffer_t *inBuffer, audio_buffer_t *outBuffer,
                                            int nFrames, float fVolumeL, float fVolumeR, 
                                            float fStart, float fFinish)
{
    float fd = fFinish - fStart;
    ALOG3("accumulateWithVolumeFade() nFrames:%i fVolumeL:%f fVolumeR:%f fStart:%f fFinish:%f diff:%f", 
            nFrames, fVolumeL, fVolumeR, fStart, fFinish, fd);
    float f;
    float fFrames = (float)nFrames;
    for (int i = 0; i < nFrames; ++i, ++inBuffer, ++outBuffer)
    {
        f = fStart + ((i / fFrames) * fd);
#ifdef DOLBY_DAP_32BIT
        *outBuffer->s32 = 
            clamp32((int64_t)(*outBuffer->s32) + (int64_t)(*inBuffer->s32 * fVolumeL * f));
#else
        *outBuffer->s16 = 
            clamp16((int32_t)(*outBuffer->s16) + (int32_t)(*inBuffer->s16 * fVolumeL * f));
#endif // DOLBY_DAP_32BIT
        ++outBuffer;
        ++inBuffer;
#ifdef DOLBY_DAP_32BIT
        *outBuffer->s32 = 
            clamp32((int64_t)(*outBuffer->s32) + (int64_t)(*inBuffer->s32 * fVolumeR * f));
#else
        *outBuffer->s16 = 
            clamp16((int32_t)(*outBuffer->s16) + (int32_t)(*inBuffer->s16 * fVolumeR * f));
#endif // DOLBY_DAP_32BIT
    }
}

/// @internal
/// Copy the input sample buffer into the output sample buffer, applying the
/// specified volume level and an additional fade component. The fade component transitions between
/// the start and finish fade volume.
///
/// NOTE: This function should be optimised with a fixed-point algorithm.
///
/// This function is similar to accumulateWithVolumeFade except that no mixing is performed.
///
/// @param inBuffer Input buffer. These samples will have the base volume
/// applied, and the additional fade volume which transitions from the start to finish level.
/// @param outBuffer Output buffer.
/// @param nFrames Number of frames in the sample buffer.
/// @param fVolumeL Base volume scale in [0.0, 1.0] to be applied to each left channel sample of pInSamples.
/// @param fVolumeR Base volume scale in [0.0, 1.0] to be applied to each right channel sample of pInSamples.
/// @param fStart Initial additional fade volume to apply to samples in pInSamples. The volume
/// transitions from this level to the finish level.
/// @param fFinish Final additional fade volume to apply to samples in pInSamples.
///
static inline void copyWithVolumeFade(audio_buffer_t *inBuffer, audio_buffer_t *outBuffer, int nFrames, 
                                        float fVolumeL, float fVolumeR, float fStart, float fFinish)
{
    float fd = fFinish - fStart;
    ALOG3("copyWithVolumeFade() fVolumeL:%f fVolumeR:%f fStart:%f fFinish:%f fd:%f", 
            fVolumeL, fVolumeR, fStart, fFinish, fd);
    float f;
    float fFrames = (float)nFrames;
    for (int i = 0; i < nFrames; ++i, ++inBuffer, ++outBuffer)
    {
        f = fStart + ((i / fFrames) * fd);
#ifdef DOLBY_DAP_32BIT
        *outBuffer->s32 = (int32_t)(*inBuffer->s32 * fVolumeL * f);
#else
        *outBuffer->s16 = (int16_t)(*inBuffer->s16 * fVolumeL * f);
#endif // DOLBY_DAP_32BIT
        ++outBuffer;
        ++inBuffer;
#ifdef DOLBY_DAP_32BIT
        *outBuffer->s32 = (int32_t)(*inBuffer->s32 * fVolumeR * f);
#else
        *outBuffer->s16 = (int16_t)(*inBuffer->s16 * fVolumeR * f);
#endif // DOLBY_DAP_32BIT
    }
}

static inline int32_t clampApplyVolumeAndCrossFade(int32_t *pTargetFadeOutSamples, int32_t *pFadeInSamples, 
                                                    float finv, float fVolume, float f)
{
    return clamp32((int64_t)(*pFadeInSamples * (finv * fVolume)) +
        (int64_t)(*pTargetFadeOutSamples * (f * fVolume)));
}

static inline int16_t clampApplyVolumeAndCrossFade(int16_t *pTargetFadeOutSamples, int16_t *pFadeInSamples, 
                                                    float finv, float fVolume, float f)
{
    return clamp16((int32_t)(*pFadeInSamples * (finv * fVolume)) + 
        (int32_t)(*pTargetFadeOutSamples * (f * fVolume)));
}

/// @internal
/// Performs a mix of the "fade-in" samples into the "target fade-out samples".
///
/// NOTE: This function should be optimised with a fixed-point algorithm.
///
/// The target fade-out samples have the specified base volume applied followed by a cross fade
/// factor. The fade-in samples are mixed into the target buffer with base volume multiplied by inverse cross fade factor.
/// Cross fading is applied per-sample basis.
///
/// The result is the target buffer consisting of a cross-fade between the samples already present
/// in the target buffer (with volume applied) and the fade-in buffer.
///
/// @param pTargetFadeOutSamples Sample buffer containing audio to be cross-faded
/// into. These samples will have the base volume applied as well as the cross-fade factor.
/// @param pFadeInSamples Sample buffer containing audio to cross-fade into
/// pTargetFadeOutSamples. These samples will have the inverse cross-fade factor applied.
/// @param nFrames Number of frames in the sample buffers.
/// @param fVolumeL Base volume scale in [0.0, 1.0] to be applied to each left channel sample of pTargetFadeOutSamples.
/// @param fVolumeR Base volume scale in [0.0, 1.0] to be applied to each right channel sample of pTargetFadeOutSamples.
/// @param fCrossFadeStart Start value of Cross fade.
/// @param fCrossFadeEnd End value of Cross fade.
///
template <class TYPE>
static inline void applyVolumeAndCrossFade(TYPE *pTargetFadeOutSamples, TYPE *pFadeInSamples, int nFrames, float fVolumeL, float fVolumeR, float fCrossFadeStart, float fCrossFadeEnd)
{
    ALOG3("applyVolumeAndCrossFade() nFrames:%i fVolumeL:%f fVolumeR:%f fCrossFadeStart:%f, fCrossFadeEnd:%f", nFrames, fVolumeL, fVolumeR, fCrossFadeStart, fCrossFadeEnd);

    float fd = (fCrossFadeEnd - fCrossFadeStart);
    float fFrames = (float)nFrames;
    float f;
    float finv;

    for (int i = 0; i < nFrames; ++i, ++pFadeInSamples, ++pTargetFadeOutSamples)
    {
        f = fCrossFadeStart + ((i / fFrames) * fd);
        finv = (1.0F - f);
        *pTargetFadeOutSamples = clampApplyVolumeAndCrossFade(pTargetFadeOutSamples, pFadeInSamples, 
                                                                finv, fVolumeL, f);
        ++pFadeInSamples;
        ++pTargetFadeOutSamples;
        *pTargetFadeOutSamples = clampApplyVolumeAndCrossFade(pTargetFadeOutSamples, pFadeInSamples, 
                                                                finv, fVolumeR, f);
    }
}

static inline int calculateExternalGain(uint32_t uVolL, uint32_t uVolR)
{
    int akExtVal = 0;

    const float fOne24 = (float)(uOne24);
    float fVolL = uVolL / fOne24;
    float fVolR = uVolR / fOne24;

    ALOG1("calculateExternalGain fVolL:%f fVolR %f", fVolL, fVolR);

    // This effect is not controlling volume.
    // It is receiving volume level indication, which actually represents the desired pre or post-gain value for AK.
    // The volume should not be updated here.
    #if 0
    #ifndef FIXED_POINT
    pContext->fLastVolumeL = fVolL;
    pContext->fLastVolumeR = fVolR;
    #else
    pContext->nLastVolumeL = uVolL;
    pContext->nLastVolumeR = uVolR;
    #endif // FIXED_POINT
    #endif // 0

    //
    // Calculate the final volume if the volume between left channel and right channel is not balanced.
    // Here we only adopt the maximum volume between the left/right channel volume.
    // TODO: This needs to be finalized.
    //
    float fVol;
    if (fVolL >= fVolR) {
        fVol = fVolL;
    } else {
        fVol = fVolR;
    }

    const int EXT_DB_MIN = -130;        // The minimum dB value (maximum attenuation) for AK parameter - 'pstg' or 'preg'.
    const int EXT_DB_MAX = 0;           // The maximum dB value (minimum attenuation) for AK parameter - 'pstg' or 'preg'.
    const float F_VOL_MIN  = 0.00000031623f; // Android volume below this value corresponds to the minimum dB value, that is,
                                             // -130dB in AK 'pstg' or 'preg' parameter. It is calculated by pow(10.0f, PSTG_DB_MIN/20).
    const float F_VOL_MAX  = 1.0f;           // Android volume above this value corresponds to the maximum dB value, that is,
                                             // 0dB in AK 'pstg' or 'preg' parameter. It is calculated by pow(10.0f, PSTG_DB_MAX/20).

    //
    // Calculate the AK pre/post-gain that needs to be applied. This calculation assumes that the Android floating point volume that ranges
    // from -0.0f to 1.0f maps logarithmically to the AK pre/post-gain parameter, ranging from -2080 (-130dB) to 0 (0dB) respectively.
    //
    akExtVal = 0;
    if (fVol <= F_VOL_MIN) {
        akExtVal = EXT_DB_MIN * DB_SCALE;
    } else if (fVol >= F_VOL_MAX) {
        akExtVal = EXT_DB_MAX * DB_SCALE;
    } else {
        float dbVal = 20.0f * log10(fVol);
        akExtVal = (int)(dbVal * DB_SCALE);
    }
    ALOG1("calculateExternalGain fVol:%f akExtVal:%i", fVol, akExtVal);

    return akExtVal;
}

///
/// Initialise engine with default configuration.
///
/// @param pContext Effect engine context.
/// @return 0 on success, and -EINVAL otherwise.
///
static int Effect_init(EffectContext *pContext)
{
    ALOG2("Effect_init(): start");

    //
    // Initialise AK
    // No setup is required here. Effect_setConfig will be called later.
    //

    //
    // Initialise default values for the config struct.
    // These will be applied first, but will be updated with the system sends the configure command.
    // Hence the values here will only be used temporarily until the actual configuration is sent.
    //
    pContext->config.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
    pContext->config.inputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
#if defined DOLBY_DAP_32BIT_TEST_IN_16BIT_FRAMEWORK
    pContext->config.inputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
#elif defined DOLBY_DAP_32BIT
    pContext->config.inputCfg.format = AUDIO_FORMAT_PCM_32_BIT;
#else
    pContext->config.inputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
#endif // DOLBY_DAP_32BIT, DOLBY_DAP_32BIT_TEST_IN_16BIT_FRAMEWORK
    pContext->config.inputCfg.samplingRate = DEFAULT_EFFECT_SAMPLERATE;
    pContext->config.inputCfg.bufferProvider.getBuffer = NULL;
    pContext->config.inputCfg.bufferProvider.releaseBuffer = NULL;
    pContext->config.inputCfg.bufferProvider.cookie = NULL;
    pContext->config.inputCfg.mask = EFFECT_CONFIG_ALL;
    pContext->config.outputCfg.accessMode = EFFECT_BUFFER_ACCESS_ACCUMULATE;
    pContext->config.outputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
#if defined DOLBY_DAP_32BIT_TEST_IN_16BIT_FRAMEWORK
    pContext->config.outputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
#elif defined DOLBY_DAP_32BIT
    pContext->config.outputCfg.format = AUDIO_FORMAT_PCM_32_BIT;
#else
    pContext->config.outputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
#endif // DOLBY_DAP_32BIT, DOLBY_DAP_32BIT_TEST_IN_16BIT_FRAMEWORK
    pContext->config.outputCfg.samplingRate = DEFAULT_EFFECT_SAMPLERATE;
    pContext->config.outputCfg.bufferProvider.getBuffer = NULL;
    pContext->config.outputCfg.bufferProvider.releaseBuffer = NULL;
    pContext->config.outputCfg.bufferProvider.cookie = NULL;
    pContext->config.outputCfg.mask = EFFECT_CONFIG_ALL;

    pContext->nGracefulSampleCountdown = 0;
    pContext->bEnabled = false;

    ALOG2("Effect_init() End");
    return 0;
}

///
/// Called on an active device change or a effect config sampling rate change.
///
/// @param pContext Effect engine context.
/// @return 0 on success, and -EINVAL otherwise.
///
static int Effect_reinit(EffectContext *pContext)
{
    bool bParamBackup = false;
    int preGain = 0, postGain = 0, vEnabled = 0, ocf = 0;

    ALOG2("Effect_reinit(): start");

    // Close and re-open the AK instance.
    if (pContext->pDs1ap != NULL)
    {
        if (pContext->akParamHandles != NULL)
        {
            // Other AK parameters that must be restored after AK is re-initialised should be stored here and restored below.
            int index = _akFindParamIndex(pContext, "preg");
            preGain  = _akGet(pContext, index, 0);
            index = _akFindParamIndex(pContext, "pstg");
            postGain = _akGet(pContext, index, 0);
            index = _akFindParamIndex(pContext, "ven");
            vEnabled = _akGet(pContext, index, 0);
            index = _akFindParamIndex(pContext, "ocf");
            ocf = _akGet(pContext, index, 0);
            bParamBackup = true;
        }

        ALOG2("Effect_reinit() Calling ak_close()");
        delete pContext->pDs1ap;
        pContext->pDs1ap = NULL;
    }

    int endpoint = _mapDeviceToAkEndpoint(pContext->activeDevice);
    int inputSamplingRate = DEFAULT_EFFECT_SAMPLERATE;
    if (pContext->config.inputCfg.samplingRate == 48000 ||
        pContext->config.inputCfg.samplingRate == 44100 ||
        pContext->config.inputCfg.samplingRate == 32000) // AK supports 32K, 44.1K and 48K input/output sampling rates only.
    {
        pContext->bFormatSupported = true;
        inputSamplingRate = pContext->config.inputCfg.samplingRate;
    }
    else // pContext->config.inputCfg.samplingRate is not supported, still create pDs1ap but we won't process with it.
    {
        ALOG1("Effect_reinit() input sampling rate %dHz is not supported", pContext->config.inputCfg.samplingRate);
        pContext->bFormatSupported = false;
    }

    // Inverse condition is used to set the bFormatSupported to false. If bFormatSupported is set to false by the sampling
    // frequency check above, then the channelcount check should not set it back to true.
    if (!(pContext->inputChannelCount == 2 ||
        pContext->inputChannelCount == 6 ||
        pContext->inputChannelCount == 8)) // AK supports 2, 6 and 8 channel configuration.
    {
        ALOG1("Effect_reinit() input number of channels (%d) not supported", pContext->inputChannelCount);
        pContext->bFormatSupported = false;
    }

    ALOG2("Effect_reinit() Calling ak_open() with endpoint %i, sampling rate %i", endpoint, inputSamplingRate);
    pContext->pDs1ap = Ds1ap::New(endpoint, inputSamplingRate, pContext->inputChannelCount, pContext->outputChannelCount);
    if (pContext->pDs1ap == NULL)
    {
        ALOGE("ERROR: Effect_reinit() ak_open() FAILED!");
        return -EINVAL;
    }
    else
    {
        pContext->deviceUsed = 0;
        if (pContext->akParamHandles != NULL)
        {
            // Re-cache the handles of all the AK parameters.
            ak_instance *pAkInstance = pContext->pDs1ap->get_ak_hndl();
            for (int i = 0; i < pContext->nParams; ++i)
                pContext->akParamHandles[i] = ak_find(pAkInstance, AK_ROOT, AK_N_HELPER(pContext->paramNames[i]));
        }
    }
    if (bParamBackup)
    {
        int index = _akFindParamIndex(pContext, "preg");
        _akSet(pContext, index, 0, preGain);
        index = _akFindParamIndex(pContext, "pstg");
        _akSet(pContext, index, 0, postGain);
        index = _akFindParamIndex(pContext, "ven");
        _akSet(pContext, index, 0, vEnabled);
        index = _akFindParamIndex(pContext, "ocf");
        _akSet(pContext, index, 0, ocf);
    }

#if (DOLBY_DAP_LOG_LEVEL >= 1)
    if (pContext->akParamHandles != NULL)
    {
        // Get the version of the AK library.
        int16_t version[AK_PARAM_VER_ARRAY_LEN];
        int verIndex = _akFindParamIndex(pContext, "ver");
        _akGetBulk(pContext, verIndex, 0, AK_PARAM_VER_ARRAY_LEN, AK_DATATYPE_INT16, version);
        char *sz = new char[AK_PARAM_VER_ARRAY_LEN * 2];
        int c = 0;
        for (int i = 0; i < AK_PARAM_VER_ARRAY_LEN; ++i)
        {
            c += sprintf(sz + c, "%i.", version[i]);
        }
        ALOG1("DS1 AK library version: %s", sz);
        delete [] sz;
    }
#endif

#ifdef DOLBY_DAP_LICENSE
    if (!_akIsLicenseValid(pContext))
        ALOGE("ERROR: DS1 AK License Authentication failed");
#endif // DOLBY_DAP_LICENSE

    ALOG2("Effect_reinit() End");
    return 0;
}

///
/// Set up the effect for the specified input and output audio configuration.
///
/// @param pContext Effect engine context.
/// @return 0 on success, and -EINVAL otherwise.
///
int Effect_setConfig(EffectContext *pContext)
{
    // This is called during EffectCreate where default values are loaded.
    // When the EFFECT_CMD_SET_CONFIG command is received this will include real values
    // that will overwrite the defaults.

    ALOG2("Effect_setConfig() start");

    //
    // NOTE: The received configuration validity check has been moved to the EFFECT_CMD_SET_CONFIG command handler.
    //

    int ret = Effect_reinit (pContext);
    if (ret != 0)
    {
        ALOGE("Effect_setConfig() re-initialization failed");
        return -EINVAL;
    }
    pContext->nGracefulSampleCountdownStart = pContext->config.outputCfg.samplingRate / COUNTDOWN_SAMPLERATE_DIVISOR;

    ALOG1("Effect_setConfig() Applying cached AK settings for current device");
    _checkDeviceAndApplySettingsToAk(pContext);

#ifdef USE_AKBUFFER
    //
    // Initialise the AK buffering module.
    //
    ALOG1("Effect_setConfig() Initialise AkBuffer");
    if (pContext->pDs1apBuffer != NULL)
    {
        Ds1apBufferDeinit(pContext->pDs1apBuffer);
        pContext->pDs1apBuffer = NULL;
    }

    // Calculate the number of input/output channels
    pContext->inputChannelCount = (pContext->config.inputCfg.channels == AUDIO_CHANNEL_OUT_MONO) ? 1 : 2;
    pContext->outputChannelCount = (pContext->config.outputCfg.channels == AUDIO_CHANNEL_OUT_MONO) ? 1 : 2;

#if (DS1APBUFFER_MODE == DS1APBUFFER_MODE_IN_PLACE_INTERNALBUFFER)
    pContext->pDs1apBuffer = Ds1apBufferInit(pContext->inputChannelCount,  pContext->outputChannelCount, AK_BLOCK_SIZE);
#else
    pContext->pDs1apBuffer = Ds1apBufferInit(pContext->inputChannelCount,  pContext->outputChannelCount);
#endif
#endif // USE_AKBUFFER

    ALOG2("Effect_setConfig() end");
    return 0;
}

///
/// Get the effect input and output audio configuration.
///
/// @param pContext Effect engine context.
/// @param pConfig  Pointer to effect_config_t structure holding input and output configuration parameters.
///
void Effect_getConfig(EffectContext *pContext, effect_config_t *pConfig)
{
    memcpy(pConfig, &pContext->config, sizeof(effect_config_t));
}

///
/// Called when data is being sent to the effect using the EFFECT_CMD_SET_PARAM.
///
/// @param pContext Effect engine context.
/// @param pParam Pointer to parameter (int32_t).
/// @param pValue Pointer to value (type and length depends on the parameter).
/// @return 0 on success, and -EINVAL otherwise.
///
int Effect_setParameter(EffectContext *pContext, void *pParam, void *pValue)
{
    int32_t param = *((int32_t *)pParam);
    ALOG2("Effect_setParameter(): param=%d", param);
    int nReturn = 0;

    if (pContext->pDs1ap == NULL && param != DS_PARAM_DEFINE_SETTINGS)
    {
        ALOGE("Effect_setParameter() ERROR: pContext->pDs1ap == NULL");
        return -EINVAL;
    }

    //The structure of pValue will depend on the the parameter
    switch (param)
    {
        //
        // DS_PARAM_DEFINE_PARAMS
        //  Value: int16: Number of AK parameters settable at java side.
        //          Array of type char*
        //              char*: the parameter names, each name contains 4 characters
        //
        case DS_PARAM_DEFINE_PARAMS:
        {
            ALOG2("Effect_setParameter() DS_PARAM_DEFINE_PARAMS");
            DSDefineParams *dp = (DSDefineParams *)pValue;
            ak_instance *pAkInstance = NULL;
            if (pContext->pDs1ap != NULL)
                pAkInstance = pContext->pDs1ap->get_ak_hndl();
            ALOG1("DS_PARAM_DEFINE_PARAMS count:%d", dp->count);

            //
            // Delete cache because the settings definition are changing.
            // AK must be reset if any setting has been performed to ensure there are no side-effects.
            //
            if (pContext->paramNames != NULL && pContext->pDs1ap != NULL)
            {
                ALOG1("DS_PARAM_DEFINE_PARAMS Resetting AK...");
                _akReset(pContext->pDs1ap);
                ALOG1("DS_PARAM_DEFINE_PARAMS Clearing parameter names...");
                for (int i = 0; i < pContext->nParams; i++)
                {
                    delete [] pContext->paramNames[i];
                }
                delete [] pContext->paramNames;
                pContext->paramNames = NULL;
                delete [] pContext->akParamHandles;
                pContext->akParamHandles = NULL;
            }

            pContext->nParams = dp->count;
            pContext->paramNames = new char*[dp->count];
            pContext->akParamHandles = new int[dp->count];
            pContext->paramNameToIndex.clear();
            ALOG1("DS_PARAM_DEFINE_PARAMS Copying parameters definitions: [index]:parameter");
            char *fp = &(dp->definitions[0]);
            for (int i = 0; i < dp->count; ++i)
            {
                int len = 0;
                for (int j = 0; j < 4; j++)
                {
                    if (fp[j] != '\0')
                        len = len + 1;
                    else
                        break;
                }
                pContext->paramNames[i] = new char[len + 1];
                for (int j = 0; j < len; j++)
                {
                    pContext->paramNames[i][j] = fp[j];
                }
                pContext->paramNames[i][len] = '\0';
                ALOG1("DS_PARAM_DEFINE_PARAMS [%i]:%s", i, pContext->paramNames[i]);
                pContext->paramNameToIndex.add(String8(pContext->paramNames[i]), i);
                if (pAkInstance != NULL)
                    pContext->akParamHandles[i] = ak_find(pAkInstance, AK_ROOT, AK_N_HELPER(pContext->paramNames[i]));

                fp = fp + 4;
            }
            break;
        }

        //
        // DS_PARAM_DEFINE_SETTINGS
        //  Value: int16: Number of parameter-offset pairs to be defined as allowed.
        //          Array of type DSParameterOffset
        //              int8: parameter
        //              int16: offset
        //
        case DS_PARAM_DEFINE_SETTINGS:
        {
            ALOG2("Effect_setParameter() DS_PARAM_DEFINE_SETTINGS");
            DSDefineSettings *ds = (DSDefineSettings *)pValue;
            ALOG1("DS_PARAM_DEFINE_SETTINGS count:%d", ds->count);

            //
            // Delete cache because the settings definition are changing.
            // AK must be reset if any setting has been performed to ensure there are no side-effects.
            //
            if (pContext->settingsCache != NULL && pContext->pDs1ap != NULL)
            {
                ALOG1("DS_PARAM_DEFINE_SETTINGS Resetting AK...");
                _akReset(pContext->pDs1ap);
            }

            ALOG1("DS_PARAM_DEFINE_SETTINGS Clearing cache...");
            pContext->nSettingsPerDevice = 0;
            delete [] pContext->settingsDefns;
            pContext->settingsDefns = NULL;
            pContext->nDevicesInCache = 0;
            delete [] pContext->deviceCache;
            pContext->deviceCache = NULL;
            delete [] pContext->settingsCache;
            pContext->settingsCache = NULL;

            //
            // Copy the new settings definitions.
            //
            pContext->paramIndexToLength.clear();
            pContext->nSettingsPerDevice = ds->count;
            pContext->settingsDefns = new DSParameterOffset[pContext->nSettingsPerDevice];
            ALOG1("DS_PARAM_DEFINE_SETTINGS Copying settings definitions: [index]:parameter.offset");
            DSParameterOffset *fp = &(ds->definitions[0]);
            int paramLength = 0;
            for (int i = 0; i < ds->count; ++i)
            {
                if  (pContext->paramNames != NULL)
                {
                    ALOG1("DS_PARAM_DEFINE_SETTINGS [%i]:%i-%s.%i", i, fp[i].parameter, pContext->paramNames[fp[i].parameter], fp[i].offset);
                }
                else
                {
                    ALOG1("DS_PARAM_DEFINE_SETTINGS [%i]:%i.%i", i, fp[i].parameter, fp[i].offset);
                }
                pContext->settingsDefns[i] = fp[i];
                paramLength++;
                if (fp[i].parameter != fp[i+1].parameter)
                {
                    pContext->paramIndexToLength.add(fp[i].parameter, paramLength);
                    paramLength = 0;
                }
            }
            break;
        }

        //
        // DS_PARAM_SINGLE_DEVICE_VALUE
        //  Value is type DSSingleDeviceValue
        //  Value:  int32: Output device
        //          int16: setting_index (as determined by previous set of DS_PARAM_DEFINE_SETTINGS)
        //          int16: length - The number of the values
        //          int16: values[n], where n is specified by the length above.
        //
        case DS_PARAM_SINGLE_DEVICE_VALUE:
        {
            ALOG2("Effect_setParameter() DS_PARAM_SINGLE_DEVICE_VALUE");
            DSSingleDeviceValue *sdv = (DSSingleDeviceValue *)pValue;
            ALOG1("DS_PARAM_SINGLE_DEVICE_VALUE device:%i setting_index:%i length:%i", sdv->device, sdv->setting_index, sdv->length);

            if (pContext->nParams == 0 || pContext->nSettingsPerDevice == 0)
            {
                ALOGE("DS_PARAM_SINGLE_DEVICE_VALUE No AK parameters or settings have been defined");
                nReturn = -EINVAL;
                break;
            }

            if (sdv->setting_index < 0 || sdv->setting_index >= pContext->nSettingsPerDevice)
            {
                if (pContext->nSettingsPerDevice == 0)
                    ALOGE("DS_PARAM_SINGLE_DEVICE_VALUE No settings have been defined!");
                else
                    ALOGE("DS_PARAM_SINGLE_DEVICE_VALUE setting_index %i is invalid (number of settings defined is %i)", sdv->setting_index, pContext->nSettingsPerDevice);
                nReturn = -EINVAL;
                break;
            }

            //
            // Update device cache.
            // Find the specified device in the cache. If there are devices in the cache but the specified device does not exist, this is an error and the command is ignored.
            // If the cache is empty, we tolerate this by creating a new cache with this device using the settings adopted currently.
            //
            int iDevice = 0;
            if (pContext->nDevicesInCache == 0)
            {
                ALOGE("DS_PARAM_SINGLE_DEVICE_VALUE Cache is empty. Creating a cache with a single device (%i) using settings from current AK instance.", sdv->device);
                delete [] pContext->deviceCache;
                delete [] pContext->settingsCache;
                pContext->nDevicesInCache = 1;
                pContext->deviceCache = new DSDevice[pContext->nDevicesInCache];
                pContext->settingsCache = new DSValue[pContext->nDevicesInCache * pContext->nSettingsPerDevice];
                //The device in the cache
                pContext->deviceCache[0] = sdv->device;
                //Initialise settings from the current AK instance to ensure default values are reasonable.
                for (int i = 0; i < pContext->nSettingsPerDevice; ++i)
                {
                    ALOG1("Initialising cache with AK settings:");
                    pContext->settingsCache[i] = _akGet(pContext, pContext->settingsDefns[i].parameter, pContext->settingsDefns[i].offset);
                    ALOG3(" settingsCache[%i] %i/%s.%i=%i", i, pContext->settingsDefns[i].parameter, pContext->paramNames[pContext->settingsDefns[i].parameter], pContext->settingsDefns[i].offset, pContext->settingsCache[i]);
                }
                //Zero: memset(pContext->settingsCache, 0, pContext->nDevicesInCache * pContext->nSettingsPerDevice * sizeof(pContext->settingsCache[0]));
                //This must therefore be the device in use at the moment.
                pContext->deviceUsed = sdv->device;
            }
            else
            {
                while (iDevice < pContext->nDevicesInCache && pContext->deviceCache[iDevice] != sdv->device)
                    ++iDevice;
                if (iDevice == pContext->nDevicesInCache)
                {
                    ALOGE("DS_PARAM_SINGLE_DEVICE_VALUE Specified device(%i) does not exist in cache", sdv->device);
                    nReturn = -EINVAL;
                    break;
                }
                ALOG1("DS_PARAM_SINGLE_DEVICE_VALUE Found device %i in cache at index %i", sdv->device, iDevice);
            }

            //
            // Update settings cache for the device with the new value/values.
            //
            DSValue *values = &pContext->settingsCache[iDevice * pContext->nSettingsPerDevice];
            int bytes_length = sdv->length * sizeof(pContext->settingsCache[0]);
            memcpy(&values[sdv->setting_index], sdv->values, bytes_length);
            if (sdv->length == 1)
                ALOG1("DS_PARAM_SINGLE_DEVICE_VALUE settingsCache[device:%i setting_index:%i] updated with value %i", iDevice, sdv->setting_index, sdv->values[0]);
            else
                ALOG1("DS_PARAM_SINGLE_DEVICE_VALUE settingsCache[device:%i setting_index:%i-%i] updated with new values", iDevice, sdv->setting_index, sdv->setting_index + sdv->length - 1);

            //
            // Set the new value in AK immediately if the specified device is the one currently being used.
            //
            if (sdv->device == pContext->deviceUsed)
            {
                DSParameterOffset fp = pContext->settingsDefns[sdv->setting_index];
                if (sdv->length == 1)
                {
                    ALOG3("DS_PARAM_SINGLE_DEVICE_VALUE Specified device is in-use, calling ak_set(%i/%s, %i) = %i", fp.parameter, pContext->paramNames[fp.parameter], fp.offset, sdv->values[0]);
                    _akSet(pContext, fp.parameter, fp.offset, sdv->values[0]);
                    // NOTE: We do NOT expect this piece of code to be executed since we don't expect band number to be changed on runtime.
                    if (strcmp(pContext->paramNames[fp.parameter], "genb") == 0)
                    {
                        int paramIndex = _akFindParamIndex(pContext, "vcnb");
                        ALOG3("DS_PARAM_SINGLE_DEVICE_VALUE Specified device is in-use, calling ak_set(%i/vcnb, %i) = %i", paramIndex, fp.offset, sdv->values[0]);
                        _akSet(pContext, paramIndex, fp.offset, sdv->values[0]);
                        pContext->nCurrentVisualizerBands = sdv->values[0];
                    }
                }
                else
                {
                    ALOG3("DS_PARAM_SINGLE_DEVICE_VALUE Specified device is in-use, calling ak_set_bulk(%i/%s, [%i-%i])=[...]", fp.parameter, pContext->paramNames[fp.parameter], fp.offset, fp.offset + sdv->length - 1);
                    _akSetBulk(pContext, fp.parameter, fp.offset, sdv->length, AK_DATATYPE_INT16, sdv->values);
                    if (strcmp(pContext->paramNames[fp.parameter], "gebf") == 0)
                    {
                        int paramIndex = _akFindParamIndex(pContext, "vcbf");
                        ALOG3("DS_PARAM_SINGLE_DEVICE_VALUE Specified device is in-use, calling ak_set_bulk(%i/vcbf, [%i-%i])=[...]", paramIndex, fp.offset, fp.offset + sdv->length - 1);
                        _akSetBulk(pContext, paramIndex, fp.offset, sdv->length, AK_DATATYPE_INT16, sdv->values);
                    }
                }
                ALOG1("DS_PARAM_SINGLE_DEVICE_VALUE returned from ak_set()/ak_set_bulk()");
            }

            break;
        }
        //
        // DS_PARAM_ALL_VALUES
        //  Value is type DSAllValues
        //
        //  Value:  int16: number of devices and settings to follow
        //          Array of type DSDeviceValues
        //              int32: output device
        //              int16: values[n], where n is specified by previous set of DS_PARAM_DEFINE_SETTINGS.
        //
        case DS_PARAM_ALL_VALUES:
        {
            ALOG2("Effect_setParameter() DS_PARAM_ALL_VALUES");
            DSAllValues *av = (DSAllValues *)pValue;
            ALOG1("DS_PARAM_ALL_VALUES device_count:%d", av->device_count);

            if (pContext->nParams == 0 || pContext->nSettingsPerDevice == 0)
            {
                ALOGE("DS_PARAM_ALL_VALUES No AK parameters or settings have been defined");
                nReturn = -EINVAL;
                break;
            }

            //
            // Regenerate cache.
            //
            ALOG1("DS_PARAM_ALL_VALUES Creating new cache...");
            delete [] pContext->deviceCache;
            delete [] pContext->settingsCache;
            pContext->nDevicesInCache = av->device_count;
            pContext->deviceCache = new DSDevice[pContext->nDevicesInCache];
            pContext->settingsCache = new DSValue[pContext->nDevicesInCache * pContext->nSettingsPerDevice];

            //
            // Copy the new values.
            //
            int bytes_per_device = pContext->nSettingsPerDevice * sizeof(pContext->settingsCache[0]);
            DSDeviceValues *dv = &(av->device_values[0]);
            for (int iDevice = 0; iDevice < av->device_count; ++iDevice)
            {
                ALOG1("DS_PARAM_ALL_VALUES Copying settings for device %i to index %i", dv->device, iDevice);
                pContext->deviceCache[iDevice] = dv->device;
                memcpy(&pContext->settingsCache[iDevice * pContext->nSettingsPerDevice], dv->values, bytes_per_device);
                //Advance the device settings pointer to the start of the next device-settings block.
                dv = (DSDeviceValues *)(((int8_t *)dv->values) + bytes_per_device);
            }

            //
            // Apply new settings.
            //
            ALOG1("DS_PARAM_ALL_VALUES Applying new settings to AK for current device...");
            _checkDeviceAndApplySettingsToAk(pContext);

            break;
        }
        case DS_PARAM_VISUALIZER_ENABLE:
        {
            ALOG2("Effect_setParameter() DS_PARAM_VISUALIZER_ENABLE");
            int *enable = (int *)pValue;
            ALOG1("DS_PARAM_VISUALIZER_ENABLE enable: %i", *enable);

            int venIndex = _akFindParamIndex(pContext, "ven");
            _akSet(pContext, venIndex, 0, *enable);
            break;
        }
        case DS_PARAM_TUNING:
        {
            ALOG2("Effect_setParameter() DS_PARAM_TUNING");

            // TODO: Implement the processing of the case DS_PARAM_TUNING
            ALOGE("Effect_setParameter(): Processing of DS_PARAM_TUNING is NOT implemented!!!");
            nReturn = -EINVAL;
            break;
        }
        default:
        {
            ALOGE("Effect_setParameter() Invalid command %d. Returning -EINVAL(%i)", param, -EINVAL);
            nReturn = -EINVAL;
            break;
        }
    }

    ALOG2("Effect_setParameter() Returning %i", nReturn);
    return nReturn;
}

///
/// Called when data is being retrieved from the effect using EFFECT_CMD_GET_PARAM.
///
/// @param pContext Effect engine context.
/// @param pParam Pointer to parameter (int32_t).
/// @param pValueSize Pointer to variable indicating maximum size of pValue, and updated with the
///     actual size of the data being retrieved.
/// @param pValue Pointer to variable to hold the retrieved data (type and length depends on the parameter).
/// @return 0 on success, and -EINVAL otherwise.
///
int Effect_getParameter(EffectContext     *pContext,
                        void              *pParam,
                        size_t            *pValueSize,
                        void              *pValue)
{
    int32_t param = *((int32_t *)pParam);
    ALOG2("Effect_getParameter(): param=%d", param);
    int nReturn = 0;

    if (pContext->pDs1ap == NULL)
    {
        ALOGE("Effect_getParameter() ERROR: pContext->pDs1ap == NULL");
        return -EINVAL;
    }

    //The structure of pValue will depend on the the parameter
    switch (param)
    {
        //
        // DS_PARAM_VISUALIZER_ENABLE
        //  Value:  visualizer on/off state
        //
        case DS_PARAM_VISUALIZER_ENABLE:
        {
            if (*pValueSize != sizeof(int))
            {
                ALOGE("DS_PARAM_VISUALIZER_ENABLE Invalid value size %i", *pValueSize);
                nReturn = -EINVAL;
            }
            else
            {
                int *pData = (int *)pValue;
                int venIndex = _akFindParamIndex(pContext, "ven");
                *pData = _akGet(pContext, venIndex, 0);
                ALOG2("DS_PARAM_VISUALIZER_ENABLE Visualizer on = %i", *pData);
            }
            break;
        }

        //
        // DS_PARAM_VISUALIZER_DATA
        //  Value:  visualizer data
        //
        case DS_PARAM_VISUALIZER_DATA:
        {
#ifdef DOLBY_DAP_BYPASS_SOUND_TYPES
            if (pContext->bBypass)
            {
                *pValueSize = 0;
                ALOG2("DS_PARAM_VISUALIZER_DATA: No new audio has been processed, as DS is bypassed");
                break;
            }
#endif // DOLBY_DAP_BYPASS_SOUND_TYPES
            if (*pValueSize != pContext->nCurrentVisualizerBands * 2 * sizeof(int16_t))
            {
                ALOGE("DS_PARAM_VISUALIZER_DATA Invalid value size %i", *pValueSize);
                nReturn = -EINVAL;
            }
            else
            {
                //
                // The retrieved visualizer data starts with "vcbg" parameter, and followed by "vcbe" parameter.
                //
                if (pContext->bHasProcessed)
                {
                    int16_t *pData = (int16_t *)pValue;
                    int index = _akFindParamIndex(pContext, "vcbg");
                    _akGetBulk(pContext, index, 0, pContext->nCurrentVisualizerBands, AK_DATATYPE_INT16, pData);
                    pData = pData + pContext->nCurrentVisualizerBands;
                    index = _akFindParamIndex(pContext, "vcbe");
                    _akGetBulk(pContext, index, 0, pContext->nCurrentVisualizerBands, AK_DATATYPE_INT16, pData);
                    pContext->bHasProcessed = false;
                }
                else
                {
                    *pValueSize = 0;
                    ALOG2("DS_PARAM_VISUALIZER_DATA: No new audio has been processed");
                }
#if (DOLBY_DAP_LOG_LEVEL >= 1)
                static int count = 0;
                if (count == 0)
                {
                    ALOG1("Visualizer data retrieved, next 19 logs will be suppressed");
                }
                if (++count == 20)
                    count = 0;
#endif
            }
            break;
        }

        //
        // DS_PARAM_VERSION
        //  Value:  AK version
        //
        case DS_PARAM_VERSION:
        {
            int16_t* pData = (int16_t*)pValue;

            if (*pValueSize != AK_PARAM_VER_ARRAY_LEN * sizeof(int16_t))
            {
                ALOGE("DS_PARAM_VERSION invalid value size %d", *pValueSize);
                nReturn = -EINVAL;
                break;
            }

            // we assume the digital of version can be stored in int16_t
            int verIndex = _akFindParamIndex(pContext, "ver");
            _akGetBulk(pContext, verIndex, 0, AK_PARAM_VER_ARRAY_LEN, AK_DATATYPE_INT16, pData);
            break;
        }

        default:
        {
            ALOGE("Effect_getParameter() Invalid command %d. Returning -EINVAL(%i)", param, -EINVAL);
            nReturn = -EINVAL;
            break;
        }
    }

    ALOG2("Effect_getParameter() Returning %i", nReturn);
    return nReturn;
}

///
/// Part of effect library interface.
///
extern "C" int EffectQueryNumberEffects (uint32_t * pNumEffects)
{
    ALOG2("EffectQueryNumberEffects() start");
    *pNumEffects = sizeof (gDescriptors) / sizeof (const effect_descriptor_t *);
    ALOG1("EffectQueryNumberEffects() pNumEffects=%i", *pNumEffects);
    ALOG2("EffectQueryNumberEffects() end");
    return 0;
}

///
/// Part of effect library interface.
///
extern "C" int EffectQueryEffect (uint32_t index, effect_descriptor_t * pDescriptor)
{
    ALOG2("EffectQueryEffect() start");
    ALOG1("EffectQueryEffect() processing index %d", index);
    if (pDescriptor == NULL)
    {
        ALOGE("EffectQueryEffect() ERROR - called with NULL pointer");
        return -EINVAL;
    }

    if (index >= sizeof (gDescriptors) / sizeof (const effect_descriptor_t *))
    {
        ALOGE("EffectQueryEffect() ERROR - index out of range %d", index);
        return -ENOENT;
    }
    memcpy (pDescriptor, gDescriptors[index], sizeof (effect_descriptor_t));
    ALOG2("EffectQueryEffect() end");
    return 0;
}

///
/// Part of effect library interface.
///
extern "C" int EffectCreate(const effect_uuid_t * uuid, int32_t sessionId, int32_t ioId, effect_handle_t *pHandle)
{
    ALOG2("EffectCreate() sessionId:%i ioId:%i", sessionId, ioId);
    int ret = 0;
    int i;
    int length = sizeof (gDescriptors) / sizeof (const effect_descriptor_t *);
    const effect_descriptor_t *desc;

    if (pHandle == NULL || uuid == NULL)
    {
        ALOGE("EffectCreate() ERROR - called with NULL pointer");
        return -EINVAL;
    }

    //go through the list of effect descriptors and see if we find one that matches
    for (i = 0; i < length; i++)
    {
        desc = gDescriptors[i];
        if (memcmp(uuid, &desc->uuid, sizeof (effect_uuid_t)) == 0)
        {
            ALOG1("EffectCreate() UUID matched effect #%d, UUID = %x", i, desc->uuid.timeLow);
            break;
        }
    }

    if (i == length)
    {
        ALOGW("EffectCreate() No effect found");
        return -ENOENT;
    }

    //
    // Setup context and return the interface
    //
    ALOG1("EffectCreate() Create context and interface");
    EffectContext *pContext = new EffectContext;

    pContext->itfe = &gDsInterface;
    pContext->audioSessionId = sessionId;
    *pHandle = (effect_handle_t)pContext;

    ret = Effect_init(pContext);
    if (ret != 0)
    {
        ALOGE("EffectCreate() ERROR - Effect_init() failed");
        delete pContext;
        return ret;
    }

    ALOG1("EffectCreate(%x) Calling Effect_setConfig()", (unsigned int)pContext);
    ret = Effect_setConfig(pContext);
    if (ret != 0)
    {
        ALOGE("EffectCreate() ERROR - Effect_setConfig() failed");
        delete pContext;
        return ret;
    }

    ALOG2("EffectCreate() end");
    return 0;
}

///
/// Part of effect library interface.
///
extern "C" int EffectRelease(effect_handle_t handle)
{
    EffectContext *pContext = (EffectContext *)handle;

    ALOG2("EffectRelease() start");
    ALOG1("EffectRelease %p", handle);
    if (pContext == NULL)
        return -EINVAL;

    // Close and de-allocate the AK instance.
    if (pContext->pDs1ap != NULL)
    {
        delete pContext->pDs1ap;
        pContext->pDs1ap = NULL;
    }

#ifdef USE_AKBUFFER
    if (pContext->pDs1apBuffer != NULL)
    {
        Ds1apBufferDeinit(pContext->pDs1apBuffer);
        pContext->pDs1apBuffer = NULL;
    }
#endif // USE_AKBUFFER

    delete pContext;

    ALOG2("EffectRelease() end");
    return 0;
}

///
/// Part of effect library interface.
///
extern "C" int EffectGetDescriptor(const effect_uuid_t *uuid, effect_descriptor_t *pDescriptor)
{
    const effect_descriptor_t *desc = NULL;

    ALOG2("EffectGetDescriptor() start");
    if (pDescriptor == NULL || uuid == NULL){
        ALOGV("EffectGetDescriptor() called with NULL pointer");
        return -EINVAL;
    }

    if (memcmp(uuid, &gDsDescriptor.uuid, sizeof(effect_uuid_t)) == 0) {
        desc = &gDsDescriptor;
    }

    if (desc == NULL) {
        return  -EINVAL;
    }

    memcpy(pDescriptor, desc, sizeof(effect_descriptor_t));

    ALOG2("EffectGetDescriptor() end");
    return 0;
}

} // namespace
} // namespace

///
/// Main processing function called by Android.
///
/// Part of effect control interface implementation.
///
int EffectProcess(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer);

extern "C" int Effect_process(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer)
{
#if defined DOLBY_DAP_32BIT_TEST_IN_16BIT_FRAMEWORK
    const int OFFSET = 16;
    const int32_t BUFFER_POOL_SIZE = 4096;
    static int32_t in_pool[BUFFER_POOL_SIZE], out_pool[BUFFER_POOL_SIZE];

    audio_buffer_t in_buf, out_buf;
    int nSamples = inBuffer->frameCount * 2;

    if (nSamples > BUFFER_POOL_SIZE)
    {
        ALOGE("Effect_process() ERROR: nSamples > BUFFER_POOL_SIZE");
        return -EINVAL;
    }
    // Initialize buffers
    in_buf.s32 = in_pool;
    in_buf.frameCount = inBuffer->frameCount;
    out_buf.s32 = (inBuffer->raw != outBuffer->raw) ? out_pool : in_pool;
    out_buf.frameCount = outBuffer->frameCount;

    // Copy input buffer into in_buf shifting it to 32 bits
    for (int i = 0; i < nSamples; ++i)
    {
        in_buf.s32[i] = static_cast<int32_t>(inBuffer->s16[i]) << OFFSET;
    }

    // Copy over output buffer if it is different
    if (inBuffer->raw != outBuffer->raw)
    {
        for (int i = 0; i < nSamples; ++i)
        {
            out_buf.s32[i] = static_cast<int32_t>(outBuffer->s16[i]) << OFFSET;
        }
    }

    // Process 32 bit buffers
    int status = EffectProcess(self, &in_buf, &out_buf);

    if (!status)
    {    
        // Copy output buffer back to 16 bit buffer
        for (int i = 0; i < nSamples; ++i)
        {
            outBuffer->s16[i] = static_cast<int16_t>(out_buf.s32[i] >> OFFSET);
        }
    }
    return status;
#else
    return EffectProcess(self, inBuffer, outBuffer);
#endif // DOLBY_DAP_32BIT_TEST_IN_16BIT_FRAMEWORK
}

int EffectProcess(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer)
{
    android::EffectContext *pContext = (android::EffectContext *)self;

    if (pContext == NULL)
    {
        ALOGE("Effect_process() ERROR: pContext == NULL");
        return -EINVAL;
    }
    else if (pContext->pDs1ap == NULL)
    {
        ALOGE("Effect_process() ERROR: pContext->pDs1ap == NULL");
        return -EINVAL;
    }

    if (inBuffer == NULL || inBuffer->raw == NULL || outBuffer == NULL || outBuffer->raw == NULL || inBuffer->frameCount != outBuffer->frameCount || inBuffer->frameCount == 0)
    {
        ALOGE("Effect_process() ERROR - NULL INPUT POINTER OR BAD FRAME COUNT");
        return -EINVAL;
    }

    #ifndef USE_AKBUFFER
    #error "USE_AKBUFFER must be defined"
    #endif // USE_AKBUFFER

#if (DOLBY_DAP_LOG_LEVEL >= 1 && DOLBY_DAP_LOG_LEVEL < 3)
    if (pContext->_nLogTimer == 0)
        ALOG1("Effect_process(%x) called on session %d (logs suppressed for next second) (inBuffer==outBuffer)?%s outputConfig.accessMode=%s", (unsigned int)pContext, pContext->audioSessionId, inBuffer->raw == outBuffer->raw ? "TRUE!" : "FALSE", (pContext->config.outputCfg.accessMode == EFFECT_BUFFER_ACCESS_ACCUMULATE ? "ACCUMULATE" : (pContext->config.outputCfg.accessMode == EFFECT_BUFFER_ACCESS_WRITE ? "WRITE" : "READ")));
    if ((pContext->_nLogTimer += outBuffer->frameCount) > pContext->config.outputCfg.samplingRate)
        pContext->_nLogTimer = 0;
#endif

    if (!pContext->bFormatSupported)
    {
        ALOG1("Effect_process(%x) input sample rate or input channel count is not supported, so bypass the DS1 effect.", (unsigned int)pContext);
        if (inBuffer->raw != outBuffer->raw)
            memcpy(outBuffer->raw, inBuffer->raw, outBuffer->frameCount * BYTES_PER_FRAME);
        ALOG1("Effect_process() End, return 0");
        return 0;
    }

    // Set the processed flag.
    pContext->bHasProcessed = true;

#ifndef USE_CROSSFADE
    if ((pContext->bEnabled)
#ifdef DOLBY_DAP_BYPASS_SOUND_TYPES
        && (!pContext->bBypass)
#endif // DOLBY_DAP_BYPASS_SOUND_TYPES
    )
    {
        //
        // Process the samples with AK.
        //
        // For accumulate buffer access, the effect needs to mix into the output buffer!
        if (pContext->config.outputCfg.accessMode == EFFECT_BUFFER_ACCESS_ACCUMULATE)
        {
            //Use AK to process in-place, then accumulate into output buffer.
            ALOG3("Effect_process(%x) Calling ak_process() and accumulating inBuffer->raw:%x outBuffer->raw:%x frameCount:%d", (unsigned int)pContext, (uint32_t)(inBuffer->raw), (uint32_t)(outBuffer->raw), outBuffer->frameCount);
            android::_akProcess(pContext->pDs1apBuffer, pContext->pDs1ap, (IO_DATUM *)(inBuffer->raw), (IO_DATUM *)(inBuffer->raw), outBuffer->frameCount);
            android::accumulate(inBuffer, outBuffer, outBuffer->frameCount * NUM_OUT_CHANNELS);
        }
        else
        {
            ALOG3("Effect_process(%x) Calling ak_process() inBuffer->raw:%x outBuffer->raw:%x frameCount:%d", (unsigned int)pContext, (uint32_t)(inBuffer->raw), (uint32_t)(outBuffer->raw), outBuffer->frameCount);
            android::_akProcess(pContext->pDs1apBuffer, pContext->pDs1ap, (IO_DATUM *)(inBuffer->raw), (IO_DATUM *)(outBuffer->raw), outBuffer->frameCount);
        }
    }
    else
    {
        ALOG1("Effect_process() Bypass the DS1 effect since it's disabled or bypassed.");
        if (inBuffer->raw != outBuffer->raw)
            memcpy(outBuffer->raw, inBuffer->raw, outBuffer->frameCount * BYTES_PER_FRAME);
        return -ENODATA;
    }
#else  // USE_CROSSFADE
    //
    // Check if the effect is currently being gracefully disabled or enabled.
    //
    if (
#ifdef DOLBY_DAP_BYPASS_SOUND_TYPES
        (pContext->bBypass) ||
#endif // DOLBY_DAP_BYPASS_SOUND_TYPES
        (!pContext->bEnabled)
        )
    {
        //
        // Effect is being gracefully disabled.
        //
        // Cross-fade between the audio still being processed with AK, and the un-processed audio.
        //
        // Depending on the buffer mode, the processed audio may need to be accumulated into the
        // output buffer. Alternatively, it is possible that the input buffer and output buffer are
        // the same (processing must occur in-place). In that case, the audio is processed in blocks
        // by AK and stored in a temporary buffer for that block. Each block is then cross-faded
        // back into the buffer with a constant volume for that block.
        //
        // When separate input and output buffers are specified, the following process is used:
        //  1. calculate start and end volume level for the size of the buffer based on how much
        //      of the graceful disable period has occurred.
        //  2. copy or accumulate (depending on buffer mode) input buffer into output buffer,
        //      using start and end volume level.
        //  3. process the input buffer with AK in-place.
        //  4. accumulate processed audio in input buffer into output buffer, using inverse start
        //      and end volume level.
        //
        if (inBuffer->raw != outBuffer->raw)
        {
            // Prepare cross fade start and end volumes for this buffer.
            float fCrossStart = pContext->nGracefulSampleCountdown / (float)(pContext->nGracefulSampleCountdownStart);
            float fCrossEnd = (pContext->nGracefulSampleCountdown - (int)(outBuffer->frameCount)) / (float)(pContext->nGracefulSampleCountdownStart);
            if (fCrossEnd < 0.0f)
                fCrossEnd = 0.0f;

            //
            // Two separate input and output buffers specified. The original (input) audio is
            // cross-faded with AK-processed audio into the output buffer.
            // This is done in multiple steps to avoid the use of a temporary buffer.
            //

            //Accumulate or copy the original input samples into output, sweeping from inverse
            // starting to ending volume level. This is the first component of the cross fade.
            //Note the input buffer needs volume to be applied.
            if (pContext->config.outputCfg.accessMode == EFFECT_BUFFER_ACCESS_ACCUMULATE)
            {
                android::accumulateWithVolumeFade(inBuffer, outBuffer, outBuffer->frameCount, 
                    pContext->fLastVolumeL, pContext->fLastVolumeR, 1.0f - fCrossStart, 1.0f - fCrossEnd);
            }
            else
            {
                android::copyWithVolumeFade(inBuffer, outBuffer, outBuffer->frameCount, 
                    pContext->fLastVolumeL, pContext->fLastVolumeR, 1.0f - fCrossStart, 1.0f - fCrossEnd);
            }
            //Process in-place with AK.
            android::_akProcess(pContext->pDs1apBuffer, pContext->pDs1ap, (IO_DATUM *)(inBuffer->raw), 
                (IO_DATUM *)(inBuffer->raw), outBuffer->frameCount);

            //Accumulate the post-processed samples, using the opposite cross-fade volume.
            //Note that AK has now applied volume, so none is required.
            android::accumulateWithVolumeFade(inBuffer, outBuffer, outBuffer->frameCount, 
                                              1.0f, 1.0f, fCrossStart, fCrossEnd);

        }
        else
        {
            //
            // Input and output buffer are the same buffer. This cannot occur for accumulate
            // buffer mode.
            // A temporary buffer is used to hold the AK-processed audio prior to cross-fading
            // back into the original audio in the buffer.
            //
            int primes = outBuffer->frameCount / AK_BLKSIZE;
            IO_DATUM *p = (IO_DATUM *)(inBuffer->raw);
            // Prepare cross fade start volume for this buffer.
            int nCross = pContext->nGracefulSampleCountdownStart - pContext->nGracefulSampleCountdown;
            int nCrossEnd = nCross;
            for (int i = 0; i < primes; ++i, p += (AK_BLKSIZE * pContext->inputChannelCount))
            {
                android::_akProcess(pContext->pDs1apBuffer, pContext->pDs1ap, p, pContext->_akPrimerBuffer, AK_BLKSIZE);

                // Prepare cross fade end volume for this buffer.
                nCrossEnd += AK_BLKSIZE;
                if (nCrossEnd > pContext->nGracefulSampleCountdownStart)
                    nCrossEnd = pContext->nGracefulSampleCountdownStart;

                android::applyVolumeAndCrossFade(p, pContext->_akPrimerBuffer, AK_BLKSIZE, pContext->fLastVolumeL, pContext->fLastVolumeR, nCross / (float)pContext->nGracefulSampleCountdownStart, (nCrossEnd - 1) / (float)pContext->nGracefulSampleCountdownStart);
                nCross = nCrossEnd;

            }
            //
            // If buffers are not multiples of AK_BLKSIZE frames, there may be residual frames
            // remaining in the input.
            //
            int remainder = outBuffer->frameCount % AK_BLKSIZE;
            ALOG3("Effect_process() - 'Input and output buffer are the same buffer' remainder = %i", remainder);
            if (remainder > 0)
            {
                android::_akProcess(pContext->pDs1apBuffer, pContext->pDs1ap, p, pContext->_akPrimerBuffer, remainder);

                // Prepare cross fade end volume for this buffer.
                nCrossEnd += remainder;
                if (nCrossEnd > pContext->nGracefulSampleCountdownStart)
                    nCrossEnd = pContext->nGracefulSampleCountdownStart;

                android::applyVolumeAndCrossFade(p, pContext->_akPrimerBuffer, remainder, pContext->fLastVolumeL, pContext->fLastVolumeR, nCross / (float)pContext->nGracefulSampleCountdownStart, (nCrossEnd - 1) / (float)pContext->nGracefulSampleCountdownStart);
            }
        }

        //
        // Check if graceful disable period has finished.
        //
        if ((pContext->nGracefulSampleCountdown -= outBuffer->frameCount) <= 0)
        {
            pContext->nGracefulSampleCountdown = 0;
            ALOG1("Effect_process() Graceful disable finished. Returning -ENODATA");
            return -ENODATA;
        }
        ALOG2("Effect_process() Effect being gracefully disabled. countdown=%i", pContext->nGracefulSampleCountdown);
    }
    else if (pContext->nGracefulSampleCountdown > 0)
    {
        //
        // Effect is being gracefully enabled.
        //
        // Cross-fade between the un-processed input buffer, and audio processed with AK.
        //
        // Depending on the buffer mode, the processed audio may need to be accumulated into the
        // output buffer. Alternatively, it is possible that the input buffer and output buffer are
        // the same (processing must occur in-place). In that case, the audio is processed in blocks
        // by AK and stored in a temporary buffer for that block. Each block is then cross-faded
        // back into the buffer with a constant volume for that block.
        //
        // When separate input and output buffers are specified, the following process is used:
        //  1. calculate start and end volume level for the size of the buffer based on how much
        //      of the graceful enabled period has occurred.
        //  2. copy or accumulate (depending on buffer mode) input buffer into output buffer,
        //      using start and end volume level.
        //  3. process the input buffer with AK in-place.
        //  4. accumulate processed audio in input buffer into output buffer, using inverse start
        //      and end volume level.
        //
        // NOTE: Because AK has not been running and was reset, inserting it immediately into the
        // audio processing can cause a glitch. The glitch is partly avoided by the cross-fading
        // during the graceful enable period. However the first few buffers processed by AK
        // will actually be muted, so these must be consumed first.
        //
        if (inBuffer->raw != outBuffer->raw)
        {
            // Prepare cross fade start and end volumes for this buffer.
            float fCrossStart = pContext->nGracefulSampleCountdown / (float)(pContext->nGracefulSampleCountdownStart);
            float fCrossEnd = (pContext->nGracefulSampleCountdown - (int)(outBuffer->frameCount)) / (float)(pContext->nGracefulSampleCountdownStart);
            if (fCrossEnd < 0.0f)
                fCrossEnd = 0.0f;

            //
            // Two separate input and output buffers specified. The original (input) audio is
            // cross-faded with AK-processed audio into the output buffer.
            // This is done in multiple steps to avoid the use of a temporary buffer.
            //

            // Accumulate or copy the original input samples into output, sweeping from a starting
            // to ending volume level. This is the first component of the cross fade.
            //Note the input buffer needs volume to be applied.
            if (pContext->config.outputCfg.accessMode == EFFECT_BUFFER_ACCESS_ACCUMULATE)
            {
                android::accumulateWithVolumeFade(inBuffer, outBuffer, outBuffer->frameCount, pContext->fLastVolumeL, pContext->fLastVolumeR, fCrossStart, fCrossEnd);
            }
            else
            {
                android::copyWithVolumeFade(inBuffer, outBuffer, outBuffer->frameCount, pContext->fLastVolumeL, pContext->fLastVolumeR, fCrossStart, fCrossEnd);
            }    
            //Process in-place with AK.
            android::_akProcess(pContext->pDs1apBuffer, pContext->pDs1ap, (IO_DATUM *)(inBuffer->raw), (IO_DATUM *)(inBuffer->raw), outBuffer->frameCount);

            //Accumulate the post-processed samples, using the inverse cross-fade volume.
            //Note that AK has now applied volume, so none is required.
            android::accumulateWithVolumeFade(inBuffer, outBuffer, outBuffer->frameCount, 
                                                1.0f, 1.0f, 1.0f - fCrossStart, 1.0f - fCrossEnd);
        }
        else
        {
            //
            // Input and output buffer are the same buffer. This cannot occur for accumulate
            // buffer mode.
            // A temporary buffer is used to hold the AK-processed audio prior to cross-fading
            // back into the original audio in the buffer.
            //
            int primes = outBuffer->frameCount / AK_BLKSIZE;
            IO_DATUM *p = (IO_DATUM *)(inBuffer->raw);
            // Prepare cross fade start volume for this buffer.
            int nCross = pContext->nGracefulSampleCountdown;
            int nCrossEnd = nCross;
            float fstart;
            float fend;

            for (int i = 0; i < primes; ++i, p += (AK_BLKSIZE * pContext->inputChannelCount))
            {
                android::_akProcess(pContext->pDs1apBuffer, pContext->pDs1ap, p, pContext->_akPrimerBuffer, AK_BLKSIZE);

                // Prepare cross fade end volume for this buffer.
                nCrossEnd -= AK_BLKSIZE;
                if (nCrossEnd < 0)
                    nCrossEnd = 0;

                // If nCrossEnd > countdown start, this indicates we are in the "pre-crossfade" period, so
                // a crossfade ratio of 1.0 will be used which means none of th AK processed audio will be audible.
                if (nCrossEnd > pContext->nGracefulSampleCountdownStart)
                {
                    fstart = 1.0f;
                    fend = 1.0f;
                }
                else
                {
                    fend = (nCrossEnd + 1) / (float)pContext->nGracefulSampleCountdownStart;
                    // This is the case, when nCrossEnd is not in "pre-crossfade" region, but the start nCross
                    // is in that region.
                    if (nCross > pContext->nGracefulSampleCountdownStart)
                        fstart = 1.0f;
                    else
                        fstart = nCross / (float)pContext->nGracefulSampleCountdownStart;
                }
                android::applyVolumeAndCrossFade(p, pContext->_akPrimerBuffer, AK_BLKSIZE, pContext->fLastVolumeL, pContext->fLastVolumeR, fstart, fend);
                nCross = nCrossEnd;
            }
            //
            // If buffers are not multiples of AK_BLKSIZE frames, there may be residual frames
            // remaining in the input.
            //
            int remainder = outBuffer->frameCount % AK_BLKSIZE;
            if (remainder > 0)
            {
                //Process the remainder in-place
                android::_akProcess(pContext->pDs1apBuffer, pContext->pDs1ap, p, pContext->_akPrimerBuffer, remainder);

                // Prepare cross fade end volume for this buffer.
                nCrossEnd -= remainder;
                if (nCrossEnd < 0)
                    nCrossEnd = 0;
                // If nCrossEnd > countdown start, this indicates we are in the "pre-crossfade" period, so
                // a crossfade ratio of 1.0 will be used which means none of th AK processed audio will be audible.
                if (nCrossEnd > pContext->nGracefulSampleCountdownStart)
                {
                    fstart = 1.0f;
                    fend = 1.0f;
                }
                else
                {
                    fend = (nCrossEnd + 1) / (float)pContext->nGracefulSampleCountdownStart;
                    // This is the case, when nCrossEnd is not in "pre-crossfade" region, but the start nCross
                    // is in that region.
                    if (nCross > pContext->nGracefulSampleCountdownStart)
                        fstart = 1.0f;
                    else
                        fstart = nCross / (float)pContext->nGracefulSampleCountdownStart;
                }
                android::applyVolumeAndCrossFade(p, pContext->_akPrimerBuffer, remainder, pContext->fLastVolumeL, pContext->fLastVolumeR, fstart, fend);

            }
        }

        //
        // Check if graceful enable period has finished.
        //
        if ((pContext->nGracefulSampleCountdown -= outBuffer->frameCount) <= 0)
        {
            pContext->nGracefulSampleCountdown = 0;
            ALOG1("Effect_process() Graceful enable finished.");
        }
        else
        {
            ALOG2("Effect_process() Effect being gracefully enabled. countdown=%i", pContext->nGracefulSampleCountdown);
        }
    }
    else
    {
        //
        // Process the samples with AK.
        //
        // For accumulate buffer access, the effect needs to mix into the output buffer!
        if (pContext->config.outputCfg.accessMode == EFFECT_BUFFER_ACCESS_ACCUMULATE)
        {
            //Use AK to process in-place, then accumulate into output buffer.
            ALOG3("Effect_process(%x) Calling ak_process() and accumulating inBuffer->raw:%x outBuffer->raw:%x frameCount:%d", (unsigned int)pContext, (uint32_t)(inBuffer->raw), (uint32_t)(outBuffer->raw), outBuffer->frameCount);
#ifdef DOLBY_DAP_OPENSLES_LPA
            if (pContext->lpaOutputBufferSize < pContext->outputChannelCount * outBuffer->frameCount)
            {
                delete [] pContext->lpaOutputBuffer;
                pContext->lpaOutputBuffer = new audio_buffer_t[pContext->outputChannelCount * outBuffer->frameCount];
                pContext->lpaOutputBufferSize = pContext->outputChannelCount * outBuffer->frameCount;
            }
            android::_akProcess(pContext->pDs1apBuffer, pContext->pDs1ap, (IO_DATUM *)(inBuffer->raw), (IO_DATUM *)(pContext->lpaOutputBuffer->raw), outBuffer->frameCount);
            android::accumulate(pContext->lpaOutputBuffer, outBuffer, pContext->outputChannelCount * outBuffer->frameCount);
#else
            android::_akProcess(pContext->pDs1apBuffer, pContext->pDs1ap, (IO_DATUM *)(inBuffer->raw), (IO_DATUM *)(inBuffer->raw), outBuffer->frameCount);
            android::accumulate(inBuffer, outBuffer, pContext->outputChannelCount * outBuffer->frameCount);
#endif // DOLBY_DAP_OPENSLES_LPA
        }
        else
        {
            ALOG3("Effect_process(%x) Calling ak_process() inBuffer->raw:%x outBuffer->raw:%x frameCount:%d", (unsigned int)pContext, (uint32_t)(inBuffer->raw), (uint32_t)(outBuffer->raw), outBuffer->frameCount);
            android::_akProcess(pContext->pDs1apBuffer, pContext->pDs1ap, (IO_DATUM *)(inBuffer->raw), (IO_DATUM *)(outBuffer->raw), outBuffer->frameCount);
        }
    }
#endif // USE_CROSSFADE

    ALOG3("Effect_process() End, return 0");
    return 0;
}

///
/// Main command function called by Android.
///
/// Part of effect control interface implementation.
///
extern "C" int Effect_command
(
    effect_handle_t self,
    uint32_t cmdCode,
    uint32_t cmdSize,
    void *pCmdData,
    uint32_t * replySize,
    void *pReplyData
)
{
    ALOG2("Effect_command()");
    android::EffectContext *pContext = (android::EffectContext *)self;
    if (pContext == NULL)
    {
        ALOGE("Effect_command() ERROR - Not initialised.");
        return -EINVAL;
    }
    ALOG2("Effect_command(%x) cmdCode:%d cmdSize:%d replySize:%s%d", (unsigned int)pContext, cmdCode, cmdSize, replySize == NULL ? "NULL" : "", replySize == NULL ? 0 : *replySize);

    switch (cmdCode)
    {
        case EFFECT_CMD_INIT:
        {
            ALOG1("Effect_command() EFFECT_CMD_INIT");
            if (pReplyData == NULL || *replySize != sizeof (int))
            {
                ALOGE("EFFECT_CMD_INIT Invalid command data");
                return -EINVAL;
            }

            //
            // Effect_init is already called from EffectCreate. Hence it need not be called here.
            //
            //*(int *) pReplyData = Effect_init (pContext);
            *(int *)pReplyData = 0;
            ALOG1("EFFECT_CMD_INIT ReplyData=%i", *(int *)pReplyData);
            break;
        }

        case EFFECT_CMD_SET_CONFIG:
        {
            ALOG1("Effect_command() EFFECT_CMD_SET_CONFIG");
            if (pCmdData == NULL || cmdSize != sizeof (effect_config_t) || pReplyData == NULL || *replySize != sizeof (int))
            {
                ALOGE("EFFECT_CMD_SET_CONFIG - Invalid command data");
                return -EINVAL;
            }

            //check that the received configuration is legitimate.
            effect_config_t *pConfig = (effect_config_t *)pCmdData;
            if ( (pConfig->outputCfg.accessMode != EFFECT_BUFFER_ACCESS_WRITE && pConfig->outputCfg.accessMode != EFFECT_BUFFER_ACCESS_ACCUMULATE) ||
                 pConfig->inputCfg.samplingRate != pConfig->outputCfg.samplingRate ||
                 pConfig->inputCfg.channels != pConfig->outputCfg.channels ||
                 pConfig->inputCfg.format != pConfig->outputCfg.format ||
                 (pConfig->inputCfg.channels != AUDIO_CHANNEL_OUT_STEREO && pConfig->inputCfg.channels != AUDIO_CHANNEL_OUT_MONO)||
#if defined DOLBY_DAP_32BIT_TEST_IN_16BIT_FRAMEWORK
                pConfig->inputCfg.format != AUDIO_FORMAT_PCM_16_BIT )
#elif defined DOLBY_DAP_32BIT
                pConfig->inputCfg.format != AUDIO_FORMAT_PCM_32_BIT )
#else
                pConfig->inputCfg.format != AUDIO_FORMAT_PCM_16_BIT )
#endif // DOLBY_DAP_32BIT, DOLBY_DAP_32BIT_TEST_IN_16BIT_FRAMEWORK
            {
                *(int *)pReplyData = -EINVAL;
            }
            else
            {
                bool bChangeNeeded = false;
                if (pConfig->inputCfg.samplingRate != pContext->config.inputCfg.samplingRate ||
                    pConfig->inputCfg.channels != pContext->config.inputCfg.channels)
                {
                    bChangeNeeded = true;
                }
                //copy the config from the config blob we've been sent
                memcpy (&pContext->config, pConfig, sizeof (effect_config_t));

                if (bChangeNeeded)
                    *(int *)pReplyData = Effect_setConfig (pContext);
                else
                    *(int *)pReplyData = 0;
            }
            ALOG1("EFFECT_CMD_SET_CONFIG Effect_setConfig() returned, ReplyData=%i", *(int *)pReplyData);
            break;
        }

        case EFFECT_CMD_GET_CONFIG:
        {
            if (pReplyData == NULL || *replySize != sizeof(effect_config_t)) {
                ALOGE("EFFECT_CMD_GET_CONFIG: ERROR");
                return -EINVAL;
            }

            android::Effect_getConfig(pContext, (effect_config_t *)pReplyData);
            break;
        }

        case EFFECT_CMD_RESET:
        {
            ALOG1("Effect_command() EFFECT_CMD_RESET: Invoking Effect_setConfig with current config");
            Effect_setConfig (pContext);
            ALOG1("EFFECT_CMD_RESET - Effect_setConfig() returned");
            break;
        }

        case EFFECT_CMD_ENABLE:
        {
            //
            // This occurs prior to Effect_process being called.
            //
            ALOG1("Effect_command() EFFECT_CMD_ENABLE");
            if (pReplyData == NULL || *replySize != sizeof (int))
            {
                ALOGE("EFFECT_CMD_ENABLE Invalid command data");
                return -EINVAL;
            }
            if (pContext->bEnabled)
            {
                ALOG1("EFFECT_CMD_ENABLE - Already enabled, ignoring.");
            }
            else
            {
                pContext->bEnabled = true;
#ifdef DOLBY_DAP_BYPASS_SOUND_TYPES
                if (!pContext->bBypass)
#endif // DOLBY_DAP_BYPASS_SOUND_TYPES
                {
                    #if DOLBY_DAP_LOG_LEVEL >= 1
                    pContext->_nLogTimer = 0;
                    #endif

                    #ifdef USE_CROSSFADE
                    //
                    // Gracefully enable the effect. If AK processing is suddenly resumed,
                    // there is a noticeable glitch because AK was previously not being
                    // used.
                    // An additional "pre-crossfade" period is added to the regular crossfade countdown. During this period, the output of AK
                    // will not be mixed in with the crossfade.
                    pContext->nGracefulSampleCountdown = pContext->nGracefulSampleCountdownStart + PRE_CROSSFADE_SAMPLES;
                    ALOG1("EFFECT_CMD_ENABLE Starting graceful enable over %i samples", pContext->nGracefulSampleCountdown);
                    #endif // USE_CROSSFADE
                }
            }
            *(int *)pReplyData = 0;
            ALOG1("EFFECT_CMD_ENABLE finished, ReplyData=%i", *(int *)pReplyData);
            break;
        }

        case EFFECT_CMD_DISABLE:
        {
            //
            // This occurs to signal that the effect is to be disabled. Effect_process will
            // continue to be called so it can disabled gracefully until it returns no-more-data.
            //
            ALOG1("Effect_command() EFFECT_CMD_DISABLE");
            if (pReplyData == NULL || *replySize != sizeof (int))
            {
                ALOGE("EFFECT_CMD_DISABLE Invalid command data");
                return -EINVAL;
            }
            if (!pContext->bEnabled)
            {
                ALOG1("EFFECT_CMD_DISABLE - Already disabled, ignoring.");
            }
            else
            {
                pContext->bEnabled = false;
#ifdef DOLBY_DAP_BYPASS_SOUND_TYPES
                if (!pContext->bBypass)
#endif // DOLBY_DAP_BYPASS_SOUND_TYPES
                {
                    #ifdef USE_CROSSFADE
                    //
                    // Gracefully disable the effect.
                    //
                    pContext->nGracefulSampleCountdown = pContext->nGracefulSampleCountdownStart;
                    ALOG1("EFFECT_CMD_DISABLE Starting graceful disable over %i samples", pContext->nGracefulSampleCountdown);
                    #endif // USE_CROSSFADE
                }
            }
            *(int *)pReplyData = 0;
            break;
        }

        case EFFECT_CMD_GET_PARAM:
        {
            ALOG2("Effect_command() EFFECT_CMD_GET_PARAM");

            if (pCmdData == NULL || cmdSize < (int)(sizeof(effect_param_t) + sizeof(int32_t)) || pReplyData == NULL || *replySize < (int)(sizeof(effect_param_t) + sizeof(int32_t)))
            {
                ALOGE("Effect_command() EFFECT_CMD_GET_PARAM - Invalid command data");
                return -EINVAL;
            }

            effect_param_t *p = (effect_param_t *)pCmdData;

            memcpy(pReplyData, pCmdData, sizeof(effect_param_t) + p->psize);

            p = (effect_param_t *)pReplyData;

            //total size of parameter + padding ie., offset to value ((psize - 1)/sizeof(int) + 1) * sizeof(int)
            int v_offset = ((p->psize - 1) / sizeof (int) + 1) * sizeof (int);
            ALOG3("Effect_command() EFFECT_CMD_GET_PARAM - Offset from start param data to value data: %d", v_offset);

            ALOG2("Effect_command() EFFECT_CMD_GET_PARAM - Calling Effect_getParameter()");
            p->status = android::Effect_getParameter (pContext, (void *) p->data, (size_t *)&p->vsize, p->data + v_offset);
            ALOG2("Effect_command() EFFECT_CMD_GET_PARAM - Returned from AK_getParamter()");

            *replySize = sizeof(effect_param_t) + v_offset + p->vsize;

            ALOG3("\tDs Effect_command EFFECT_CMD_GET_PARAM "
                    "*pCmdData %d, *replySize %d, *pReplyData %d ",
                    *(int32_t *)((char *)pCmdData + sizeof(effect_param_t)),
                    *replySize,
                    *(int16_t *)((char *)pReplyData + sizeof(effect_param_t) + v_offset));

            break;
        }

        case EFFECT_CMD_SET_PARAM:
        {
            ALOG1("Effect_command() EFFECT_CMD_SET_PARAM");

            if (pCmdData == NULL || (cmdSize < (int)(sizeof (effect_param_t) + sizeof (int32_t))) || pReplyData == NULL || *replySize != (int)sizeof(int32_t))
            {
                ALOGE("EFFECT_CMD_SET_PARAM Invalid command data");
                return -EINVAL;
            }

            effect_param_t *p = (effect_param_t *)pCmdData;

            //
            // The parameter code is an int32
            //
            if (p->psize != sizeof(int32_t))
            {
                ALOGE("EFFECT_CMD_SET_PARAM - ERROR, psize is not sizeof(int32_t)");
                return -EINVAL;
            }

            ALOG3("EFFECT_CMD_SET_PARAM cmdSize is %d\n"
                "\tsizeof(effect_param_t) is  %d\n"
                "\tp->psize is %d\n"
                "\tp->vsize is %d"
                "\n", cmdSize, sizeof (effect_param_t), p->psize, p->vsize);

            //total size of parameter + padding ie., offset to value ((psize - 1)/sizeof(int) + 1) * sizeof(int)
            int v_offset = ((p->psize - 1) / sizeof (int) + 1) * sizeof (int);
            ALOG2("EFFECT_CMD_SET_PARAM Offset from start param data to value data: %d", v_offset);

            ALOG2("Effect_command() EFFECT_CMD_SET_PARAM Calling Effect_setParameter()");
            *(int *) pReplyData = android::Effect_setParameter (pContext, (void *) p->data, p->data + v_offset);
            ALOG2("EFFECT_CMD_SET_PARAM Returned from Effect_setParameter()");

            break;
        }

        case EFFECT_CMD_SET_DEVICE:
        {
            ALOG2("Effect_command() EFFECT_CMD_SET_DEVICE");
            audio_devices_t device = *(audio_devices_t *)pCmdData;

            ALOG1("EFFECT_CMD_SET_DEVICE audio_device:%d", device);

            if (pContext->activeDevice != (int)device) {
                bool bChangeNeeded = false;
                int oldDevice = pContext->activeDevice;
                pContext->activeDevice = device;
                if (android::_mapDeviceToAkEndpoint(device) != android::_mapDeviceToAkEndpoint(oldDevice))
                {
                    bChangeNeeded = true;
                }
                else
                {
                    int deviceIndex = android::_getDeviceIndex(pContext);
                    if (deviceIndex >= 0 && deviceIndex < pContext->nDevicesInCache)
                    {
                        if (pContext->deviceCache[deviceIndex] != pContext->deviceUsed)
                        {
                            bChangeNeeded = true;
                        }
                    }
                }
                if (bChangeNeeded)
                {
                    ALOG1("EFFECT_CMD_SET_DEVICE Applying settings to AK for new device...");
                    Effect_setConfig (pContext);
                }
            }

            ALOG2("EFFECT_CMD_SET_DEVICE End");
            break;
        }
        #ifdef DOLBY_DAP_OPENSLES_PREGAIN
        case EFFECT_CMD_DOLBY_SET_PREGAIN:
        {
            ALOG2("EFFECT_CMD_DOLBY_SET_PREGAIN L:%x R:%x", *(uint32_t *)pCmdData, *(((uint32_t *)pCmdData) + 1));

            if (pContext->audioSessionId == DS_GLOBAL_EFFECT_AUDIO_SESSIONID)
            {
                #ifdef AK_AUDIO_DUMP
                curVolume = *(uint32_t *)pCmdData;
                if (curVolume < AUDIO_DUMP_VOL_MIN)
                {
                    if (pFilePcmInputDump != NULL)
                    {
                        ALOG2("DOLBY_AUDIO_DUMP: Close the file for PCM Input dump");
                        fclose(pFilePcmInputDump);
                        pFilePcmInputDump = NULL;
                    }
                    if (pFilePcmOutputDump != NULL)
                    {
                        ALOG2("DOLBY_AUDIO_DUMP: Close the file for PCM Output dump");
                        fclose(pFilePcmOutputDump);
                        pFilePcmOutputDump = NULL;
                    }
                    bAudioDumpStopped = true;
                }
                #endif // AK_AUDIO_DUMP

                int akPregVal = android::calculateExternalGain( *(uint32_t *)pCmdData, *(((uint32_t *)pCmdData) + 1));
                //
                // Set the AK pre-gain attenuation based on the input Android volume.
                //
                if (pContext->pDs1ap != NULL) {
                    int pregIndex = _akFindParamIndex(pContext, "preg");
                    android::_akSet(pContext, pregIndex, 0, akPregVal);
                } else {
                    ALOGE("EFFECT_CMD_DOLBY_SET_PREGAIN ERROR - pContext->pDs1ap == NULL");
                }

                //
                // Send back a new attenuation level only if pReplyData is valid.
                //
                if (pReplyData != NULL)
                {
                    ALOG1("EFFECT_CMD_DOLBY_SET_PREGAIN Returning current volume level");
                    (*(uint32_t *)pReplyData) = (*(uint32_t *)pCmdData);
                    (*(((uint32_t *)pReplyData) + 1)) = (*(((uint32_t *)pCmdData) + 1));
                }
            }
            break;
        }
        #endif // DOLBY_DAP_OPENSLES_PREGAIN

        case EFFECT_CMD_SET_VOLUME:
        {
            ALOG2("EFFECT_CMD_SET_VOLUME L:%x R:%x", *(uint32_t *)pCmdData, *(((uint32_t *)pCmdData) + 1));

            #ifdef DOLBY_DAP_OPENSLES_LPA_POSTGAIN
            //
            // We assume in case of LPA, volume is applied post the effects and EFFECT_CMD_SET_VOLUME is called from AudioFlinger.cpp.
            // Hence the DAP "postgain" parameter is set.
            //
            if (pContext->audioSessionId != DS_GLOBAL_EFFECT_AUDIO_SESSIONID) // Assumed to be an LPA mode session
            {
                int akPstgVal = android::calculateExternalGain( *(uint32_t *)pCmdData, *(((uint32_t *)pCmdData) + 1));
                //
                // Set the AK post-gain attenuation based on the LPA track volume.
                //
                if (pContext->pDs1ap != NULL) {
                    android::_akSet(pContext, _akFindParamIndex(pContext, "pstg"), 0, akPstgVal);
                } else {
                    ALOGE("EFFECT_CMD_SET_VOLUME ERROR - pContext->pDs1ap == NULL");
                }

                //
                // Send back a new attenuation level only if pReplyData is valid.
                //
                if (pReplyData != NULL)
                {
                    ALOG1("EFFECT_CMD_SET_VOLUME Returning current volume level");
                    (*(uint32_t *)pReplyData) = (*(uint32_t *)pCmdData);
                    (*(((uint32_t *)pReplyData) + 1)) = (*(((uint32_t *)pCmdData) + 1));
                }
            }
            #endif // DOLBY_DAP_OPENSLES_LPA_POSTGAIN
            break;
        }

        case EFFECT_CMD_SET_AUDIO_MODE:
        {
            ALOG2("Effect_command() EFFECT_CMD_SET_AUDIO_MODE");
            audio_mode_t mode = *(audio_mode_t *)pCmdData;
            ALOG1("EFFECT_CMD_SET_AUDIO_MODE audio_mode:%d", mode);
            pContext->audioMode = mode;
            //!!!ERA code to modify behaviour based on in-call/ringing etc.

            ALOG2("EFFECT_CMD_SET_AUDIO_MODE End");
            break;
        }

#ifdef DOLBY_DAP_BYPASS_SOUND_TYPES
        case EFFECT_CMD_DOLBY_DAP_SET_BYPASS:
        {
            uint32_t crossFaded;

            pContext->bBypass = *(uint32_t *)pCmdData;
            crossFaded        = *(((uint32_t *)pCmdData) + 1);
            ALOG2("Effect_command() EFFECT_CMD_DOLBY_DAP_SET_BYPASS");
            ALOGE("EFFECT_CMD_DOLBY_DAP_SET_BYPASS: bBypass:%d, crossFade:%d", pContext->bBypass, crossFaded);

#ifdef USE_CROSSFADE
            // if bBypass == 0 and bCrossFaded ==1, then set the value of nGracefulSampleCountdown for process to work correctly.
            // if bBypass == 1, then process using both the bools correctly.
            if (pContext->bBypass == false)
            {
                pContext->nGracefulSampleCountdown = ((crossFaded == true) && pContext->bEnabled) ? pContext->nGracefulSampleCountdownStart + PRE_CROSSFADE_SAMPLES : 0;
            }
            else
            {
                pContext->nGracefulSampleCountdown = ((crossFaded == true) && pContext->bEnabled) ? pContext->nGracefulSampleCountdownStart : 0;
            }
#endif // USE_CROSSFADE
            break;
        }
#endif // DOLBY_DAP_BYPASS_SOUND_TYPES
        default:
        {
            ALOGE("Effect_command() Unknown command code:%d", cmdCode);
            return -EINVAL;
        }
    }
    return 0;
}

///
/// Main getDescriptor function called by Android.
///
/// Part of effect control interface implementation.
///
extern "C" int Effect_getDescriptor(effect_handle_t   self,
                                    effect_descriptor_t *pDescriptor)
{
    android::EffectContext * pContext = (android::EffectContext *)self;
    const effect_descriptor_t *desc;

    if (pContext == NULL || pDescriptor == NULL) {
        ALOGV("Effect_getDescriptor() invalid param");
        return -EINVAL;
    }

    desc = &android::gDsDescriptor;

    memcpy(pDescriptor, desc, sizeof(effect_descriptor_t));

    return 0;
}


///
/// Implementation of effect_interface_t interface for the Ds effect.
///
const struct effect_interface_s gDsInterface =
{
    Effect_process,
    Effect_command,
    Effect_getDescriptor,
    NULL
};

audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
    tag : AUDIO_EFFECT_LIBRARY_TAG,
    version : EFFECT_LIBRARY_API_VERSION,
    name : "Effect Ds Library",
    implementor : "Dolby Laboratories",
    //query_num_effects : android::EffectQueryNumberEffects,
    //query_effect : android::EffectQueryEffect,
    create_effect : android::EffectCreate,
    release_effect : android::EffectRelease,
    get_descriptor : android::EffectGetDescriptor,
};
