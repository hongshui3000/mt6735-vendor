/******************************************************************************
 *  This program is protected under international and U.S. copyright laws as
 *  an unpublished work. This program is confidential and proprietary to the
 *  copyright owners. Reproduction or disclosure, in whole or in part, or the
 *  production of derivative works therefrom without the express permission of
 *  the copyright owners is prohibited.
 *
 *                 Copyright (C) 2011-2012 by Dolby Laboratories,
 *                             All rights reserved.
 ******************************************************************************/

#ifndef ANDROID_EFFECTDS_H_
#define ANDROID_EFFECTDS_H_

#include "effect_ds.h"

#if __cplusplus
extern "C" {
#endif

typedef struct _DSSpeakerCoeffs
{
    int32_t sample_rate_index;
    int32_t size;
    char coeffs[0];
} __attribute__((packed)) DSSpeakerCoeffs;

typedef struct _DSParameterOffset
{
    int8_t  parameter;
    int16_t offset;
} __attribute__((packed)) DSParameterOffset;

typedef struct _DSDefineParams
{
    int16_t count;
    char definitions[0];
} __attribute__((packed)) DSDefineParams;

typedef struct _DSDefineSettings
{
    int16_t count;
    DSParameterOffset definitions[0];
} __attribute__((packed)) DSDefineSettings;

typedef int32_t DSDevice;
typedef int16_t DSValue;

typedef struct _DSSingleDeviceValue
{
    DSDevice device;
    int16_t setting_index;
    int16_t length;
    DSValue values[0];
} __attribute__((packed)) DSSingleDeviceValue;

typedef struct _DSDeviceValues
{
    DSDevice device;
    DSValue values[0];
} __attribute__((packed)) DSDeviceValues;

typedef struct _DSAllValues
{
    int16_t device_count;
    DSDeviceValues device_values[0];
} __attribute__((packed)) DSAllValues;

#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECTDS_H_*/

