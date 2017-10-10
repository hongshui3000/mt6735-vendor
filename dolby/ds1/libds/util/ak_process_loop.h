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

#ifndef _AK_PROCESS_LOOP
#define _AK_PROCESS_LOOP

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h> //for int16_t

#if defined DOLBY_DAP_32BIT
#define IO_DATUM        int32_t
#else
#define IO_DATUM        int16_t
#endif

#define AK_BLOCK_SIZE   256



/*
  ak_process_loop

  This method prepares necessary variables before calling ak_process.
  ak_processes exactly 256 samples.

  Note: This function must be called with samples in multiples of 256

  Parameters:
    audioInDlbBuffer            This buffer holds the input channel count and we have
                                to set the pointer based on loop count.

    audioOutDlbBuffer           This buffer holds the output channel count and we have
                                to set the pointer based on loop count.

    ak_hndl                     handle to ak instance

    scratch_mem                 scratch memory pointer which we initilized during ak init

    inputPtr                    input buffer pointer

    outputPtr                   output buffer pointer. inputPtr != outputPtr

    sampleCount                 samples to be processed by AK. Must be 256 not multiples of 32 as in TDAS

  Returns:
    success (0) or failure (-1)
*/
int ak_process_loop(
                    dlb_buffer *audioInDlbBuffer,
                    dlb_buffer *audioOutDlbBuffer,
                    ak_instance *ak_hndl, void *scratch_mem,
                    IO_DATUM *inputPtr, IO_DATUM *outputPtr,
                    int sampleCount
                    );

#ifdef __cplusplus
}
#endif

#endif //_AK_PROCESS_LOOP
