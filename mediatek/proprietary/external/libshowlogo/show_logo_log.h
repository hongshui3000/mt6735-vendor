

#ifndef _SHOW_LOGO_LOG_H
#define _SHOW_LOGO_LOG_H

#ifdef __cplusplus
extern "C" {
#endif


#ifdef  BUILD_LK

#include <debug.h>
#include <lib/zlib.h>

#include <utils/Log.h>
#include <cutils/klog.h>

#ifndef  LOG_ANIM
#define  LOG_ANIM(x...)     dprintf(0, x)  //do { KLOG_ERROR("charger", x); } while (0)

#endif

#else

#include <cutils/xlog.h>
#include "zlib.h"

#include <utils/Log.h>
#include <cutils/klog.h>

#ifndef  LOG_ANIM
#define  LOG_ANIM(x...)       XLOGD(x)  //do { KLOG_ERROR("charger", x); } while (0)

#endif

#endif


        
    
#ifdef __cplusplus
}
#endif
#endif
