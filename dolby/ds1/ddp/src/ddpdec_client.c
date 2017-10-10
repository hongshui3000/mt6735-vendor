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

#define LOG_TAG "ddpdec_client"
#include <utils/Log.h>

#ifdef DOLBY_UDC_AUDIO_DUMP
#include <stdbool.h>
#endif // DOLBY_UDC_AUDIO_DUMP
/**** Module Dependencies ****/

/* subroutine headers */
#include "udc_api.h"

/* executive headers */
#include "udc_exec.h"
#include "udc_user.h"
#include "ddpdec_client.h"

const int acmodchans[8]={ 2, //11
                          1, //10
                          2, //20
                          3, //30
                          3, //21
                          4, //31
                          4, //22
                          5  //32
                        };

#ifdef DOLBY_UDC_AUDIO_DUMP
//
// Usage: 
// The retrieved audio dump will be under /data folder, so first ensure /data folder is writable.
//
FILE*    pFileUdcPcmOutputDump = NULL;
bool     bUdcAudioDumpStopped = true;
#endif // DOLBY_UDC_AUDIO_DUMP

/*****************************************************************
* ddpdec_open: Initialize the UDC subroutine memory.
*****************************************************************/
void *ddpdec_open(tDdpDecoderExternal *pExt)
{
    int	err;
    int sub_err;
    char *p_outbuf = NULL;
    char *p_outbuf_align = NULL;
    char *udcmem = NULL;
    UDCEXEC *pdecexec;
    ddpi_udc_query_ip queryip;
    int i;

    pdecexec = (UDCEXEC *)malloc(sizeof(UDCEXEC)) ;
    if (pdecexec == NULL)
    {
        return NULL;
    }

    /* initialize executive memory to zero */
    memset(pdecexec,0,sizeof(UDCEXEC));

    /* query the library to find out the static configuration */
    err = ddpi_udc_query(&(pdecexec->query_op));
    if (err)
    {
        goto BAIL;
    }

    /* initialize executive parameters to defaults */
    err = initexecparams(&(pdecexec->execparams));
    if (err)
    {
        goto BAIL;
    }

    /* initialize subroutine parameters to defaults */
    for (i = 0; i < DDPI_UDC_PCMOUT_COUNT; i++)
    {
        err = initsubparams(&((pdecexec->subparams)[i]));
        if (err)
        {
            goto BAIL;
        }
    }

	/* initialize query input parameters to zero */
	memset(&queryip, 0, sizeof(queryip));

    /* query subroutine for configuration information */
	if (pdecexec->execparams.numofddout == 1)
    {
        queryip.converter = 1;
	}
	else
	{
		queryip.converter = 0;
	}
    queryip.num_outputs = pdecexec->execparams.numofpcmout;
    queryip.num_main_outputs = pdecexec->execparams.numofmainpcmout;
	queryip.output_datatype = DLB_BUFFER_INT_LEFT;

    /* Set maxnumchannels */
    for (i = 0; i < queryip.num_outputs; i++)
    {
	    queryip.outputs[i].maxnumchannels = pdecexec->execparams.outnchans[i];
    }

    sub_err = ddpi_udc_query_mem(&queryip, &pdecexec->query_mem_op);
    if (sub_err)
    {
        err = ERR_QUERY_FAILED;
        goto BAIL;
    }


    ALOG3("Memory requirements: UDC %d bytes, output buffers: %d bytes\n",
        pdecexec->query_mem_op.udc_size,
        pdecexec->query_mem_op.outputbuffersize);

    /* Allocate output buffers and take care of alignment; the original pointer must be used
       for freeing the memory */
    p_outbuf = (char *) calloc(1, pdecexec->query_mem_op.outputbuffersize+(DDPI_UDC_MIN_MEMORY_ALIGNMENT-1));
    if (!p_outbuf)
    {
        err = ERR_MEMORY;
        goto BAIL;
    }
    p_outbuf_align = (char *) ((((long) p_outbuf) +(DDPI_UDC_MIN_MEMORY_ALIGNMENT-1)) & ~(DDPI_UDC_MIN_MEMORY_ALIGNMENT-1));

    /* allocate memory for subroutine and take care of alignment */
    udcmem = (char *)calloc(1, pdecexec->query_mem_op.udc_size + (DDPI_UDC_MIN_MEMORY_ALIGNMENT-1));
    pdecexec->p_dechdl = (void *)((long)(udcmem+(DDPI_UDC_MIN_MEMORY_ALIGNMENT-1)) & ~(DDPI_UDC_MIN_MEMORY_ALIGNMENT-1));
    if (!pdecexec->p_dechdl)
    {
        err = ERR_MEMORY;
        goto BAIL;
    }

    pExt->pOutMem = p_outbuf;
    pExt->pUdcMem = udcmem;

    /* display banner info (regardless of verbose mode) */
    err = displaybanner(&(pdecexec->query_op));
    if (err)
    {
        goto BAIL;
    }

    /* open (initialize) subroutine memory */
    sub_err = ddpi_udc_open(&queryip, p_outbuf_align, pdecexec->p_dechdl);
    if (sub_err)
    {
        err = ERR_OPEN_FAILED;
        goto BAIL;
    }

    /* set subroutine parameters */
    sub_err = setsubparams(queryip.num_outputs, &(pdecexec->subparams[0]), pdecexec->p_dechdl);
    if (sub_err)
    {
        err = ERR_PROCESSING;
        goto BAIL;
    }

    /* Initialize buffers */
    pdecexec->ptop.pcmoutbfds = pdecexec->a_outputbuf;
    pdecexec->ptop.ddoutbfd = &pdecexec->ddoutbuf;
    for (i = 0; i < queryip.num_outputs; i++)
    {
        pdecexec->a_outputbuf[i].ppdata = (void **) malloc(sizeof(void *) * DDPI_UDC_MAXPCMOUTCHANS);
    }
    pdecexec->ddoutbuf.ppdata = (void **) malloc(sizeof(void *));

#ifdef DOLBY_UDC_AUDIO_DUMP	
    // Launch an audio dump session if it is stopped.
    if (bUdcAudioDumpStopped)	
    {
        ALOG1("DOLBY_UDC_AUDIO_DUMP: Open the file for PCM output dump");
        pFileUdcPcmOutputDump = fopen("/data/dolby/udcPcmOutput.pcm", "wb");
        if (pFileUdcPcmOutputDump == NULL)
        {
            ALOGE("DOLBY_UDC_AUDIO_DUMP: Failed to open audio dump file. /data/dolby/ file system writable?");
            ALOGE("DOLBY_UDC_AUDIO_DUMP: You may use chmod to the directory above.");
        }
        else
        {
            bUdcAudioDumpStopped = false;
        }
    }
#endif // DOLBY_UDC_AUDIO_DUMP

    return (void *) pdecexec;

BAIL:
    if (pdecexec != NULL)
    {
        free(pdecexec);
    }
    if (p_outbuf != NULL)
    {
        free(p_outbuf);
    }
    if (udcmem != NULL)
    {
        free(udcmem);
    }
    return NULL;
}

/*****************************************************************
* ddpdec_close: Perform all clean up necessary to close the UDC.
*****************************************************************/
void ddpdec_close(tDdpDecoderExternal *pExt, void *pMem)
{
    UDCEXEC   *pdecexec = (UDCEXEC *) pMem;
    char *p_outbuf = pExt->pOutMem;
    char *udcmem = pExt->pUdcMem;
    int i;

    if (pdecexec == NULL)
    {
        return;
    }

    /* Free DEC_EXEC Memory */
    if (pdecexec->p_dechdl)
    {
        /* close subroutine memory */
        ddpi_udc_close(pdecexec->p_dechdl);

        /* free subroutine memory */
        free(udcmem);
    }

    /* Free PCM buffers */
    if (p_outbuf)
    {
        free(p_outbuf);
    }

    /* Free data buffers */
    for (i = 0; i < DDPI_UDC_PCMOUT_COUNT; i++)
    {
        if (pdecexec->a_outputbuf[i].ppdata)
        {
            free(pdecexec->a_outputbuf[i].ppdata);
        }
    }

    /* Free data buffers */
    if (pdecexec->ddoutbuf.ppdata)
    {
        free(pdecexec->ddoutbuf.ppdata);
    }

    free(pdecexec);

#ifdef DOLBY_UDC_AUDIO_DUMP	
    if (pFileUdcPcmOutputDump != NULL)
    {
        ALOG1("DOLBY_UDC_AUDIO_DUMP: Close the file for PCM Output dump");
        fclose(pFileUdcPcmOutputDump);
        pFileUdcPcmOutputDump = NULL;
    }
    bUdcAudioDumpStopped = true;
#endif // DOLBY_UDC_AUDIO_DUMP

    return;
}

/*****************************************************************
* configureChannelRouting:
*****************************************************************/
int configureChannelRouting(UDCEXEC *p_udcexec, tDdpDecoderExternal *pExt)
{
    ALOG2("ddpdec_client.c|int configureChannelRouting()");

    int err;
    int acmod, lfeOn;
    int depFrmFlag;
    int channelMap;
    int outChanNum;

    // API for extract timeslice metadata
    ddpi_udc_timeslice_mdat timeslice_mdat;
    err = ddpi_udc_gettimeslice_mdat(p_udcexec->p_dechdl, 1, &timeslice_mdat);

    // bit stream related information
    ALOG2("pgm_count=%d\n", timeslice_mdat.pgm_count);
    ALOG2("dep_substream_count = %d", timeslice_mdat.pgm_mdat[0].dep_substream_count);
    ALOG2("aggregate channel_map = %d", timeslice_mdat.pgm_mdat[0].channel_map);
    ALOG2("I0: frm_present=%d\n", timeslice_mdat.pgm_mdat[0].ind_frm_mdat.frm_present);
    ALOG2("I0: frm_id=%d\n", timeslice_mdat.pgm_mdat[0].ind_frm_mdat.frm_id);
    ALOG2("I0: minbsi_valid=%d\n", timeslice_mdat.pgm_mdat[0].ind_frm_mdat.minbsi_valid);
    ALOG2("I0: acmod=%d\n", timeslice_mdat.pgm_mdat[0].ind_frm_mdat.minbsi_mdat.acmod);
    ALOG2("I0: lfeon=%d\n", timeslice_mdat.pgm_mdat[0].ind_frm_mdat.minbsi_mdat.lfeon);
    ALOG2("I0: chanmap=%d\n", timeslice_mdat.pgm_mdat[0].ind_frm_mdat.minbsi_mdat.chanmap);
    ALOG2("D0: frm_present=%d\n", timeslice_mdat.pgm_mdat[0].dep_frm_mdat[0].frm_present);
    ALOG2("D0: frm_id=%d\n", timeslice_mdat.pgm_mdat[0].dep_frm_mdat[0].frm_id);
    ALOG2("D0: minbsi_valid=%d\n", timeslice_mdat.pgm_mdat[0].dep_frm_mdat[0].minbsi_valid);
    ALOG2("D0: acmod=%d\n", timeslice_mdat.pgm_mdat[0].dep_frm_mdat[0].minbsi_mdat.acmod);
    ALOG2("D0: lfeon=%d\n", timeslice_mdat.pgm_mdat[0].dep_frm_mdat[0].minbsi_mdat.lfeon);
    ALOG2("D0: chanmap=%d\n", timeslice_mdat.pgm_mdat[0].dep_frm_mdat[0].minbsi_mdat.chanmap);


    if (timeslice_mdat.pgm_mdat[0].ind_frm_mdat.frm_present)
    {
        acmod = timeslice_mdat.pgm_mdat[0].ind_frm_mdat.minbsi_mdat.acmod;
        lfeOn = timeslice_mdat.pgm_mdat[0].ind_frm_mdat.minbsi_mdat.lfeon;
    }
    else
    {
        acmod = BSI_ACMOD_20;
        lfeOn = 0;
    }
    depFrmFlag = timeslice_mdat.pgm_mdat[0].dep_substream_count;
    channelMap = timeslice_mdat.pgm_mdat[0].channel_map;

    /* The DD+ decoder shall use the following mapping of input channel configrations to output channel configurations
        1.	1.0 , 2.0, 2.1 shall be mapped to 2.0
        2.	3.0, ..., 5.1 shall be mapped to 5.1
        3.	6.0, ..., 7.1 shall be mapped to 7.1
    */
    switch (pExt->nPCMOutMaxChannels)
    {
        case 2:
        {
            acmod=BSI_ACMOD_20;
            lfeOn=0;
            outChanNum = 2;
            break;
        }
        case 6:
        {
            if (acmod > BSI_ACMOD_20)
            {
                acmod = BSI_ACMOD_32;
                lfeOn=1;
                outChanNum = 6;
            }
            else
            {
                acmod = BSI_ACMOD_20;
                lfeOn=0;
                outChanNum = 2;
            }
            break;
        }
        case 8:
        {
            if (depFrmFlag)
            {
                if ((channelMap & 0x300) != 0)
                {
                    acmod = BSI_ACMOD_322;
                    lfeOn = 1;
                    outChanNum = 8;
                }
                else
                {
                    acmod = BSI_ACMOD_32;
                    lfeOn=1;
                    outChanNum = 6;
                }
            }
            else
            {
                if (acmod > BSI_ACMOD_20)
                {
                    acmod = BSI_ACMOD_32;
                    lfeOn=1;
                    outChanNum = 6;
                }
                else
                {
                    acmod = BSI_ACMOD_20;
                    lfeOn=0;
                    outChanNum = 2;
                }
            }
            break;
        }
        default:
        {
            acmod=BSI_ACMOD_20;
            lfeOn=0;
            outChanNum = 2;
        }
    }


    if (err == ERR_NO_ERROR) //this is always true
    {
        int instance = 0;

        p_udcexec->execparams.outnchans[DDPI_UDC_PCMOUT_MAIN] = outChanNum;
        if (acmod != p_udcexec->subparams[0].outchanconfig)
        {
            p_udcexec->subparams[0].outchanconfig = acmod;
            err = ddpi_udc_setoutparam(
                p_udcexec->p_dechdl,
                instance,
                DDPI_UDC_OUTCTL_OUTMODE_ID,
                &p_udcexec->subparams[0].outchanconfig,
                sizeof(p_udcexec->subparams[0].outchanconfig));
            if (err != 0)
            {
                ALOGE("ddpdec_client.c|  > DD+ DECODER : ddpi_udc_setoutparam( DDPI_UDC_OUTCTL_OUTMODE_ID, %d ) FAILED (err = %d) **** ", acmod, err);
                return err;
            }
        }

        if (lfeOn != p_udcexec->subparams[0].outlfe)
        {
            p_udcexec->subparams[0].outlfe = lfeOn;
            err = ddpi_udc_setoutparam(
                p_udcexec->p_dechdl,
                instance,
                DDPI_UDC_OUTCTL_OUTLFEON_ID,
                &p_udcexec->subparams[0].outlfe,
                sizeof(p_udcexec->subparams[0].outlfe));
            if (err != 0)
            {
                ALOGE("ddpdec_client.c|  > DD+ DECODER : ddpi_udc_setoutparam( DDPI_UDC_OUTCTL_OUTLFEON_ID, %d ) FAILED (err = %d) **** ", lfeOn, err);
                return err;
            }
        }

        //if the current downmix mode isn't what is set, change it
        if (pExt->downmixMode != p_udcexec->subparams[0].stereomode)
        {
            p_udcexec->subparams[0].stereomode = pExt->downmixMode;
            err = ddpi_udc_setoutparam(
                p_udcexec->p_dechdl,
                instance,
                DDPI_UDC_OUTCTL_STEREOMODE_ID,
                &p_udcexec->subparams[0].stereomode,
                sizeof(p_udcexec->subparams[0].stereomode));
            if (err != 0)
            {
                ALOGE("ddpdec_client.c|  > DD+ DECODER : ddpi_udc_setoutparam( DDPI_UDC_OUTCTL_STEREOMODE_ID, %d ) FAILED (err = %d) **** ", pExt->downmixMode, err);
                return err;
            }
        }

        //if the current DRC mode isn't what is set, change it
        if (pExt->drcMode != p_udcexec->subparams[0].compmode)
        {
            p_udcexec->subparams[0].compmode = pExt->drcMode;
            err = ddpi_udc_setoutparam(
                p_udcexec->p_dechdl,
                instance,
                DDPI_UDC_OUTCTL_COMPMODE_ID,
                &p_udcexec->subparams[0].compmode,
                sizeof(p_udcexec->subparams[0].compmode));
            if (err != 0)
            {
                ALOGE("ddpdec_client.c|  > DD+ DECODER : ddpi_udc_setoutparam( DDPI_UDC_OUTCTL_COMPMODE_ID, %d ) FAILED (err = %d) **** ", pExt->drcMode, err);
                return err;
            }
        }
    }
    return err;
}

/*****************************************************************
* process_32bitoutput:
*****************************************************************/
inline static void process_32bitoutput(char *p_buf, int nblkspcm, int outnchans, int *p_samp[], ptrdiff_t nstride)
{
    int ch;
    int s;
    int samp;

    if ((long)p_buf & 0x03)
    {
        /* non 32-bit align */
        char *p_outbuf = p_buf;
        
        /* The samples are already interleaved */
        for (s = 0; s < (int)(nblkspcm * FIO_BLKSIZE); s++)
        {
            for (ch = 0; ch < outnchans; ch++)
            {
                samp = p_samp[ch][s * nstride];
    
                *p_outbuf++ = samp & 0xff;
                *p_outbuf++ = (samp >> 8) & 0xff;
                *p_outbuf++ = (samp >> 16) & 0xff;
                *p_outbuf++ = samp >> 24;
            }
        }
    }
    else
    {
        /* 32-bit align */
        int *p_outbuf = (int *)p_buf;
        
        /* The samples are already interleaved */
        for (s = 0; s < (int)(nblkspcm * FIO_BLKSIZE); s++)
        {
            for (ch = 0; ch < outnchans; ch++)
            {
                samp = p_samp[ch][s * nstride];
                *p_outbuf++ = samp;
            }
        }
    }
}

/*****************************************************************
* process_16bitoutput:
*****************************************************************/
inline static void process_16bitoutput(char *p_buf, int nblkspcm, int outnchans, int *p_samp[], ptrdiff_t nstride)
{
    int ch;
    int s;
    int samp;

    if ((long)p_buf & 0x01)
    {
        /* non 16-bit align */
        char *p_outbuf = p_buf;
    
        /* The samples are already interleaved */
        for (s = 0; s < (int)(nblkspcm * FIO_BLKSIZE); s++)
        {
            for (ch = 0; ch < outnchans; ch++)
            {
                samp = p_samp[ch][s * nstride];
    
                /* Round according to PCM output bits */
                /* y = (int)(x + 0.5) */
                if (samp <= INT_MAX - 0x8000)
                {
                    samp += 0x8000;
                }
    
                *p_outbuf++ = (samp >> 16) & 0xff;
                *p_outbuf++ = samp >> 24;
            }
        }
    }
    else
    {
        /* 16-bit align */
        short *p_outbuf = (short *)p_buf;
        
        /* The samples are already interleaved */
        for (s = 0; s < (int)(nblkspcm * FIO_BLKSIZE); s++)
        {
            for (ch = 0; ch < outnchans; ch++)
            {
                samp = p_samp[ch][s * nstride];
                
                /* Round according to PCM output bits */
                /* y = (int)(x + 0.5) */
                if (samp <= INT_MAX - 0x8000)
                {
                    samp += 0x8000;
                }
    
                *p_outbuf++ = samp >> 16;
            }
        }
    }
}

/*****************************************************************
* processudcoutput:
*****************************************************************/
int processudcoutput(
    UDCEXEC *p_udcexec, char *p_buf)      /* modify	*/
{
    /* declare local variables */
    int err = DDPI_UDC_ERR_NO_ERROR;
    int	pcmout_index;

    /* handle any errors reported in the process timeslice output params */
    for (pcmout_index = DDPI_UDC_PCMOUT_MAIN; pcmout_index < p_udcexec->execparams.numofpcmout; pcmout_index++)
    {
        switch (p_udcexec->ptop.errflag[pcmout_index])
        {
            /* some errors do not affect the output and processing can continue */
            case DDPI_UDC_ERR_NO_ERROR:
            {
                break;
            }
            case DDPI_UDC_ERR_INVALID_TIMESLICE:
            {
                ALOGE("Invalid timeslice");
                break;
            }
            case DDPI_UDC_ERR_PROG_MISSING:
            {
                ALOGE("Error processing");
                break;
            }
            case DDPI_UDC_ERR_PROG_INCONSISTENT_NBLKS:
            {
                ALOGE("Main and associated audio programs have inconsistent blocks per frame");
                break;
            }
            case DDPI_UDC_ERR_PROG_INCONSISTENT_FS:
            {
                ALOGE("Main and associated audio programs have inconsistent sample rates");
                break;
            }
            default:
            /* unknown or unexpected error code */
            {
                break;
            }
        }

        /* Update nblkspcm and samplerate even if no PCM data output is required */
        p_udcexec->nblkspcm = p_udcexec->ptop.nblkspcm;
        p_udcexec->samplerate = p_udcexec->ptop.pcmsamplerate;

        /* Write out the PCM data if required */
        if (p_udcexec->ptop.pcmdatavalid[pcmout_index])
        {
            int ch;
            int s;
            int samp;
            int *p_samp[FIO_MAXPCMCHANS];
            /* Assign the output buffers */
            for (ch = 0; ch < p_udcexec->execparams.outnchans[pcmout_index]; ch++)
            {
                p_samp[ch] = (int *) p_udcexec->a_outputbuf[pcmout_index].ppdata[p_udcexec->execparams.chanrouting[pcmout_index][ch]];
            }

            #if (BYTES_PER_UDC_SAMPLE == 4)
            process_32bitoutput(p_buf, p_udcexec->nblkspcm, p_udcexec->execparams.outnchans[pcmout_index], p_samp, p_udcexec->a_outputbuf[pcmout_index].nstride);
#else
            process_16bitoutput(p_buf, p_udcexec->nblkspcm, p_udcexec->execparams.outnchans[pcmout_index], p_samp, p_udcexec->a_outputbuf[pcmout_index].nstride);
#endif // BYTES_PER_UDC_SAMPLE
        }
    }

error:
    return err ? ERR_PROCESS_OUTPUT : 0;
}

/*****************************************************************
* ddpdec_process: Add a certain amount of bytes to the input of the decoder.
* and Processe a DD/DD+ timeslice.
*****************************************************************/
int ddpdec_process(tDdpDecoderExternal *pExt, void *pMem)
{
    UDCEXEC   *pdecexec = (UDCEXEC *) pMem;
    unsigned int bytesconsumed;
    unsigned int avail;
    int timeslicecomplete;
    char *p_buf;
    char *p_dst;
    char *p_outbuf_align = NULL;
    char *p_outbuf;
    int err;

    if (pExt == NULL)
    {
        ALOGE("ddpdec_client.c|  > DD+ DECODER : ddpdec_process FAILED! pExt == NULL");
        return DDPAUDEC_INIT_FAILURE;
    }
    if (pMem == NULL)
    {
        ALOGE("ddpdec_client.c|  > DD+ DECODER : ddpdec_process FAILED! pdecexec == NULL");
        return DDPAUDEC_INIT_FAILURE;
    }

    p_dst = pExt->pOutputBuffer;
    p_outbuf = pExt->pOutMem;
    p_buf = pExt->pInputBuffer;
    avail = pExt->inputBufferCurrentLength;

    err = ddpi_udc_addbytes(pdecexec->p_dechdl, p_buf, avail, DDPI_UDC_DDPIN_0, &bytesconsumed, &timeslicecomplete);

    ALOG3("ddpi_udc_addbytes: mem=%x, avail=%d, bytesconsumed=%d, timeslice=%d, err=%d\n", pdecexec->p_dechdl, avail, bytesconsumed, timeslicecomplete, err);

    /* in case of error, just exit */
    if (err)
    {
        ALOG3(stderr, "ddpi_udc_addbytes returned %d\n", err);
        if (pdecexec->execparams.quitonerr)
        {
            return ERR_PROCESSING;
        }
    }

    pExt->inputBufferUsedLength += bytesconsumed;

    if (timeslicecomplete)
    {
        /*Only setup the channel routing if we've been asked to do so at startup
        or on a change of output mode */
        if (pExt->updatedChannelRouting == 1)
        {
            // up/downmix and downmix mode controlled from stagefright layer. Clear the changed flag if it was set.
            pExt->updatedChannelRouting = 0;
            configureChannelRouting(pdecexec, pExt);
        }

        /*********************************************\
        *  Call subroutine function to process frame  *
        \*********************************************/
        ddpi_udc_processtimeslice(
                pdecexec->p_dechdl,
                &(pdecexec->ptop));

        /* Do additional processing and write output buffer */
        processudcoutput(pdecexec, p_dst);

        /* Clear the PCM output buffer */
        p_outbuf_align = (char *) ((((long) p_outbuf) +(DDPI_UDC_MIN_MEMORY_ALIGNMENT-1)) & ~(DDPI_UDC_MIN_MEMORY_ALIGNMENT-1));
        memset(p_outbuf_align, 0, pdecexec->query_mem_op.outputbuffersize);

        pExt->frameLengthSamplesPerChannel = pdecexec->nblkspcm * FIO_BLKSIZE;
        pExt->samplingRate = pdecexec->samplerate;
        pExt->nrChans = pdecexec->execparams.outnchans[DDPI_UDC_PCMOUT_MAIN];

        ALOG3("ddpdec_process: one frame processed");

        return DDPAUDEC_SUCCESS;
    }
    else
    {
        ALOG3("ddpdec_process: incomplete frame");

        return DDPAUDEC_INCOMPLETE_FRAME;
    }
}
