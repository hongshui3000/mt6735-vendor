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
#ifndef DDPDEC_CLIENT_H
#define DDPDEC_CLIENT_H

#ifdef __cplusplus
extern "C"
{
#endif

#if (DOLBY_UDC_LOG_LEVEL >= 1)
#define ALOG1(...) ALOGI(__VA_ARGS__)
#if (DOLBY_UDC_LOG_LEVEL >= 2)
#define ALOG2(...) ALOGD(__VA_ARGS__)
#if (DOLBY_UDC_LOG_LEVEL >= 3)
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

#define DEC_MAXPCMCHANS 8
#define GBL_MAXBLOCKS 6
#define GBL_BYTESPERWRD BYTES_PER_UDC_SAMPLE
#define GBL_BLKSIZE 256
#define GBL_MAXDDPFRMWRDS 2048
#define DDPI_FMI_FRMSIZEWRDS 3

/* acmod values */
#define	    BSI_ACMOD_11	        0		/*!< Dual mono: ch1,ch2	*/
#define	    BSI_ACMOD_10	        1		/*!< Mono: C */
#define	    BSI_ACMOD_20	        2		/*!< Stereo: L,R */
#define	    BSI_ACMOD_30	        3		/*!< 3/0: L,C,R */
#define	    BSI_ACMOD_21	        4		/*!< 2/1: L,R,S	*/
#define	    BSI_ACMOD_31	        5		/*!< 3/1: L,C,R,S */
#define	    BSI_ACMOD_22	        6		/*!< 2/2: L,R,Ls,Rs */
#define	    BSI_ACMOD_32	        7		/*!< 3/2: L,C,R,Ls,Rs */

#define     BSI_ACMOD_322       21    /*!< 3/2/2: L,C,R,Ls,Rs,Lrs,Rrs */

    /*----------------------------------------------------------------------------
    ; ENUMERATED TYPEDEF'S
    ----------------------------------------------------------------------------*/
    /*
     * This enumeration holds the possible return values for the main decoder
     * function. The plan was to easily distinguish whether an error was
     * recoverable (streaming mode) or not. Presently no errors are recoverable,
     * which is a result of not supporting ADTS in this release.
     */
    typedef enum ERROR_CODE
    {
        DDPAUDEC_SUCCESS            =  0,
        DDPAUDEC_OPEN_FAILURE       = 10,
        DDPAUDEC_INIT_FAILURE       = 20,
        DDPAUDEC_INVALID_HDR        = 30,
        DDPAUDEC_FRM_PARAM_ERROR    = 40,
        DDPAUDEC_INVALID_FRAME      = 50,
        DDPAUDEC_INCOMPLETE_FRAME   = 60,
        DDPAUDEC_FRM_CLEAN_FAILURE  = 70,
        DDPAUDEC_FRM_CLOSE_FAILURE  = 80,
    } ERROR_CODE;

    /*----------------------------------------------------------------------------
    ; STRUCTURES TYPEDEF'S
    ----------------------------------------------------------------------------*/
    /*
     * This structure is used to communicate information in to and out of the
     * DD+ decoder.
     */

    typedef struct
#ifdef __cplusplus
                // To allow forward declaration of this struct in C++
                tDdpDecoderExternal
#endif
    {
        /*
         * INPUT:
         * Allocate output buffers and take care of alignment; the original pointer must be used
         * for freeing the memory.
         */
        char *pOutMem;

        /*
         * INPUT:
         * Allocate memory for subroutine and take care of alignment.
         */
        char *pUdcMem;

        /*
         * INPUT:
         * Pointer to the input buffer that contains the encoded bistream data.
         * The data is filled in such that the first bit transmitted is
         * the most-significant bit (MSB) of the first array element.
         * The buffer is accessed in a linear fashion for speed, and the number of
         * bytes consumed varies frame to frame.
         * The calling environment can change what is pointed to between calls to
         * the decode function, library, as long as the inputBufferCurrentLength,
         * and inputBufferUsedLength are updated too. Also, any remaining bits in
         * the old buffer must be put at the beginning of the new buffer.
         */
        char  *pInputBuffer;

        /*
         * INPUT: (but what is pointed to is an output)
         * Pointer to the output buffer to hold the 16-bit PCM audio samples.
         * If the output is stereo, both left and right channels will be stored
         * in this one buffer. Presently it must be of length of 2048 points.
         * The format of the buffer is set by the parameter outputFormat.
         */
        char   *pOutputBuffer;

        /*
         * INPUT:
         * Downmix config. This defines whether we will output 2-channel LtRt, LoRo from the
         * internal downmixer, use an external downmixer, or output the stream
         * as-is
         */
        int     downmixMode;

        /*
         * INPUT:
         * DRC mode. Controls whether the decoder will operate in Portable, RF or LINE mode
         */
        int    drcMode;

        /*
         * INPUT:
         * This flags that the output downmix configuration has been updated
         * This will cause the decoder to re-configure the channel routing. This flag will be
         * cleared once this has been acted upon.
         */
        int     updatedChannelRouting;

        /*
         * INPUT:
         * Number of valid bytes in the input buffer, set by the calling
         * function. After decoding the bitstream the library checks to
         * see if it when past this value; it would be to prohibitive to
         * check after every read operation. This value is not modified by
         * the DD+ decoder library.
         */
        int     inputBufferCurrentLength;

        /*
         * INPUT/OUTPUT:
         * Number of elements used by the library, initially set to zero
         */
        int     inputBufferUsedLength;

        /*
         * OUTPUT:
         * The sampling rate decoded from the bitstream, in units of
         * samples/second. For this release of the library this value does
         * not change from frame to frame, but future versions will.
         */
        int   samplingRate;

        /*
         * OUTPUT:
         * This value is the number of output PCM samples per channel.
         */
        int     frameLengthSamplesPerChannel;

        /* OUTPUT:
         *    This value is the number of channels to be expected in the
         *    output from the decoder. It can be deduced from the output
         *    configuration but this is simpler.
         */
        int    nrChans;

        /* OUTPUT
         * This stores the maximum number of channels supported by the
         * current audio sink device
         */
        int     nPCMOutMaxChannels;

        /* OUTPUT
         * This stores the currently DS on/off state
         */
        int     nActiveDsState;

        /* OUTPUT
         * This stores the currently active Endpoint
         */
        int     nActiveEndpoint;

    } tDdpDecoderExternal;

/*****************************************************************
* ddpdec_open: Initialize the UDC subroutine memory.
*****************************************************************/
void* ddpdec_open(tDdpDecoderExternal *pExt);

/*****************************************************************
* ddpdec_close: Perform all clean up necessary to close the UDC.
*****************************************************************/
void ddpdec_close(tDdpDecoderExternal *pExt, void *pMem);

/*****************************************************************
* ddpdec_process: Add a certain amount of bytes to the input of the decoder.
* and Processe a DD/DD+ timeslice.
*****************************************************************/
int ddpdec_process(tDdpDecoderExternal *pExt, void *pMem);

    /*----------------------------------------------------------------------------
    ; END
    ----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif


#endif  /* DDPDEC_CLIENT_H */
