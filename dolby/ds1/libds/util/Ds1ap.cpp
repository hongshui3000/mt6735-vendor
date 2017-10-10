/*
 * This program is protected under international and U.S. copyright laws as
 * an unpublished work. This program is confidential and proprietary to the
 * copyright owners. Reproduction or disclosure, in whole or in part, or the
 * production of derivative works therefrom without the express permission of
 * the copyright owners is prohibited.
 *
 *                Copyright (C) 2011-2013 by Dolby Laboratories,
 *                            All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
using namespace std;

#include <cutils/log.h>
#ifdef DOLBY_DAP_LICENSE
#include <sys/stat.h>
#endif // ENDIF DOLBY_DAP_LICENSE
#include "Ds1ap.h"
#include "ak_process_loop.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "Ds1ap"

/**
 * DS1AP logging support.
 *
 * 0 for No logging and 1 for verbose logging.
 */
#define DS1AP_LOGGING_EXTRA 1




#ifdef DOLBY_DAP_LICENSE
#define LICENSE_FILE "/data/dolby/ds1-ak.lic"

static void* lic_buf  = NULL;
static int   lic_size = 0;
#endif // ENDIF DOLBY_DAP_LICENSE

static const char channel_mapping_stereo[] =  {
                                                AK_CHAN_L,
                                                AK_CHAN_R
                                              };

static const char channel_mapping_5_dot_1[] = {
                                                AK_CHAN_L,
                                                AK_CHAN_R,
                                                AK_CHAN_C,
                                                AK_CHAN_LFE,
                                                AK_CHAN_Ls,
                                                AK_CHAN_Rs
                                              };

static const char channel_mapping_7_dot_1[] = {
                                                AK_CHAN_L,
                                                AK_CHAN_R,
                                                AK_CHAN_C,
                                                AK_CHAN_LFE,
                                                AK_CHAN_Ls,
                                                AK_CHAN_Rs,
                                                AK_CHAN_Lb,
                                                AK_CHAN_Rb
                                               };

/**
 * Creates new Ds1ap instance.
 *
 * @param endp              Current End Point.
 * @param sampleRate        Track sampling rate.
 * @param audioInChannels   Number of audio input channels.
 * @param audioOutChannels  Number of audio output channels.
 *
 * @return new Ds1ap instance or NULL on failure
 *
 */
Ds1ap *Ds1ap::New(int endp, int sampleRate, int audioInChannels, int audioOutChannels)
{
    Ds1ap *p = new Ds1ap();

    if (p->init(endp, sampleRate, audioInChannels, audioOutChannels))
    {
        ALOGE("DS1ap Init failed");
        delete p;
        p = NULL;
    }

    return p;
}

Ds1ap::Ds1ap()
{
    ak_hndl = NULL;
    scratch_mem = NULL;
    p_pool = NULL;
    scratch = NULL;
}
/**
 * Calls open and if it is successful, sets the ak input channel configuration.
 *
 * @param endp                Current End point.
 * @param sampleRate          Track sample rate.
 * @param audioInChannels     Number of audio input channels.
 * @param audioOutChannels    Number of audio output channels.
 *
 * @return 0 on success or -1 on failure.
 *
 * @internal
 */
int Ds1ap::init(int endp, int sampleRate, int audioInChannels, int audioOutChannels)
{
    int status;
    const char *channel_mapping;

#if DS1AP_LOGGING_EXTRA
    ALOGI("endp %d, sampleRate %d, audioInChannel %d, audioOutChannels %d", endp, sampleRate, audioInChannels, audioOutChannels);
#endif
    if(open(endp))
    {
        ALOGE("DS1ap open failed");
        return -1;
    }
    switch(audioInChannels)
    {
        case 2:
            channel_mapping = channel_mapping_stereo;
            break;
        case 6:
            channel_mapping = channel_mapping_5_dot_1;
            break;
        case 8:
            channel_mapping = channel_mapping_7_dot_1;
            break;
        default:
            ALOGE("unsupported Input Channels (%d)", audioInChannels);
            return -1;
    }

    // audioOutChannels is not used at present as default is 2
    status = ak_set_input_config(ak_hndl, AK_ROOT, sampleRate, AK_BLOCK_SIZE, audioInChannels, channel_mapping);

    if (status)
    {
        ALOGE("ERROR(%d): Cannot configure AK with rate %d and blocksize %d", status, sampleRate, AK_BLOCK_SIZE);
        return -1;
    }

#if DS1AP_LOGGING_EXTRA
    ALOGI("Ds1ap initialization success");
#endif

    return 0;
}

/**
 * Allocates memory for ak_instance. It also gets amount of memory
 * needed for ak engine and scratch memory and allocates it. It then starts
 * the ak engine.
 *
 * @param endp        For DAPv1 versions less then 2, endpoint is init time parameter, 
 *                    we need to set it before ak_start is called.
 *                    For DAPv1 versions equal or greater then 2, endpoint is no longer
 *                    init time parameter and we need to set it after ak_start.
 *
 * @return 0 on success or -1 on failure.
 *
 * @internal
 */
int Ds1ap::open(int endp)
{
    int            status = 0;
    size_t         align_bytes;
    ak_memory_pool scratch_needs;

    ak_hndl = (ak_instance *) malloc(ak_size(NULL));

    if (ak_open(ak_hndl))
    {
        ALOGE("ERROR: ak_open fails\n");
        return -1;
    }
    int hndl = ak_find(ak_hndl, AK_ROOT, AK_N("init"));
    
    // For DAPv1 2.0.0 or above "endp" is not init time parameter.
    // Hence for backward compatibility first index of version array
    // is compared with 2.
    int16_t ver[4] = {0};
    int hndl_v = ak_find(ak_hndl, AK_ROOT, AK_N("ver"));
    ak_get_bulk(ak_hndl, hndl_v, 0, 4, sizeof(ver)/sizeof(ver[0]), ver);

    int hndl_t;
    if (ver[0] < 2)
    {
        ALOGI("AK VERSION %d ", ver[0]);
        hndl_t = ak_find(ak_hndl, hndl, AK_N("endp"));
        ak_set(ak_hndl, hndl_t, 0, endp);
    }

#ifdef DOLBY_DAP_LICENSE
    if (lic_buf == NULL)
    {
        //get the License from the license file
        struct stat statBuf;
        FILE *fp_lic = NULL;

        status = stat(LICENSE_FILE, &statBuf);
        if (status != 0)
        {
            ALOGE("ERROR: DS1 AK License file status retrieval failure!");
        }
        else
        {
            lic_size = statBuf.st_size;
            lic_buf = (void *)malloc(lic_size);
            if (lic_buf == NULL)
            {
                ALOGE("ERROR: DS1 AK License buffer allocation failure!");
            }
            else
            {
                fp_lic = fopen(LICENSE_FILE, "rb");
                if (fp_lic != NULL)
                {
                    int ret = fread(lic_buf, 1, lic_size, fp_lic);
                    if (ret != lic_size)
                        ALOGE("ERROR: DS1 AK License key read failure!");
                    fclose(fp_lic);
                    fp_lic = NULL;
                }
                else
                {
                    ALOGE("ERROR: DS1 AK License file open failure!");
                    free(lic_buf);
                    lic_buf = NULL;
                }
            }
        }
    }

    //set license buffer size to AK
    if (lic_size != 0)
    {
        hndl_t = ak_find(ak_hndl, hndl, AK_N("lcsz"));
        ak_set(ak_hndl, hndl_t, 0, lic_size);
    }
#endif // ENDIF DOLBY_DAP_LICENSE

    //set memory to AK engine
    p_pool = (ak_memory_pool* ) malloc(ak_num_pools() * sizeof(p_pool[0]));
    status = ak_needs(ak_hndl, AK_ROOT, p_pool);
    if (status)
    {
        ALOGE("ERROR(%d): Failed to determine AK memory needs", status);
        return -1;
    }

    p_mem = (void **) malloc( sizeof(void*) * ak_num_pools());
    for (int p = 0; p < ak_num_pools(); p++)
    {
        align_bytes = (1 << p_pool[p].alignment) - 1;
        p_mem[p] = malloc(p_pool[p].size + align_bytes);

        // Manually align the address
        p_pool[p].pmem = (void*)
                         (((char*) p_mem[p] + align_bytes - (char*) 0) & ~align_bytes);
    }

    status = ak_scratch(ak_hndl, AK_ROOT, &scratch_needs);
    if (status)
    {
        ALOGE("ERROR(%d): Failed to determine AK scratch needs.\n", status);
        return -1;
    }

    // Allocate memory for scratch data. We manually align the scratch
    // buffer as we did for the main  memory pools.
    //

    align_bytes = (1 << scratch_needs.alignment) - 1;
    scratch = malloc(align_bytes + scratch_needs.size);
    scratch_mem = (void*) (((char*) scratch + align_bytes - (char*) 0) & ~align_bytes);

    status = ak_start(ak_hndl, p_pool, NULL, NULL);
    if(status)
    {
        ALOGE("ERROR(%d): Failed to start AK.\n", status);
        return -1;
    }

    ALOGI("ak_started");

    // In case if version of DAPv1 library is equal or greater than 2, then 
    // endp is no longer init time parameter and we have to set it after 
    // ak_start
    if (ver[0] >= 2)
    {
        ALOGI("AK VERSION %d ", ver[0]);
        hndl_t = ak_find(ak_hndl, AK_ROOT, AK_N("endp"));
        ak_set(ak_hndl, hndl_t, 0, endp);
    }

#ifdef DOLBY_DAP_LICENSE
    // Set license to AK
    if (lic_buf != NULL)
    {
        ALOGI("Set license to AK ...");
        hndl = ak_find(ak_hndl, AK_ROOT, AK_N("lcpt"));
        status = ak_set_bulk(ak_hndl, hndl, 0, lic_size, AK_DATATYPE_CHAR, lic_buf);
        if (status != AK_OK)
        {
            ALOGE("ERROR: DS1 AK lcpt setting ERROR");
        }
    }
#endif // ENDIF DOLBY_DAP_LICENSE

    return 0;
}

/**
 * Shuts down AK and free's the allocated memory.
 *
 * @internal
 */
void Ds1ap::close()
{
    ak_stop(ak_hndl);

    if (scratch)
        free(scratch);

    for (int p = 0; p < ak_num_pools(); p++)
    {
        if (p_mem[p])
            free(p_mem[p]);
    }
    free(p_mem);
    p_mem = NULL;

    if (p_pool)
        free(p_pool);

    p_pool = NULL;

    ak_close(ak_hndl);

    if (ak_hndl)
        free(ak_hndl);

    ak_hndl = NULL;

#if DS1AP_LOGGING_EXTRA
    ALOGI("ak closed");
#endif
}

Ds1ap::~Ds1ap()
{
	close();
}
