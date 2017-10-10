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

#define LOG_TAG "udc_user"
#include <utils/Log.h>

/**** Module Dependencies ****/

/* subroutine headers */
#include "udc_api.h"

/* executive headers */
#include "udc_exec.h"
#include "udc_user.h"
#include "ddpdec_client.h"

/* Dolby cinema channel ordering */
typedef enum {
	DLB_CH_NONE = -1,    /*!< Channel not in use */
	DLB_CH_LEFT =  0,    /*!< Left channel */
	DLB_CH_CNTR =  1,    /*!< Right channel */
	DLB_CH_RGHT =  2,    /*!< Center channel */
	DLB_CH_LSUR =  3,    /*!< Left surround channel */
	DLB_CH_MSUR =  3,    /*!< Mono surround channel */
	DLB_CH_RSUR =  4,    /*!< Right surround channel */
	DLB_CH_LFE  =  5,    /*!< Low frequency effects channel */
    DLB_CH_EXT1 =  6,    /*!< Extension channel 1 */
    DLB_CH_EXT2 =  7,    /*!< Extension channel 2 */
} DLB_CH;

/* Map from WAV channel id to Dolby channel id */
DLB_CH wav2dlb_chidmap[DDPI_UDC_MAXPCMOUTCHANS] =
{
    DLB_CH_LEFT,    /*!< Left channel */
    DLB_CH_RGHT,    /*!< Right channel */
    DLB_CH_CNTR,    /*!< Center channel */
    DLB_CH_LFE,     /*!< Low frequency effects channel */
    DLB_CH_LSUR,    /*!< Left surround channel */
    DLB_CH_RSUR,    /*!< Right surround channel */
    DLB_CH_EXT1,    /*!< Extension channel 1 */
    DLB_CH_EXT2,    /*!< Extension channel 2 */
};

/* Alternate mapping for MS standard Lrs/Rrs config */
DLB_CH wav2dlb_altchidmap[DDPI_UDC_MAXPCMOUTCHANS] =
{
    DLB_CH_LEFT,    /*!< Left channel */
    DLB_CH_RGHT,    /*!< Right channel */
    DLB_CH_CNTR,    /*!< Center channel */
    DLB_CH_LFE,     /*!< Low frequency effects channel */
    DLB_CH_EXT1,    /*!< Extension channel 1 */
    DLB_CH_EXT2,    /*!< Extension channel 2 */
    DLB_CH_LSUR,    /*!< Left surround channel */
    DLB_CH_RSUR,    /*!< Right surround channel */
};

/*****************************************************************\
* displaybanner: display version, copyright, and configuration info
\*****************************************************************/
int  displaybanner(
    const ddpi_udc_query_op  *p_decparams)                 /* input  */
{
    /* check input arguments */
//    assert(p_decparams);

    /* display version info */
    ALOG1("\n*** DDPlus Unified Decoder Simulation  Version %d.%d.%d ***",
           p_decparams->version_major,
           p_decparams->version_minor,
           p_decparams->version_update);

    /* display configuration info */
    ALOG1("\n7.1-channel decoder-converter, ");
    ALOG1("\n5.1-channel dual-decoder-converter, ");

    ALOG1("Karaoke unsupported, ");
    ALOG1("TPNP supported. ");

    /* display copyright info */
    ALOG1("%s", p_decparams->p_copyright);

    return (ERR_NO_ERROR);

}

/*****************************************************************\
*	initexecparams: initialize variables used by the executive
\*****************************************************************/
int initexecparams(
    EXECPARAMS               *p_execparams)                /* modify */
{
    /* declare local variables */
    int ch;
    int i;

    /* check input arguments */
//    assert(p_execparams);
    p_execparams->alt71wavrouting = 0;
    p_execparams->pcmwordtype = FIO_PCMINT16;
    p_execparams->quitonerr = 1;
    p_execparams->numofddpin = 1;
    p_execparams->numofddout = 0;
    p_execparams->numofpcmout = 1;
    p_execparams->numofmainpcmout = 1;
    for (i = 0; i < DDPI_UDC_PCMOUT_COUNT; i++)
    {
        p_execparams->outnchans[i] = DDPI_UDC_MAXPCMOUTCHANS;
        for (ch = 0; ch < DDPI_UDC_MAXPCMOUTCHANS; ch++)
        {
            if (p_execparams->alt71wavrouting)
                p_execparams->chanrouting[i][ch] = wav2dlb_altchidmap[ch];
            else
                p_execparams->chanrouting[i][ch] = wav2dlb_chidmap[ch];
        }
    }

    return (ERR_NO_ERROR);
}

/*****************************************************************\
* initsubparams:	initialize variables used by the subroutine
\*****************************************************************/
int initsubparams(
    SUBPARAMS                *p_subparams)                 /* output */
{
    /* check input arguments */
//    assert(p_subparams);

    p_subparams->runningmode = DDPI_UDC_DECODE_ONLY;
    p_subparams->compmode = DDPI_UDC_COMP_PORTABLE_L14;
    p_subparams->outlfe = DDPI_UDC_LFEOUTSINGLE;
    p_subparams->outchanconfig = BSI_ACMOD_20;
    p_subparams->pcmscale = DEFAULTSCALEFACTOR;
    p_subparams->dynscalehigh = DEFAULTSCALEFACTOR;
    p_subparams->dynscalelow = DEFAULTSCALEFACTOR;
    p_subparams->stereomode = DDPI_UDC_STEREOMODE_AUTO;
    p_subparams->dualmode = DDPI_UDC_DUAL_STEREO;

    p_subparams->inputmode = DDPI_UDC_INPUTMODE_SINGLEINPUT;
    p_subparams->associdselect = -1;
    p_subparams->mdctbandlimit = 0;

    p_subparams->dec_errorconcealflag	= 1;
    p_subparams->dec_errorconcealtype	= DDPI_UDC_PCMCONCEAL_ALWAYSRPT;
    p_subparams->cnv_errorconcealflag   = 1;

    return (ERR_NO_ERROR);
}

/*****************************************************************\
* setsubparams: set subroutine parameters
\*****************************************************************/
int setsubparams(
    int numofoutputs,
    SUBPARAMS *p_subparams,
    void *p_dechdl)
{
    /* declare local variables */
    int err;
    int instance;
    SUBPARAMS *p_subparamsno;

    /* check input arguments */
//    assert(p_subparams);
//    assert(p_dechdl);

    p_subparamsno = p_subparams;

    for(instance = 0; instance < numofoutputs; instance++)
    {
        err = ddpi_udc_setoutparam(
            p_dechdl,
            instance,
            DDPI_UDC_OUTCTL_OUTLFEON_ID,
            &p_subparamsno->outlfe,
            sizeof(p_subparamsno->outlfe));
        if (err) goto error;

        err = ddpi_udc_setoutparam(
            p_dechdl,
            instance,
            DDPI_UDC_OUTCTL_OUTMODE_ID,
            &p_subparamsno->outchanconfig,
            sizeof(p_subparamsno->outchanconfig));
        if (err) goto error;

        err = ddpi_udc_setoutparam(
            p_dechdl,
            instance,
            DDPI_UDC_OUTCTL_DUALMODE_ID,
            &p_subparamsno->dualmode,
            sizeof(p_subparamsno->dualmode));
        if (err) goto error;

        err = ddpi_udc_setoutparam(
            p_dechdl,
            instance,
            DDPI_UDC_OUTCTL_COMPMODE_ID,
            &p_subparamsno->compmode,
            sizeof(p_subparamsno->compmode));
        if (err) goto error;

        err = ddpi_udc_setoutparam(
            p_dechdl,
            instance,
            DDPI_UDC_OUTCTL_OUTPCMSCALE_ID,
            &p_subparamsno->pcmscale,
            sizeof(p_subparamsno->pcmscale));
        if (err) goto error;

        err = ddpi_udc_setoutparam(
            p_dechdl,
            instance,
            DDPI_UDC_OUTCTL_STEREOMODE_ID,
            &p_subparamsno->stereomode,
            sizeof(p_subparamsno->stereomode));
        if (err) goto error;

        err = ddpi_udc_setoutparam(
            p_dechdl,
            instance,
            DDPI_UDC_OUTCTL_DRCSCALEHIGH_ID,
            &p_subparamsno->dynscalehigh,
            sizeof(p_subparamsno->dynscalehigh));
        if (err) goto error;

        err = ddpi_udc_setoutparam(
            p_dechdl,
            instance,
            DDPI_UDC_OUTCTL_DRCSCALELOW_ID,
            &p_subparamsno->dynscalelow,
            sizeof(p_subparamsno->dynscalelow));
        if (err) goto error;

        err = ddpi_udc_setoutparam(
            p_dechdl,
            instance,
            DDPI_UDC_OUTCTL_MDCTBANDLIMIT_ID,
            &p_subparamsno->mdctbandlimit,
            sizeof(p_subparamsno->mdctbandlimit));
        if (err) goto error;

        err = ddpi_udc_setoutparam(
            p_dechdl,
            instance,
            DDPI_UDC_OUTCTL_DRCSUPPRESS_ID,
            &p_subparamsno->drcsuppressmode,
            sizeof(p_subparamsno->drcsuppressmode));
        if (err) goto error;

        p_subparamsno = (SUBPARAMS *)((char *)p_subparamsno + sizeof(SUBPARAMS));
    }

    err = ddpi_udc_setprocessparam(
        p_dechdl,
        DDPI_UDC_CTL_RUNNING_MODE_ID,
        &p_subparams->runningmode,
        sizeof(p_subparams->runningmode));
    if (err) goto error;

    /* Note: inputmode MUST be sent before associdselect */
    err = ddpi_udc_setprocessparam(
        p_dechdl,
        DDPI_UDC_CTL_INPUTMODE_ID,
        &p_subparams->inputmode,
        sizeof(p_subparams->inputmode));
    if (err) goto error;

    err = ddpi_udc_setprocessparam(
        p_dechdl,
        DDPI_UDC_CTL_SUBSTREAMSELECT_ID,
        &p_subparams->associdselect,
        sizeof(p_subparams->associdselect));
    if (err) goto error;

    err = ddpi_udc_setprocessparam(
        p_dechdl,
        DDPI_UDC_CTL_ERRORCONCEAL_ID,
        &p_subparams->dec_errorconcealflag,
        sizeof(p_subparams->dec_errorconcealflag));
    if (err) goto error;

    err = ddpi_udc_setprocessparam(
        p_dechdl,
        DDPI_UDC_CTL_ERRORMAXRPTS_ID,
        &p_subparams->dec_errorconcealtype,
        sizeof(p_subparams->dec_errorconcealtype));
    if (err) goto error;

    err = ddpi_udc_setprocessparam(
        p_dechdl,
        DDPI_UDC_CTL_CNV_ERRORCONCEAL_ID,
        &p_subparams->cnv_errorconcealflag,
        sizeof(p_subparams->cnv_errorconcealflag));
    if (err) goto error;

    return (ERR_NO_ERROR);
error:
    return (err);
}
