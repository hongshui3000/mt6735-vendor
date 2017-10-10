/*
 * This program is protected under international and U.S. copyright laws as
 * an unpublished work. This program is confidential and proprietary to the
 * copyright owners. Reproduction or disclosure, in whole or in part, or the
 * production of derivative works therefrom without the express permission of
 * the copyright owners is prohibited.
 *
 *                Copyright (C) 2011-2012 by Dolby Laboratories,
 *                            All rights reserved.
 *
 */

#ifndef _DS1AP_H_
#define _DS1AP_H_

#include "ak.h"

class Ds1ap
{

public:
    static Ds1ap	*New(int endp, int sampleRate, int audioInChannels, int audioOutChannels);
    ~Ds1ap();

    inline ak_instance *get_ak_hndl(void) { return ak_hndl; }
    inline void *get_ak_scratch_mem(void) { return scratch_mem; }

private:
    Ds1ap();

    int open(int endp);
    void close();

    int init(int endp, int nInSampleRate, int nInChannels, int nOutChannels);

    ak_instance     *ak_hndl;
    ak_memory_pool  *p_pool;
    void*           *p_mem;
    void            *scratch;
    void            *scratch_mem;
};

#endif //_DS1AP_H_
