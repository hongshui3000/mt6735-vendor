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

/**
 * Prints time taken for each ak_process.
 * Uncomment it to enable profiling.
 */
//#define DS1AP_PROFILING

#if defined (DS1AP_PROFILING)
#include <sys/time.h>
#endif

#include <stdlib.h>
#include "Ds1apBuffer.h"

/**
* logging modes:
* 0 - No logging.
* 1 - Initialisation/deinitialisation logging for Android.
* 2 - Verbose logging (during processing) for Android.
 */
#define DS1AP_BUFFER_LOGGING 1

#if defined (DS1AP_BUFFER_LOGGING)
#include <cutils/log.h>
#endif


#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG	"Ds1apBuffer"



#define BUFFER_SAMPLES AK_BLOCK_SIZE

/**
 * Maximum number of channels that this module supports for input audio data.
 *
 * This value can be changed if desired.
*/
#define MAX_INPUT_CHANNELS 8

/**
 * Maximum number of channels that this module supports for output audio data.
 *
 * This value can be changed if desired.
*/
#define MAX_OUTPUT_CHANNELS 2

/**
 * Size in bytes required for the processed samples buffer to accommodate
 * the maximum number of channels.
*/
#define MAX_PROCESSEDBUFFER_BYTES (BUFFER_SAMPLES * MAX_OUTPUT_CHANNELS * sizeof(IO_DATUM))

/**
 * Internal state data for Ds1apBuffer module.
*/
typedef struct Ds1apBuffer_t
{
    /*
      _pInputBuffer
      Buffer of input audio sample data awaiting processing by DS1AP.
      This is statically allocated at file scope. This can be changed to dynamic
      allocation in Ds1apBufferInit() if desired. However there 'may' be some
      memory-access benefit to this static stack-based allocation.
    */
    char _pInputBuffer[BUFFER_SAMPLES * MAX_INPUT_CHANNELS * sizeof(IO_DATUM)];

    /*
      _pInputBufferHead
      Head write position (next position to write new samples to) in _pInputBuffer.
    */
    char *_pInputBufferHead;

    /*
      _pProcessedBuffer
      Buffer of processed audio samples from DS1AP.
      This is statically allocated at file scope. This can be changed to dynamic
      allocation in Ds1apBufferInit() if desired. However there 'may' be some
      memory-access benefit to this static stack-based allocation.
    */
    char _pProcessedBuffer[MAX_PROCESSEDBUFFER_BYTES];

    /*
      _pProcessedBufferHead
      Head read position (next sample to read) from _pProcessedBuffer.
    */
    char *_pProcessedBufferHead;

    /*
      _nBufferSampleCount
      Number of samples per channel in _pInputBuffer. Corresponds to the head
      write position (next position to write new samples to) in _pInputBuffer,
      and the head read position (next sample to read) in _pProcessedBuffer.
    */
    int _nBufferSampleCount;

    /*
      _nInBytesPerTotalSample
      Bytes per sample (all channels) being input to DS1AP.
    */
    int _nInBytesPerTotalSample;

    /*
      _nOutBytesPerTotalSample
      Bytes per sample (all channels) being output by DS1AP.
    */
    int _nOutBytesPerTotalSample;

    /*
      _nAudioInChannels
      Number of channels being input to DS1AP.
    */
    int _nAudioInChannels;

    /*
      _nAudioOutChannels
      Number of channels being output by DS1AP.
    */
    int _nAudioOutChannels;

    /*
      _pAudioInChannelData
      Input channel data pointers being input to DS1AP.
    */
    void* _pAudioInChannelData[MAX_INPUT_CHANNELS];

    /*
    _pAudioOutChannelData
    Output channel data pointers being output by DS1AP.
    */
    void* _pAudioOutChannelData[MAX_OUTPUT_CHANNELS];

    /*
      _audioInDlbBuffer
      Input channel dlb buffer being input to DS1AP.
    */
    dlb_buffer  _audioInDlbBuffer;

    /*
     _audioOutDlbBuffer
     Output channel dlb buffer being output by DS1AP.
    */
    dlb_buffer  _audioOutDlbBuffer;


#if (DS1APBUFFER_MODE == DS1APBUFFER_MODE_IN_PLACE_INTERNALBUFFER)
    /*
      _pInternalBuffer
      Internal buffer used during in-place processing.
    */
    void *_pInternalBuffer;
    /*
      _nInternalBufferSamples
      Current size in samples per channel of internal buffer.
    */
    int _nInternalBufferSamples;
#endif /*(DS1APBUFFER_MODE == DS1APBUFFER_MODE_IN_PLACE_INTERNALBUFFER)*/

} Ds1apBuffer;

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
)
{
#if (DS1AP_BUFFER_LOGGING >= 1)
    ALOGI("Ds1apBufferInit");
#endif

    Ds1apBuffer *p = malloc(sizeof(Ds1apBuffer));
    if (p == NULL)
        return NULL;
    p->_nInBytesPerTotalSample = (sizeof(IO_DATUM) * nInChannels);
    p->_nOutBytesPerTotalSample = (sizeof(IO_DATUM) * nOutChannels);

    p->_nAudioInChannels    = nInChannels;
    p->_nAudioOutChannels   = nOutChannels;

    /* Fill audioIn, audioOut dlb_buffer */
    p->_audioInDlbBuffer.nchannel      = p->_nAudioInChannels;
    p->_audioInDlbBuffer.nstride       = p->_nAudioInChannels;
#ifdef DOLBY_DAP_32BIT
    //p->_audioInDlbBuffer.data_type     = DLB_BUFFER_INT_LEFT;
    p->_audioInDlbBuffer.data_type     = DLB_BUFFER_LONG_32;
#else
    p->_audioInDlbBuffer.data_type     = DLB_BUFFER_SHORT_16;
#endif 
    p->_audioInDlbBuffer.ppdata        = p->_pAudioInChannelData;

    p->_audioOutDlbBuffer.nchannel     = p->_nAudioOutChannels;
    p->_audioOutDlbBuffer.nstride      = p->_nAudioOutChannels;
#ifdef DOLBY_DAP_32BIT
//    p->_audioOutDlbBuffer.data_type    = DLB_BUFFER_INT_LEFT;
    p->_audioOutDlbBuffer.data_type    = DLB_BUFFER_LONG_32;
#else
    p->_audioOutDlbBuffer.data_type    = DLB_BUFFER_SHORT_16;
#endif
    p->_audioOutDlbBuffer.ppdata       = p->_pAudioOutChannelData;

    /* Dynamically allocated buffers would be allocated here. */

    /* Fill ProcessedBuffer with silence. */
    memset(p->_pProcessedBuffer, 0, MAX_PROCESSEDBUFFER_BYTES);

    /* Initialise head pointers and sample count. */
    p->_pInputBufferHead = p->_pInputBuffer;
    p->_pProcessedBufferHead = p->_pProcessedBuffer;
    p->_nBufferSampleCount = 0;

#if (DS1APBUFFER_MODE == DS1APBUFFER_MODE_IN_PLACE_INTERNALBUFFER)
    /* Allocate the internal buffer to the initial (minimum) size. */
    p->_nInternalBufferSamples = nInitialInternalBufferSamples;
    p->_pInternalBuffer = malloc(nInitialInternalBufferSamples * p->_nOutBytesPerTotalSample);
#endif
    return p;
}

/**
 * De-initialises the DS1AP buffered processing module.
 *
 * If the input and processed buffers are being allocated dynamically, they need
 * to be de-allocated in this function.
 *
 * @param pDs1apBuffer              Pointer to a Ds1apBuffer previously initialised by
 *                                  a call to Ds1apBufferInit().
*/
void Ds1apBufferDeinit(void *pDs1apBuffer)
{
#if (DS1AP_BUFFER_LOGGING >= 1)
    ALOGI("Ds1apBufferDeinit");
#endif
    /*
     If the input and processed buffers were dynamically allocated, they would
     need to be de-allocated here.
    */

    if (pDs1apBuffer != NULL)
    {
#if (DS1APBUFFER_MODE == DS1APBUFFER_MODE_IN_PLACE_INTERNALBUFFER)
        /* Free the internal buffer. */
        free(((Ds1apBuffer *)pDs1apBuffer)->_pInternalBuffer);
#endif
        free(pDs1apBuffer);
    }
}

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
void
Ds1apBufferProcessSamples
(
    void *pDs1apBuffer,
    ak_instance *ak_hndl,
    void *scratch_mem,
    IO_DATUM *pInputData,
    IO_DATUM *pOutputData,
    int nSamples
)
{
    /*
    General principles of the buffering and processing mechanism:
    - The InputBuffer contains input samples that have not been processed.
    - The ProcessedBuffer contains processed samples from DS1AP.
    - The ProcessedBuffer is always "full" of valid samples (except during
    certain parts of the procedure below). It is initialised with silence.
    - The 'head' position in the InputBuffer (the place where the next input
    sample should be copied) is always equivalent to the 'head' position in
    the ProcessedBuffer (the place where the next processed sample should be
    copied from into the output data memory).
    - For every sample that is input, a processed sample will be placed in the
    output data memory. The processed sample will be 32 samples "old".
    - Whenever the InputBuffer is filled, it can be processed by DS1AP and the
    output is placed in the ProcessedBuffer.
    - When the function is called, the following steps occur:
        1. If the InputBuffer is not empty and there are enough samples input
        to fill it, it is filled from the input audio data. The same number of
        samples are copied from the ProcessedBuffer to the output data memory.
        The full InputBuffer is then processed with DS1AP and the samples are
        placed into the ProcessedBuffer (hence it is now still full). The
        InputBuffer is therefore empty.

        2. If there are not enough input samples remaining to fill the
        InputBuffer completely, copy the remaining input audio samples into the
        InputBuffer. Copy the same number from the ProcessedBuffer to the
        output data memory.
        The processing request is now complete.

        3. There are enough input samples remaining to fill the InputBuffer.
        If there are even more (more than 2 * 256) input samples remaining,
        all processed samples in ProcessedBuffer could be placed in the output
        data memory, and then DS1AP invoked directly on the input audio data
        and the processed samples are placed directly into the output data
        memory. To accommodate in-place processing, the direct invocation of
       DS1APS performed first, and the output shifted to the required location.
        The samples in ProcessedBuffer are then copied to the output data
        memory before the direct-processed samples. The last full batch of
        32 input samples are processed by DS1AP and placed in the
        ProcessedBuffer. Any remaining few input samples (i.e. less than 32)
        are then copied to the InputBuffer, ready for the next invocation.
    */
    int mode_in_place;
    int nInternalBufferBytes;

    Ds1apBuffer *p = (Ds1apBuffer *)pDs1apBuffer;
    int n;
    int nInBytesTotal = p->_nInBytesPerTotalSample;
    int nOutBytesTotal = p->_nOutBytesPerTotalSample;
    char *pbyInputData = (char *)pInputData;
    char *pbyOutputData;

    mode_in_place = (pInputData == pOutputData);

    if (!mode_in_place) /* 2 independent buffers */
    {
        /*
          Separate OutputData.
        */
        pbyOutputData = (char *)pOutputData;
        #if (DS1AP_BUFFER_LOGGING >= 2)
        if
        (
          (pbyOutputData < pbyInputData + (nSamples * nInBytesTotal))
          &&
          (pbyOutputData + (nSamples * nOutBytesTotal) > pbyInputData)
        )
        {
            ALOGE("Ds1apBufferProcessSamples: pInputData and pOutputData not equal (seperated) but overlap! Failing.");
            return;
        }
        #endif
    }
    else /* in place buffers */
    {
#if (DS1APBUFFER_MODE == DS1APBUFFER_MODE_IN_PLACE_INTERNALBUFFER)
        /* Use internal buffer as output buffer. */
        nInternalBufferBytes = nSamples * nOutBytesTotal;

        /* Check the status of the internal buffer. */
        if (nSamples > p->_nInternalBufferSamples)
        {
            /* Existing internal buffer not large enough, reallocate. */
            #if (DS1AP_BUFFER_LOGGING >= 2)
            ALOGD("Ds1apBufferProcessSamples: internalBuffer resized from %i to %i samples per channel.", p->_nInternalBufferSamples, nSamples);
            #endif

            free (p->_pInternalBuffer);
            p->_nInternalBufferSamples = nSamples;
            p->_pInternalBuffer = malloc(nInternalBufferBytes);
        }
        /* Internal buffer is the output buffer. */
        pbyOutputData = p->_pInternalBuffer;
#elif (DS1APBUFFER_MODE == DS1APBUFFER_MODE_IN_PLACE_SHIFT)
        /*
            Single input buffer for in-place processing.
        */
        pbyOutputData = (char *)pInputData;
#else
        #error "Invalid DS1APBUFFER_MODE value."
#endif
    } /* (!mode_in_place) */

#if (DS1AP_BUFFER_LOGGING >= 2)
    ALOGD("Ds1apBufferProcessSamples: nSamples: %i nBufferSampleCount: %i", nSamples, p->_nBufferSampleCount);
#endif

    /*
        First fill and process the InputBuffer if it's not empty (i.e. if
        nBufferSampleCount > 0 and there are enough samples in InputData to do so.
    */
    if (p->_nBufferSampleCount > 0 && (p->_nBufferSampleCount + nSamples) >= BUFFER_SAMPLES)
    {
        /*
          There are some samples in the InputBuffer and there are enough samples
          in InputData to fill the InputBuffer.
          Fill the InputBuffer and process it so the InputBuffer is then empty.
        */
        /* Determine how many samples are required to fill the InputBuffer. */
        int nCopySamples = BUFFER_SAMPLES - p->_nBufferSampleCount;
        /* Copy the InputData into the InputBuffer and adjust its head. */
        memcpy(p->_pInputBufferHead, pbyInputData, n = nCopySamples * nInBytesTotal);
        pbyInputData += n;
        /* Copy the ProcessedBuffer into OutputData and adjust its head. */
        memcpy(pbyOutputData, p->_pProcessedBufferHead, n = nCopySamples * nOutBytesTotal);
        pbyOutputData += n;
        /* Reduce the nSamples remaining to be processed. */
        nSamples -= nCopySamples;
        /*
          Now the InputBuffer is full and ProcessedBuffer is ready for more
          processed data. No need to update nBufferSampleCount as it would simply
          be equal to BUFFER_SAMPLES.
          Process the InputBuffer into the ProcessedBuffer.
        */
        ak_process_loop(&p->_audioInDlbBuffer, &p->_audioOutDlbBuffer, ak_hndl, scratch_mem,
        			(IO_DATUM *)p->_pInputBuffer, (IO_DATUM *)p->_pProcessedBuffer, BUFFER_SAMPLES);


        /* InputBuffer is now empty and ProcesedBuffer is full of processed samples. */
        /* Reset heads and counter. */
        p->_pInputBufferHead = p->_pInputBuffer;
        p->_pProcessedBufferHead = p->_pProcessedBuffer;
        p->_nBufferSampleCount = 0;

        #if (DS1AP_BUFFER_LOGGING >= 2)
        ALOGD("Ds1apBufferProcessSamples: Partial buffer filled and processed. nSamples(remaining)=%i", nSamples);
        #endif
    }
    /*
      At this point, either the InputBuffer is empty (nBufferSampleCount == 0), or there
      are not enough samples remaining to fill the InputBuffer.
    */
    if ((p->_nBufferSampleCount + nSamples) < BUFFER_SAMPLES)
    {
        /*
          There are not enough samples remaining in InputData to completely
          fill InputBuffer.
        */
        /* Copy the remaining samples to the head of the InputBuffer and adjust the head. */
        memcpy(p->_pInputBufferHead, pbyInputData, n = nSamples * nInBytesTotal);
        p->_pInputBufferHead += n;
        /*
          Copy the same number of samples from the head of the ProcessedBuffer to
          OutputData and adjust the head.
        */
        memcpy(pbyOutputData, p->_pProcessedBufferHead, n = nSamples * nOutBytesTotal);
        p->_pProcessedBufferHead += n;
        /* Adjust the count of the number of samples in the buffers as per heads. */
        p->_nBufferSampleCount += nSamples;

        /*
          All samples from InputData have been copied and the same number
          of samples have been placed in OutputData.
        */
        #if (DS1AP_BUFFER_LOGGING >= 2)
        ALOGD("Buffer partially filled to _nBufferSampleCount=%i", p->_nBufferSampleCount);
        #endif
    }
    else
    {
        int nDirectSamples;

        /*
            The InputBuffer must be empty and the ProcessedBuffer is full. There
            are at least BUFFER_SAMPLES remaining to be processed.
            DS1AP will be invoked directly on the InputData. If there is at least
            2 * BUFFER_SAMPLES remaining, DS1AP will output directly to the
            OutputData. Care must be taken in the case of in-place processing.

            The state of the ProcessedBuffer and InputData at this stage are:
            ProcessedBuffer: [ P'(BUFFER_SAMPLES) ]
            InputData: [ D(n*BUFFER_SAMPLES) | R(BUFFER_SAMPLES) | X(x) ]
            where
            - P'(BUFFER_SAMPLES) represents BUFFER_SAMPLES processed samples from a previous call.
            - D(n*BUFFER_SAMPLES) represents n multiples of BUFFER_SAMPLES samples, n >= 0.
            - R(BUFFER_SAMPLES) represents a reserve of the final BUFFER_SAMPLES samples in InputData,
                excluding the trailing extra samples X.
            - X(x) represents the residual extra x samples, 0 <= x < BUFFER_SAMPLES.

            The state of the OutputData, ProcessedBuffer and InputBuffer after
            execution will be:
            ProcessedBuffer: [ R'(BUFFER_SAMPLES) ]
            OutputData: [ P'(BUFFER_SAMPLES) | D'(n*BUFFER_SAMPLES) | R'(x) ]
            InputBuffer: [ X(x) ]
            where
            - P'(BUFFER_SAMPLES) represents the samples from the ProcessedBuffer.
            - D'(n*BUFFER_SAMPLES) represents the n*32 samples directly processed from
                InputData into OutputData.
            - R' represents the processed reserve samples. The ProcessedBuffer
                contains all R'(BUFFER_SAMPLES) processed directly from InputData. Then
                x samples of R' are copied to the end of OutputData.
            - X represents the original residual extra samples.
        */
        #if (DS1AP_BUFFER_LOGGING >= 2)
        ALOGD("Check direct processing with nSamples: %i", nSamples);
        if (!(p->_nBufferSampleCount == 0 && p->_pInputBufferHead == p->_pInputBuffer && p->_pProcessedBufferHead == p->_pProcessedBuffer))
            ALOGD("Buffer heads should be 0");
        #endif

        /*
          We know nBufferSampleCount == 0 and so the heads of the buffers are at
          the beginning.

          Logically, the next step is to copy the (full) ProcessedBuffer into
          the OutputData. However, for in-place processing (same input and
          output memory was supplied by caller), this will be performed after
          any ak_process is performed in-place.

          However the nSamples remaining must still be reduced to account for
          the "reserved" BUFFER_SAMPLES samples (R(256) in the above scheme).
        */
        nSamples -= BUFFER_SAMPLES;
        /*
            Calculate how many samples can be processed directly from InputData
            to OutputData.
        */
        nDirectSamples = nSamples - (nSamples % BUFFER_SAMPLES);
        /*
          There are sufficient samples to do direct processing from
          InputData to OutputData.

          Check for in-place buffer processing, i.e. if the original
          InputData and OutputData pointers were the same. If so, the
          direct processing of samples into OutputData will not work
          because it will overwrite the input data before it is read
          by DS1AP.

          NOTE: In-place processing occurs when the original InputData
          and OutputData supplied to this function are equal. However,
          it is possible for InputData and OutputData to be different
          at this point in the code, because some samples may have already
          been copied from/to the memory, and if the output format has
          less channels than the input, then the current value of the
          OutputData pointer will be less than the InputData pointer.
        */
#if (DS1APBUFFER_MODE == DS1APBUFFER_MODE_IN_PLACE_SHIFT)
        if (mode_in_place)
        {
            /*
                In-place processing: DS1AP will process samples directly
                and in-place in the InputData/OutputData. Then, the
                processed samples are shifted right and processed samples
                from the previous call (in ProcessedBuffer) are inserted.
                Summary:
                a. Copy the reserved BUFFER_SAMPLES from the InputData to the
                    InputBuffer, because these would have been overwritten
                    during the right shift below.
                b. If there are sufficient samples for direct processing:
                    b1. Process samples directly in-place.
                    b2. Shift samples in buffer to the right by BUFFER_SAMPLES.
                c. Insert samples in the ProcessedBuffer into the hole of
                    BUFFER_SAMPLES made by the right shift.
                d. Process the BUFFER_SAMPLES that were copied to InputBuffer
                    and store in the ProcessedBuffer.
            */
            /* Save the reserved BUFFER_SAMPLES for later processing. */
            memcpy(p->_pInputBuffer, pbyInputData + (nDirectSamples * nInBytesTotal), BUFFER_SAMPLES * nInBytesTotal);

            /* Check if there are sufficient samples for direct processing. */
            if (nDirectSamples > 0)
            {
                int nShiftBytes;
                char *pw;
                char *pr;

                #if (DS1AP_BUFFER_LOGGING >= 2)
                ALOGI("Ds1apBufferProcessSamples: Direct processing (IN_PLACE_SHIFT) nDirectSamples: %i", nDirectSamples);
                #endif
                /* Process in-place. */
                ds1ap_process(pDs1apBuffer, pakHndl, pscratchMem, (IO_DATUM *)pbyInputData, (IO_DATUM *)pbyOutputData, nDirectSamples);
                /* Shift processed samples right by BUFFER_SAMPLES per channel */
                nShiftBytes = BUFFER_SAMPLES * nOutBytesTotal;
                pw = pbyOutputData + (nDirectSamples * nOutBytesTotal);
                pr = pw - nShiftBytes;
                do
                {
                    memcpy(pw, pr, nShiftBytes);
                    pr -= nShiftBytes;
                    pw -= nShiftBytes;
                } while (pr >= pbyOutputData);
            }
            #if (DS1AP_BUFFER_LOGGING >= 2)
            else
            {
                /* Not enough samples for direct processing. */
                ALOGI("Ds1apBufferProcessSamples: Insufficient samples for direct processing (IN_PLACE_SHIFT)");
            }
            #endif /*(DS1AP_BUFFER_LOGGING >= 2)*/

            /* Insert (full) ProcessedBuffer at start of OutputData. */
            memcpy(pbyOutputData, p->_pProcessedBuffer, BUFFER_SAMPLES * nOutBytesTotal);
            /*
                Adjust OutputData and InputData heads due to direct
                processing and the copy of samples from ProcessedBuffer.
            */
            n = nDirectSamples + BUFFER_SAMPLES;
            pbyOutputData += (n * nOutBytesTotal);
            pbyInputData += (n * nInBytesTotal);
            /*
                Reduce the nSamples remaining to be processed due to the
                direct processing.
            */
            nSamples -= nDirectSamples;
            /*
                There is now nSamples remaining in InputData.

                The final block of BUFFER_SAMPLES from InputData that was
                copied to the InputBuffer above now needs to be processed
                into the ProcessedBuffer. InputData head was adjusted above.
            */
            ds1ap_process(pDs1apBuffer, pakHndl, pscratchMem, (IO_DATUM *)p->_pInputBuffer, (IO_DATUM *)p->_pProcessedBuffer, BUFFER_SAMPLES);
        }
        else /* (!mode_in_place) */
#endif /* (DS1APBUFFER_MODE == DS1APBUFFER_MODE_IN_PLACE_SHIFT) */
        /*    Independent buffers or in-place processing using internal buffer. */
        {
            /*
              Non-in-place processing: DS1AP will process samples directly
              from InputData to OutputData. There is no concern with samples
              in the OutputData being overwritten.
              Summary:
              a. Copy the ProcessedBuffer into the OutputData.
              b. If there are sufficient samples for direct processing:
                b1. Process samples directly from InputData to OutputData
                    (OutputData pointer is ahead by BUFFER_SAMPLES).
              c. Process the reserved BUFFER_SAMPLES into the ProcessedBuffer.
            */
            /* Copy the (full) ProcessedBuffer into the OutputData and adjust the head. */
            memcpy(pbyOutputData, p->_pProcessedBuffer, n = BUFFER_SAMPLES * nOutBytesTotal);
            pbyOutputData += n;
            /* Check if there are sufficient samples for direct processing. */
            if (nDirectSamples > 0)
            {
                #if (DS1AP_BUFFER_LOGGING >= 2)
                    #if (DS1APBUFFER_MODE == DS1APBUFFER_MODE_IN_PLACE_INTERNALBUFFER)
                    ALOGD("Ds1apBufferProcessSamples: Direct processing (IN_PLACE_INTERNALBUFFER) nDirectSamples: %i", nDirectSamples);
                    #else
                    ALOGD("Ds1apBufferProcessSamples: Direct processing (INDEPENDENT_BUFFERS) nDirectSamples: %i", nDirectSamples);
                    #endif
                #endif
                /* Process straight from InputData directly into OutputData and adjust heads. */
                ak_process_loop(&p->_audioInDlbBuffer, &p->_audioOutDlbBuffer, ak_hndl, scratch_mem, (IO_DATUM *)pbyInputData, (IO_DATUM *)pbyOutputData, nDirectSamples);
                pbyInputData += (nDirectSamples * nInBytesTotal);
                pbyOutputData += (nDirectSamples * nOutBytesTotal);
                /* Reduce the nSamples remaining to be processed. */
                nSamples -= nDirectSamples;
            }
            #if (DS1AP_BUFFER_LOGGING >= 2)
            else
            {
                /* Not enough samples for direct processing. */
                #if (DS1APBUFFER_MODE == DS1APBUFFER_MODE_IN_PLACE_INTERNALBUFFER)
                ALOGD("Ds1apBufferProcessSamples: Insufficient samples for direct processing (IN_PLACE_INTERNALBUFFER)");
                #else
                ALOGD("Ds1apBufferProcessSamples: Insufficient samples for direct processing (INDEPENDENT_BUFFERS)");
                #endif
            }
            #endif /*(DS1AP_BUFFER_LOGGING >= 2)*/
            /*
              There is now BUFFER_SAMPLES + nSamples remaining in InputData.
              Note that above the full BUFFER_SAMPLES from ProcessedBuffer
              were copied to OutputData (and nSamples was already
              decremented by BUFFER_SAMPLES above). So this equivalent of
              samples in InputData still remain, plus the extra residual
              samples (= nSamples).
            */
            ak_process_loop(&p->_audioInDlbBuffer, &p->_audioOutDlbBuffer, ak_hndl, scratch_mem, (IO_DATUM *)pbyInputData, (IO_DATUM *)p->_pProcessedBuffer, BUFFER_SAMPLES);
            pbyInputData += (BUFFER_SAMPLES * nInBytesTotal);
        }

        #if (DS1AP_BUFFER_LOGGING >= 2)
        ALOGD("Ds1apBufferProcessSamples: Finished, nSamples(remaining)=%i", nSamples);
        #endif

        /*
          Copy the remaining nSamples from InputData into the InputBuffer and
          set its head (it is currently at the beginning).
        */
        memcpy(p->_pInputBuffer, pbyInputData, n = nSamples * nInBytesTotal);
        p->_pInputBufferHead = p->_pInputBuffer + n;
        /*
          Copy the same number of samples from the start of ProcessedBuffer
          into OutputData and set ProcessedBuffer head.
        */
        memcpy(pbyOutputData, p->_pProcessedBuffer, n = nSamples * nOutBytesTotal);
        p->_pProcessedBufferHead = p->_pProcessedBuffer + n;
        /* Adjust the buffer sample count as per the heads. */
        p->_nBufferSampleCount = nSamples;

        /*
          All samples from InputData have been copied and the same number
          of samples have been placed in OutputData.
        */
#if (DS1APBUFFER_MODE == DS1APBUFFER_MODE_IN_PLACE_INTERNALBUFFER)
        if (mode_in_place)
        {
            /*
              The internal buffer was used to support in-place processing.
              Copy the samples from the internal buffer to the caller's buffer.
            */
            memcpy(pInputData, p->_pInternalBuffer, nInternalBufferBytes);
        }
#endif
    }
}
