#ifndef _PRMT_IGD_H_
#define _PRMT_IGD_H_

//#include <linux/config.h> keith remove
//#include <config/autoconf.h> keith remove

#include "libcwmp.h"
#include "cwmp_porting.h"
#include "apmib.h" //keith add.
//#ifdef VOIP_SUPPORT //Keith add for tr069
//#include "kernel_config.h"
//#endif

//#include <rtk/options.h> keith remove
//#include <rtk/sysconfig.h> keith remove
//#include "adsl_drv.h" keith remove
//#include "utility.h" keith remove
#include "prmt_apply.h"
#ifdef CONFIG_MIDDLEWARE
#include <rtk/midwaredefs.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define	TR098_DEBUG
#define	TR098_TRACE
#define	TR098_ERROR
#define	TR098_WARNING

#ifdef TR098_DEBUG
#define tr098_printf(fmt, arg...) fprintf(stderr, "[TR098][FILE]%s:[FUNCTION]%s:[LINE]%d - "fmt"\n",__FILE__,__FUNCTION__,__LINE__,##arg)
#else
#define tr098_printf(a,...) do{}while(0)
#endif

#ifdef TR098_TRACE
#define tr098_trace() fprintf(stderr, "[TR098][FILE]%s:[FUNCTION]%s:[LINE]%d\n",__FILE__,__FUNCTION__,__LINE__)
#else
#define tr098_trace(a,...) do{}while(0)
#endif

#ifdef TR098_ERROR
#define tr098_error(fmt, arg...) fprintf(stderr, "[TR098][ERROR][%d]"fmt"\n",__LINE__,##arg)
#else
#define tr098_error(a,...) do{}while(0)
#endif

#ifdef TR098_WARNING
#define tr098_warning(fmt, arg...) fprintf(stderr, "[TR098][WARNING][%d]"fmt"\n",__LINE__,##arg)
#else
#define tr098_warning(a,...) do{}while(0)
#endif

extern struct CWMP_LEAF tLANConfigSecurityLeaf[];
extern struct CWMP_LEAF tIGDLeaf[];
extern struct CWMP_NODE tIGDObject[];
extern struct CWMP_NODE tROOT[];
#ifdef CONFIG_MIDDLEWARE
extern struct CWMP_NODE mw_tROOT[];
#endif



#if defined(_PRMT_X_CT_EXT_ENABLE_)
#define VENDOR_NAME "CT-COM_"
#else
#define VENDOR_NAME "EXT_" 
#endif

#define CREATE_NAME(node) "X_"VENDOR_NAME#node


int getIGD(char *name, struct CWMP_LEAF *entity, int *type, void **data);

int getLANConfSec(char *name, struct CWMP_LEAF *entity, int *type, void **data);
int setLANConfSec(char *name, struct CWMP_LEAF *entity, int type, void *data);

#ifdef __cplusplus
}
#endif

#endif /*_PRMT_IGD_H_*/
