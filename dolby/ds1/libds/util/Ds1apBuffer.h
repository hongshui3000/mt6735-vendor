/*
 * This program is protected under international and U.S. copyright laws as
 * an unpublished work. This program is confidential and proprietary to the
 * copyright owners. Reproduction or disclosure, in whole or in part, or the
 * production of derivative works therefrom without the express permission of
 * the copyright owners is prohibited.
 *
 *              Copyright (C) 2011-2012 by Dolby Laboratories,
 *                            All rights reserved.
 *
 */


/**
 * Buffering support for DS1AP processing.
 *
 * This module implements a buffering mechanism allowing the caller to request
 * processing of arbitrarily-sized buffers of samples by DS1AP. This module uses
 * two buffers that need only be the minimum size of 256 samples as required by
 * DS1AP. As a result, the latency (delay) introduced need only be 256 samples.
 *
 */
#ifndef _DS1APBUFFER_H
#define _DS1APBUFFER_H

/**
 * DS1APBUFFER_MODE.
 * Determines which buffer processing mode will be supported by Ds1apBuffer in the
 * case of one same in/out buffer setup.
 * See Ds1apBufferConfig.h for details of each mode. The mode is chosen in
 * Ds1apBufferConfig.h by defining DS1APBUFFER_MODE to one of the defined values.
*/
#define DS1APBUFFER_MODE_IN_PLACE_INTERNALBUFFER     1
#define DS1APBUFFER_MODE_IN_PLACE_SHIFT              2
#ifdef __cplusplus
extern "C"
{
#endif

#include "ak.h"
#include "ak_process_loop.h" /* for IO_DATUM */

/* Include the configuration/mode for Ds1apBuffer */
#include "Ds1apBufferConfig.h"


/**
 * Initialises the DS1AP buffered processing module.
 *
 * THIS FUNCTION MUST BE CALLED EACH TIME THE INPUT AUDIO FORMAT CHANGES.
 *
 * DS1AP MUST BE RE-INITIALISED EACH TIME THE INPUT AUDIO FORMAT CHANGES.
 *
 * NOTE: The current implementation of this module uses statically-allocated
 * buffers at file scope, sized to fit the maximum number of input and output
 * channels. These buffers will be quite small (e.g. 8 channels * 32 samples if
 * BUFFER_SAMPLES is 32). If it is required that these are dynamically
 * allocated, this should be done in this function.
 *
 * @param nInChannels       The number of channels per sample in the input
 *                          audio data that will be supplied to
 *                          Ds1apBufferProcessSamples(). THIS MUST BE THE SAME AS
 *                          THE DS1AP_IN_CHANS SETTING OF DS1AP.
 *
 * @param nOutChannels      The number of channels per sample that is to be
 *                          expected from DS1AP. This is usually 2, unless a
 *                          special "mono-output" DS1AP version is used, in which
 *                          case it must be the same as the DS1AP_OUT_CHANS
 *                          setting.
 *
 * @param nInitialInternalBufferSamples (Only available when DS1APBUFFER_MODE is
 *                                      DS1APBUFFER_MODE_IN_PLACE_INTERNALBUFFER)
 *                                      The number of samples per channel that the internal
 *                                      buffer will be allocated with. This is therefore the
 *                                      minimum size of the internal buffer.
 *
 * @return A pointer to the new instance of Ds1apBuffer, or NULL on failure.
*/
void *Ds1apBufferInit
(
    int nInChannels,
    int nOutChannels
#if (DS1APBUFFER_MODE == DS1APBUFFER_MODE_IN_PLACE_INTERNALBUFFER)
    , int nInitialInternalBufferSamples
#endif
);


/**
 * De-initialises the DS1AP buffered processing module.
 *
 * If the input and processed buffers are being allocated dynamically, they need
 * to be de-allocated in this function.
 *
 * @param pDs1apBuffer              Pointer to a Ds1apBuffer previously initialised by
 *                                  a call to Ds1apBufferInit().
*/
void Ds1apBufferDeinit(void *pPostProcessBuffer);

/**
 * Accepts input audio data samples for processing, and places the same number
 * of samples in the audio output data. The output data samples correspond to
 * processed input audio data that is 256 samples "old" (assuming BUFFER_SAMPLES
 * is 256). That is, there is a throughput delay introduced equal to the buffer
 * size.
 *
 * When this function is first invoked, the first BUFFER_SAMPLES of processed
 * samples output will contain silence.
 *
 * IF THE INPUT AUDIO FORMAT CHANGES, THIS MODULE MUST BE RE-INITIALISED WITH
 * THE NEW FORMAT, INCLUDING RE-INITIALISATION OF DS1AP.
 *
 * @param pDs1apBuffer              Pointer to an initialised Ds1apBuffer instance.
 *
 * @param ak_hndl                   Pointer to ak handle to go ak_process.
 *
 * @param scratch_mem               Pointer to ak scratch memory that is needed for ak_process.
 *
 *                                  This memory is calculated during ak initialization.
 * @param pInputData                The input audio data samples to be processed. This
 *                                  must contain nSamples for each channel as specified
 *                                  by nInChannels in Ds1apBufferInit().
 *
 * @param pOutputData               Pointer to where processed audio data should be
 *                                  placed. The first samples will correspond to input
 *                                  audio data from an earlier time. This memory must
 *                                  be large enough to contain nSamples * nOutChannels
 *                                  individual samples.
 *
 * @param nSamples                  The number of samples (per channel) in pInputData,
 *                                  and the number of samples to be placed in
 *                                  pOutputData (per channel).
*/
void Ds1apBufferProcessSamples
(
    void *pDs1apBuffer,
    ak_instance *ak_hndl,
    void *scratch_mem,
    IO_DATUM *pInputData,
    IO_DATUM *pOutputData,
    int nSamples
);

#ifdef __cplusplus
}
#endif
#endif /*_DS1APBUFFER_H*/
