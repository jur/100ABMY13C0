/*
* Copyright c                  Realtek Semiconductor Corporation, 2008  
* All rights reserved.                                                    
* 
* Program : just for driver debug
* Abstract :                                                           
* Author : Hyking Liu (Hyking_liu@realsil.com.tw)               
* -------------------------------------------------------
*/
#ifndef RTL865X_PROC_DEBUG_H
#define RTL865X_PROC_DEBUG_H
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define RTL865X_PROC_DIR_NAME "rtl865x"

#if		defined(BATMAN_ENABLE)
	#include <linux/netdevice.h>
	#include <net/rtl/rtl_nic.h>

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)
		#define NETDRV_PRIV(X) netdev_priv(X)
	#else
		#define NETDRV_PRIV(X) ((X)->priv)
	#endif
struct port_status {
	int	link;
	int	duplex;
	int	speed;
};
int batadv_get_port_status(struct net_device *, struct port_status *);
#endif	//BATMAN_ENABLE

int32 rtl865x_proc_debug_init(void);
int32 rtl865x_proc_debug_cleanup(void);
#endif

