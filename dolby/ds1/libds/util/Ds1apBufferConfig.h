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

#ifndef _DS1AP_BUFFERCONFIG_H
#define _DS1AP_BUFFERCONFIG_H


/*
    The Ds1apBuffer module supports two buffer setup modes. The mode is
    checked at run-time:

    1. Support processing using two seperate buffers.
        Processing occurs from an input data buffer and the processed output
        is placed in the output data buffer. The buffers must point to different
        memory locations.
        NOTE: THE MEMORY OF EACH DATA BUFFER MUST NOT OVERLAP.

    2. The input and output pointers points to the same buffer. The processed
        samples are placed in this same buffer.

    DS1APBUFFER_MODE
    Determines which buffer processing mode will be supported by Ds1apBuffer in the
    case of one same in/out buffer setup (case 2 here above).

    DS1APBUFFER_MODE values:

    * DS1APBUFFER_MODE_IN_PLACE_INTERNALBUFFER
        This option uses an internal memory buffer as the output buffer during
        processing. The contents are then copied back into the single supplied
        data buffer to resemble in-place processing.

        The internal memory buffer is initially allocated using a minimum size
        parameter in Ds1apBufferInit(). The buffer may be re-allocated during a
        call to Ds1apBufferProcessSamples() if it is not large enough to fulfil
        the request. However the buffer size is never reduced, and is only
        released when Ds1apBufferDeinit() is called.

        NOTE: This option requires additional memory to be allocated. It is
        possible that a large buffer could be allocated if a processing request
        specifies a large number of samples.

        NOTE: THIS OPTION CAN ONLY BE USED WHEN CALLING malloc() DURING AUDIO
        PROCESSING IS ALLOWED.

    * DS1APBUFFER_MODE_IN_PLACE_SHIFT
        Support processing in-place in the supplied data buffer. The processed
        samples are placed in the same buffer.

        NOTE: This option uses an in-place memory shift which introduces
        increased complexity and operations during processing to move a large
        portion of the buffer.
*/
#define DS1APBUFFER_MODE     DS1APBUFFER_MODE_IN_PLACE_INTERNALBUFFER



#endif /*_DS1AP_BUFFERCONFIG_H*/
