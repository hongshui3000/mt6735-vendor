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


#include "ak.h"
#include "ak_process_loop.h"

#include <cutils/log.h> /* for android logging */

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ak_process_loop"


//#define DS1AP_PROFILING

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
                    )
{
    int c;
    int status;
    int sampleCount_loop;
    int loop = 0;
    IO_DATUM *inPtr = inputPtr;
    IO_DATUM *outPtr = outputPtr;
    int audioInChannels = audioInDlbBuffer->nchannel;
    int audioOutChannels = audioOutDlbBuffer->nchannel;

    /* make sure sampleCount is multiples of AK_BLOCK_SIZE, otherwise return error */
    if (sampleCount % AK_BLOCK_SIZE)
    {
        ALOGE("ak_process_loop: sampleCount not in Multiple of %d", AK_BLOCK_SIZE);
        return -1;
    }

    sampleCount_loop = sampleCount / AK_BLOCK_SIZE;

    for (loop = 0; loop < sampleCount_loop; loop++)
    {
        for (c = 0; c < audioInChannels; c++)
        {
            audioInDlbBuffer->ppdata[c] = inPtr + c;
        }

        /** Copy the block of input data into AK's input buffer using ak_set_input_buffer().
         *  This will convert the buffer to AK's internal DLB_LFRACT data type. */
        status = ak_set_input_buffer (ak_hndl, AK_ROOT, audioInDlbBuffer, 0, AK_BLOCK_SIZE);
        if (status != AK_OK)
        {
            ALOGE("ak_set_input_buffer failed %d", status);
            return -1;
        }

#if defined (DS1AP_PROFILING)
        struct timeval time;
        float msec_diff, msec_start, msec_stop;

        gettimeofday(&time, NULL);
        msec_start = time.tv_sec * 1000  + time.tv_usec / 1000;
#endif
        status = ak_process(ak_hndl, scratch_mem);
        if (status != AK_OK)
        {
            ALOGE("ERROR(%d):Failed to process audio.\n", status);
            return -1;
        }
#if defined (DS1AP_PROFILING)
        gettimeofday(&time, NULL);
        msec_stop = time.tv_sec * 1000  + time.tv_usec / 1000;
        msec_diff = msec_stop - msec_start;

        ALOGI("time in msec %f", msec_diff);
#endif

        for (c = 0; c < audioOutChannels; c++)
        {
            audioOutDlbBuffer->ppdata[c]  = outPtr + c;
        }
        /** Copy the resulting output data into the output buffer using ak_get_output_buffer(),
         *  this will convert the data back to the format specified in the buffer descriptor
         *  (which in this example has been specified as 16 bit interleaved shorts). */
        status = ak_get_output_buffer (ak_hndl, AK_ROOT, audioOutDlbBuffer, 0, AK_BLOCK_SIZE);
        if (status != AK_OK)
        {
            ALOGE("ak_get_output_buffer failed %d", status);
            return -1;
        }

        inPtr += (AK_BLOCK_SIZE * audioInChannels);
        outPtr += (AK_BLOCK_SIZE * audioOutChannels);
    }

    return 0;
}
