/*
 *      Operation routines for FLASH MIB access
 *
 *      Authors: David Hsu	<davidhsu@realtek.com.tw>
 *
 *      $Id: flash.c,v 1.93 2009/09/15 02:12:32 bradhuang Exp $
 *
 */


/* System include files */
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <ctype.h>
#include <regex.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/wireless.h>
#define noPARSE_TXT_FILE

#define WLAN_FAST_INIT
#define BR_SHORTCUT	

/* Local include files */
#include "apmib.h"
#include "mibtbl.h"

#include <linux/if.h>
#include <linux/if_ether.h> 
#include <linux/if_pppox.h> 
#include <linux/if_packet.h> 
#ifdef RTL_DEF_SETTING_IN_FW
//#include "../defconfig/def_setting_common.h"
#include "../apmib/def_setting_config.h"
#endif
typedef enum {DEFAULT_SW=0,DEFAULT_ALL=1,DEFAULT_HW=2,DEFAULT_BLUETOOTHHW=4,DEFAULT_CUSTOMERHW=8,DEFAULT_SAVEDEF} WRITE_DEFAULT_TYPE_T;

//---------------------------------------------------------------------
#ifdef SYS_DIAGNOSTIC
#define RTK_SUCCESS (1)
#define RTK_FAILED  (0)

#define DUMP_DIAG_LOG_FILE "/tmp/dumpDiagnosticLog.txt"
#define DUMP_DIAG_LOG_FILE_BAK "/tmp/dumpDiagnosticLog_bak.txt"

#define SSID_LEN 32
#define IP_ADDR_T 0x02
#define NET_MASK_T 0x04
#define HW_ADDR_T 0x08
#define RTL8651_IOCTL_GETWANLINKSTATUS 2000
#define _DHCPC_PROG_NAME	"udhcpc"
#define _DHCPC_PID_PATH		"/etc/udhcpc"
#define _PATH_PROCNET_ROUTE	"/proc/net/route"
#define RTF_UP			0x0001          //route usable
#define RTF_GATEWAY		0x0002          //destination is a gateway
#define _PATH_DEVICE_MAC_BRAND "/etc/device_mac_brand.txt"
#define TERMINAL_RATE_INFO     "/var/terminal_rate_info"
#define MAX_TERM_NUMBER 64

typedef enum { IP_ADDR, DST_IP_ADDR, SUBNET_MASK, DEFAULT_GATEWAY, HW_ADDR } ADDR_T;
typedef enum {DIAG_TYPE_SYS_SUMMARY=0,DIAG_TYPE_WLAN_SCAN,DIAG_TYPE_WLAN_CONNECT,DIAG_TYPE_WLAN_THROUGHPUT}sys_diagnostic_type;

typedef enum _rtk_wlan_mac_state 
{
    RTK_STATE_DISABLED=0, RTK_STATE_IDLE, RTK_STATE_SCANNING, RTK_STATE_STARTED, RTK_STATE_CONNECTED, RTK_STATE_WAITFORKEY
} rtk_wlan_mac_state;

typedef struct bss_info
{
    unsigned char state;
    unsigned char channel;
    unsigned char txRate;
    unsigned char bssid[6];
    unsigned char rssi, sq;	// RSSI  and signal strength
    unsigned char ssid[SSID_LEN+1];
} RTK_BSS_INFO, *RTK_BSS_INFOp;

enum LINK_TYPE 
{
	RTK_LINK_ERROR =0,
	RTK_ETHERNET,
	RTK_5G,
	RTK_24G
};

struct rtk_link_type_info 
{
	enum LINK_TYPE  type;		//terminal type(2.4G,5G,ethernet)
	struct in_addr ip;			//terminal ip
	unsigned char  mac[6];		//terminal mac
	unsigned int download_speed;//terminal download speed(bytes/s)
	unsigned int upload_speed;	//terminal upload speed(bytes/s)
	unsigned int last_rx_bytes;
	unsigned int last_tx_bytes;	
	unsigned int cur_rx_bytes;
	unsigned int cur_tx_bytes;  
	unsigned int all_bytes;		//all traffic(bytes)	
	int port_number;			//terminal phy port number
	// modify by chenbo(realtek)
	unsigned char brand[16];
	unsigned int link_time;
};
enum WAN_STATUS 
{
	DISCONNECTED = 0, FIXED_IP_CONNECTED, FIXED_IP_DISCONNECTED,
	GETTING_IP_FROM_DHCP_SERVER, DHCP,
	PPPOE_CONNECTED, PPPOE_DISCONNECTED,
	PPTP_CONNECTED, PPTP_DISCONNECTED,
	L2TP_CONNECTED, L2TP_DISCONNECED,
	USB3G_CONNECTED, USB3G_MODEM_INIT,
	USB3G_DAILING, USB3G_REMOVED, USB3G_DISCONNECTED
};

typedef struct wan_info
{
	enum WAN_STATUS status;
	struct in_addr ip;
	struct in_addr mask;
	struct in_addr def_gateway;
	struct in_addr dns1;
	struct in_addr dns2;
	struct in_addr dns3;
	unsigned char mac[6];
}RTK_WAN_INFO, *RTK_WAN_INFOp;
typedef struct rtk_wlan_sta_info 
{
 unsigned short aid;
 unsigned char  addr[6];
 unsigned long  tx_packets;
 unsigned long  rx_packets;
 unsigned long  expired_time; // 10 msec unit
 unsigned short flag;
 unsigned char  txOperaRates;
 unsigned char  rssi;
 unsigned long  link_time;  // 1 sec unit
 unsigned long  tx_fail;
 unsigned long  tx_bytes;
 unsigned long  rx_bytes;
 unsigned char  network;
 unsigned char  ht_info; // bit0: 0=20M mode, 1=40M mode; bit1: 0=longGI, 1=shortGI
 unsigned char  RxOperaRate;
 unsigned char  resv[5];
} RTK_WLAN_STA_INFO_T, *RTK_WLAN_STA_INFO_Tp;
struct rtk_terminal_rate_info 
{
	unsigned char  mac[6];		//terminal mac
	unsigned int download_speed;//terminal download speed(bytes/s)
	unsigned int upload_speed;	//terminal upload speed(bytes/s)
	unsigned int rx_bytes;
	unsigned int tx_bytes;  
	unsigned int all_bytes;		//all traffic(bytes)	
	int port_number;			//terminal phy port number
	unsigned int link_flag;
};
struct rtk_dev_link_time
{
	unsigned char ip[16];
	unsigned char mac[24];
	unsigned int link_time;
	int is_alive;
};
#endif
//---------------------------------------------------------------------

//#define SDEBUG(fmt, args...) printf("[%s %d]"fmt,__FUNCTION__,__LINE__,## args)
#define SDEBUG(fmt, args...) {}
//#define P2P_DEBUG(fmt, args...) printf("[%s %d]"fmt,__FUNCTION__,__LINE__,## args)
#define P2P_DEBUG(fmt, args...) {}

extern char *apmib_load_csconf(void);
extern char *apmib_load_dsconf(void);
extern char *apmib_load_hwconf(void);
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
extern char *apmib_load_bluetooth_hwconf(void);
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
extern char *apmib_load_customer_hwconf(void);
#endif
#ifdef CONFIG_RTL_COMAPI_CFGFILE
//extern int comapi_initWlan(char *ifname);
extern int dumpCfgFile(char *ifname, struct wifi_mib *pmib, int idx);
#endif
extern int Encode(unsigned char *ucInput, unsigned int inLen, unsigned char *ucOutput);
extern int Decode(unsigned char *ucInput, unsigned int inLen, unsigned char *ucOutput);
extern unsigned int mib_get_setting_len(CONFIG_DATA_T type);
extern unsigned int mib_tlv_save(CONFIG_DATA_T type, void *mib_data, unsigned char *mib_tlvfile, unsigned int *tlv_content_len);
extern unsigned int mib_get_real_len(CONFIG_DATA_T type);
extern unsigned int mib_get_flash_offset(CONFIG_DATA_T type);

/* Constand definitions */
#define DEC_FORMAT	("%d")
#define UDEC_FORMAT ("%u")
#define BYTE5_FORMAT	("%02x%02x%02x%02x%02x")
#define BYTE6_FORMAT	("%02x%02x%02x%02x%02x%02x")
#define BYTE13_FORMAT	("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x")
#define STR_FORMAT	("%s")
#define HEX_FORMAT	("%02x")
#define SCHEDULE_FORMAT	("%d,%d,%d,%d")
#ifdef HOME_GATEWAY
#define PORTFW_FORMAT	("%s, %d, %d, %d, %d")

//InstanceNum RootIdx VWlanIdx IsConfigured RfBandAvailable
#define CWMP_WLANCONF_FMT	("%d, %d, %d, %d %d")

#define PORTFILTER_FORMAT ("%d, %d, %d")
#define IPFILTER_FORMAT	("%s, %d")
#define TRIGGERPORT_FORMAT ("%d, %d, %d, %d, %d, %d")
#endif
#define MACFILTER_FORMAT ("%02x%02x%02x%02x%02x%02x")
#define MACFILTER_COLON_FORMAT ("%02x:%02x:%02x:%02x:%02x:%02x")
#define WDS_FORMAT	("%02x%02x%02x%02x%02x%02x,%u")
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
#define DHCPRSVDIP_FORMAT ("%d,%02x%02x%02x%02x%02x%02x,%s,%s")
#else
#define DHCPRSVDIP_FORMAT ("%02x%02x%02x%02x%02x%02x,%s,%s")
#endif
#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT) || defined(CONFIG_RTL_HW_VLAN_SUPPORT)
#define VLANCONFIG_FORMAT ("%s,%d,%d,%d,%d,%d,%d,%d")
#else
#define VLANCONFIG_FORMAT ("%s,%d,%d,%d,%d,%d,%d")
#endif
#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
#define DHCP_OPTION_FORMAT	("%d, %d, %d, %d, %d, %s, %d, %d, %d")
#define DHCPS_SERVING_POOL_FORMAT	("%d, %d, %s, %d, %d, %d, %s, %d, %s, %s, %d, %s, %d, %s, %s, %d, %d, %s, %s, %s, %s, %s, %s, %s, %s, %d, %s, %d, %d")
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */
#ifdef HOME_GATEWAY
#ifdef QOS_OF_TR069
#define QOSQUEUE_FORMAT ("%d, %d, %d, %s, %s, %s, %d, %d, %d, %d, %d, %s, %s, %d, %d") //15
#define QOSCLASS_FORMAT ("%d, %d, %d, %s, %s, %d, %s, %s, %s, %d, %s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %2x:%2x:%2x:%2x:%2x:%2x, %2x:%2x:%2x:%2x:%2x:%2x, %d, %2x:%2x:%2x:%2x:%2x:%2x, %2x:%2x:%2x:%2x:%2x:%2x, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %s, %d, %s, %s, %d, %s, %s, %d, %s, %d, %s, %d, %s, %d, %s, %d, %d, %d, %s, %s, %d, %d, %d, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d")
//mark_qos
#define QOSPOLICER_FORMAT ("%d, %d, %d, %s, %d, %d, %d, %d, %d, %s, %s, %s, %s, %s")
#define QOSQUEUESTATS_FORMAT ("%d, %d, %s, %d, %s")

#define QOSAPP_FORMAT ("%d, %d, %s, %s, %s, %s, %d, %d, %d, %d, %d, %d, %d")
#define QOSFLOW_FORMAT ("%d, %d, %s, %s, %s, %s, %s, %d, %d, %d, %d, %d, %d, %d, %d")
#endif
#ifdef VPN_SUPPORT
//#define IPSECTUNNEL_FORMAT ("%d, %d, %s, %d, %s, %d, %d, %s , %d, %s, %d, %d, %d, %d, %d, %d, %s, %d, %d, %d, %lu, %lu, %d, %s, %s, %s")
#define IPSECTUNNEL_FORMAT ("%d, %d, %s, %d, %s, %d, %d, %s , %d, %s, %d, %d,  %d, %d,  %s, %d, %d, %d, %lu, %lu, %d, %s, %s, %s, %d, %s, %s, %d, %d, %s")
#endif
#ifdef CONFIG_IPV6
#define RADVD_FORMAT ("%u, %s, %u, %u, %u, %u, %u, %u, %u, %u ,%u, %u, %s, %u, %u, %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x, %u, %u, %u, %u, %u, %u, %s, %u, %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x, %u, %u, %u, %u, %u, %u, %s, %d")
#define DNSV6_FORMAT ("%d, %s")
#define DHCPV6S_FORMAT ("%d, %s, %s, %s, %s")
/*enable, ifname, pd's len, pd's id, pd's ifname*/
#define DHCPV6C_FORMAT ("%d, %s, %d, %d, %s")

#define ADDR6_FORMAT ("%d, %d, %d, %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x, %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x")
#define ADDRV6_FORMAT ("%d, %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x")

#endif
#ifdef CONFIG_RTL_AIRTIME
#define AIRTIME_FORMAT	("%s, %02x%02x%02x%02x%02x%02x, %d")
#endif /* CONFIG_RTL_AIRTIME */
#endif

#ifdef CONFIG_RTL_WAPI_SUPPORT
#define CA_CERT "/var/myca/CA.cert"
#endif

#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
static char wlan_1x_ifname[16];
#endif
#ifndef CONFIG_MTD_NAND
#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE)
#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_WEB_BACKUP_ENABLE)
static char *Web_dev_name[2]=
{
	"/dev/mtdblock0", "/dev/mtdblock2"
};
#endif
#endif
#else
#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE)
#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_WEB_BACKUP_ENABLE)
static char *Web_dev_name[2]=
{
	"/dev/mtdblock2", "/dev/mtdblock5"
};
#endif
#endif

#endif

#define SPACE	(' ')
#define EOL	('\n')

#define LOCAL_ADMIN_BIT 0x02
#if defined(SET_MIB_FROM_FILE)
#define SYSTEM_CONF_FILE "/var/sys.conf"
static int set_mib_from_file=0;
#endif
#ifdef CONFIG_APP_SIMPLE_CONFIG
unsigned char sc_default_pin[]="57289961";
#endif


#if defined(DOT11K) 
#define NEIGHBOR_REPORT_FILE "/proc/%s/rm_neighbor_report"
#endif

#if 0
typedef enum { 
	HW_MIB_AREA=1, 
	HW_MIB_WLAN_AREA,
#ifdef CUSTOMER_HW_SETTING_SUPPORT
	CUSTOMER_HW_AREA,
#endif
	DEF_MIB_AREA,
	DEF_MIB_WLAN_AREA,
	MIB_AREA,
	MIB_WLAN_AREA
} CONFIG_AREA_T;
#endif

static CONFIG_AREA_T config_area;
static int send_ppp_msg(int sessid, unsigned char *peerMacStr);
static int send_l2tp_msg(unsigned short l2tp_ns, int buf_length, unsigned char *l2tp_buff, unsigned char *lanIp, unsigned char *serverIp);
#if 1//!defined(CONFIG_RTL_8198C)
#define RTL_L2TP_POWEROFF_PATCH 1
#endif

/* Macro definition */
static int _is_hex(char c)
{
    return (((c >= '0') && (c <= '9')) ||
            ((c >= 'A') && (c <= 'F')) ||
            ((c >= 'a') && (c <= 'f')));
}

static int string_to_hex(char *string, unsigned char *key, int len)
{
	char tmpBuf[4];
	int idx, ii=0;
	for (idx=0; idx<len; idx+=2) {
		tmpBuf[0] = string[idx];
		tmpBuf[1] = string[idx+1];
		tmpBuf[2] = 0;
		if ( !_is_hex(tmpBuf[0]) || !_is_hex(tmpBuf[1]))
			return 0;

		key[ii++] = (unsigned char) strtol(tmpBuf, (char**)NULL, 16);
	}
	return 1;
}

static void convert_lower(char *str)
{	int i;
	int len = strlen(str);
	for (i=0; i<len; i++)
		str[i] = tolower(str[i]);
}

static int APMIB_GET(int id, void *val)
{
	if (config_area == DEF_MIB_AREA || config_area == DEF_MIB_WLAN_AREA)
		return apmib_getDef(id, val);
	else
		return apmib_get(id, val);
}

static int APMIB_SET(int id, void *val)
{
	if (config_area == DEF_MIB_AREA || config_area == DEF_MIB_WLAN_AREA)
		return apmib_setDef(id, val);
	else
		return apmib_set(id, val);
}

#if defined(SET_MIB_FROM_FILE)
static int write_line_to_file(char *filename, int mode, char *line_data)
{
	unsigned char tmpbuf[512];
	int fh=0;

	if(mode == 1) {/* write line datato file */
		
		fh = open(filename, O_RDWR|O_CREAT|O_TRUNC);
		
	}else if(mode == 2){/*append line data to file*/
		
		fh = open(filename, O_RDWR|O_APPEND);	
	}
	
	
	if (fh < 0) {
		fprintf(stderr, "Create %s error!\n", filename);
		return 0;
	}


	sprintf((char *)tmpbuf, "%s", line_data);
	write(fh, tmpbuf, strlen((char *)tmpbuf));



	close(fh);
	return 1;
}
#endif

/* Local declarations routines */
static int flash_read(char *buf, int offset, int len);
static int writeDefault(WRITE_DEFAULT_TYPE_T isAll);
static int searchMIB(char *token);
static void getMIB(char *name, int id, TYPE_T type, int num, int array_separate, char **val);
static void setMIB(char *name, int id, TYPE_T type, int len, int valnum, char **val);
static void dumpAll(void);
static void dumpAllHW(void);
static void showHelp(void);
static void showAllMibName(void);
static void showAllHWMibName(void);
static void showSetACHelp(void);

#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_)
static void showSetMeshACLHelp(void);
#endif
static void showSetVlanConfigHelp(void);
#if defined(SAMBA_WEB_SUPPORT)
static void showSetSambaAccountHelp(void);
static void showSetSambaShareinfoHelp(void);
#endif
#ifdef CONFIG_RTL_ETH_802DOT1X_SUPPORT
static void showSetEthDot1xConfigHelp(void);
#endif
static void showSetWdsHelp(void);
static int read_flash_webpage(char *prefix, char *file);
#ifdef TLS_CLIENT
static int read_flash_cert(char *prefix, char *certfile);
#endif
#ifdef VPN_SUPPORT
static int read_flash_rsa(char *prefix);
#endif
#ifdef HOME_GATEWAY
static void showSetPortFwHelp(void);
static void showSetPortFilterHelp(void);
static void showSetIpFilterHelp(void);
static void showSetMacFilterHelp(void);
static void showSetUrlFilterHelp(void);
static void showSetTriggerPortHelp(void);
#ifdef GW_QOS_ENGINE
//static void showSetQosHelp(void);
#endif

#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
static void showSetDhcpServerOptionHelp(void);
static void showSetDhcpClientOptionHelp(void);
static void showSetDhcpsServingPoolHelp(void);
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */

#ifdef ROUTE_SUPPORT
static void showSetStaticRouteHelp(void);
#endif

#ifdef CONFIG_RTL_AIRTIME
static void showSetAirTimeHelp(void);
#endif /* CONFIG_RTL_AIRTIME */

#ifdef VPN_SUPPORT
static void showSetIpsecTunnelHelp(void);
#endif
#if defined(MULTI_WAN_SUPPORT) 
static int generatePPPConf(int is_pppoe, char *option_file, char *pap_file, char *chap_file , char* order_file);
#else
static int generatePPPConf(int is_pppoe, char *option_file, char *pap_file, char *chap_file);
#endif
#endif

#ifdef TLS_CLIENT
static void showSetCertRootHelp(void);
static void showSetCertUserHelp(void);
#endif
#ifdef RTK_CAPWAP
static void showCapwapSetWtpHelp();
#endif
static void generateWpaConf(char *outputFile, int isWds);
static int generateHostapdConf(void);
#if defined(CONFIG_RTL_ETH_802DOT1X_SUPPORT)
static void generateEthDot1xConf(char *outputFile);
#endif

#ifdef WLAN_FAST_INIT
static int initWlan(char *ifname);
#endif

#ifdef WIFI_SIMPLE_CONFIG
static int updateWscConf(char *in, char *out, int genpin, char *wlanif_name);
#if defined(CONFIG_RTL_COMAPI_CFGFILE)
static int defaultWscConf(char *in, char *out);
#endif
#endif

#ifdef COMPRESS_MIB_SETTING
int flash_mib_checksum_ok(int offset);
int flash_mib_compress_write(CONFIG_DATA_T type, char *data, PARAM_HEADER_T *pheader, unsigned char *pchecksum);
#endif

#ifdef PARSE_TXT_FILE
static int parseTxtConfig(char *filename, APMIB_Tp pConfig);
static int getToken(char *line, char *value);
static int set_mib(APMIB_Tp pMib, int id, void *value);
static void getVal2(char *value, char **p1, char **p2);
#ifdef HOME_GATEWAY
static void getVal3(char *value, char **p1, char **p2, char **p3);
static void getVal4(char *value, char **p1, char **p2, char **p3, char **p4);
static void getVal5(char *value, char **p1, char **p2, char **p3, char **p4, char **p5);
#endif

#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
static void getVal9(char *value, char **p1, char **p2, char **p3, char **p4, 
	char **p5, char **p6, char **p7,char **p8,char **p9);
static void getVal29(char *value, char **p1, char **p2, char **p3, char **p4, char **p5, char **p6, char **p7,\
	char **p8, char **p9, char **p10, char **p11, char **p12, char **p13, char **p14, char **p15, char **p16,\
	char **p17, char **p18, char **p19, char **p20, char **p21, char **p22, char **p23, char **p24, char **p25,\
	char **p26, char **p27, char **p28, char **p29);
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */

#ifdef MIB_TLV

//int mib_write_to_raw(const mib_table_entry_T *mib_tbl, void *data, char *pfile, unsigned int *idx);
#endif


static int acNum;

#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_)
static int meshAclNum;
#endif

static int wdsNum;

#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
static int capPortalAllowNum;
#endif
#ifdef HOME_GATEWAY
static int macFilterNum, portFilterNum, ipFilterNum, portFwNum, triggerPortNum, staticRouteNum;
static int urlFilterNum;
#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
static int dhcpdOptNum, dhcpcOptNum, servingPoolNum;
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */
#if defined(GW_QOS_ENGINE) || defined(QOS_BY_BANDWIDTH)
static int qosRuleNum;
#endif

#ifdef QOS_OF_TR069
static int QosClassNum;
static int QosQueueNum;
static int tr098_appconf_num;
static int tr098_flowconf_num;
//mark_qos
static int QosPolicerNum;
static int QosQueueStatsNum;
#endif
#ifdef CONFIG_RTL_AIRTIME
static int airTimeNum;
#endif /* CONFIG_RTL_AIRTIME */
#endif
static int dhcpRsvdIpNum;
#ifdef TLS_CLIENT
static int certRootNum, certUserNum ;
#endif
#if defined(VLAN_CONFIG_SUPPORTED)
static int vlanConfigNum=MAX_IFACE_VLAN_CONFIG;	
#endif
#if defined(SAMBA_WEB_SUPPORT)
static int sambaAccountConfigNum;	
static int sambaShareInfoNum;
#endif
#if defined(CONFIG_RTL_ETH_802DOT1X_SUPPORT)
static int ethdot1xConfigNum=MAX_ELAN_DOT1X_PORTNUM;	
#endif

static is_wlan_mib=0;
#ifdef TR181_SUPPORT
#ifdef CONFIG_IPV6
static int ipv6DhcpcSendOptNum;
#endif
static int dnsClientServerNum;
#endif

#if defined(RTK_INBAND_NEW_FEATURE)

/*
	ret value:
	0 : fail
	1 : success
*/
int get_ap_status()
{
	
	return 0;
}

#endif


/////////////////////////////////////////////////////////////////////////////////////////
static char __inline__ *getVal(char *value, char **p)
{
	int len=0;

	while (*value == ' ' ) value++;

	*p = value;

	while (*value && *value!=',') {
		value++;
		len++;
	}

	if ( !len ) {
		*p = NULL;
		return NULL;
	}

	if ( *value == 0)
		return NULL;

	*value = 0;
	value++;

	return value;
}
#endif  // PARSE_TXT_FILE

static int flash_write(char *buf, int offset, int len)
{
	int fh;
	int ok=1;

#ifdef CONFIG_MTD_NAND
	if (offset == CURRENT_SETTING_OFFSET)
	{
		fh = open("/config/current_setting.bin", O_RDWR | O_CREAT);
	}
	else
	{
		fh = open(FLASH_DEVICE_SETTING, O_RDWR | O_CREAT);
	}
#else
	fh = open(FLASH_DEVICE_NAME, O_RDWR);
#endif

	if ( fh == -1 )
		return 0;

	lseek(fh, offset, SEEK_SET);

	if ( write(fh, buf, len) != len)
		ok = 0;

	close(fh);
	sync();

	return ok;
}

//////////////////////////////////////////////////////////////////////

static int SetWlan_idx(char * wlan_iface_name)
{
	int idx;
	
		idx = atoi(&wlan_iface_name[4]);
		if (idx >= NUM_WLAN_INTERFACE) {
				printf("invalid wlan interface index number!\n");
				return 0;
		}
		wlan_idx = idx;
		vwlan_idx = 0;
	
#ifdef MBSSID		
		
		if (strlen(wlan_iface_name) >= 9 && wlan_iface_name[5] == '-' &&
				wlan_iface_name[6] == 'v' && wlan_iface_name[7] == 'a') {
				idx = atoi(&wlan_iface_name[8]);
				if (idx >= NUM_VWLAN_INTERFACE) {
					printf("invalid virtual wlan interface index number!\n");
					return 0;
				}
				
				vwlan_idx = idx+1;
				idx = atoi(&wlan_iface_name[4]);
				wlan_idx = idx;
		}
#endif	
#ifdef UNIVERSAL_REPEATER
				if (strlen(wlan_iface_name) >= 9 && wlan_iface_name[5] == '-' &&
						!memcmp(&wlan_iface_name[6], "vxd", 3)) {
					vwlan_idx = NUM_VWLAN_INTERFACE;
					idx = atoi(&wlan_iface_name[4]);
					wlan_idx = idx;
				}
	
#endif				
				
return 1;		
}		

static char *get_token(char *data, char *token)
{
	char *ptr=data;
	int len=0, idx=0;

	while (*ptr && *ptr != '\n' ) {
		if (*ptr == '=') {
			if (len <= 1)
				return NULL;
			memcpy(token, data, len);

			/* delete ending space */
			for (idx=len-1; idx>=0; idx--) {
				if (token[idx] !=  ' ')
					break;
			}
			token[idx+1] = '\0';
			
			return ptr+1;
		}
		len++;
		ptr++;
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////
static int get_value(char *data, char *value)
{
	char *ptr=data;	
	int len=0, idx, i;

	while (*ptr && *ptr != '\n' && *ptr != '\r') {
		len++;
		ptr++;
	}

	/* delete leading space */
	idx = 0;
	while (len-idx > 0) {
		if (data[idx] != ' ') 
			break;	
		idx++;
	}
	len -= idx;

	/* delete bracing '"' */
	if (data[idx] == '"') {
		for (i=idx+len-1; i>idx; i--) {
			if (data[i] == '"') {
				idx++;
				len = i - idx;
			}
			break;
		}
	}

	if (len > 0) {
		memcpy(value, &data[idx], len);
		value[len] = '\0';
	}
	return len;
}

static int setSystemTime_flash(void)
{
	#include <time.h>		
	int cur_year=0;
	//int time_mode=0;
	struct tm tm_time;
	time_t tm;
	FILE *fp;
	char *month_index[]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	char week_date[20], month[20], date[20], year[20], tmp1[20];
	int i;
	char buffer[200];
	regex_t re;
	regmatch_t match[2];
	int status;

	if ( !apmib_init()) {
		printf("Initialize AP MIB failed in setSystemTime_flash!\n");
		return -1;
	}

	set_timeZone();
	fp = fopen("/proc/version", "r");
	if (!fp) {
		printf("Read /proc/version failed!\n");
	}else
		{
			fgets(buffer, sizeof(buffer), fp);
			fclose(fp);
		}

		if (regcomp(&re, "#[0-9][0-9]* \\(.*\\)$", 0) == 0)
		{
			status = regexec(&re, buffer, 2, match, 0);
			regfree(&re);
			if (status == 0 &&
				match[1].rm_so >= 0)
			{
				buffer[match[1].rm_eo] = 0;
   				
			}
		}
	//Mon Nov 10 16:42:19 CST 2008
	
	memset(&tm_time, 0 , sizeof(tm_time));
	sscanf(&buffer[match[1].rm_so],"%s %s %s %d:%d:%d %s %s", week_date, month, date, &(tm_time.tm_hour), &(tm_time.tm_min), &(tm_time.tm_sec), tmp1, year);
	tm_time.tm_isdst = -1;  /* Be sure to recheck dst. */
		for(i=0;i< 12;i++){
			if(strcmp(month_index[i], month)==0){
				tm_time.tm_mon = i;
				break;
			}
		}
		tm_time.tm_year = atoi(year) - 1900;
		tm_time.tm_mday =atoi(date);
		tm = mktime(&tm_time);
		if(tm < 0){
			fprintf(stderr, "set Time Error for tm!");
		}
		if(stime(&tm) < 0){
			fprintf(stderr, "set system Time Error");
		}
	
	if ( !apmib_init()) {
		printf("Initialize AP MIB failed in setSystemTime_flash!\n");
		return -1;
	}
	//apmib_get( MIB_NTP_ENABLED, (void *)&time_mode);

	//if(time_mode == 0)
	{		
		apmib_get( MIB_SYSTIME_YEAR, (void *)&cur_year);

		if(cur_year != 0){
			tm_time.tm_year = cur_year - 1900;
			apmib_get( MIB_SYSTIME_MON, (void *)&(tm_time.tm_mon));
			apmib_get( MIB_SYSTIME_DAY, (void *)&(tm_time.tm_mday));
			apmib_get( MIB_SYSTIME_HOUR, (void *)&(tm_time.tm_hour));
			apmib_get( MIB_SYSTIME_MIN, (void *)&(tm_time.tm_min));
			apmib_get( MIB_SYSTIME_SEC, (void *)&(tm_time.tm_sec));
			tm = mktime(&tm_time);
			if(tm < 0){
				fprintf(stderr, "make Time Error!\n");
			}
			if(stime(&tm) < 0){
				fprintf(stderr, "set system Time Error\n");
			}
		}
	}

	system("echo done > /var/system/set_time");
	return 0;	
}
//////////////////////////////////////////////////////////////////////
static void readFileSetParam(char *file)
{
	FILE *fp;
	char line[200], token[40], value[100], *ptr;
	int idx, hw_setting_found=0, ds_setting_found=0, cs_setting_found=0;
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		int bluetooth_hw_setting_found=0;
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
	int customer_hw_setting_found=0;
#endif
	char *arrayval[2];
	mib_table_entry_T *pTbl;

	
	if ( !apmib_init()) {
		printf("Initialize AP MIB failed!\n");
		return;
	}

	fp = fopen(file, "r");
	if (fp == NULL) {
		printf("read file [%s] failed!\n", file);
		return;
	}

	arrayval[0] = value;

	while ( fgets(line, 200, fp) ) {
		ptr = get_token(line, token);
		if (ptr == NULL)
			continue;
		if (get_value(ptr, value)==0)
			continue;

		idx = searchMIB(token);
		if ( idx == -1 ) {
			printf("invalid param [%s]!\n", token);
			return;
		}
		if (config_area == HW_MIB_AREA || config_area == HW_MIB_WLAN_AREA) {
			hw_setting_found = 1;
			if (config_area == HW_MIB_AREA)
				pTbl = hwmib_table;
			else
				pTbl = hwmib_wlan_table;
		}
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
				else  if (config_area == BLUETOOTH_HW_AREA) {
					bluetooth_hw_setting_found = 1;			
					pTbl = bluetooth_hwmib_table;
				}
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		else  if (config_area == CUSTOMER_HW_AREA) {
			customer_hw_setting_found = 1;			
			pTbl = customer_hwmib_table;
		}
#endif
		else if (config_area == DEF_MIB_AREA || config_area == DEF_MIB_WLAN_AREA) {
			ds_setting_found = 1;
			if (config_area == DEF_MIB_AREA)
				pTbl = mib_table;
			else
				pTbl = mib_wlan_table;
		}
		else {
			cs_setting_found = 1;
			if (config_area == MIB_AREA)
				pTbl = mib_table;
			else
				pTbl = mib_wlan_table;
		}
		config_area = 0;
		setMIB(token, pTbl[idx].id, pTbl[idx].type, pTbl[idx].size, 1, arrayval);
	}
	fclose(fp);

	if (hw_setting_found)
		apmib_update(HW_SETTING);
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		if (bluetooth_hw_setting_found)
			apmib_update(BLUETOOTH_HW_SETTING);
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
	if (customer_hw_setting_found)
		apmib_update(CUSTOMER_HW_SETTING);
#endif
	if (ds_setting_found)
		apmib_update(DEFAULT_SETTING);
	if (cs_setting_found)
		apmib_update(CURRENT_SETTING);
}

#if defined(SET_MIB_FROM_FILE)
static void readFileSetMib(char *file)
{
	FILE *fp;
	char line[200], token[50], value[100], *ptr;
	int idx, hw_setting_found=0, ds_setting_found=0, cs_setting_found=0;
	char *arrayval[100];
	char temp[100][50];
	int para_num=0;
	mib_table_entry_T *pTbl;
	int wlan_idx_bak, vwlan_idx_bak;
	char cmd[100];
	int tbl_idx=1;
	int tbl_num = 0;
	char tbl_name[50];
	strcpy(tbl_name,"NONE");
	
	wlan_idx_bak = wlan_idx;
	vwlan_idx_bak = vwlan_idx;
		
	if ( !apmib_init()) {
		printf("Initialize AP MIB failed!\n");
		return;
	}

	fp = fopen(file, "r");
	if (fp == NULL) {
		printf("read file [%s] failed!\n", file);
		return;
	}
	set_mib_from_file = 1;
	while ( fgets(line, 200, fp) ) {
		if(line[0] == ';' || line[0] == '#')
			continue;
		if(ptr=strchr(line, ';'))
		{
			int i;
			for(i=1;i<ptr-line-1;i++)
			{
				if(*(ptr-i) != '\ ' && *(ptr-i) != '\t')
					break;
			}
			if(i != ptr-line-1)
				*(ptr-i+1) = '\0';
			else
				continue;
		}
		ptr = get_token(line, token);
		if (ptr == NULL)
			continue;
		if (get_value(ptr, value)==0)
			continue;

		if(strstr(token, "SSID")
				|| !strcmp(token , "NTP_TIMEZONE") 
#if defined(CONFIG_APP_SIMPLE_CONFIG)
				|| !strcmp(token , "SC_DEVICE_NAME")
#endif
		)
		{
			int ii,tmp_idx=0;
			for(ii=0;ii<strlen(value);ii++)
			{
				if(*(value+ii) != '\\')
					temp[para_num][tmp_idx++] = *(value+ii);
			}
			temp[para_num][tmp_idx] = '\0';
			arrayval[para_num] = temp[para_num];
			para_num++;
		}
		else if(strstr(token,"L2TP_PAYLOAD") && !strstr(token,"L2TP_PAYLOAD_NUM"))
		{//change L2TP_PAYLOAD from hex to string			
			sprintf(temp[para_num],"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
					value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7],
					value[8], value[9], value[10], value[11], value[12], value[13], value[14], value[15],
					value[16], value[17], value[18], value[19], value[20], value[21], value[22], value[23],
					value[24], value[25], value[26], value[27], value[28], value[29], value[30], value[31],
					value[32], value[33], value[34], value[35], value[36], value[37], value[38], value[39], 
					value[40], value[41], value[42], value[43], value[44], value[45], value[46], value[47], 
					value[48], value[49], value[50], value[51], value[52], value[53]);
			arrayval[para_num] = temp[para_num];
			para_num++;
		}
		else
		{
			char *p; 
			p = strtok(value, " ");
			while(p)
			{
				
				sscanf(p, "%s", temp[para_num]);
				arrayval[para_num] = temp[para_num];
				para_num++;
				p = strtok(NULL, " "); 
			}
			
		}
		arrayval[para_num] = NULL;

		//analysis TBL mib
		if(strstr(token,"_TBL_NUM")) 
		{
			if(!strstr(line,"_TBL_NUM=0"))
			{
				tbl_num = 0;
				strcpy(tbl_name,token);
				char *ptmp;
				ptmp = strstr(tbl_name,"TBL");
				*(ptmp+3) = '\0';
			}
		}
		else if(strstr(token,"_TBL"))
		{
			if(strstr(token,tbl_name))
			{
				char *p; 
				char tmp_ch[10];

				//delete the x of TBLx, flash set xxx delall
				strcpy(token,tbl_name);
				if(0 == tbl_num)
				{
					if(!strstr(token,"SCHEDULE_TBL"))
					{
						sprintf(cmd,"flash set %s delall",tbl_name);
						system(cmd);
					}
				}
				tbl_num++;
				
				//delete','
				para_num = 0;
				p = strtok(value, ",");
				while(p)
				{
					sscanf(p, "%s", temp[para_num]);
					arrayval[para_num] = temp[para_num];
					para_num++;
					p = strtok(NULL, ","); 
				}
				
				//add "add"
				int i;
				for(i=para_num;i>0;i--)
					strcpy(temp[i],temp[i-1]);
				strcpy(temp[0],"add");
				arrayval[para_num] = temp[para_num];
				para_num++;
				arrayval[para_num] = NULL;

				//special MIB
				if(strstr(token,"VLANCONFIG_TBL")) 
				{
					strcpy(tmp_ch,temp[1]);
					strcpy(temp[1],temp[2]);
					strcpy(temp[2],tmp_ch);
					strcpy(temp[7],temp[8]);
					para_num--;
					arrayval[para_num] = NULL;
				}
				else if(strstr(token,"SCHEDULE_TBL")) 
				{
					for(i=para_num;i>0;i--)
						strcpy(temp[i],temp[i-1]);
					strcpy(temp[0],"mod");
					sprintf(temp[1],"%d",tbl_num);
					arrayval[para_num] = temp[para_num];
					para_num++;
					arrayval[para_num] = NULL;
				}
			}
			else
			{
				para_num = 0;
				continue;
			}
			//printf("****%s %d: para_num=%d %s %s %s %s %s %s %s %s %s %s\n",__FUNCTION__,__LINE__,para_num,token,arrayval[0],arrayval[1],arrayval[2],arrayval[3],arrayval[4],arrayval[5],arrayval[6],arrayval[7],arrayval[8]);
		}
		
		idx = searchMIB(token);
		if ( idx == -1 ) {
			//printf("invalid param [%s]!\n", token);
			para_num = 0;
			continue;
		}
		if (config_area == HW_MIB_AREA || config_area == HW_MIB_WLAN_AREA) {
			hw_setting_found = 1;
			if (config_area == HW_MIB_AREA)
				pTbl = hwmib_table;
			else
				pTbl = hwmib_wlan_table;
		}
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		else if (config_area == BLUETOOTH_HW_AREA ) {
				pTbl = bluetooth_hwmib_table;
		}
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		else if (config_area == CUSTOMER_HW_AREA ) {
				pTbl = customer_hwmib_table;
		}
#endif
		else if (config_area == DEF_MIB_AREA || config_area == DEF_MIB_WLAN_AREA) {
			
			ds_setting_found = 1;
			if (config_area == DEF_MIB_AREA)
				pTbl = mib_table;
			else
				pTbl = mib_wlan_table;
		}
		else {
			
			cs_setting_found = 1;
			if (config_area == MIB_AREA)
				pTbl = mib_table;
			else
				pTbl = mib_wlan_table;
		}
		
		if(strstr(token, "WLAN0"))
			wlan_idx = 0;
		else if(strstr(token, "WLAN1"))
			wlan_idx = 1;
		else
			wlan_idx = 0;
		
		if(strstr(token, "VAP"))
		{
			vwlan_idx = atoi(&token[9])+1;
		}
		else if(strstr(token, "VXD"))
		{
			vwlan_idx = 5;
		}
		else
			vwlan_idx = 0;
		
		setMIB(token, pTbl[idx].id, pTbl[idx].type, pTbl[idx].size, para_num, arrayval);
		para_num = 0;
	}
	printf("setMIB end...\n");
	fclose(fp);
	
	if (hw_setting_found)
	{
		apmib_update(HW_SETTING);
	}
	if (ds_setting_found)
	{
		apmib_update(DEFAULT_SETTING);
	}
	if (cs_setting_found)
	{
		apmib_update(CURRENT_SETTING);
	}
	
	wlan_idx = wlan_idx_bak;
	vwlan_idx = vwlan_idx_bak;
	set_mib_from_file = 0;
}
#endif
//////////////////////////////////////////////////////////////////////
static int resetDefault(void)
{
	char *defMib;
	int fh;
    PARAM_HEADER_T header;
    unsigned char checksum;
    char *data;
    int offset;
    //int status;
#ifdef COMPRESS_MIB_SETTING
	//unsigned char* pContent = NULL;

	//COMPRESS_MIB_HEADER_T compHeader;
	//unsigned char *expPtr, *compPtr;
	//unsigned int expLen = header.len+sizeof(PARAM_HEADER_T);
	//unsigned int compLen;
	//unsigned int real_size = 0;
	//int zipRate=0;
#endif // #ifdef COMPRESS_MIB_SETTING

#ifdef _LITTLE_ENDIAN_
	char *pdata;
#endif

#ifdef MIB_TLV
	unsigned char *pfile = NULL;
	unsigned char *mib_tlv_data = NULL;
	unsigned int tlv_content_len = 0;
	unsigned int mib_tlv_max_len = 0;
#endif

#ifdef CONFIG_MTD_NAND
	if (offset == CURRENT_SETTING_OFFSET)
	{
		fh = open("/config/current_setting.bin", O_RDWR | O_CREAT);
	}
	else
	{
		fh = open(FLASH_DEVICE_SETTING, O_RDWR | O_CREAT);
	}
#else
	fh = open(FLASH_DEVICE_NAME, O_RDWR);
#endif

#if CONFIG_APMIB_SHARED_MEMORY == 1	
    apmib_sem_lock();
#endif

	if ((defMib=apmib_dsconf()) == NULL) {
		printf("Default configuration invalid!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1	
        apmib_sem_unlock();
#endif
		return -1;
	}
#ifdef _LITTLE_ENDIAN_
	pdata=malloc(sizeof(APMIB_T));
	if(pdata)
	{
		memcpy(pdata,defMib,sizeof(APMIB_T));
		data=pdata;
	}
	else{
		printf("malloc memory failed\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(defMib, DSCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(defMib);
#endif
		
		return -1;
	}
	swap_mib_value(data,DEFAULT_SETTING);
#else
	data=defMib;
#endif

#ifdef MIB_TLV

//mib_display_data_content(DEFAULT_SETTING, pMib, sizeof(APMIB_T));	

	mib_tlv_max_len = mib_get_setting_len(DEFAULT_SETTING)*4;
	tlv_content_len = 0;
	pfile = malloc(mib_tlv_max_len);
	memset(pfile, 0x00, mib_tlv_max_len);
	
	if( pfile != NULL && mib_tlv_save(DEFAULT_SETTING, (void*)data, pfile, &tlv_content_len) == 1)
	{
		mib_tlv_data = malloc(tlv_content_len+1); // 1:checksum
		if(mib_tlv_data != NULL)
		{
			memcpy(mib_tlv_data, pfile, tlv_content_len);
		}			
		free(pfile);
	}
	
	if(mib_tlv_data != NULL)
	{
		sprintf((char *)header.signature, "%s%02d", DEFAULT_SETTING_HEADER_TAG, DEFAULT_SETTING_VER);
		header.len = tlv_content_len+1;
		data = (char *)mib_tlv_data;
		checksum = CHECKSUM((unsigned char *)data, header.len-1);
		data[tlv_content_len] = CHECKSUM((unsigned char *)data, tlv_content_len);
		//mib_display_tlv_content(DEFAULT_SETTING, data, header.len);		
	}
	else
	{
#endif //#ifdef MIB_TLV
	data = (char *)defMib;	
	sprintf((char *)header.signature, "%s%02d", DEFAULT_SETTING_HEADER_TAG, DEFAULT_SETTING_VER);
	header.len = sizeof(APMIB_T) + sizeof(checksum);
	checksum = CHECKSUM((unsigned char *)data, header.len-1);

#ifdef MIB_TLV
	}
#endif	

#ifdef MIB_TLV
	if(mib_tlv_data != NULL)
	{
		
		sprintf((char *)header.signature, "%s%02d", CURRENT_SETTING_HEADER_TAG, CURRENT_SETTING_VER);
		header.len = tlv_content_len+1;
		checksum = CHECKSUM((unsigned char *)data, header.len-1);
		//mib_display_tlv_content(CURRENT_SETTING, data, header.len);		
	}
	else
	{
#endif
	sprintf((char *)header.signature, "%s%02d", CURRENT_SETTING_HEADER_TAG, CURRENT_SETTING_VER);
	header.len = sizeof(APMIB_T) + sizeof(checksum);

#ifdef MIB_TLV
	}
#endif

	lseek(fh, CURRENT_SETTING_OFFSET, SEEK_SET);
#ifdef COMPRESS_MIB_SETTING
	if(flash_mib_compress_write(CURRENT_SETTING, data, &header, &checksum) == 1)
	{
		COMP_TRACE(stderr,"\r\n flash_mib_compress_write CURRENT_SETTING DONE, __[%s-%u]", __FILE__,__LINE__);			
	}
	else
	{
#endif //#ifdef COMPRESS_MIB_SETTING
		if ( write(fh, (const void *)&dsHeader, sizeof(dsHeader))!=sizeof(dsHeader) ) {
			printf("write cs header failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		    apmib_shm_free(defMib, DSCONF_SHM_KEY);
		    apmib_sem_unlock();
#else
			free(defMib);
#endif
#ifdef _LITTLE_ENDIAN_
			free(pdata);
#endif
			return -1;
		}
		if ( write(fh, (const void *)defMib, dsHeader.len) != dsHeader.len ) {
			printf("write cs MIB failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		    apmib_shm_free(defMib, DSCONF_SHM_KEY);
		    apmib_sem_unlock();
#else
			free(defMib);
#endif
#ifdef _LITTLE_ENDIAN_
			free(pdata);
#endif
			return -1;
		}
#ifdef COMPRESS_MIB_SETTING	
	}
#endif
	close(fh);
	sync();

	offset = CURRENT_SETTING_OFFSET;
#ifdef COMPRESS_MIB_SETTING
	if(flash_mib_checksum_ok(offset) == 1)
	{
		COMP_TRACE(stderr,"\r\n CURRENT_SETTING hecksum_ok\n");
	}
#endif


#if CONFIG_APMIB_SHARED_MEMORY == 1
    apmib_load_csconf(); //read current settings diectly from flash 
	apmib_shm_free(defMib, DSCONF_SHM_KEY);
	apmib_sem_unlock();
#else
	free(defMib);
#endif
#ifdef _LITTLE_ENDIAN_
	free(pdata);
#endif

	return 0;
}


#if defined(CONFIG_APP_TR069)		
int save_cwmpnotifylist()
{
	
	
	FILE *fp;

	fp = fopen( "/var/CWMPNotify.txt", "r" );
	if(fp)
	{
		char line[200];
		unsigned char *notifyList;
		int notifyListLen = CWMP_NOTIFY_LIST_LEN+28; //28:"flash set CWMP_NOTIFY_LIST "
		notifyList=malloc(notifyListLen); 
		
		if(notifyList==NULL)
		{
			fprintf(stderr,"\r\n ERR:notifyList malloc fail! __[%s-%u]",__FILE__,__LINE__);
		}
		else
		{
			char insertStr[200];
			
			memset(insertStr, 0x00, sizeof(insertStr));
				
			memset(notifyList,0x00,notifyListLen);
			sprintf(notifyList,"%s","flash set CWMP_NOTIFY_LIST ");

			while ( fgets(line, 200, fp) )
			{
				
				memset(insertStr, 0x00, sizeof(insertStr));
				
				if(strlen(line) != 0)
				{
					char *strptr = line; // A1 A2 A3
					char *str1,*str2,*str3;
					
					str1 = strsep(&strptr," ");

					str2 = strsep(&strptr," ");
	
					str3 = strsep(&strptr,"\n");

					
					if(str1 == NULL || str2 == NULL || str3 == NULL)
						;
					else
						sprintf(insertStr,"%s]%s]%s[",str1,str2,str3);

				}				
//printf("\r\n insertStr=[%s],__[%s-%u]\r\n",insertStr,__FILE__,__LINE__);

				if(strlen(notifyList)+strlen(insertStr) >= notifyListLen)					
				{
					printf("\r\n Overflow notifyListLen length [%s],__[%s-%u]\r\n",notifyListLen,__FILE__,__LINE__);					
					break;
				}
				strcat(notifyList, insertStr);
			}
			
			if(strlen(insertStr) != 0) //no any notify date need to be saved
					system(notifyList);								

			
			if(notifyList)
				free(notifyList);
		}
		
		fclose(fp);
	}
	
	
	return 0;
}
#endif


#if defined(CONFIG_RTL_8812_SUPPORT)

#define B1_G1	40
#define B1_G2	48

#define B2_G1	56
#define B2_G2	64

#define B3_G1	104
#define B3_G2	112
#define B3_G3	120
#define B3_G4	128
#define B3_G5	136
#define B3_G6	144

#define B4_G1	153
#define B4_G2	161
#define B4_G3	169
#define B4_G4	177

void assign_diff_AC(unsigned char* pMib, unsigned char* pVal)
{
	int x=0, y=0;

	memset((pMib+35), pVal[0], (B1_G1-35));
	memset((pMib+B1_G1), pVal[1], (B1_G2-B1_G1));
	memset((pMib+B1_G2), pVal[2], (B2_G1-B1_G2));
	memset((pMib+B2_G1), pVal[3], (B2_G2-B2_G1));
	memset((pMib+B2_G2), pVal[4], (B3_G1-B2_G2));
	memset((pMib+B3_G1), pVal[5], (B3_G2-B3_G1));
	memset((pMib+B3_G2), pVal[6], (B3_G3-B3_G2));
	memset((pMib+B3_G3), pVal[7], (B3_G4-B3_G3));
	memset((pMib+B3_G4), pVal[8], (B3_G5-B3_G4));
	memset((pMib+B3_G5), pVal[9], (B3_G6-B3_G5));
	memset((pMib+B3_G6), pVal[10], (B4_G1-B3_G6));
	memset((pMib+B4_G1), pVal[11], (B4_G2-B4_G1));
	memset((pMib+B4_G2), pVal[12], (B4_G3-B4_G2));
	memset((pMib+B4_G3), pVal[13], (B4_G4-B4_G3));

}
#endif
#ifdef TR181_SUPPORT
//#ifdef CONFIG_IPV6
void handle_tr181_get_ipv6(char*inputName)
{
	char buff[256]={0};
	int retVal=0;

	if(!inputName||!inputName[0])
	{
		printf("invalid input!\n");
		return;
	}
	if ( !apmib_init()) {
		printf("Initialize AP MIB failed!\n");
		return -1;
	}
	retVal=tr181_ipv6_get(inputName,(void*)buff);
	switch(retVal)
	{
		case 1:
			printf("%d\n",*(int*)buff);
			break;
		case 2:
			printf("%s\n",buff);
			break;
#ifdef CONFIG_IPV6
		case 3:
			{
				struct duid_t dhcp6c_duid=*((struct duid_t*)buff);				
				printf("%04x%04x%02x%02x%02x%02x%02x%02x\n",dhcp6c_duid.duid_type,dhcp6c_duid.hw_type,
					dhcp6c_duid.mac[0],dhcp6c_duid.mac[1],dhcp6c_duid.mac[2],dhcp6c_duid.mac[3],
					dhcp6c_duid.mac[4],dhcp6c_duid.mac[5]);
				break;
			}
#endif
		default:
			break;
	}
}
void handle_tr181_set_ipv6(char*inputName,char*value)
{
	char buff[256]={0};
	int retVal=0,intValue=0,setValRet=0;
	if(!inputName||!inputName[0]||!value||!value[0])
	{
		printf("invalid input!\n");
		return;
	}
	if ( !apmib_init()) {
		printf("Initialize AP MIB failed!\n");
		return -1;
	}

	retVal=tr181_ipv6_get(inputName,(void*)buff);
	switch(retVal)
	{
		case 1:
			intValue=atoi(value);
			setValRet=tr181_ipv6_set(inputName,(void*)&intValue);			
			break;
		case 2:
			strcpy(buff,value);
			setValRet=tr181_ipv6_set(inputName,(void*)buff);
			break;		
		default:
			return;
	}
	switch(setValRet)
	{
		case 1:
			printf("OK!\n");
			break;
		case 2:
			printf("need reinit!\n");
			break;
		case 3:
			printf("need reboot!\n");
			break;
		default:
			printf("fail!\n");
			break;
	}
	apmib_update(CURRENT_SETTING);
}
//#endif
#endif
static int updateConfigIntoFlash(unsigned char *data, int total_len, int *pType, int *pStatus);
//////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------
#ifdef SYS_DIAGNOSTIC
#define SYS_DIAGNOSTIC_STATUS "/tmp/diagnostic_status"
#define SYS_DIAGNOSTIC_STATUS_DOING "doing"
#define SYS_DIAGNOSTIC_STATUS_DONE "done"
#define SYS_DIAGNOSTIC_STATUS_INVALID_ARG "invalid_arg"
void rtk_dump_log_command(char * command)
{
	char buff[256]={0};
	if(command)
	{
		sprintf(buff,"date >> %s 2>/dev/null",DUMP_DIAG_LOG_FILE);
		system(buff);
		sprintf(buff,"echo \"%s:\" >> %s 2>/dev/null",command,DUMP_DIAG_LOG_FILE);
		system(buff);
		sprintf(buff,"%s >> %s 2>&1",command,DUMP_DIAG_LOG_FILE);
		system(buff);
	}
}
static int re865xIoctl(char *name, unsigned int arg0, unsigned int arg1, unsigned int arg2, unsigned int arg3)
{
	unsigned int args[4];
	struct ifreq ifr;
	int sockfd;

	args[0] = arg0;
	args[1] = arg1;
	args[2] = arg2;
	args[3] = arg3;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("fatal error socket\n");
		return -3;
	}

	strcpy((char*)&ifr.ifr_name, name);
	((unsigned int *)(&ifr.ifr_data))[0] = (unsigned int)args;

	if (ioctl(sockfd, SIOCDEVPRIVATE, &ifr)<0)
	{
		perror("device ioctl:");
		close(sockfd);
		return -1;
	}
	close(sockfd);
	return 0;
} /* end re865xIoctl */

int getWanLink(char *interface)
{
	unsigned int    ret;
	unsigned int    args[0];

	re865xIoctl(interface, RTL8651_IOCTL_GETWANLINKSTATUS, (unsigned int)(args), 0, (unsigned int)&ret) ;
	return ret;
}

#ifdef CONFIG_SMART_REPEATER
int getWispRptIface(char**pIface,int wlanId)
{
	int rptEnabled=0,wlanMode=0,opMode=0;
	char wlan_wanIfName[16]={0};
	if(wlanId == 0)
		apmib_get(MIB_REPEATER_ENABLED1, (void *)&rptEnabled);
	else if(1 == wlanId)
		apmib_get(MIB_REPEATER_ENABLED2, (void *)&rptEnabled);
#if defined(CONFIG_RTL_TRI_BAND)
	else if(2 == wlanId)
		apmib_get(MIB_REPEATER_ENABLED3, (void *)&rptEnabled);
#endif	
	else 	
		return -1;
	apmib_get(MIB_OP_MODE,(void *)&opMode);
	if(opMode!=WISP_MODE)
		return -1;
	apmib_save_wlanIdx();
	
	sprintf(wlan_wanIfName,"wlan%d",wlanId);
	SetWlan_idx(wlan_wanIfName);
	//for wisp rpt mode,only care root ap
	apmib_get(MIB_WLAN_MODE, (void *)&wlanMode);
	if((AP_MODE==wlanMode || AP_MESH_MODE==wlanMode || MESH_MODE==wlanMode) && rptEnabled)
	{
		if(wlanId == 0)
			*pIface = "wlan0-vxd";
		else if(1 == wlanId)
			*pIface = "wlan1-vxd";
		else return -1;
	}else
	{
		char * ptmp = strstr(*pIface,"-vxd");
		if(ptmp)
			memset(ptmp,0,sizeof(char)*strlen("-vxd"));
	}
	apmib_recov_wlanIdx();
	return 0;
}
#endif
int rtk_getInAddr( char *interface, int type, void *pAddr )
{
    struct ifreq ifr;
    int skfd, found=0;
	struct sockaddr_in *addr;
    skfd = socket(AF_INET, SOCK_DGRAM, 0);

    strcpy(ifr.ifr_name, interface);
    if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0){
    		close( skfd );
		return (0);
	}
    if (type == HW_ADDR_T) {
    	if (ioctl(skfd, SIOCGIFHWADDR, &ifr) >= 0) {
		memcpy(pAddr, &ifr.ifr_hwaddr, sizeof(struct sockaddr));
		found = 1;
	}
    }
    else if (type == IP_ADDR_T) {
	if (ioctl(skfd, SIOCGIFADDR, &ifr) == 0) {
		addr = ((struct sockaddr_in *)&ifr.ifr_addr);
		*((struct in_addr *)pAddr) = *((struct in_addr *)&addr->sin_addr);
		found = 1;
	}
    }
    else if (type == NET_MASK_T) {
	if (ioctl(skfd, SIOCGIFNETMASK, &ifr) >= 0) {
		addr = ((struct sockaddr_in *)&ifr.ifr_addr);
		*((struct in_addr *)pAddr) = *((struct in_addr *)&addr->sin_addr);
		found = 1;
	}
    }else {
    	
    	if (ioctl(skfd, SIOCGIFDSTADDR, &ifr) >= 0) {
		addr = ((struct sockaddr_in *)&ifr.ifr_addr);
		*((struct in_addr *)pAddr) = *((struct in_addr *)&addr->sin_addr);
		found = 1;
	}
    	
    }
    close( skfd );
    return found;

}

static int getPid(char *filename)
{
	struct stat status;
	char buff[100];
	FILE *fp;

	if ( stat(filename, &status) < 0)
		return -1;
	fp = fopen(filename, "r");
	if (!fp) {
        fprintf(stderr, "Read pid file error!\n");
		return -1;
   	}
	fgets(buff, 100, fp);
	fclose(fp);

	return (atoi(buff));
}

int isDhcpClientExist(char *name)
{
	char tmpBuf[100];
	struct in_addr intaddr;

	if ( rtk_getInAddr(name, IP_ADDR, (void *)&intaddr ) ) {
		snprintf(tmpBuf, 100, "%s/%s-%s.pid", _DHCPC_PID_PATH, _DHCPC_PROG_NAME, name);
		if ( getPid(tmpBuf) > 0)
			return 1;
	}
	return 0;
}


int getInterfaces(char* lanIface,char* wanIface)
{
	int opmode=-1,wisp_wanid=0, wlan_idx_bak,vwlan_idx_bak;
	DHCP_T dhcp;
	
	if(!apmib_get( MIB_WAN_DHCP, (void *)&dhcp))
		return -1;
	if(!apmib_get(MIB_OP_MODE, (void *)&opmode))
		return -1;
	if(!lanIface||!wanIface)
	{
		fprintf(stderr,"invalid input!!\n");
		return -1;
	}
	strcpy(lanIface,"br0");
	switch(dhcp)
	{
	case DHCP_DISABLED:
	case DHCP_CLIENT:
	case DHCP_SERVER:        
		if(opmode==WISP_MODE)
		{
//#ifdef CONFIG_RTL_DUAL_PCIESLOT_BIWLAN
#ifdef CONFIG_RTL_DUAL_PCIESLOT_BIWLAN_D
			apmib_get(MIB_WISP_WAN_ID,(void*)&wisp_wanid);
#endif
			//printf("%s %d wisp_wanid=%d\n", __FUNCTION__, __LINE__, wisp_wanid);
#if defined(CONFIG_SMART_REPEATER)				
			int wlan_mode,i=0;	
			wlan_idx_bak=wlan_idx;
			vwlan_idx_bak=vwlan_idx;
			//apmib_save_idx();
			for(i=0;i<NUM_WLAN_INTERFACE;i++)
			{
				//apmib_set_wlanidx(i);
				wlan_idx=i;
				apmib_get(MIB_WLAN_MODE, (void *)&wlan_mode);
				if(wlan_mode == CLIENT_MODE)
				{
					if(i==wisp_wanid)
						sprintf(wanIface, "wlan%d",i);													
				}else
				{
					if(i==wisp_wanid)
						sprintf(wanIface, "wlan%d-vxd",i);
				}
			}
			//apmib_revert_idx();	
			wlan_idx=wlan_idx_bak;
			vwlan_idx=vwlan_idx_bak;
			
#else
			if(wisp_wanid==0)
				strcpy(wanIface,"wlan0");
			else
				strcpy(wanIface,"wlan1");
#endif
		}
		else
			strcpy(wanIface,"eth1");
		break;
	case PPPOE:
	case L2TP:
	case PPTP:
		strcpy(wanIface,"ppp0");
		break;
	}
	return (0);
}

static inline int iw_get_ext(int skfd, char *ifname, int request, struct iwreq *pwrq)
{
	/* Set device name */
	strncpy(pwrq->ifr_name, ifname, IFNAMSIZ);
	/* Do the request */
	return(ioctl(skfd, request, pwrq));
}

int getWlBssInfo(char *interface, RTK_BSS_INFOp pInfo)
{
#ifndef NO_ACTION
	int skfd=0;
	struct iwreq wrq;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;
	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0)
	/* If no wireless name : no wireless extensions */
	{
		close( skfd );
		return -1;
	}

	wrq.u.data.pointer = (caddr_t)pInfo;
	wrq.u.data.length = sizeof(RTK_BSS_INFO);

	if (iw_get_ext(skfd, interface, SIOCGIWRTLGETBSSINFO, &wrq) < 0)
	{
		close( skfd );
		return -1;
	}
	close( skfd );
#else
	memset(pInfo, 0, sizeof(RTK_BSS_INFO)); 
#endif

	return 0;
}

int isConnectPPP()
{
	struct stat status;

	if ( stat("/etc/ppp/link", &status) < 0)
		return 0;

	return 1;
}


int getDefaultRoute(char *interface, struct in_addr *route)
{
	char buff[1024], iface[16];
	char gate_addr[128], net_addr[128], mask_addr[128];
	int num, iflags, metric, refcnt, use, mss, window, irtt;
	FILE *fp = fopen(_PATH_PROCNET_ROUTE, "r");
	char *fmt;
	int found=0;
	unsigned long addr;

	if (!fp) {
		printf("Open %s file error.\n", _PATH_PROCNET_ROUTE);
		return 0;
    }

	fmt = "%16s %128s %128s %X %d %d %d %128s %d %d %d";

	while (fgets(buff, 1023, fp)) {
		num = sscanf(buff, fmt, iface, net_addr, gate_addr,
		     		&iflags, &refcnt, &use, &metric, mask_addr, &mss, &window, &irtt);
		if (num < 10 || !(iflags & RTF_UP) || !(iflags & RTF_GATEWAY) || strcmp(iface, interface))
	    		continue;
		sscanf(gate_addr, "%lx", &addr );
		*route = *((struct in_addr *)&addr);

		found = 1;
		break;
	}

    	fclose(fp);
    	return found;
}

int rtk_get_wan_info(RTK_WAN_INFOp info)
{
	if(info == NULL)
		return RTK_FAILED;
	
	memset(info, 0, sizeof(RTK_WAN_INFO));

	DHCP_T dhcp;
	OPMODE_T opmode=-1;
	unsigned int wispWanId=0;
	char *iface=NULL;
	struct in_addr	intaddr;
	struct sockaddr hwaddr;
	unsigned char *pMacAddr;
	int isWanPhyLink = 0;
	RTK_BSS_INFO bss;
	FILE *fp;
	char str[32];
	
	if(!apmib_get(MIB_WAN_DHCP, (void *)&dhcp))
		return RTK_FAILED;

	if(!apmib_get(MIB_OP_MODE, (void *)&opmode) )
		return RTK_FAILED;

	if(!apmib_get(MIB_WISP_WAN_ID, (void *)&wispWanId))
		return RTK_FAILED;

	if(opmode == BRIDGE_MODE)
	{
		return RTK_FAILED;
	}

	if(opmode != WISP_MODE)
	{
 		isWanPhyLink=getWanLink("eth1");
 	}

	// get status
	if ( dhcp == DHCP_CLIENT)
	{
		if(opmode == WISP_MODE)
		{
			if(0 == wispWanId)
				iface = "wlan0";
			else if(1 == wispWanId)
				iface = "wlan1";
#ifdef CONFIG_SMART_REPEATER
			if(getWispRptIface(&iface,wispWanId)<0)
				return RTK_FAILED;
#endif
		}
		else
			iface = "eth1";
	 	if (!isDhcpClientExist(iface))
			info->status = GETTING_IP_FROM_DHCP_SERVER;
		else{
			if(isWanPhyLink < 0)
				info->status = GETTING_IP_FROM_DHCP_SERVER;
			else
				info->status = DHCP;
		}
	}
	else if ( dhcp == DHCP_DISABLED )
	{
		if (opmode == WISP_MODE)
		{
			char wan_intf[MAX_NAME_LEN] = {0};
			char lan_intf[MAX_NAME_LEN] = {0};
			
			getInterfaces(lan_intf,wan_intf);
			memset(&bss, 0x00, sizeof(bss));
			getWlBssInfo(wan_intf, &bss);
			if (bss.state == RTK_STATE_CONNECTED){
				info->status = FIXED_IP_CONNECTED;
			}
			else
			{
				info->status = FIXED_IP_DISCONNECTED;
			}				
		}
		else
		{
			if(isWanPhyLink < 0)
				info->status = FIXED_IP_DISCONNECTED;
			else
				info->status = FIXED_IP_CONNECTED;
		}
	}
	else if ( dhcp ==  PPPOE )
	{
		if ( isConnectPPP())
		{
			if(isWanPhyLink < 0)
				info->status = PPPOE_DISCONNECTED;
			else
				info->status = PPPOE_CONNECTED;
		}else
			info->status = PPPOE_DISCONNECTED;
	}
	else if ( dhcp ==  PPTP )
	{
		if ( isConnectPPP()){
			if(isWanPhyLink < 0)
				info->status = PPTP_DISCONNECTED;
			else
				info->status = PPTP_CONNECTED;
		}else
			info->status = PPTP_DISCONNECTED;
	}
	else if ( dhcp ==  L2TP )
	{
		if ( isConnectPPP()){
			if(isWanPhyLink < 0)
				info->status = L2TP_DISCONNECED;
			else
				info->status = L2TP_CONNECTED;
		}else
			info->status = L2TP_DISCONNECED;
	}
#ifdef RTK_USB3G
	else if ( dhcp == USB3G ) {
		int inserted = 0;
		char str[32];

		if (isConnectPPP()){
			info->status = USB3G_CONNECTED;
		}else {
			int retry = 0;

OPEN_3GSTAT_AGAIN:
			fp = fopen("/var/usb3g.stat", "r");

			if (fp !=NULL) {
				fgets(str, sizeof(str),fp);
				fclose(fp);
			}
			else if (retry < 5) {
				retry++;
				goto OPEN_3GSTAT_AGAIN;
			}

			if (str != NULL && strstr(str, "init")) {
				info->status = USB3G_MODEM_INIT;
			}
			else if (str != NULL && strstr(str, "dial")) {
				info->status = USB3G_DAILING;
			}
			else if (str != NULL && strstr(str, "remove")) {
				info->status = USB3G_REMOVED;
			}
			else
				info->status = USB3G_DISCONNECTED;
		}
    }
#endif /* #ifdef RTK_USB3G */

	// get ip, mask, default gateway

	if ( dhcp == PPPOE || dhcp == PPTP || dhcp == L2TP || dhcp == USB3G )
	{
		iface = "ppp0";
		if ( !isConnectPPP() )
			iface = NULL;
	}
	else if (opmode == WISP_MODE){
		if(0 == wispWanId)
			iface = "wlan0";
		else if(1 == wispWanId)
			iface = "wlan1";
#ifdef CONFIG_SMART_REPEATER
		if(getWispRptIface(&iface,wispWanId)<0)
			return RTK_FAILED;
#endif			
	}
	else
		iface = "eth1";

	if(opmode != WISP_MODE)
	{
		if(iface){
			if((isWanPhyLink = getWanLink("eth1")) < 0){
				info->ip.s_addr = 0;				
			}
		}	
	}
	
	intaddr.s_addr = 0;
	if ( iface && rtk_getInAddr(iface, IP_ADDR_T, (void *)&intaddr ) && ((isWanPhyLink >= 0)) )
		info->ip.s_addr = intaddr.s_addr;
	else
		info->ip.s_addr = 0;

	intaddr.s_addr = 0;
	if ( iface && rtk_getInAddr(iface, NET_MASK_T, (void *)&intaddr ) && ((isWanPhyLink >= 0) ))
		info->mask.s_addr = intaddr.s_addr;
	else
		info->mask.s_addr = 0;

	intaddr.s_addr = 0;
	if ( iface && getDefaultRoute(iface, &intaddr) && ((isWanPhyLink >= 0) ))
		info->def_gateway.s_addr = intaddr.s_addr;
	else
		info->def_gateway.s_addr = 0;

	if(opmode == WISP_MODE)
	{
		if(0 == wispWanId)
			iface = "wlan0";
		else if(1 == wispWanId)
			iface = "wlan1";
#ifdef CONFIG_SMART_REPEATER
		if(getWispRptIface(&iface,wispWanId)<0)
			return RTK_FAILED;
#endif			
	}	
	else
		iface = "eth1";
	
	if(rtk_getInAddr(iface, HW_ADDR_T, (void *)&hwaddr))
	{
		pMacAddr = (unsigned char *)hwaddr.sa_data;
		memcpy(info->mac, pMacAddr, 6);
	}
	else
		memset(info->mac, 0, 6);

#ifdef RTK_USB3G
	if (dhcp == USB3G)
		memset(info->mac, 0, 6);
#endif /* #ifdef RTK_USB3G */

	// get dns
	if(isWanPhyLink<0 || info->ip.s_addr == 0)
	{
		info->dns1.s_addr=0;
		info->dns2.s_addr=0;
		info->dns3.s_addr=0;
	}
	else
	{	
		fp = fopen("/var/resolv.conf", "r");

		if (fp ==NULL) {
			return RTK_FAILED;
		}

		fgets(str, sizeof(str),fp);
		inet_aton(str+11, &intaddr);
		info->dns1.s_addr = intaddr.s_addr;
		intaddr.s_addr = 0;
		if(!feof(fp))
		{
			fgets(str, sizeof(str),fp);
			inet_aton(str+11, &intaddr);
			info->dns2.s_addr = intaddr.s_addr;
			intaddr.s_addr = 0;
			if(!feof(fp))
			{
				fgets(str, sizeof(str),fp);
				inet_aton(str+11, &intaddr);
				info->dns3.s_addr = intaddr.s_addr;
			}
			else
				info->dns3.s_addr = 0;
		}
		else
			info->dns2.s_addr = 0;
		
		fclose(fp);
	}
	return RTK_SUCCESS;
}


void rtk_wifi_diagnostic()
{
	int i,j,k,log_count=0;
	char buff[1024];
	char interf[16];
	struct ifreq ifr;
	int skfd;
	int orig_value_838,orig_value_824,orig_value_82c,tmp_orig_value,tmp_addr;
	int orig_value[15];
	
	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	

PRINT_LOG_AGAIN:	
	for(i=0; i<NUM_WLAN_INTERFACE; i++)
	{
		//interface wlan0/wlan1
		sprintf(interf,"wlan%d",i);
		strcpy(ifr.ifr_name, interf);
		if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
			continue;
		if(!(ifr.ifr_flags & IFF_UP))
			continue;

		sprintf(buff,"cat /proc/wlan%d/stats",i);
		rtk_dump_log_command(buff);
		sprintf(buff,"cat /proc/wlan%d/sta_info",i);
		rtk_dump_log_command(buff);
		sprintf(buff,"cat /proc/wlan%d/buf_info",i);
		rtk_dump_log_command(buff);
		sprintf(buff,"cat /proc/wlan%d/desc_info",i);
		rtk_dump_log_command(buff);
		sprintf(buff,"cat /proc/wlan%d/mib_misc",i);
		rtk_dump_log_command(buff);
		if(!log_count)
		{
			sprintf(buff,"cat /proc/wlan%d/sta_dbginfo",i);
			rtk_dump_log_command(buff);
			
			sprintf(buff,"cat /proc/wlan%d/mib_all",i);
			rtk_dump_log_command(buff);
			//DIG AMPDU SIFS RETRY
			sprintf(buff,"echo \"DIG:\" >>%s",DUMP_DIAG_LOG_FILE);
			system(buff);
			
			sprintf(buff,"iwpriv wlan%d read_reg b,c50",i);
			rtk_dump_log_command(buff);
			sprintf(buff,"echo \"AMPDU:\" >>%s",DUMP_DIAG_LOG_FILE);
			system(buff);
			sprintf(buff,"iwpriv wlan%d read_reg dw,4c8",i);
			rtk_dump_log_command(buff);
			sprintf(buff,"echo \"SIFS:\" >>%s",DUMP_DIAG_LOG_FILE);
			system(buff);
			sprintf(buff,"iwpriv wlan%d read_reg b,63f",i);
			rtk_dump_log_command(buff);
			sprintf(buff,"echo \"Retry:\" >>%s",DUMP_DIAG_LOG_FILE);
			system(buff);
			sprintf(buff,"iwpriv wlan%d read_reg w,42a",i);
			rtk_dump_log_command(buff);
			
			//reg_dump/reg_dump all
			sprintf(buff,"cat /proc/wlan%d/reg_dump",i);
			rtk_dump_log_command(buff);
		}
		//interface wlani-vaj
		for(j=0; j<4; j++)
		{	
			sprintf(buff,"wlan%d-va%d",i,j);
			strcpy(ifr.ifr_name, buff);
			if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
				continue;
			if(!(ifr.ifr_flags & IFF_UP))
				continue;
			sprintf(buff,"cat /proc/wlan%d-va%d/stats",i,j);
			rtk_dump_log_command(buff);
			sprintf(buff,"cat /proc/wlan%d-va%d/sta_info",i,j);
			rtk_dump_log_command(buff);
			if(!log_count)
			{
				sprintf(buff,"cat /proc/wlan%d-va%d/sta_dbginfo",i,j);
				rtk_dump_log_command(buff);
				sprintf(buff,"cat /proc/wlan%d-va%d/mib_all",i,j);
				rtk_dump_log_command(buff);		
			}
		}
			
		//interface wlan0-vxd/wlan1-vxd
		sprintf(buff,"wlan%d-vxd",i);
		strcpy(ifr.ifr_name, buff);
		if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
			continue;
		if(!(ifr.ifr_flags & IFF_UP))
			continue;			
		sprintf(buff,"cat /proc/wlan%d-vxd/stats",i);
		rtk_dump_log_command(buff);
		sprintf(buff,"cat /proc/wlan%d-vxd/sta_info",i);
		rtk_dump_log_command(buff);
		if(!log_count)
		{
			sprintf(buff,"cat /proc/wlan%d-vxd/sta_dbginfo",i);
			rtk_dump_log_command(buff);
			sprintf(buff,"cat /proc/wlan%d-vxd/mib_all",i);
			rtk_dump_log_command(buff);
		}
	}		
	
	if(++log_count < 3)
	{
		sleep(1);
		goto PRINT_LOG_AGAIN;
	}
	close(skfd);
}


void format_mac(unsigned char str_mac[6],unsigned char *mac)
{
	int i,j;
	i = 0;
	j = 0;
	while(str_mac[i] != 0)
	{
		if(str_mac[i]>='0' && str_mac[i]<='9')
		{
			mac[j] = mac[j]*16 + str_mac[i] - '0';
		}
		else if(str_mac[i]>='a' && str_mac[i]<='f')
		{
			mac[j] = mac[j]*16 + str_mac[i] - 'a' + 10;
		}
		else if(str_mac[i]>='A' && str_mac[i]<='F')
		{
			mac[j] = mac[j]*16 + str_mac[i] - 'A' + 10;
		}
		else if(str_mac[i] == ':')
		{
			j++;
		}
		i++;
	}
}

/*get arp info from proc/net/arp */
int getArpInfo(struct rtk_link_type_info *info,int *term_number,int entry_number)
{	
	int count =0;
	int term_index = 0 ;
	FILE *fp = NULL;
	unsigned char buffer[128],ip_str[32],mac_str[32],interface_name[12];

	///////// modify by chenbo(realtek)
	struct in_addr br0_ip, br0_netmask, br0_netip;
	//apmib_get(MIB_IP_ADDR, (void *)&br0_ip);
	rtk_getInAddr("br0", IP_ADDR_T, (void *)&br0_ip);	
	//apmib_get(MIB_SUBNET_MASK, (void *)&br0_netmask);
	rtk_getInAddr("br0", NET_MASK_T, (void *)&br0_netmask);
	br0_netip.s_addr=br0_ip.s_addr & br0_netmask.s_addr;
	/////////
	
	//system("cat /proc/net/arp  > /var/tmpResult");
	//if((fp = fopen("/var/tmpResult","r+")) != NULL)
	if((fp = fopen("/proc/net/arp","r+")) != NULL)
	{
		while(fgets(buffer, 128, fp))
		{
			++count;
			if(count ==1)	continue;
			
			sscanf(buffer,"%s %*s %*s %s %*s %s",ip_str,mac_str,interface_name);
			//printf("%s.%d. ip_str(%s)\n",__FUNCTION__,__LINE__,ip_str);
			//printf("%s.%d. mac_str(%s)\n",__FUNCTION__,__LINE__,mac_str);	
			//printf("%s.%d. interface_name(%s)\n",__FUNCTION__,__LINE__,interface_name);
			if(strcmp(interface_name,"br0")==0)
			if(strcmp(mac_str,"00:00:00:00:00:00")!=0)
			{
				inet_aton(ip_str, &(info[term_index].ip));	
				//modify by chenbo(realtek)
				if((info[term_index].ip.s_addr & br0_netmask.s_addr)!=br0_netip.s_addr)
					continue;
				
				format_mac(mac_str,&(info[term_index].mac));
				++term_index;				

				if(term_index >= entry_number)
					break;
				//printf("%s,%d.update success\n",__FUNCTION__,__LINE__);
			}
		}
		fclose(fp);
		if(term_index != 0)
		{
			*term_number = term_index;
			return RTK_SUCCESS;
		}
	}	
	return RTK_FAILED;
}


int getWlStaNum( char *interface, int *num )
{
#ifndef NO_ACTION
	int skfd=0;
	unsigned short staNum;
	struct iwreq wrq;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;
	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0)
	{
	/* If no wireless name : no wireless extensions */
		close( skfd );
		return -1;
	}
	wrq.u.data.pointer = (caddr_t)&staNum;
	wrq.u.data.length = sizeof(staNum);

	if (iw_get_ext(skfd, interface, SIOCGIWRTLSTANUM, &wrq) < 0)
	{
		close( skfd );
		return -1;
	}
	*num  = (int)staNum;

	close( skfd );
#else
	*num = 0 ;
#endif

	return 0;
}


int getWlStaInfo( char *interface,  RTK_WLAN_STA_INFO_Tp pInfo )
{
#ifndef NO_ACTION
    int skfd=0;
    struct iwreq wrq;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;
    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0){
      /* If no wireless name : no wireless extensions */
      close( skfd );
        return -1;
	}
    wrq.u.data.pointer = (caddr_t)pInfo;
    wrq.u.data.length = sizeof(RTK_WLAN_STA_INFO_T) * (MAX_STA_NUM+1);
    memset(pInfo, 0, sizeof(RTK_WLAN_STA_INFO_T) * (MAX_STA_NUM+1));

    if (iw_get_ext(skfd, interface, SIOCGIWRTLSTAINFO, &wrq) < 0){
    	close( skfd );
		return -1;
	}
    close( skfd );
#else
    return -1;
#endif
    return 0;
}

int rtk_get_wlan_sta( unsigned char *ifname,  RTK_WLAN_STA_INFO_Tp pInfo)
{
	if(!ifname || !pInfo)
		return RTK_FAILED;
	if(getWlStaInfo(ifname,pInfo) < 0)
		return RTK_FAILED;
	return RTK_SUCCESS;
}
int equal_mac(unsigned char mac1[6],unsigned char mac2[6])
{
	if((mac1[0]==mac2[0])&& (mac1[1]==mac2[1])&&(mac1[2]==mac2[2])&&
	   (mac1[3]==mac2[3])&& (mac1[4]==mac2[4])&&(mac1[5]==mac2[5]))
	   return RTK_SUCCESS;
	return RTK_FAILED;
}

int check_wifi(char *wifi_interface, unsigned char mac[6],struct rtk_link_type_info *info)
{
	int sta_num = 0 ;
	RTK_WLAN_STA_INFO_Tp pInfo;
	int i;
	char *buff;
	#if 0
	printf("%s.%d. mac(%02x:%02x:%02x:%02x:%02x:%02x)\n",
	__FUNCTION__,__LINE__,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);	
	#endif
	
	if(!getWlStaNum(wifi_interface,&sta_num))
	{
		if(sta_num <= 0)
			return RTK_FAILED;
	}
	else		
		return RTK_FAILED;
	
	buff = malloc(sizeof(RTK_WLAN_STA_INFO_T) * (MAX_STA_NUM+1));
	if(!buff)
		return RTK_FAILED;
	
	memset(buff,0,sizeof(RTK_WLAN_STA_INFO_T) * (MAX_STA_NUM +1));	
	if(rtk_get_wlan_sta(wifi_interface,(RTK_WLAN_STA_INFO_Tp)buff) == RTK_SUCCESS)
	{		
		for(i = 1; i <= sta_num; ++i)
		{
			pInfo = (RTK_WLAN_STA_INFO_Tp)&buff[i*sizeof(RTK_WLAN_STA_INFO_T)];
			if(equal_mac(mac,pInfo->addr))
			{				
				info->cur_rx_bytes = pInfo->rx_bytes;
				info->cur_tx_bytes = pInfo->tx_bytes;
				free(buff);
				return RTK_SUCCESS;
			}
		}
	}	
	free(buff);
	return RTK_FAILED;
}

/* get port status info by proc/rtl865x/asicCounter */
void GetPortStatus(int port_number,struct rtk_link_type_info *info)
{
	/*fill cur_rx /cur_tx parememter */
	FILE *fp=NULL;
	int  line_cnt =0;
	unsigned char buffer[128];
	//system("cat /proc/rtl865x/asicCounter  > /var/tmpResult");	
	
	//if((fp = fopen("/var/tmpResult","r+")) != NULL)
	if((fp = fopen("/proc/rtl865x/asicCounter","r+")) != NULL)
	{
		while(fgets(buffer, 128, fp))
		{
			line_cnt++;
			if(line_cnt == 12*port_number+3)	//update receive bytes
			{
				sscanf(buffer," Rcv %u ",&(info->cur_rx_bytes));
			}
			
			if(line_cnt == 12*port_number+10)	//update send bytes
			{
				sscanf(buffer," Snd %u ",&(info->cur_tx_bytes));
				fclose(fp);
				return ;
			}
		}
	}
	fclose(fp);
}

int check_layer2(unsigned char mac[6],struct rtk_link_type_info *info)
{
	FILE *fp = NULL;
	unsigned char mac_str[32],buffer[128];
	memset(mac_str,0,32);

	#if 0
	printf("%s.%d. mac(%02x:%02x:%02x:%02x:%02x:%02x)\n",
		__FUNCTION__,__LINE__,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	#endif
	
	sprintf(mac_str,"%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	//system("cat /proc/rtl865x/l2 > /var/tmpResult");	

	//if((fp = fopen("/var/tmpResult","r+")) != NULL)
	if((fp = fopen("/proc/rtl865x/l2","r+")) != NULL)
	{	
		while(fgets(buffer, 128, fp))
		{
			if(strstr(buffer,mac_str))
			{
				unsigned char *p;
				p = strstr(buffer,"mbr");
				sscanf(p,"mbr(%d",&(info->port_number));
				fclose(fp);
				GetPortStatus(info->port_number,info);
				return RTK_SUCCESS;
			}
		}
		fclose(fp);
	}
	return RTK_FAILED;
}

int rtk_get_link_info_by_mac(unsigned char mac[6],struct rtk_link_type_info *info)
{
	struct rtk_link_type_info	tmp_info;

	if(check_wifi("wlan0",mac,&tmp_info))
	{
		info->type = RTK_5G;	
		info->last_rx_bytes = tmp_info.cur_rx_bytes;
		info->last_tx_bytes = tmp_info.cur_tx_bytes;
		return RTK_SUCCESS;
	}

	if(check_wifi("wlan1",mac,&tmp_info))
	{
		info->type = RTK_24G;	
		info->last_rx_bytes = tmp_info.cur_rx_bytes;
		info->last_tx_bytes = tmp_info.cur_tx_bytes;		
		return RTK_SUCCESS;
	}	

	if(check_layer2(mac,&tmp_info))
	{
		info->type = RTK_ETHERNET;
		info->port_number = tmp_info.port_number;
		info->last_rx_bytes = tmp_info.cur_rx_bytes;
		info->last_tx_bytes = tmp_info.cur_tx_bytes;
		return RTK_SUCCESS;
	}

	info->type = RTK_LINK_ERROR;
	return RTK_FAILED;
}


int rtk_get_device_brand(unsigned char *mac, char *mac_file, char *brand)
{	
	if(mac==NULL || mac_file==NULL || brand==NULL)
		return RTK_FAILED;
	
	FILE *fp;
	int index;
	unsigned char prefix_mac[16], mac_brand[64];
	char *pchar;
	int found=0;
	if((fp= fopen(mac_file, "r"))==NULL)
		return RTK_FAILED;

	sprintf(prefix_mac, "%02X-%02X-%02X", mac[0], mac[1], mac[2]);

	for(index = 0 ; index < 8; ++index)
	{
		if((prefix_mac[index]  >= 'a')  && (prefix_mac[index]<='f'))
			prefix_mac[index] -= 32;
	}

	//printf("%s.%d. str(%s)\n",__FUNCTION__,__LINE__,prefix_mac);

	while(fgets(mac_brand, sizeof(mac_brand), fp))
	{			
		mac_brand[strlen(mac_brand)-1]='\0';		
		if((pchar=strstr(mac_brand, prefix_mac))!=NULL)
		{
			pchar+=9;
			strcpy(brand, pchar);
			found=1;
			break;
		}
	}
	fclose(fp);
	
	if(found==1)
		return RTK_SUCCESS;
	
	return RTK_FAILED;
}


int  rtk_get_terminal_rate( int term_number,struct rtk_link_type_info *info)
{
	int linkFlag =0;
	int ret =RTK_FAILED;
	FILE *fp=NULL;
	unsigned char buffer[128];
	unsigned char strtmp[24]={0};
	unsigned char macAddr[12]={0};
	struct rtk_terminal_rate_info rate_entry[MAX_TERM_NUMBER];	
	struct rtk_link_type_info tmp_info[MAX_TERM_NUMBER];
	int findFlag=0;
	int idx =0,i=0,j=0;
	int fh=0; 
	
	for(i=0; i<term_number; i++){
		info[i].download_speed=0;
		info[i].upload_speed=0;
		info[i].all_bytes=0;
	}
	
	if((fp = fopen(TERMINAL_RATE_INFO,"r")) != NULL)
	{
		
		if((fh=fileno(fp))<0){
			fclose(fp);
			goto out;
		}
		flock(fh, LOCK_EX);
		
		while((fgets(buffer, sizeof(buffer), fp) )&& (idx<MAX_TERM_NUMBER))
		{		
			
			buffer[strlen(buffer)-1]='\0';
			sscanf(buffer,"%u %u %s %u %u %u %u",&rate_entry[idx].link_flag,
				&rate_entry[idx].port_number, strtmp,
				&rate_entry[idx].upload_speed, &rate_entry[idx].download_speed,&rate_entry[idx].rx_bytes,&rate_entry[idx].tx_bytes);
			string_to_hex(strtmp, macAddr, 12);
			rate_entry[idx].all_bytes=rate_entry[idx].rx_bytes+rate_entry[idx].tx_bytes;
			memcpy(rate_entry[idx].mac,macAddr,6);
			#if 0
			printf("[%d]:%u %u %x:%x:%x:%x:%x:%x %d %d %d %d %d,[%s]:[%d]\n",idx,rate_entry[idx].link_flag,rate_entry[idx].port_number,
			rate_entry[idx].mac[0], rate_entry[idx].mac[1],	rate_entry[idx].mac[2], 
			rate_entry[idx].mac[3],rate_entry[idx].mac[4], rate_entry[idx].mac[5],
			rate_entry[idx].upload_speed, rate_entry[idx].download_speed,rate_entry[idx].rx_bytes,rate_entry[idx].tx_bytes,
			rate_entry[idx].all_bytes,__FUNCTION__,__LINE__);
			#endif
			idx++;
		}
		flock(fh, LOCK_UN);
		fclose(fp);
	}
	
	for(i=0;i<term_number;++i)
	{		
		linkFlag =-1;
		if(info[i].type==RTK_ETHERNET)
			linkFlag =0;
		else if ((info[i].type==RTK_24G)||(info[i].type==RTK_5G))
			linkFlag=1;
		else
		{
			continue;
		}
		#if 0
		printf("[%d]:%u %u %x:%x:%x:%x:%x:%x %d %d %d [%s]:[%d]\n",i,linkFlag,info[i].port_number,
					info[i].mac[0], info[i].mac[1],info[i].mac[2], info[i].mac[3],info[i].mac[4], info[i].mac[5],
					info[i].upload_speed, info[i].download_speed,info[i].all_bytes,	__FUNCTION__,__LINE__);
		#endif
		for(j=0;j<idx;j++)
		{
			if(linkFlag ==rate_entry[ j ].link_flag)
			{
				if(((linkFlag==0)&&(info[i].port_number==rate_entry[j].port_number))
				||((linkFlag==1)&&(equal_mac(info[i].mac,rate_entry[j].mac)==RTK_SUCCESS)))
				{
					findFlag=1;
					info[i].upload_speed =rate_entry[j].upload_speed;
					info[i].download_speed=rate_entry[j].download_speed;
					info[i].all_bytes =rate_entry[j].all_bytes;
					#if 0
					printf("-----[%d]:%u %u %x:%x:%x:%x:%x:%x %d %d %d [%s]:[%d]\n",i,linkFlag,info[i].port_number,
					info[i].mac[0], info[i].mac[1],info[i].mac[2], info[i].mac[3],info[i].mac[4], info[i].mac[5],
					info[i].upload_speed, info[i].download_speed,info[i].all_bytes,	__FUNCTION__,__LINE__);
					#endif
				}
			}
		}
		
	}
	
	if(findFlag)
		ret =RTK_SUCCESS;
out :
	return ret;
}

void clean_error_station(int term_number, int* real_num,struct rtk_link_type_info *info)
{
	int cur_index, last_index;
	cur_index = 0;
	last_index = term_number - 1;
	while(cur_index <= last_index)
	{
		if((info+cur_index)->type == RTK_LINK_ERROR)
		{
			memcpy((info+cur_index),(info+cur_index+1),(last_index-cur_index)*sizeof(struct rtk_link_type_info));
			--term_number;
			--last_index;
		}
		else
		{
			++cur_index;
		}
	}
	*real_num = term_number;
}

int rtk_get_lan_dev_link_time(int *entry_num, struct rtk_link_type_info *info)
{
	if(*entry_num==0 || info==NULL)
		return RTK_FAILED;
	
	int idx=0, i, j, find_mac=0;
	int tmp_idx=0;
	char line_buffer[128], tmp_mac_str[18], tmp_ip_str[16];	
	FILE *fp;
	int fh=0; 
	struct rtk_dev_link_time dev_entry[MAX_TERM_NUMBER];	
	struct rtk_link_type_info tmp_info[MAX_TERM_NUMBER];

	unsigned char *pMacAddr;
	unsigned char br0_mac[6];
	struct in_addr br0_ip, br0_netmask, br0_netip;
	struct sockaddr hwaddr;
	
	memset(tmp_info, 0, sizeof(tmp_info));	
	rtk_getInAddr("br0", IP_ADDR_T, (void *)&br0_ip);	
	rtk_getInAddr("br0", NET_MASK_T, (void *)&br0_netmask);
	br0_netip.s_addr=br0_ip.s_addr & br0_netmask.s_addr;	
	rtk_getInAddr("br0", HW_ADDR_T, (void *)&hwaddr);
	pMacAddr = (unsigned char *)hwaddr.sa_data;
	memcpy(br0_mac, pMacAddr, 6);
	for(i=0; i<*entry_num; i++)
		info[i].link_time=0;
	
	if((fp= fopen("/var/lan_dev_link_time", "r"))==NULL)
		return RTK_FAILED;
	if((fh=fileno(fp))<0)
		return RTK_FAILED;
	
	flock(fh, LOCK_EX);
	while(fgets(line_buffer, sizeof(line_buffer), fp) && idx<MAX_TERM_NUMBER)
	{			
		line_buffer[strlen(line_buffer)-1]='\0';
		sscanf(line_buffer,"%s %s %u %d",dev_entry[idx].ip, dev_entry[idx].mac, &dev_entry[idx].link_time, &dev_entry[idx].is_alive);
		dev_entry[idx].ip[15]='\0';
		dev_entry[idx].mac[17]='\0';
		idx++;
	}
	flock(fh, LOCK_UN);
	fclose(fp);
	
	for(i=0;i<*entry_num;i++)
	{		
		find_mac=0;
		for(j=0;j<idx;j++)
		{
			sprintf(tmp_mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", info[i].mac[0], info[i].mac[1], info[i].mac[2],
			info[i].mac[3],info[i].mac[4],info[i].mac[5]);
			tmp_mac_str[17]='\0';
			strcpy(tmp_ip_str, inet_ntoa(info[i].ip));			
			if(strcmp(tmp_mac_str, dev_entry[j].mac)==0)
			{
				find_mac=1;
				if(dev_entry[j].is_alive && strcmp(tmp_ip_str, dev_entry[j].ip)==0)
				{
					info[i].link_time=dev_entry[j].link_time;
					memcpy(&tmp_info[tmp_idx++], &info[i], sizeof(struct rtk_link_type_info));
					break;
				}				
			}	
		}
		if(j>=idx && find_mac==0)
			memcpy(&tmp_info[tmp_idx++], &info[i], sizeof(struct rtk_link_type_info));
	}
	
	memcpy(info, tmp_info, sizeof(struct rtk_link_type_info)*tmp_idx);	
	*entry_num=tmp_idx;
	
	return RTK_SUCCESS;
}

int rtk_get_terminal_info(unsigned int *real_num,struct rtk_link_type_info *info, unsigned int empty_entry_num)
{
	int term_number = 0 ;
	int i;	
	if(real_num == NULL || info == NULL)
	{
		return RTK_FAILED;
	}
	/*get arp/mac info from proc/net/arp */
	if(!getArpInfo(info,&term_number,empty_entry_num))
	{
		printf("%s.%d.get terminal fail\n",__FUNCTION__,__LINE__);
		return RTK_FAILED;
	}

    // modify by dingzhihao(realtek)
	//*real_num = term_number;
    // end

	/*get link type/rx��tx info by terminal mac*/
	for(i = 0 ;i < term_number; ++i)
	{
		rtk_get_link_info_by_mac(info[i].mac,(info+i));	
		// modify by chenbo(realtek)
		rtk_get_device_brand(info[i].mac,_PATH_DEVICE_MAC_BRAND,info[i].brand);
	}
	
	rtk_get_terminal_rate( term_number,info);
    // modify by dingzhihao(realtek)
	clean_error_station(term_number,real_num,info);
    // end
	////system("echo 1 > /var/lan_dev_link_time_flag");
	//sleep(2);
	////while(isFileExist("/var/lan_dev_link_time_flag"))
	////	;	
	rtk_get_lan_dev_link_time(real_num, info);
	/*dump terminal info*/
	//dumpAllTerminalInfo(term_number,info);
	return RTK_SUCCESS;
	
}
void rtk_wlan_diagnostic(sys_diagnostic_type type)
{
	char buff[256]={0};
	char tmpbuf[64]={0};
	char wlan_iface[32] = {0};
	int i;
	FILE* fp;
	if(type == DIAG_TYPE_WLAN_SCAN)
	{
		sprintf(buff,"echo \"###########################  diagnostic wlan scan failed  ############################\" >>%s",DUMP_DIAG_LOG_FILE);
		system(buff);
		for(i = 0;i < NUM_WLAN_INTERFACE;i++)
		{
			sprintf(wlan_iface,"wlan%d",i);
			sprintf(buff,"cat /proc/wlan%d/mib_all",i);
			rtk_dump_log_command(buff);
			sprintf(buff,"iwpriv wlan%d dump_mib",i);
			rtk_dump_log_command(buff);
			sprintf(buff,"cat /proc/wlan%d/SS_Result",i);
			rtk_dump_log_command(buff);
			//TODO 25M/40M?
		}
	}
	else if(type == DIAG_TYPE_WLAN_CONNECT)
	{
		sprintf(buff,"echo \"###########################  diagnostic wlan connect failed  ############################\" >>%s",DUMP_DIAG_LOG_FILE);
		system(buff);
		for(i = 0;i < NUM_WLAN_INTERFACE;i++)
		{
			sprintf(buff,"cat /proc/wlan%d/mib_all",i);
			rtk_dump_log_command(buff);
			sprintf(buff,"iwpriv wlan%d dump_mib",i);
			rtk_dump_log_command(buff);
			sprintf(buff,"cat /proc/wlan%d/stats",i);
			rtk_dump_log_command(buff);
			sprintf(buff,"cat /proc/wlan%d/cam_info",i);
			rtk_dump_log_command(buff);
		}
		sprintf(buff,"echo \"###########################  driver log ############################\" >>%s",DUMP_DIAG_LOG_FILE);
		system(buff);
		
		sprintf(buff,"iwpriv wlan0 set_mib debug_warn=0xffffffff");
		system(buff);
		sprintf(buff,"iwpriv wlan0 set_mib debug_trace=0xffffffff");
		system(buff);
		sprintf(buff,"iwpriv wlan0 set_mib debug_info=0xffffffff");
		system(buff);
		sprintf(buff,"iwpriv wlan0 set_mib diagnostic=1");
		system(buff);
		sleep(10);
		sprintf(buff,"cat /proc/wlan0/diagnostic");
		system(buff);
		sprintf(buff,"iwpriv wlan0 set_mib debug_warn=0");
		system(buff);
		sprintf(buff,"iwpriv wlan0 set_mib debug_trace=0");
		system(buff);
		sprintf(buff,"iwpriv wlan0 set_mib debug_info=0");
		system(buff);
		sprintf(buff,"iwpriv wlan0 set_mib diagnostic=0");
		system(buff);
		
		sprintf(buff,"cat /proc/wlan0/diagnostic");
		rtk_dump_log_command(buff);
	}
	else if(type == DIAG_TYPE_WLAN_THROUGHPUT)
	{
		sprintf(buff,"echo \"###########################  diagnostic wlan low throughput  ############################\" >>%s",DUMP_DIAG_LOG_FILE);
		system(buff);
		sprintf(buff,"iwpriv wlan0 set_mib diagnostic=1");
		system(buff);
		for(i=0;i<NUM_WLAN_INTERFACE;i++)
		{
			sprintf(buff,"cat /proc/wlan%d/sta_info",i);
			rtk_dump_log_command(buff);
			sprintf(buff,"iwpriv wlan%d set_mib rssi_dump=1",i);
			system(buff);
		}
		sleep(10);
		for(i=0;i<NUM_WLAN_INTERFACE;i++)
		{
			sprintf(buff,"iwpriv wlan%d set_mib rssi_dump=0",i);
			system(buff);
		}
		sprintf(buff,"iwpriv wlan0 set_mib diagnostic=0");
		system(buff);
		
		sprintf(buff,"echo \"###########################  driver log ############################\" >>%s",DUMP_DIAG_LOG_FILE);
		system(buff);
		sprintf(buff,"cat /proc/wlan0/diagnostic");
		rtk_dump_log_command(buff);
	}
}
void rtk_sys_diagnostic_summary(void)
{
	char buff[256]={0};
	char tmpbuf[64]={0};
	RTK_WAN_INFO wan_info={0};
	int ret;
	char * status_str[]={"Disconnected","fixed IP connected","fixed IP disconnected",
		"getting IP from DHCP server","DHCP","PPPoE connected","PPPoE disconnected",
		"PPTP connected","PPTP disconnected","L2TP connected","L2TP disconnected",
		"Brian 5BGG","USB3G connected","USB3G modem Initializing","USB3G dialing",
		"USB3G removed","USB3G disconnected"};
	sprintf(buff,"echo %s > %s",SYS_DIAGNOSTIC_STATUS_DOING,SYS_DIAGNOSTIC_STATUS);	
	system(buff);
	sprintf(buff,"rm %s 2>/dev/null",DUMP_DIAG_LOG_FILE_BAK);
	system(buff);
	sprintf(buff,"cp %s %s 2>/dev/null",DUMP_DIAG_LOG_FILE,DUMP_DIAG_LOG_FILE_BAK);
	system(buff);
	sprintf(buff,"rm %s 2>/dev/null",DUMP_DIAG_LOG_FILE);
	system(buff);
	sprintf(buff,"echo \"###########################  system info  ############################\">>%s",DUMP_DIAG_LOG_FILE);
	system(buff);
	rtk_dump_log_command("cat /proc/version");
	rtk_dump_log_command("cat /etc/version");
	rtk_dump_log_command("free");
	rtk_dump_log_command("ps");
	rtk_dump_log_command("ifconfig");
	
	sprintf(buff,"echo \"###########################  wan setting and status  ############################\" >>%s",DUMP_DIAG_LOG_FILE);
	system(buff);

	rtk_get_wan_info(&wan_info);
	
	sprintf(buff,"echo \"wan status=%s ip=%s ",status_str[wan_info.status],inet_ntoa(wan_info.ip));
	
	sprintf(tmpbuf,"mask=%s ",inet_ntoa(wan_info.mask));
	strcat(buff,tmpbuf);
	sprintf(tmpbuf,"def_gateway=%s ",inet_ntoa(wan_info.def_gateway));
	strcat(buff,tmpbuf);
	sprintf(tmpbuf,"dns1=%s ",inet_ntoa(wan_info.dns1));
	strcat(buff,tmpbuf);
	sprintf(tmpbuf,"dns2=%s \" >>%s",inet_ntoa(wan_info.dns2),DUMP_DIAG_LOG_FILE);
	strcat(buff,tmpbuf);
	system(buff);
	
	switch(wan_info.status)
	{
		case DISCONNECTED:
			break;
		case FIXED_IP_CONNECTED:
		case FIXED_IP_DISCONNECTED:
			break;
		case GETTING_IP_FROM_DHCP_SERVER:
		case DHCP:
			break;
		case PPPOE_CONNECTED:			
		case PPPOE_DISCONNECTED:			
			rtk_dump_log_command("cat /etc/ppp/chap-secrets");
			rtk_dump_log_command("cat /etc/ppp/options");
			break;
		case PPTP_CONNECTED:
		case PPTP_DISCONNECTED:
			break;
		case L2TP_CONNECTED:
		case L2TP_DISCONNECED:
			break;
		default:
			break;
	}
	
	rtk_dump_log_command("cat /etc/resolv.conf");
	rtk_dump_log_command("cat /proc/net/nf_conntrack");
	rtk_dump_log_command("route -n");
	rtk_dump_log_command("iptables -nvL");
	rtk_dump_log_command("iptables -t nat -nvL");
	rtk_dump_log_command("cat /proc/filter_table");
	sprintf(buff,"echo \"###########################  lan setting and status  ############################\" >>%s",DUMP_DIAG_LOG_FILE);
	system(buff);
	rtk_dump_log_command("cat /var/udhcpd.conf");	
	rtk_dump_log_command("cat /proc/net/arp");
	rtk_dump_log_command("cat /proc/rtl865x/port_status");
	sprintf(buff,"echo \"########################### wifi status  ############################\" >>%s",DUMP_DIAG_LOG_FILE);
	system(buff);
	
	rtk_wifi_diagnostic();

	sprintf(buff,"echo \"########################### first time system loading status(total 3 times)  ############################\" >>%s",DUMP_DIAG_LOG_FILE);
	system(buff);
	rtk_dump_log_command("cat /proc/stat");
	rtk_dump_log_command("cat /proc/softirqs");
	rtk_dump_log_command("cat /proc/interrupts");
	rtk_dump_log_command("cat /proc/net/softnet_stat");
	rtk_dump_log_command("cat /proc/meminfo");
	rtk_dump_log_command("cat /proc/uptime");
	rtk_dump_log_command("cat /proc/loadavg");

	sprintf(buff,"echo \"########################### ethernet status  ############################\" >>%s",DUMP_DIAG_LOG_FILE);
	system(buff);
	sprintf(buff,"echo \"first time read  asicCounter(total 3 times)\" >>%s",DUMP_DIAG_LOG_FILE);
	system(buff);
	rtk_dump_log_command("cat /proc/rtl865x/asicCounter");
	rtk_dump_log_command("cat /proc/rtl865x/acl");
	rtk_dump_log_command("cat /proc/rtl865x/l2");
	rtk_dump_log_command("cat /proc/rtl865x/portRate");
	rtk_dump_log_command("cat /proc/rtl865x/storm_control");
	rtk_dump_log_command("cat /proc/rtl865x/arp");
	rtk_dump_log_command("cat /proc/rtl865x/l3");
	rtk_dump_log_command("cat /proc/rtl865x/port_bandwidth");
	rtk_dump_log_command("cat /proc/rtl865x/swMCast");
	rtk_dump_log_command("cat /proc/rtl865x/mac");
	rtk_dump_log_command("cat /proc/rtl865x/port_status");
	rtk_dump_log_command("cat /proc/rtl865x/sw_l2");
	rtk_dump_log_command("cat /proc/rtl865x/debug_mode");
	rtk_dump_log_command("cat /proc/rtl865x/pppoe");
	rtk_dump_log_command("cat /proc/rtl865x/sw_l3");
	rtk_dump_log_command("cat /proc/rtl865x/diagnostic");
	rtk_dump_log_command("cat /proc/rtl865x/memory");
	rtk_dump_log_command("cat /proc/rtl865x/priority_decision");
	rtk_dump_log_command("cat /proc/rtl865x/sw_napt");
	sprintf(buff,"echo \"second time read  asicCounter(total 3 times)\" >>%s",DUMP_DIAG_LOG_FILE);
	system(buff);
	
	rtk_dump_log_command("cat /proc/rtl865x/asicCounter");
	rtk_dump_log_command("cat /proc/rtl865x/eventMgr");
	rtk_dump_log_command("cat /proc/rtl865x/mirrorPort");
	rtk_dump_log_command("cat /proc/rtl865x/privSkbInfo");
	rtk_dump_log_command("cat /proc/rtl865x/sw_netif");
	rtk_dump_log_command("cat /proc/rtl865x/fc_threshold");
	rtk_dump_log_command("cat /proc/rtl865x/mmd");
	rtk_dump_log_command("cat /proc/rtl865x/pvid");
	rtk_dump_log_command("cat /proc/rtl865x/sw_nexthop");
	rtk_dump_log_command("cat /proc/rtl865x/hs");
	rtk_dump_log_command("cat /proc/rtl865x/napt");
	rtk_dump_log_command("cat /proc/rtl865x/queue_bandwidth");
	rtk_dump_log_command("cat /proc/rtl865x/hwMCast");
	rtk_dump_log_command("cat /proc/rtl865x/netif");
	rtk_dump_log_command("cat /proc/rtl865x/vlan");
	rtk_dump_log_command("cat /proc/rtl865x/igmp");
	rtk_dump_log_command("cat /proc/rtl865x/nexthop");
	rtk_dump_log_command("cat /proc/rtl865x/soft_aclChains");
	rtk_dump_log_command("cat /proc/rtl865x/ip");
	rtk_dump_log_command("cat /proc/rtl865x/phyReg");
	rtk_dump_log_command("cat /proc/rtl865x/stats");
	sprintf(buff,"echo \"third time read  asicCounter(total 3 times)\" >>%s",DUMP_DIAG_LOG_FILE);
	system(buff);
	rtk_dump_log_command("cat /proc/rtl865x/asicCounter");

	sprintf(buff,"echo \"########################### second time system loading status(total 3 times)  ############################\" >>%s",DUMP_DIAG_LOG_FILE);
	system(buff);
	rtk_dump_log_command("cat /proc/stat");
	rtk_dump_log_command("cat /proc/softirqs");
	rtk_dump_log_command("cat /proc/interrupts");
	rtk_dump_log_command("cat /proc/net/softnet_stat");
	rtk_dump_log_command("cat /proc/meminfo");
	rtk_dump_log_command("cat /proc/uptime");
	rtk_dump_log_command("cat /proc/loadavg");

	sprintf(buff,"echo \"########################### igmp status ############################\" >>%s",DUMP_DIAG_LOG_FILE);
	system(buff);
	rtk_dump_log_command("ps");
	rtk_dump_log_command("brctl show");
	/*igmp proxy*/
	rtk_dump_log_command("cat /proc/net/ip_mr_vif");
	rtk_dump_log_command("cat /proc/net/ip_mr_cache");
	/*snooping*/
	rtk_dump_log_command("cat /proc/br_igmpsnoop");
	rtk_dump_log_command("cat /proc/br_igmpquery");
	rtk_dump_log_command("cat /proc/br_mCastFastFwd");
	rtk_dump_log_command("cat /proc/br_igmpsnoop");
	rtk_dump_log_command("cat /proc/br_mldquery");
	rtk_dump_log_command("cat /proc/br_mldsnoop");
	rtk_dump_log_command("cat /proc/br_igmpDb");
	/*m2u*/
	rtk_dump_log_command("cat /proc/wlan0/sta_info");
	rtk_dump_log_command("cat /proc/wlan1/sta_info");
	/*passthru*/
	rtk_dump_log_command("cat /proc/custom_Passthru");
	/*hw mcast*/
	rtk_dump_log_command("cat /proc/rtl865x/swMCast");

	
	sprintf(buff,"echo \"########################### qos status ############################\" >>%s",DUMP_DIAG_LOG_FILE);
	system(buff);
	rtk_dump_log_command("ps");
	rtk_dump_log_command("ifconfig");
#if 0
	rtk_dump_log_command("flash get QOS_RULE_TBL");
	rtk_dump_log_command("flash get QOS_MONOPOLY_ENABLED");
	rtk_dump_log_command("flash get QOS_MONOPOLY_MAC");
	rtk_dump_log_command("flash get QOS_MONOPOLY_TIME");
	rtk_dump_log_command("flash get QOS_MONOPOLY_DEFAULT_BANDWIDTH");
#endif
	rtk_dump_log_command("cat /proc/qos");
	/*qos mnp*/
	rtk_dump_log_command("cat /var/qos_mon_info");
	/*uplink*/
	rtk_dump_log_command("tc qdisc show dev eth1");
	rtk_dump_log_command("tc class show dev eth1");
	rtk_dump_log_command("tc filter show dev eth1");
	
	rtk_dump_log_command("tc qdisc show dev ppp0");
	rtk_dump_log_command("tc class show dev ppp0");
	rtk_dump_log_command("tc filter show dev ppp0");
	/*down link*/
	rtk_dump_log_command("tc qdisc show dev br0");
	rtk_dump_log_command("tc class show dev br0");
	rtk_dump_log_command("tc filter show dev br0");
	
	rtk_dump_log_command("iptables -t mangle -nvL");
	
	sprintf(buff,"echo \"########################### tcp-ip protocal stack and fastpath status	############################\" >>%s",DUMP_DIAG_LOG_FILE);
	system(buff);
	sprintf(buff,"ifconfig >>%s",DUMP_DIAG_LOG_FILE);
	system(buff);
	rtk_dump_log_command("cat /proc/net/arp");
	rtk_dump_log_command("cat /proc/net/dev");
	rtk_dump_log_command("cat /proc/net/route");
	rtk_dump_log_command("cat /proc/net/nf_conntrack");
	rtk_dump_log_command("cat /proc/sys/net/ipv4/ip_forward");
	rtk_dump_log_command("cat /proc/sys/net/nf_conntrack_max");

	rtk_dump_log_command("cat /proc/fast_nat");
	rtk_dump_log_command("cat /proc/fast_pppoe");
	rtk_dump_log_command("cat /proc/fast_l2tp");
	rtk_dump_log_command("cat /proc/fast_pptp");
	rtk_dump_log_command("cat /proc/fp_path");
	rtk_dump_log_command("cat /proc/fp_napt");
	rtk_dump_log_command("cat /proc/hw_nat");
	sprintf(buff,"echo \"########################### third time system loading status(total 3 times)  ############################\" >>%s",DUMP_DIAG_LOG_FILE);
	system(buff);
	rtk_dump_log_command("cat /proc/stat");
	rtk_dump_log_command("cat /proc/softirqs");
	rtk_dump_log_command("cat /proc/interrupts");
	rtk_dump_log_command("cat /proc/net/softnet_stat");
	rtk_dump_log_command("cat /proc/meminfo");
	rtk_dump_log_command("cat /proc/uptime");
	rtk_dump_log_command("cat /proc/loadavg");

	sprintf(buff,"echo \"########################### cgi nas ntp and url filter ############################\" >>%s",DUMP_DIAG_LOG_FILE);
	system(buff);
	rtk_dump_log_command("cat /tmp/cgiClientTmp");
	rtk_dump_log_command("cat /tmp/ntp_tmp");
	rtk_dump_log_command("cat /etc/TZ");
	rtk_dump_log_command("cat /proc/filter_table");

	sprintf(buff,"echo \"########################### get_terminal_info ############################\" >>%s",DUMP_DIAG_LOG_FILE);
	system(buff);
	{
		unsigned int real_num=0,i=0;
		struct rtk_link_type_info *p = malloc(32*sizeof(struct rtk_link_type_info));					//malloc memory
		ret = rtk_get_terminal_info(&real_num,p,32);//call function
		if(ret == RTK_SUCCESS)
		{
			for(i=0;i<real_num;i++)
			{
				sprintf(buff,"echo \"%d: LINK_TYPE(%d)	IP(%s)	MAC(%02x%02x%02x%02x%02x%02x)	DOWNLOAD_SPEED(%u)	UPLOAD_SPEED(%u)	LASTRX(%u)	LASTTX(%u)	CURRENTRX(%u)	CURRENTTX(%u)	ALLBYTE(%u) PORTNUM(%d) BRAND(%s)	LINKTIME(%u)\" >>%s",
									  i,p[i].type,inet_ntoa(p[i].ip)
											,p[i].mac[0],p[i].mac[1],p[i].mac[2],p[i].mac[3],p[i].mac[4],p[i].mac[5],
																								p[i].download_speed,p[i].upload_speed,p[i].last_rx_bytes,
																																			p[i].last_tx_bytes,p[i].cur_rx_bytes,p[i].cur_tx_bytes,p[i].all_bytes,p[i].port_number,
																																																					p[i].brand,p[i].link_time,DUMP_DIAG_LOG_FILE);
				system(buff);
			}
		}
	}

	sprintf(buff,"echo \"########################### others ############################\" >>%s",DUMP_DIAG_LOG_FILE);
	system(buff);
   	rtk_dump_log_command("flash all");
	sprintf(buff,"echo %s > %s",SYS_DIAGNOSTIC_STATUS_DONE,SYS_DIAGNOSTIC_STATUS);	
	system(buff);
}
void rtk_sys_diagnostic(sys_diagnostic_type type)
{
	char buff[256]={0};
	sprintf(buff,"echo %s > %s",SYS_DIAGNOSTIC_STATUS_DOING,SYS_DIAGNOSTIC_STATUS);	
	system(buff);
	sprintf(buff,"rm %s 2>/dev/null",DUMP_DIAG_LOG_FILE_BAK);
	system(buff);
	sprintf(buff,"cp %s %s 2>/dev/null",DUMP_DIAG_LOG_FILE,DUMP_DIAG_LOG_FILE_BAK);
	system(buff);
	sprintf(buff,"rm %s 2>/dev/null",DUMP_DIAG_LOG_FILE);
	system(buff);
	switch(type)
	{
		case DIAG_TYPE_SYS_SUMMARY:
			rtk_sys_diagnostic_summary();
			break;
		case DIAG_TYPE_WLAN_SCAN:
		case DIAG_TYPE_WLAN_CONNECT:
		case DIAG_TYPE_WLAN_THROUGHPUT:
			rtk_wlan_diagnostic(type);
			break;
		default:
			break;
	}
	sprintf(buff,"echo %s > %s",SYS_DIAGNOSTIC_STATUS_DONE,SYS_DIAGNOSTIC_STATUS);	
	system(buff);
}

#endif
//--------------------------------------------------------------------
#ifdef SYS_DIAGNOSTIC
static void sys_diagnostic_usage(void)
{
	printf("Usage:\n");
	printf("flash sys_diagnostic summary: the summary of the system status\n");
	printf("flash sys_diagnostic wlan_scan:when wlan scan failed.\n");
	printf("flash sys_diagnostic wlan_connect:when wlan connect failed.\n");
	printf("flash sys_diagnostic wlan_throughput:when wlan throughput is low.\n");
}
static sys_diagnostic_type get_diagnostic_type(char * arg)
{
	sys_diagnostic_type ret;
	if(arg == NULL)
	{
		return DIAG_TYPE_SYS_SUMMARY;
	}
	if(!strcmp(arg,"summary"))
	{
		ret = DIAG_TYPE_SYS_SUMMARY;
	}
	else if(!strcmp(arg,"wlan_scan"))
	{
		ret = DIAG_TYPE_WLAN_SCAN;
	}
	else if(!strcmp(arg,"wlan_connect"))
	{
		ret = DIAG_TYPE_WLAN_CONNECT;
	}
	else if(!strcmp(arg,"wlan_throughput"))
	{
		ret = DIAG_TYPE_WLAN_THROUGHPUT;
	}
	return ret;
}
#endif
#if defined(CONFIG_APP_BT_REPEATER_CONFIG)
#if defined(CONFIG_RTK_BLUETOOTH_HW_RTL8822B_S)
#define BT_CONFIG_FILE 			"/lib/firmware/rtlbt/rtl8822b_config"
#elif defined(CONFIG_RTK_BLUETOOTH_HW_RTL8761A_S)
#define BT_CONFIG_FILE				 "/lib/firmware/rtlbt/rtl8761a_config"
#endif

static int dump_bt_config_file(void)
{
	int fd,size,i;
	unsigned char buffer[256];
	fd = open(BT_CONFIG_FILE,O_RDONLY);
	if(fd!=-1)
	{
		fprintf(stderr,"%s:\n",BT_CONFIG_FILE);
		size = read(fd,buffer,sizeof(buffer));
		for(i=0;i<size;i++)
		{
			fprintf(stderr,"%02x ",buffer[i]);
			if(((i+1)%16) == 0)
			{
				fprintf(stderr,"\n");
			}
		}
		fprintf(stderr,"\n");
		close(fd);
	}
	else
	{
		fprintf(stderr,"Failed to open Bluetooth config file!\n");
		return -1;
	}
}
#endif

int main(int argc, char** argv)
{
	int argNum=1, action=0, idx, num, valNum=0;
	char mib[100]={0}, valueArray[256][100], *value[256], *ptr;
#ifdef PARSE_TXT_FILE
	char filename[100]={0};
	APMIB_T apmib;
#endif
	mib_table_entry_T *pTbl=NULL;

if(0)
{
	int kk=0;
	for(kk=0; kk < argc ; kk++)
	{
		printf("\r\n argv[%d]=[%s],__[%s-%u]\r\n",kk, argv[kk],__FILE__,__LINE__);

	}
}

	if ( argc > 1 ) {
#ifdef PARSE_TXT_FILE
		if ( !strcmp(argv[argNum], "-f") ) {
			if (++argNum < argc)
				sscanf(argv[argNum++], "%s", filename);
		}
#endif
		if ( !strcmp(argv[argNum], "get") ) {
			action = 1;
			if (++argNum < argc) {
				if (argc > 3 && !memcmp(argv[argNum], "wlan", 4)) {
					int idx;
#ifdef MBSSID
					if (strlen(argv[argNum]) >= 9 && argv[argNum][5] == '-' &&
						argv[argNum][6] == 'v' && argv[argNum][7] == 'a') {
						idx = atoi(&argv[argNum][8]);
						if (idx >= NUM_VWLAN_INTERFACE) {
							printf("invalid virtual wlan interface index number!\n");
							return 0;
						}
						vwlan_idx = idx+1;
					}
#ifdef UNIVERSAL_REPEATER
				if (strlen(argv[argNum]) >= 9 && argv[argNum][5] == '-' &&
						!memcmp(&argv[argNum][6], "vxd", 3)) {
					vwlan_idx = NUM_VWLAN_INTERFACE;
				}
#endif									
#endif
					idx = atoi(&argv[argNum++][4]);
					if (idx >= NUM_WLAN_INTERFACE) {
						printf("invalid wlan interface index number!\n");
						goto normal_return;
					}
					wlan_idx = idx;
				}
#ifdef MULTI_WAN_SUPPORT
				else if(argc > 3 && !memcmp(argv[argNum], "wan", 3))
				{
					int idx = atoi(&argv[argNum++][3]);
					if(idx>WANIFACE_NUM){
						printf("invalid wan interface index number!\n");
						goto normal_return;
					}
	//				fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);
					apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&idx);
				}
#endif
				sscanf(argv[argNum], "%s", mib);
				while (++argNum < argc) {
					sscanf(argv[argNum], "%s", valueArray[valNum]);
					value[valNum] = valueArray[valNum];
					valNum++;
				}
				value[valNum]= NULL;
			}
		}
		else if ( !strcmp(argv[argNum], "set") ) {
			action = 2;
			if (++argNum < argc) {
				if (argc > 4 && !memcmp(argv[argNum], "wlan", 4)) {
					int idx;
#ifdef MBSSID
					if (strlen(argv[argNum]) >= 9 && argv[argNum][5] == '-' &&
						argv[argNum][6] == 'v' && argv[argNum][7] == 'a') {
						idx = atoi(&argv[argNum][8]);
						if (idx >= NUM_VWLAN_INTERFACE) {
							printf("invalid virtual wlan interface index number!\n");
							return 0;
						}
						vwlan_idx = idx+1;
					}
#ifdef UNIVERSAL_REPEATER
				if (strlen(argv[argNum]) >= 9 && argv[argNum][5] == '-' &&
						!memcmp(&argv[argNum][6], "vxd", 3)) {
					vwlan_idx = NUM_VWLAN_INTERFACE;
				}
#endif									
#endif
					idx = atoi(&argv[argNum++][4]);
					if (idx >= NUM_WLAN_INTERFACE) {
						printf("invalid wlan interface index number!\n");
						goto normal_return;
					}
					wlan_idx = idx;
				}
#ifdef MULTI_WAN_SUPPORT
				else if(argc > 3 && !memcmp(argv[argNum], "wan", 3))
				{
					int idx = atoi(&argv[argNum++][3]);
					if(idx>WANIFACE_NUM){
						printf("invalid wan interface index number!\n");
						goto normal_return;
					}
					apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&idx);
				}
#endif
				sscanf(argv[argNum], "%s", mib);
				int SettingSSID = 0;
				if( !strcmp(mib , "SSID") ||
					!strcmp(mib , "WLAN0_SSID") ||
					!strcmp(mib , "WLAN0_WSC_SSID") ||
					!strcmp(mib , "WLAN0_VAP0_SSID") ||
					!strcmp(mib , "WLAN0_VAP1_SSID") ||
					!strcmp(mib , "WLAN0_VAP2_SSID") ||
					!strcmp(mib , "WLAN0_VAP3_SSID") ||	
					!strcmp(mib , "WLAN0_VAP4_SSID") ||	
					!strcmp(mib , "REPEATER_SSID1") ||	
					!strcmp(mib , "REPEATER_SSID2") ||
					!strcmp(mib , "NTP_TIMEZONE") 
#if defined(CONFIG_APP_SIMPLE_CONFIG)
					|| !strcmp(mib , "SC_DEVICE_NAME")
#endif
					){
					
					SettingSSID = 1;
				}
				
				while (++argNum < argc) {
					if(SettingSSID == 1){						

						//memcpy(valueArray[valNum] , argv[argNum] , strlen(argv[argNum]));
						strcpy(valueArray[valNum] , argv[argNum] );
						value[valNum] = valueArray[valNum];
						valNum++;
						break;
					}					
					sscanf(argv[argNum], "%s", valueArray[valNum]);
					value[valNum] = valueArray[valNum];
					valNum++;
				}
				value[valNum]= NULL;
			}
		}
#if defined(SET_MIB_FROM_FILE)
		else if ( !strcmp(argv[argNum], "setconf") ) {
			
			char line_buffer[256]={0};
			int i=0;
			int string_setting = 0;
			
			if(argc == 3)
			{
				if (++argNum < argc)
				{
				
					if(!memcmp(argv[argNum], "start", 3))
					{
						//printf("write setting to config file now\n");
						write_line_to_file(SYSTEM_CONF_FILE, 1, "#Realtek System config setting file\n");
					}
					else if(!memcmp(argv[argNum], "end", 3))
					{
						readFileSetMib(SYSTEM_CONF_FILE);
					}
					
				}
			}
			else if(argc == 4)
			{
				argNum++;
				sscanf(argv[argNum], "%s", mib);
#if 0				
				if( strstr(mib, "SSID")
					|| !strcmp(mib , "NTP_TIMEZONE") 
#if defined(CONFIG_APP_SIMPLE_CONFIG)
					|| !strcmp(mib , "SC_DEVICE_NAME")
#endif
				)
				{
					string_setting = 1;
					while (++argNum < argc) {
						strcpy(valueArray[valNum] , argv[argNum] );
						value[valNum] = valueArray[valNum];
						valNum++;
						
						break;
					}
					value[valNum]= NULL;
				}
				else
#endif					
				{
					sprintf(line_buffer,"%s=%s\n", argv[2], argv[3]);
					write_line_to_file(SYSTEM_CONF_FILE, 2, line_buffer);
				}
			}
			else
			{
				sprintf(line_buffer,"%s=%s ", argv[2], argv[3]);
				for(i=4; i<argc-1; i++)
				{
					strcat(line_buffer, argv[i]);
					strcat(line_buffer, " ");
				}

				//argc don't add ' ' at the end for line
				strcat(line_buffer, argv[i]);				
				strcat(line_buffer, "\n");
				write_line_to_file(SYSTEM_CONF_FILE, 2, line_buffer);
			}


			if(string_setting == 0)
				return 0;
			else
				action = 2;

		}	
#endif


		else if(!strcmp(argv[argNum], "virtual_flash_init"))
		{
#ifdef AP_CONTROLER_SUPPORT
			if(apmib_virtual_flash_malloc()<0)
				return -1;
			//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
			writeDefault(DEFAULT_ALL);
			//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
			
			return 1;
#else
			return 1;
#endif
		}
		else if ( !strcmp(argv[argNum], "all") ) {
			action = 3;
		}
		else if ( !strcmp(argv[argNum], "default") ) {
			return writeDefault(DEFAULT_ALL);
		}
		else if ( !strcmp(argv[argNum], "default-sw") ) {
#if defined(CONFIG_APP_APPLE_HOMEKIT)
            printf("hapserver hap_reset\n");
            system("hapserver hap_reset");
#endif
			return writeDefault(DEFAULT_SW);
		}
		else if ( !strcmp(argv[argNum], "default-hw") ) {
			return writeDefault(DEFAULT_HW);
		}
		else if ( !strcmp(argv[argNum], "default-bluetoothhw") ) {
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
			return writeDefault(DEFAULT_BLUETOOTHHW);
#endif
			return 0;
		}
		else if ( !strcmp(argv[argNum], "default-customerhw") ) {
#ifdef CUSTOMER_HW_SETTING_SUPPORT
			return writeDefault(DEFAULT_CUSTOMERHW);
#endif
			return 0;
		}
		else if ( !strcmp(argv[argNum], "reset") ) {
#if defined(CONFIG_APP_APPLE_HOMEKIT)
            printf("hapserver hap_reset\n");
            system("hapserver hap_reset");
#endif
#ifdef RTL_DEF_SETTING_IN_FW
			return writeDefault(DEFAULT_SW);
#else
			return resetDefault();
#endif
		}
#ifdef DEFSETTING_AUTO_UPDATE
		else if ( !strcmp(argv[argNum], "save-defsetting") ) {
			int val=1;
			apmib_init();
			flash_write(def_setting_data,DEFAULT_SETTING_OFFSET,sizeof(def_setting_data));

			apmib_set(MIB_NEED_AUTO_UPDATE,(void*)&val);
			apmib_update(CURRENT_SETTING);
			return 0;
		}
#endif
		else if ( !strcmp(argv[argNum], "hostapd") ){
			return generateHostapdConf();
		}
		else if ( !strcmp(argv[argNum], "extr") ) {
			if (++argNum < argc) {
				char prefix[20], file[20]={0};
				int ret;
				sscanf(argv[argNum], "%s", prefix);
				if (++argNum < argc)
					sscanf(argv[argNum], "%s", file);
				ret  = read_flash_webpage(prefix, file);
				if (ret == 0) // success
					unlink(file);
				return ret;
			}
			printf("Usage: %s web prefix\n", argv[0]);
			return -1;
		}
#ifdef TLS_CLIENT
		else if ( !strcmp(argv[argNum], "cert") ) {
			if (++argNum < argc) {
				char prefix[20], file[20]={0};
				int ret;
				sscanf(argv[argNum], "%s", prefix);
				if (++argNum < argc)
					sscanf(argv[argNum], "%s", file);
				ret  = read_flash_cert(prefix,file);
				//if (ret == 0) // success
				//	unlink(file);
				return ret;
			}
			printf("Usage: %s cert prefix\n", argv[0]);
			return -1;
		}
#endif
#ifdef VPN_SUPPORT
		else if ( !strcmp(argv[argNum], "rsa") ) {
			if (++argNum < argc) {
				char prefix[20], file[20]={0};
				int ret;
				sscanf(argv[argNum], "%s", prefix);
				if (++argNum < argc)
					sscanf(argv[argNum], "%s", file);
				ret  = read_flash_rsa(prefix);
				//if (ret == 0) // success
				//	unlink(file);
				return ret;
			}
			printf("Usage: %s rsa prefix\n", argv[0]);
			return -1;
		}
#endif
		else if ( !strcmp(argv[argNum], "test-hwconf") ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1	
            apmib_sem_lock();
#endif
			if ((ptr=apmib_hwconf()) == NULL) {
#if CONFIG_APMIB_SHARED_MEMORY == 1	
                apmib_sem_unlock();
#endif
				return -1;
			}
#if CONFIG_APMIB_SHARED_MEMORY == 1
		    apmib_shm_free(ptr, HWCONF_SHM_KEY);
		    apmib_sem_unlock();
#else
			free(ptr);
#endif
			return 0;
		}
		else if( !strcmp(argv[argNum], "test-bluetoothhwconf") ){
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
#if CONFIG_APMIB_SHARED_MEMORY == 1	
            apmib_sem_lock();
#endif
			if ((ptr=apmib_bluetoothHwconf()) == NULL) {
#if CONFIG_APMIB_SHARED_MEMORY == 1	
                apmib_sem_unlock();
#endif
				return -1;
			}
#if CONFIG_APMIB_SHARED_MEMORY == 1
		    apmib_shm_free(ptr, BLUETOOTH_HW_SHM_KEY);
		    apmib_sem_unlock();
#else
			free(ptr);
#endif						
#endif
			return 0;
		}
		else if( !strcmp(argv[argNum], "test-customerhwconf") ){
#ifdef CUSTOMER_HW_SETTING_SUPPORT
#if CONFIG_APMIB_SHARED_MEMORY == 1	
		            apmib_sem_lock();
#endif
					if ((ptr=apmib_customerHwconf()) == NULL) {
#if CONFIG_APMIB_SHARED_MEMORY == 1	
		                apmib_sem_unlock();
#endif
						return -1;
					}
#if CONFIG_APMIB_SHARED_MEMORY == 1
				    apmib_shm_free(ptr, CUSTOMER_HW_SHM_KEY);
				    apmib_sem_unlock();
#else
					free(ptr);
#endif						
#endif
					return 0;
				}

		else if ( !strcmp(argv[argNum], "test-dsconf") ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1	
            apmib_sem_lock();
#endif
			if ((ptr=apmib_dsconf()) == NULL) {
#if CONFIG_APMIB_SHARED_MEMORY == 1	
                apmib_sem_unlock();
#endif
				return -1;
			}
#if CONFIG_APMIB_SHARED_MEMORY == 1
		    apmib_shm_free(ptr, DSCONF_SHM_KEY);
		    apmib_sem_unlock();
#else
			free(ptr);
#endif
			return 0;
		}

		else if(strcmp(argv[argNum], "test-backupdsconf") == 0) 
		{
#ifdef BACKUP_DEFAULT_SETTING_SUPPORT
#if CONFIG_APMIB_SHARED_MEMORY == 1	
            apmib_sem_lock();
#endif
			if((ptr = apmib_backupdsconf()) == NULL) 
			{
#if CONFIG_APMIB_SHARED_MEMORY == 1	
                apmib_sem_unlock();
#endif
				return -1;
			}
#if CONFIG_APMIB_SHARED_MEMORY == 1
		    apmib_shm_free(ptr, BACKUP_DSCONF_SHM_KEY);
		    apmib_sem_unlock();
#else
			free(ptr);
#endif
			return 0;
#endif
			return -1;
		}

		else if ( !strcmp(argv[argNum], "test-csconf") ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1	
            apmib_sem_lock();
#endif
			if ((ptr=apmib_csconf()) == NULL) {
#if CONFIG_APMIB_SHARED_MEMORY == 1	
                apmib_sem_unlock();
#endif
				return -1;
			}
#if CONFIG_APMIB_SHARED_MEMORY == 1
		    apmib_shm_free(ptr, CSCONF_SHM_KEY);
		    apmib_sem_unlock();
#else
			free(ptr);
#endif
			return 0;
		}
		else if ( !strcmp(argv[argNum], "default-fc") ) {
			int value=0;

			apmib_init();
			value=BRIDGE_MODE;
			apmib_set(MIB_OP_MODE,(void *)&value);
			apmib_get(MIB_DHCP, (void *)&value);        //for FC, dhcp server not allowed when client
			if( value > DHCP_CLIENT )   value=DHCP_DISABLED;
			apmib_set(MIB_DHCP, (void *)&value);
			value = 0xc0a801fa;
			apmib_set(MIB_IP_ADDR, (void *)&value);     //for FC, default IP to 192.168.1.250
			value=BANDMODESINGLE;
			apmib_set(MIB_WLAN_BAND2G5G_SELECT,(void *)&value);
			value=SMACSPHY;
			SetWlan_idx("wlan0");
			apmib_set(MIB_WLAN_MAC_PHY_MODE,(void *)&value);
			//value=0;
			//apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&value);
			/*
			SetWlan_idx("wlan1");
			value=SMACSPHY;
			apmib_set(MIB_WLAN_MAC_PHY_MODE,(void *)&value);
			value=1;
			apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&value);
			*/
			apmib_update(CURRENT_SETTING);

			return 0;
		} else if ( !strcmp(argv[argNum], "wpa") ) {
			int isWds = 0;
			if ((argNum+2) < argc) {
				if (memcmp(argv[++argNum], "wlan", 4)) {
					printf("Miss wlan_interface argument!\n");
					return 0;
				}
				
#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
				strcpy(wlan_1x_ifname, argv[argNum]);
#endif

#ifdef MBSSID
				if (strlen(argv[argNum]) >= 9 && argv[argNum][5] == '-' &&
						argv[argNum][6] == 'v' && argv[argNum][7] == 'a') {
					idx = atoi(&argv[argNum][8]);
					if (idx >= NUM_VWLAN_INTERFACE) {
						printf("invalid virtual wlan interface index number!\n");
						return 0;
					}
					vwlan_idx = idx+1;
				}
#ifdef UNIVERSAL_REPEATER
				if (strlen(argv[argNum]) >= 9 && argv[argNum][5] == '-' &&
						!memcmp(&argv[argNum][6], "vxd", 3)) {
					vwlan_idx = NUM_VWLAN_INTERFACE;
				}
#endif				
#endif
				wlan_idx = atoi(&argv[argNum][4]);
				if (wlan_idx >= NUM_WLAN_INTERFACE) {
					printf("invalid wlan interface index number!\n");
					goto normal_return;
				}

				if (((argc-1) > (argNum+1)) && !strcmp(argv[argNum+2], "wds")) 
					isWds = 1;

				generateWpaConf(argv[argNum+1], isWds);
				goto normal_return;
			}
			else {
				printf("Miss arguments [wlan_interface config_filename]!\n");
				return 0;
			}
		}
		#if defined(CONFIG_RTL_ETH_802DOT1X_SUPPORT)
		else if(!strcmp(argv[argNum], "ethdot1x")){
			generateEthDot1xConf(argv[argNum+1]);
			return 0;
		}
		#endif
#ifdef CONFIG_APP_WEAVE
		else if(strcmp(argv[argNum], "set_weaveConf") == 0){
			if(argv[argNum+1]){
				char *weaveConfPath=argv[argNum+1];
				//printf("%s:%d weaveConfPath=%s\n",__FUNCTION__,__LINE__,weaveConfPath);
				FILE *fp=fopen(weaveConfPath,"r+");
				//printf("%s:%d\n",__FUNCTION__,__LINE__);
				unsigned char weave_config[MAX_WEAVE_CONFIG_LENGTH]={0}; 
				int weave_resgitered=0;
				int weave_saved=0;
				//printf("%s:%d\n",__FUNCTION__,__LINE__);
				if(!fp){
					printf("open %s fail!\n",argv[argNum]);
					return 0;
				}
				//printf("%s:%d\n",__FUNCTION__,__LINE__);
				weave_saved=fread(weave_config,1,MAX_WEAVE_CONFIG_LENGTH,fp);
				if(!feof(fp)){
					printf("read file error!");
					fclose(fp);
					return 0;
				}
				//printf("%s:%d weave_saved=%d\n",__FUNCTION__,__LINE__,weave_saved);
				fclose(fp);
				//printf("%s:%d weave_config=%s\n",__FUNCTION__,__LINE__,weave_config);
				if((!strstr(weave_config,"\"cloud_id\":"))||strstr(weave_config,"\"cloud_id\": \"\",")){
                    weave_resgitered=0;
                }else
                    weave_resgitered=1;
                apmib_init();
				apmib_set(MIB_WEAVE_REGISTERED,&weave_resgitered);
				apmib_set(MIB_WEAVE_CONFIG,weave_config);
				apmib_update(CURRENT_SETTING);
				return 0;
			}else{
				printf("Invalid input!\n");
				return 0;
			}
		}
#endif
		// set flash parameters by reading from file
		else if ( !strcmp(argv[argNum], "-param_file") ) {
			vwlan_idx = 0;
			if ((argNum+2) < argc) {
				if (memcmp(argv[++argNum], "wlan", 4)) {
					printf("Miss wlan_interface argument!\n");
					return 0;
				}
#ifdef MBSSID
				if (strlen(argv[argNum]) >= 9 && argv[argNum][5] == '-' &&
						argv[argNum][6] == 'v' && argv[argNum][7] == 'a') {
					idx = atoi(&argv[argNum][8]);
					if (idx >= NUM_VWLAN_INTERFACE) {
						printf("invalid virtual wlan interface index number!\n");
						return 0;
					}
					vwlan_idx = idx+1;
				}
#ifdef UNIVERSAL_REPEATER
				if (strlen(argv[argNum]) >= 9 && argv[argNum][5] == '-' &&
						!memcmp(&argv[argNum][6], "vxd", 3)) {
					vwlan_idx = NUM_VWLAN_INTERFACE;
				}
#endif				
#endif
				wlan_idx = atoi(&argv[argNum][4]);
				if (wlan_idx >= NUM_WLAN_INTERFACE) {
					printf("invalid wlan interface index number!\n");
					goto normal_return;
				}
				readFileSetParam(argv[argNum+1]);

#if defined(CONFIG_RTL_ULINKER)
				if(strlen(argv[argNum]) == 5) //only wlan0 or wlan1
				{
					int wlanMode = AP_MODE;
					int rptEnabled = 0;

					apmib_get( MIB_WLAN_MODE, (void *)&wlanMode);
					if(wlan_idx == 0)
						apmib_get(MIB_REPEATER_ENABLED1, (void *)&rptEnabled);
#if !defined(CONFIG_RTL_TRI_BAND)
					else
						apmib_get(MIB_REPEATER_ENABLED2, (void *)&rptEnabled);
#else
					else if(wlan_idx == 1)
						apmib_get(MIB_REPEATER_ENABLED2, (void *)&rptEnabled);

					else if(wlan_idx == 2)
						apmib_get(MIB_REPEATER_ENABLED3, (void *)&rptEnabled);
#endif					

					if(wlanMode == CLIENT_MODE)
					{
						vwlan_idx = ULINKER_CL_MIB;
					}
					else if(wlanMode == AP_MODE)
					{
						vwlan_idx = ULINKER_AP_MIB;
						if(rptEnabled == 1)
							vwlan_idx = ULINKER_RPT_MIB;
					}
					
					
					readFileSetParam(argv[argNum+1]);
				}
					
#endif
				
#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)				
				
				if(strcmp(argv[argNum],"wlan0-vxd") == 0 || strcmp(argv[argNum],"wlan1-vxd") == 0)					
				{
					char tmpStr[MAX_SSID_LEN];		
											
					apmib_get(MIB_WLAN_SSID, (void *)tmpStr);

					apmib_set(MIB_WLAN_WSC_SSID, (void *)tmpStr);
										
					if(wlan_idx == 0)
						apmib_set(MIB_REPEATER_SSID1, (void *)tmpStr);
					else
						apmib_set(MIB_REPEATER_SSID2, (void *)tmpStr);	
						
					apmib_update(CURRENT_SETTING);
				}
#endif // #if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)				
			}
			else
				printf("Miss arguments [wlan_interface param_file]!\n");
			return 0;
		}
		else if (strcmp(argv[argNum], "import-config") == 0) // import config setting
		{    
			FILE *fp=NULL;		
			struct stat fpstatus;
			int status=0, type=0;
			unsigned char tmpfilename[20];


			sprintf(tmpfilename,"%s",argv[argNum+1]);
			
			fp = fopen(tmpfilename, "r");
			if (fp == NULL)
			{
				printf("open hw setting file failed!\n");										
			}
			else
			{
				char line[1024];
				unsigned char cmdline[1024];
				unsigned char shcmdline[1024];
				memset(line,0x00,sizeof(line));
				
				while ( fgets(line, 1000, fp) )
				{					
					memset(cmdline,0x00,sizeof(cmdline));
					memset(shcmdline,0x00,sizeof(shcmdline));
					if(strlen(line)-1 != 0)
					{
							unsigned char is_power_setting=0;
							char *lineptr = line;
							char *mib_name_ptr;
							char *mib_value_ptr;

							mib_name_ptr = strsep(&lineptr,"=");
							mib_value_ptr = strsep(&lineptr,"=");
	
							if( strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_CCK_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_CCK_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_HT40_1S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_HT40_1S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_5G_HT40_1S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_5G_HT40_1S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_HT40_2S") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_HT20") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_OFDM") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_CCK_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_CCK_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_HT40_1S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_HT40_1S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_5G_HT40_1S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_5G_HT40_1S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_HT40_2S") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_HT20") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_OFDM") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_20BW1S_OFDM1T_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_40BW2S_20BW2S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_OFDM2T_CCK2T_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_40BW3S_20BW3S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_OFDM3T_CCK3T_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_40BW4S_20BW4S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_OFDM4T_CCK4T_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_20BW1S_OFDM1T_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_40BW2S_20BW2S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_40BW3S_20BW3S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_40BW4S_20BW4S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_RSVD_OFDM4T_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_80BW1S_160BW1S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_80BW2S_160BW2S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_80BW3S_160BW3S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_80BW4S_160BW4S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_20BW1S_OFDM1T_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_40BW2S_20BW2S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_OFDM2T_CCK2T_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_40BW3S_20BW3S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_OFDM3T_CCK3T_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_40BW4S_20BW4S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_OFDM4T_CCK4T_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_20BW1S_OFDM1T_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_40BW2S_20BW2S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_40BW3S_20BW3S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_40BW4S_20BW4S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_RSVD_OFDM4T_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_80BW1S_160BW1S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_80BW2S_160BW2S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_80BW3S_160BW3S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN0_TX_POWER_DIFF_5G_80BW4S_160BW4S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_20BW1S_OFDM1T_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_40BW2S_20BW2S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_OFDM2T_CCK2T_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_40BW3S_20BW3S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_OFDM3T_CCK3T_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_40BW4S_20BW4S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_OFDM4T_CCK4T_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_20BW1S_OFDM1T_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_40BW2S_20BW2S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_40BW3S_20BW3S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_40BW4S_20BW4S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_RSVD_OFDM4T_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_80BW1S_160BW1S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_80BW2S_160BW2S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_80BW3S_160BW3S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_80BW4S_160BW4S_A") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_20BW1S_OFDM1T_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_40BW2S_20BW2S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_OFDM2T_CCK2T_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_40BW3S_20BW3S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_OFDM3T_CCK3T_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_40BW4S_20BW4S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_OFDM4T_CCK4T_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_20BW1S_OFDM1T_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_40BW2S_20BW2S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_40BW3S_20BW3S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_40BW4S_20BW4S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_RSVD_OFDM4T_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_80BW1S_160BW1S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_80BW2S_160BW2S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_80BW3S_160BW3S_B") == 0
							||  strcmp(mib_name_ptr,"HW_WLAN1_TX_POWER_DIFF_5G_80BW4S_160BW4S_B") == 0
							)
							{
								is_power_setting = 1;
							}
							
							if( strcmp(mib_name_ptr,"HW_HW_WLAN0_WLAN_ADDR") == 0
							||  strcmp(mib_name_ptr,"HW_HW_WLAN1_WLAN_ADDR") == 0 
							)
							{
								mib_name_ptr+=3;
							}

							if(is_power_setting == 0 )
							{
								sprintf(cmdline,"flash set %s %s",mib_name_ptr,mib_value_ptr);																
							}
							else
							{
								sprintf(cmdline,"flash set %s",mib_name_ptr);

//printf("\r\n mib_value_ptr=[%s],__[%s-%u]\r\n",mib_value_ptr,__FILE__,__LINE__);
								
								while( strlen(mib_value_ptr)-1 != 0 )
								{
									unsigned char power_str[4];
									unsigned int power_val;					
												
									memcpy(power_str,mib_value_ptr,2);
									power_str[2] = '\0';
									
									sscanf(power_str,"%x",&power_val);
									
//printf("\r\n power_str=[%s],__[%s-%u]\r\n",power_str,__FILE__,__LINE__);
//printf("\r\n power_val=[%d],__[%s-%u]\r\n",power_val,__FILE__,__LINE__);
									
									memset(power_str,0x00,sizeof(power_str));
									sprintf(power_str," %d",power_val);
									
									strcat(cmdline,power_str);
									mib_value_ptr+=2;
								}								
							}
							//printf("%s\r\n",cmdline);
							sprintf(shcmdline,"echo %s >> /tmp/set_config.sh",cmdline);
							//system(cmdline);
							system(shcmdline);
						
					}
					memset(line,0x00,sizeof(line));
				}
				fclose(fp);
			}

			return 0;     
		}
// added by rock /////////////////////////////////////////
#ifdef VOIP_SUPPORT
		else if ( !strcmp(argv[argNum], "size") ) {
			printf("Flash Setting Size = %d, MIB Size = %d\n",
				CURRENT_SETTING_OFFSET - DEFAULT_SETTING_OFFSET, sizeof(*pMib));
			return 0;
		}
		else if (strcmp(argv[argNum], "voip") == 0) {

			if (argc >= 2) // have voip param
				return flash_voip_cmd(argc - 2, &argv[2]);
			else
				return flash_voip_cmd(0, NULL);
		}
		else if (strcmp(argv[argNum], "config-file") == 0) // import
		{    
			FILE *fp=NULL;
			unsigned char *buf;
			struct stat fpstatus;
			int status=0, type=0;
			
			
			if (!(fp = fopen(argv[++argNum], "rb"))){
				fprintf(stderr, "VoIP Command Error: open configuration file fail!\n");
				return -1;
			}    
			
			if(fstat(fileno(fp), &fpstatus) < 0){
				fprintf(stderr, "VoIP Command Error: fstat fail!\n");
				fclose(fp);
				return -1;
			}    
			
			buf=(char *)malloc(fpstatus.st_size);
			fread(buf, 1, fpstatus.st_size, fp); 
			fclose(fp);
			
			if(!apmib_init()){
				fprintf(stderr, "apmib_init fail!\n");
				free(buf);
				return -1;
			}    
			
			updateConfigIntoFlash(buf, fpstatus.st_size, &type, &status);
			
			if (status == 0 || type == 0){ // checksum error
				fprintf(stderr, "Invalid configuration file!\n");
				free(buf);
				return 1; /* It indidates same or older version in autocfg.sh */
			} 
			else {
				if (type) { // upload success
					if(!apmib_reinit()){
						fprintf(stderr, "apmib_reinit fail!\n");
						goto back;
					}    
					fprintf(stderr, "Update successfully!\n");
				}    
			}    
			back:
			free(buf);
			return 0;     
		}    
#endif		
		else if (strcmp(argv[argNum], "tblchk") == 0) {
			// do mib tbl check
			return mibtbl_check();
		}
		
#ifdef SYS_DIAGNOSTIC
		else if(strcmp(argv[argNum],"sys_diagnostic") == 0)//generate system diagnostic files
		{
			sys_diagnostic_type diag_type;
			char cmdBuf[256] = {0};
			if(argc <3 || (diag_type=get_diagnostic_type(argv[argNum+1]))<0)
			{
				sys_diagnostic_usage();
				sprintf(cmdBuf,"echo %s > %s",SYS_DIAGNOSTIC_STATUS_INVALID_ARG,SYS_DIAGNOSTIC_STATUS); 
				return -1;
			}
			if(!apmib_init())
			{
				printf("Initialize AP MIB failed!\n");
				return -1;
			}
			rtk_sys_diagnostic(diag_type);
			return 0;
		}
#endif

#ifdef CONFIG_RTL_WAPI_SUPPORT
		else if(0==strcmp(argv[argNum],"wapi-check"))
		{
			/*if WAPI ,check need to init CA*/
			 struct stat status;
			int init;
			if ( !apmib_init()) {
				printf("Initialize AP MIB failed!\n");
				return -1;
			}
			apmib_get(MIB_WLAN_WAPI_CA_INIT,(void *)&init);
			if(init)
				return 0;
			/*check if CA.cert exists. since the defauts maybe load*/
			if (stat(CA_CERT, &status) < 0)
			{
				system("initCAFiles.sh");
				init=1;
				if(!apmib_set(MIB_WLAN_WAPI_CA_INIT,(void *)&init))
					printf("set MIB_WLAN_WAPI_CA_INIT error\n");
				apmib_update(CURRENT_SETTING);
			}
			return 0;
		}
#else	
	else if(0==strcmp(argv[argNum],"wapi-check"))
	{
		//printf("WAPI Support Not Enabled\n");
		return 0;
	}
#endif
#ifdef WLAN_FAST_INIT
		else if ( !strcmp(argv[argNum], "set_mib") ) {
			if ((argNum+1) < argc) {
				if (memcmp(argv[++argNum], "wlan", 4)) {
					printf("Miss wlan_interface argument!\n");
					return -1;
				}
#ifdef MBSSID
				if (strlen(argv[argNum]) >= 9 && argv[argNum][5] == '-' &&
						argv[argNum][6] == 'v' && argv[argNum][7] == 'a') {
					idx = atoi(&argv[argNum][8]);
					if (idx >= NUM_VWLAN_INTERFACE) {
						printf("invalid virtual wlan interface index number!\n");
						return 0;
					}
					vwlan_idx = idx+1;
				}
#ifdef UNIVERSAL_REPEATER
				if (strlen(argv[argNum]) >= 9 && argv[argNum][5] == '-' &&
						!memcmp(&argv[argNum][6], "vxd", 3)) {
					vwlan_idx = NUM_VWLAN_INTERFACE;
				}
#endif
#endif
				wlan_idx = atoi(&argv[argNum][4]);
				if (wlan_idx >= NUM_WLAN_INTERFACE) {
					printf("invalid wlan interface index number!\n");
					goto error_return;
				}
				
#ifdef MBSSID
				int ret;
   #ifdef CONFIG_RTL_COMAPI_CFGFILE
      #if defined(CONFIG_RTL_92D_SUPPORT)
    		char tmpstr[10];
    		sprintf(tmpstr,"wlan%d",wlan_idx);
    	
    		SetWlan_idx(argv[argNum]);
    	     //ret = comapi_initWlan(argv[argNum]);
    	     ret = initWlan(argv[argNum]);
    		SetWlan_idx(tmpstr);
      #else
            //ret = comapi_initWlan(argv[argNum]);
            ret = initWlan(argv[argNum]);
      #endif
   #else   		
		ret = initWlan(argv[argNum]);
   #endif 
	vwlan_idx = 0;
	return ret;
#else
	return initWlan(argv[argNum]); 
#endif

			}
			else
				printf("Miss arguments [wlan_interface]!\n");
			return -1;
		}
#endif
#ifdef HOME_GATEWAY
		else if ( !strcmp(argv[argNum], "gen-pppoe") ) { // generate pppoe config file
#if defined(MULTI_WAN_SUPPORT) 
			if ((argNum+4) < argc) {			
				return generatePPPConf(1, argv[argNum+1], argv[argNum+2], 
					argv[argNum+3],argv[argNum+4]);
#else
			if ((argNum+3) < argc) {
				return generatePPPConf(1, argv[argNum+1], argv[argNum+2], 
					argv[argNum+3]);
#endif
			}
			else {
				printf("Miss arguments [option-file pap-file chap-file]!\n");
				return -1;
			}
		}
		else if ( !strcmp(argv[argNum], "gen-pptp") ) { // generate pptp config file
#if defined(MULTI_WAN_SUPPORT)
			if ((argNum+4) < argc) {
				return generatePPPConf(0, argv[argNum+1], argv[argNum+2], 
					argv[argNum+3],argv[argNum+4]);
#else
			if ((argNum+3) < argc) {
				return generatePPPConf(0, argv[argNum+1], argv[argNum+2], argv[argNum+3]);
#endif
			}
			else {
				printf("Miss arguments [option-file pap-file chap-file]!\n");
				return -1;
			}
		}
		else if ( !strcmp(argv[argNum], "clearppp") ) 	//To send PPP LCP(Termination Request)
		{
			int sessid = atoi(argv[++argNum]);
			unsigned char * pppServMacPtr=(unsigned char *)argv[++argNum];
			return send_ppp_msg(sessid, pppServMacPtr);
		}
#if defined(RTL_L2TP_POWEROFF_PATCH)//patch for l2tp by jiawenjian
		else if ( !strcmp(argv[argNum], "clearl2tp"))
		{
			unsigned short l2tp_ns = atoi(argv[++argNum]);
			int l2tp_buf_length = atoi(argv[++argNum]);
			unsigned char * lanIp = (unsigned char *)argv[++argNum];
			unsigned char * serverIp = (unsigned char *)argv[++argNum];
			unsigned char * l2tp_Buff = (unsigned char *)argv[++argNum];
					
			return send_l2tp_msg(l2tp_ns, l2tp_buf_length, l2tp_Buff, lanIp, serverIp);
		}
#endif

#endif		

		else if ( !strcmp(argv[argNum], "settime") ) {
			return setSystemTime_flash();			
		}


#ifdef WIFI_SIMPLE_CONFIG
		else if ( !strcmp(argv[argNum], "upd-wsc-conf") ) {
			return updateWscConf(argv[argNum+1], argv[argNum+2], 0, argv[argNum+3]);
		}
#ifdef CONFIG_RTL_COMAPI_CFGFILE
        else if ( !strcmp(argv[argNum], "def-wsc-conf") ) {
			return defaultWscConf(argv[argNum+1], argv[argNum+2]);			
		}
#endif
		else if ( !strcmp(argv[argNum], "gen-pin") ) {
#ifdef WIFI_SIMPLE_CONFIG
			char strtmp[10];
			sprintf(strtmp,"wlan%d",wlan_idx);
//printf("\r\n strtmp=[%s],__[%s-%u]\r\n",strtmp,__FILE__,__LINE__);			
			return updateWscConf(0, 0, 1, strtmp);
#endif			
		}		
#endif // WIFI_SIMPLE_CONFIG
	else if ( !strcmp(argv[argNum], "gethw") ) {
			action = 4;
			if (++argNum < argc) {
				if (argc > 3 && !memcmp(argv[argNum], "wlan", 4)) {
					int idx;
					idx = atoi(&argv[argNum++][4]);
					if (idx >= NUM_WLAN_INTERFACE) {
						printf("invalid wlan interface index number!\n");
						return 0;
					}
					wlan_idx = idx;
				}
				sscanf(argv[argNum], "%s", mib);
				while (++argNum < argc) {
					sscanf(argv[argNum], "%s", valueArray[valNum]);
					value[valNum] = valueArray[valNum];
					valNum++;
				}
				value[valNum]= NULL;
			}
		}else if ( !strcmp(argv[argNum], "sethw") ) {
			action = 5;
			if (++argNum < argc) {
				if (argc > 4 && !memcmp(argv[argNum], "wlan", 4)) {
					int idx;
					idx = atoi(&argv[argNum++][4]);
					if (idx >= NUM_WLAN_INTERFACE) {
						printf("invalid wlan interface index number!\n");
						return 0;
					}
					wlan_idx = idx;
				}
				sscanf(argv[argNum], "%s", mib);
				while (++argNum < argc) {
					sscanf(argv[argNum], "%s", valueArray[valNum]);
					value[valNum] = valueArray[valNum];
					valNum++;
				}
				value[valNum]= NULL;
			}
		}else if ( !strcmp(argv[argNum], "allhw") ) {
			action = 6;
		}
#ifdef CONFIG_APP_BT_REPEATER_CONFIG
		else if ( !strcmp(argv[argNum], "dump_btconfig") ) {
			dump_bt_config_file();
			return 0;
		}
#endif
		
#if defined(CONFIG_APP_TR069)
		else if ( !strcmp(argv[argNum], "save_cwmpnotifylist") ) {			
			return save_cwmpnotifylist();
		}
#endif	
#ifdef TR181_SUPPORT
		else if( !strcmp(argv[argNum], "tr181_get") )
		{
//#ifdef CONFIG_IPV6
			handle_tr181_get_ipv6(argv[argNum+1]);
			return 0;			
//#endif
		}
		else if( !strcmp(argv[argNum], "tr181_set") )
		{
//#ifdef CONFIG_IPV6
			handle_tr181_set_ipv6(argv[argNum+1],argv[argNum+2]);
			return 0;			
//#endif			
		}
#endif
	}

	if ( action == 0) {
		showHelp();
		goto error_return;
	}
	if ( (action==1 && !mib[0]) ||
	     (action==2 && !mib[0]) ) {
		showAllMibName();
		goto error_return;
	}

	if ( action==2 && (!strcmp(mib, "MACAC_ADDR") || !strcmp(mib, "DEF_MACAC_ADDR"))) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetACHelp();
			goto error_return;
		}
		if ( (!strcmp(value[0], "del") && !value[1]) || (!strcmp(value[0], "add") && !value[1]) ) {
			showSetACHelp();
			goto error_return;
		}
	}

#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_) // below code copy above ACL code
	if ( action==2 && (!strcmp(mib, "MESH_ACL_ADDR") || !strcmp(mib, "DEF_MESH_ACL_ADDR"))) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetMeshACLHelp();
			goto error_return;
		}
		if ( (!strcmp(value[0], "del") && !value[1]) || (!strcmp(value[0], "add") && !value[1]) ) {
			showSetMeshACLHelp();
			goto error_return;
		}
	}
#endif
#if defined(VLAN_CONFIG_SUPPORTED)

	if ( action==2 && (!strcmp(mib, "VLANCONFIG_TBL") || !strcmp(mib, "DEF_VLANCONFIG_TBL")) ) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetVlanConfigHelp();
			goto error_return;
		}
	}
#endif
#if defined(SAMBA_WEB_SUPPORT)
	if ( action==2 && (!strcmp(mib, "STORAGE_USER_TBL") || !strcmp(mib, "DEF_STORAGE_USER_TBL")) ) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetSambaAccountHelp();
			goto error_return;
		}
	}
	if ( action==2 && (!strcmp(mib, "STORAGE_SHAREINFO_TBL") || !strcmp(mib, "DEF_STORAGE_SHAREINFO_TBL")) ) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetSambaShareinfoHelp();
			goto error_return;
		}
	}	
#endif
#if defined(CONFIG_RTL_ETH_802DOT1X_SUPPORT)
	
		if ( action==2 && (!strcmp(mib, "ELAN_DOT1X_TBL") || !strcmp(mib, "ELAN_DOT1X_DELALL")) ) {
			if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
				showSetEthDot1xConfigHelp();
				goto error_return;
			}
		}
#endif	
	

	if ( action==2 && (!strcmp(mib, "WDS") || !strcmp(mib, "DEF_WDS"))) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetWdsHelp();
			goto error_return;
		}
		if ( (!strcmp(value[0], "del") && !value[1]) || (!strcmp(value[0], "add") && !value[1]) ) {
			showSetWdsHelp();
			goto error_return;
		}
	}

#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	if ( action==2 && (!strcmp(mib, "CAP_PORTAL_ALLOW_TBL") || !strcmp(mib, "DEF_CAP_PORTAL_ALLOW_TBL")) ) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			goto error_return;
		}
	}
#endif

#ifdef HOME_GATEWAY
	if ( action==2 && (!strcmp(mib, "PORTFW_TBL") || !strcmp(mib, "DEF_PORTFW_TBL"))) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetPortFwHelp();
			goto error_return;
		}
	}
	if ( action==2 && (!strcmp(mib, "PORTFILTER_TBL") || !strcmp(mib, "DEF_PORTFILTER_TBL")) ) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetPortFilterHelp();
			goto error_return;
		}
	}
	if ( action==2 && (!strcmp(mib, "IPFILTER_TBL") || !strcmp(mib, "DEF_IPFILTER_TBL"))) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetIpFilterHelp();
			goto error_return;
		}
	}
	if ( action==2 && (!strcmp(mib, "MACFILTER_TBL") || !strcmp(mib, "DEF_MACFILTER_TBL")) ) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetMacFilterHelp();
			goto error_return;
		}
	}
	if ( action==2 && (!strcmp(mib, "URLFILTER_TBL") || !strcmp(mib, "DEF_URLFILTER_TBL")) ) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetUrlFilterHelp();
			goto error_return;
		}
	}
	if ( action==2 && (!strcmp(mib, "TRIGGERPORT_TBL") || !strcmp(mib, "DEF_TRIGGERPORT_TBL")) ) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetTriggerPortHelp();
			goto error_return;
		}
	}
#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
	if ( action==2 && (!strcmp(mib, "DHCP_SERVER_OPTION_TBL") || !strcmp(mib, "DEF_DHCP_SERVER_OPTION_TBL"))) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetDhcpServerOptionHelp();
			goto error_return;
		}
	}
	if ( action==2 && (!strcmp(mib, "DHCP_CLIENT_OPTION_TBL") || !strcmp(mib, "DEF_DHCP_CLIENT_OPTION_TBL"))) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetDhcpClientOptionHelp();
			goto error_return;
		}
	}
	if ( action==2 && (!strcmp(mib, "DHCPS_SERVING_POOL_TBL") || !strcmp(mib, "DEF_DHCPS_SERVING_POOL_TBL"))) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetDhcpsServingPoolHelp();
			goto error_return;
		}
	}
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */

#ifdef ROUTE_SUPPORT
	if ( action==2 && (!strcmp(mib, "STATICROUTE_TBL") || !strcmp(mib, "DEF_STATICROUTE_TBL")) ) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetStaticRouteHelp();
			goto error_return;
		}
	}
#endif //ROUTE

#ifdef CONFIG_RTL_AIRTIME
	if ( action==2 && 
		(!strcmp(mib, "AIRTIME_TBL") || !strcmp(mib, "DEF_AIRTIME_TBL") ||
		!strcmp(mib, "AIRTIME2_TBL") || !strcmp(mib, "DEF_AIRTIME2_TBL"))) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetAirTimeHelp();
			goto error_return;
		}
		if ( (!strcmp(value[0], "del") && !value[1]) || (!strcmp(value[0], "add") && !value[1]) ) {
			showSetAirTimeHelp();
			goto error_return;
		}
	}
#endif /* CONFIG_RTL_AIRTIME */

#endif
#ifdef HOME_GATEWAY
#ifdef VPN_SUPPORT
	if ( action==2 && (!strcmp(mib, "IPSECTUNNEL_TBL") || !strcmp(mib, "DEF_IPSECTUNNEL_TBL")) ) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetIpsecTunnelHelp();
			goto error_return;
		}
	}
#endif
#endif
#ifdef TLS_CLIENT
	if ( action==2 && (!strcmp(mib, "CERTROOT_TBL") || !strcmp(mib, "DEF_CERTROOT_TBL")) ) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetCertRootHelp();
			goto error_return;
		}
	}
	if ( action==2 && (!strcmp(mib, "CERTUSER_TBL") || !strcmp(mib, "DEF_CERTUSER_TBL")) ) {
		if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall"))) {
			showSetCertUserHelp();
			goto error_return;
		}
	}	
#endif

	switch (action) {
	case 1: // get

#ifdef PARSE_TXT_FILE
		if ( filename[0] ) {
			if ( parseTxtConfig(filename, &apmib) < 0) {
				printf("Parse text file error!\n");
				goto error_return;
			}

			if ( !apmib_init(&apmib)) {
				printf("Initialize AP MIB failed!\n");
				goto error_return;
			}
		}
		else

#endif
		if ( !apmib_init()) {
			printf("Initialize AP MIB failed!\n");
			goto error_return;
		}

		idx = searchMIB(mib);

		if ( idx == -1 ) {
			showHelp();
			showAllMibName();
			goto error_return;
		}
		num = 1;

		if (config_area == DEF_MIB_WLAN_AREA || config_area == MIB_WLAN_AREA) { // wlan default or current
			if (mib_wlan_table[idx].id == MIB_WLAN_MACAC_ADDR)
				APMIB_GET(MIB_WLAN_MACAC_NUM, (void *)&num);
			else if (mib_wlan_table[idx].id == MIB_WLAN_WDS)
				APMIB_GET(MIB_WLAN_WDS_NUM, (void *)&num);
			else if(mib_wlan_table[idx].id == MIB_WLAN_SCHEDULE_TBL)
				APMIB_GET(MIB_WLAN_SCHEDULE_TBL_NUM, (void *)&num);
		}
#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_) // below code copy above ACL code
#if 0
		else if (config_area == MIB_AREA) {	// mib_table
			if (mib_table[idx].id == MIB_WLAN_MESH_ACL_ADDR)
			{				
				APMIB_GET(MIB_WLAN_MESH_ACL_NUM, (void *)&num);
			}
			
		}
#else
		else if (mib_table[idx].id == MIB_WLAN_MESH_ACL_ADDR)
			APMIB_GET(MIB_WLAN_MESH_ACL_NUM, (void *)&num);		
#endif

#endif

#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	else if (!strcmp(mib, "CAP_PORTAL_ALLOW_TBL"))
		APMIB_GET(MIB_CAP_PORTAL_ALLOW_TBL_NUM, (void *)&num);
#endif

#ifdef HOME_GATEWAY
		else if (!strcmp(mib, "PORTFW_TBL"))
			APMIB_GET(MIB_PORTFW_TBL_NUM, (void *)&num);
		else if (!strcmp(mib, "PORTFILTER_TBL"))
			APMIB_GET(MIB_PORTFILTER_TBL_NUM, (void *)&num);
		else if (!strcmp(mib, "IPFILTER_TBL"))
			APMIB_GET(MIB_IPFILTER_TBL_NUM, (void *)&num);
		else if (!strcmp(mib, "MACFILTER_TBL"))
			APMIB_GET(MIB_MACFILTER_TBL_NUM, (void *)&num);
		else if (!strcmp(mib, "URLFILTER_TBL"))
			APMIB_GET(MIB_URLFILTER_TBL_NUM, (void *)&num);
		else if (!strcmp(mib, "TRIGGERPORT_TBL"))
			APMIB_GET(MIB_TRIGGERPORT_TBL_NUM, (void *)&num);

#if defined(GW_QOS_ENGINE) || defined(QOS_BY_BANDWIDTH)
		else if (!strcmp(mib, "QOS_RULE_TBL"))
			APMIB_GET(MIB_QOS_RULE_TBL_NUM, (void *)&num);
#endif

#ifdef QOS_OF_TR069
       else if (!strcmp(mib, "QOS_CLASS_TBL"))
       APMIB_GET(MIB_QOS_CLASS_TBL_NUM, (void *)&num);
       else if (!strcmp(mib, "QOS_QUEUE_TBL"))
       APMIB_GET(MIB_QOS_QUEUE_TBL_NUM, (void *)&num);
	   
		else if (!strcmp(mib, "TR098_QOS_APP_TBL"))
	   		APMIB_GET(MIB_TR098_QOS_APP_TBL_NUM, (void *)&num);
		else if (!strcmp(mib, "TR098_QOS_FLOW_TBL"))
			APMIB_GET(MIB_TR098_QOS_FLOW_TBL_NUM, (void *)&num);
      else if (!strcmp(mib, "QOS_POLICER_TBL")) //mark_qos
       APMIB_GET(MIB_QOS_POLICER_TBL_NUM, (void *)&num);  
	 else if (!strcmp(mib, "QOS_QUEUESTATS_TBL"))
       APMIB_GET(MIB_QOS_QUEUESTATS_TBL_NUM, (void *)&num);	   	
#endif
#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
	else if (!strcmp(mib, "DHCP_SERVER_OPTION_TBL"))
		APMIB_GET(MIB_DHCP_SERVER_OPTION_TBL_NUM, (void *)&num);
	else if (!strcmp(mib, "DHCP_CLIENT_OPTION_TBL"))
		APMIB_GET(MIB_DHCP_CLIENT_OPTION_TBL_NUM, (void *)&num);
	else if (!strcmp(mib, "DHCPS_SERVING_POOL_TBL"))
		APMIB_GET(MIB_DHCPS_SERVING_POOL_TBL_NUM, (void *)&num);
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */

#ifdef CONFIG_RTL_AIRTIME
	else if (!strcmp(mib, "AIRTIME_TBL"))
		APMIB_GET(MIB_AIRTIME_TBL_NUM, (void *)&num);
	else if (!strcmp(mib, "AIRTIME2_TBL"))
		APMIB_GET(MIB_AIRTIME2_TBL_NUM, (void *)&num);
#endif /* CONFIG_RTL_AIRTIME */

#ifdef ROUTE_SUPPORT
		else if (!strcmp(mib, "STATICROUTE_TBL"))
			APMIB_GET(MIB_STATICROUTE_TBL_NUM, (void *)&num);
#endif //ROUTE
#endif

#ifdef HOME_GATEWAY
#ifdef VPN_SUPPORT
		else if (!strcmp(mib, "IPSECTUNNEL_TBL"))
			APMIB_GET(MIB_IPSECTUNNEL_TBL_NUM, (void *)&num);
#endif
#endif

#ifdef TLS_CLIENT
		else if (!strcmp(mib, "CERTROOT_TBL"))
			APMIB_GET(MIB_CERTROOT_TBL_NUM, (void *)&num);
		else if (!strcmp(mib, "CERTUSER_TBL"))
			APMIB_GET(MIB_CERTUSER_TBL_NUM, (void *)&num);			
#endif
#ifdef TR181_SUPPORT
#ifdef CONFIG_IPV6
		else if (!strcmp(mib, "IPV6_DHCPC_SENDOPT_TBL"))
			num=IPV6_DHCPC_SENDOPT_NUM;
#endif
		else if (!strcmp(mib, "DNS_CLIENT_SERVER_TBL"))
			num=DNS_CLIENT_SERVER_NUM;
#endif
		else if (!strcmp(mib, "DHCPRSVDIP_TBL"))
			APMIB_GET(MIB_DHCPRSVDIP_TBL_NUM, (void *)&num);
#if defined(VLAN_CONFIG_SUPPORTED)
		else if (!strcmp(mib, "VLANCONFIG_TBL"))
		{
			APMIB_GET(MIB_VLANCONFIG_TBL_NUM, (void *)&num);
		}
#endif

#if defined(SAMBA_WEB_SUPPORT)
		else if (!strcmp(mib, "STORAGE_USER_TBL"))
		{
			APMIB_GET(MIB_STORAGE_USER_TBL_NUM, (void *)&num);
		}
		else if (!strcmp(mib, "STORAGE_SHAREINFO_TBL"))
		{
			APMIB_GET(MIB_STORAGE_SHAREINFO_TBL_NUM, (void *)&num);
		}
#endif

#if defined(CONFIG_RTL_ETH_802DOT1X_SUPPORT)
		else if (!strcmp(mib, "ELAN_DOT1X_TBL"))
		{
			APMIB_GET(MIB_ELAN_DOT1X_TBL_NUM, (void *)&num);
		}
#endif
#if defined(CONFIG_8021Q_VLAN_SUPPORTED)
		else if (!strcmp(mib, "VLAN_TBL"))
		{
			APMIB_GET(MIB_VLAN_TBL_NUM, (void *)&num);
		}
#endif

#ifdef WLAN_PROFILE
		else if (!strcmp(mib, "PROFILE_TBL1"))
			APMIB_GET(MIB_PROFILE_NUM1, (void *)&num);
		else if (!strcmp(mib, "PROFILE_TBL2"))
			APMIB_GET(MIB_PROFILE_NUM2, (void *)&num);
#if defined(CONFIG_RTL_TRI_BAND)		
		else if (!strcmp(mib, "PROFILE_TBL3"))
			APMIB_GET(MIB_PROFILE_NUM3, (void *)&num);
#endif
#endif

		if (config_area == HW_MIB_AREA)
			pTbl = hwmib_table;
		else if (config_area == HW_MIB_WLAN_AREA)
			pTbl = hwmib_wlan_table;
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		else if (config_area == BLUETOOTH_HW_AREA)
			pTbl = bluetooth_hwmib_table;
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		else if (config_area == CUSTOMER_HW_AREA)
			pTbl = customer_hwmib_table;
#endif

		else if (config_area == DEF_MIB_AREA || config_area == MIB_AREA)
			pTbl = mib_table;
#ifdef MULTI_WAN_SUPPORT
		else if(config_area == DEF_MIB_WAN_AREA || config_area == MIB_WAN_AREA)
		{	pTbl = mib_waniface_tbl;
		}
		
#endif
		else
			pTbl = mib_wlan_table;

		getMIB(mib, pTbl[idx].id, pTbl[idx].type, num, 1 ,value);
		break;

	case 2: // set
		if ( !apmib_init()) {
			printf("Initialize AP MIB failed!\n");
			goto error_return;
		}
		idx = searchMIB(mib);
		if ( idx == -1 ) {
			showHelp();
			showAllMibName();
			goto error_return;
		}
		if ( valNum < 1) {
			showHelp();
			goto error_return;
		}
		if (config_area == HW_MIB_AREA)
			pTbl = hwmib_table;
		else if (config_area == HW_MIB_WLAN_AREA)
			pTbl = hwmib_wlan_table;
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		else if (config_area == BLUETOOTH_HW_AREA)
			pTbl = bluetooth_hwmib_table;
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		else if (config_area == CUSTOMER_HW_AREA)
			pTbl = customer_hwmib_table;
#endif
		else if (config_area ==DEF_MIB_AREA  || config_area == MIB_AREA)
			pTbl = mib_table;
#ifdef MULTI_WAN_SUPPORT
		else if(config_area == DEF_MIB_WAN_AREA || config_area == MIB_WAN_AREA)
		{	
			pTbl = mib_waniface_tbl;
		}
#endif
		else
			pTbl = mib_wlan_table;
		setMIB(mib, pTbl[idx].id, pTbl[idx].type, pTbl[idx].size, valNum, value);
		break;

	case 3: // all
		dumpAll();
		break;
	case 4: // gethw
		if ( !apmib_init_HW()) {
			printf("Initialize AP HW MIB failed!\n");
			return -1;
		}
		idx = searchMIB(mib);
		if ( idx == -1 ) {
			showHelp();
			showAllHWMibName();
			return -1;
		}
		num = 1;
		if (config_area == HW_MIB_AREA)
			pTbl = hwmib_table;
		else if (config_area == HW_MIB_WLAN_AREA)
			pTbl = hwmib_wlan_table;
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		else if (config_area == BLUETOOTH_HW_AREA)
			pTbl = bluetooth_hwmib_table;
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		else if (config_area == CUSTOMER_HW_AREA)
			pTbl = customer_hwmib_table;
#endif
		getMIB(mib, pTbl[idx].id, pTbl[idx].type, num, 1 ,value);
		break;
		
		case 5: // sethw
		if ( !apmib_init_HW()) {
			printf("Initialize AP MIB failed!\n");
			return -1;
		}
		idx = searchMIB(mib);
		if ( idx == -1 ) {
			showHelp();
			showAllHWMibName();
			return -1;
		}
		if ( valNum < 1) {
			showHelp();
			return -1;
		}
		if (config_area == HW_MIB_AREA)
			pTbl = hwmib_table;
		else if (config_area == HW_MIB_WLAN_AREA)
			pTbl = hwmib_wlan_table;
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		else if (config_area == BLUETOOTH_HW_AREA)
			pTbl = bluetooth_hwmib_table;
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		else if (config_area == CUSTOMER_HW_AREA)
			pTbl = customer_hwmib_table;
#endif

		setMIB(mib, pTbl[idx].id, pTbl[idx].type, pTbl[idx].size, valNum, value);
		break;
		case 6: // allhw
		dumpAllHW();
		break;	
	}

normal_return:
	vwlan_idx = 0;

	return 0;

error_return:
	vwlan_idx = 0;
	
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////
/*
static unsigned char getWLAN_ChipVersion()
{
	FILE *stream;
	typedef enum { CHIP_UNKNOWN=0, CHIP_RTL8188C=1, CHIP_RTL8192C=2} CHIP_VERSION_T;
	CHIP_VERSION_T chipVersion = CHIP_UNKNOWN;	
	
	stream = fopen ( "/proc/wlan0/mib_rf", "r" );
	if ( stream != NULL )
	{		
		char *strtmp;
		char line[100];
								 
		while (fgets(line, sizeof(line), stream))
		{
			
			strtmp = line;
			while(*strtmp == ' ')
			{
				strtmp++;
			}
			

			if(strstr(strtmp,"RTL8192SE") != 0)
			{
				chipVersion = CHIP_UNKNOWN;
			}
			else if(strstr(strtmp,"RTL8188C") != 0)
			{
				chipVersion = CHIP_RTL8188C;
			}
			else if(strstr(strtmp,"RTL8192C") != 0)
			{
				chipVersion = CHIP_RTL8192C;
			}
		}			
		fclose ( stream );
	}

	return chipVersion;


}
*/


#if defined(CONFIG_APP_TR069) && defined(WLAN_SUPPORT)
void Init_WlanConf(APMIB_Tp AP_Mib)
{
	int i;
	AP_Mib->cwmp_WlanConf_Enabled =1;
	AP_Mib->cwmp_WlanConf_EntryNum=MAX_CWMP_WLANCONF_NUM;
	for(i=0;i<MAX_CWMP_WLANCONF_NUM;i++){
		
		if(i==0){
			#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_DUAL_PCIESLOT_BIWLAN_D)
			AP_Mib->cwmp_WlanConfArray[i].InstanceNum=1;
			AP_Mib->cwmp_WlanConfArray[i].RootIdx=0;
			AP_Mib->cwmp_WlanConfArray[i].VWlanIdx=0;
			AP_Mib->cwmp_WlanConfArray[i].IsConfigured=1;
			AP_Mib->cwmp_WlanConfArray[i].RfBandAvailable=PHYBAND_5G;
			#else
			AP_Mib->cwmp_WlanConfArray[i].InstanceNum=1;
			AP_Mib->cwmp_WlanConfArray[i].RootIdx=0;
			AP_Mib->cwmp_WlanConfArray[i].VWlanIdx=0;
			AP_Mib->cwmp_WlanConfArray[i].IsConfigured=1;
			AP_Mib->cwmp_WlanConfArray[i].RfBandAvailable=PHYBAND_2G;
			#endif
		}else if(i ==1){
			
			
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_DUAL_PCIESLOT_BIWLAN_D)
			AP_Mib->cwmp_WlanConfArray[i].InstanceNum=2;
			AP_Mib->cwmp_WlanConfArray[i].RootIdx=1;
			AP_Mib->cwmp_WlanConfArray[i].VWlanIdx=0;
			AP_Mib->cwmp_WlanConfArray[i].IsConfigured=1;
			AP_Mib->cwmp_WlanConfArray[i].RfBandAvailable=PHYBAND_2G;
		#else
			AP_Mib->cwmp_WlanConfArray[i].InstanceNum=0;
			AP_Mib->cwmp_WlanConfArray[i].RootIdx=0;
			AP_Mib->cwmp_WlanConfArray[i].VWlanIdx=1;
			AP_Mib->cwmp_WlanConfArray[i].IsConfigured=0;
			AP_Mib->cwmp_WlanConfArray[i].RfBandAvailable=PHYBAND_2G;
		#endif
		
		}else{
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_DUAL_PCIESLOT_BIWLAN_D)
			if( i > (NUM_VWLAN_INTERFACE+1) ){
				AP_Mib->cwmp_WlanConfArray[i].InstanceNum=0;
				AP_Mib->cwmp_WlanConfArray[i].RootIdx=1;
				AP_Mib->cwmp_WlanConfArray[i].VWlanIdx=i-(NUM_VWLAN_INTERFACE+1);
				AP_Mib->cwmp_WlanConfArray[i].IsConfigured=0;
				AP_Mib->cwmp_WlanConfArray[i].RfBandAvailable=PHYBAND_2G;
				
			}else{
				AP_Mib->cwmp_WlanConfArray[i].InstanceNum=0;
				AP_Mib->cwmp_WlanConfArray[i].RootIdx=0;
				AP_Mib->cwmp_WlanConfArray[i].VWlanIdx=i-1;
				AP_Mib->cwmp_WlanConfArray[i].IsConfigured=0;
				AP_Mib->cwmp_WlanConfArray[i].RfBandAvailable=PHYBAND_5G;
			}
			#else
			if( i > (NUM_VWLAN_INTERFACE) ){
				AP_Mib->cwmp_WlanConfArray[i].InstanceNum=0;
				AP_Mib->cwmp_WlanConfArray[i].RootIdx=1;
				AP_Mib->cwmp_WlanConfArray[i].VWlanIdx=i-(NUM_VWLAN_INTERFACE);
				AP_Mib->cwmp_WlanConfArray[i].IsConfigured=0;
				AP_Mib->cwmp_WlanConfArray[i].RfBandAvailable=PHYBAND_2G;
				
			}else{
				AP_Mib->cwmp_WlanConfArray[i].InstanceNum=0;
				AP_Mib->cwmp_WlanConfArray[i].RootIdx=0;
				AP_Mib->cwmp_WlanConfArray[i].VWlanIdx=i;
				AP_Mib->cwmp_WlanConfArray[i].IsConfigured=0;
				AP_Mib->cwmp_WlanConfArray[i].RfBandAvailable=PHYBAND_2G;
			}
			#endif
		}
	}
}
#endif



#ifdef CONFIG_RTL_TRI_BAND
PHYBAND_TYPE_T wlanPhyBandDef[] = {PHYBAND_5G, PHYBAND_5G, PHYBAND_2G}; /* phybandcheck */
#else
#ifdef CONFIG_RTL_92D_SUPPORT
PHYBAND_TYPE_T wlanPhyBandDef[] = {PHYBAND_5G, PHYBAND_2G}; /* phybandcheck */
#else
PHYBAND_TYPE_T wlanPhyBandDef[] = {PHYBAND_2G, PHYBAND_2G}; /* phybandcheck */
#endif
#endif
/* read hw nic0 addr */
#define HW_NIC0_ADDR_OFFSET		7	
#define HW_WLAN_SETTING_OFFSET  	19

#ifdef RTL_DEF_SETTING_IN_FW

static int writeDefault(WRITE_DEFAULT_TYPE_T isAll)
{
	char * ptr=NULL;
	
	int ret=0;

	if(isAll==DEFAULT_HW
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		|| isAll==DEFAULT_BLUETOOTHHW
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		|| isAll==DEFAULT_CUSTOMERHW
#endif
	)
	{
		/*flash default-hw*/
		return updateHardcodeMib(isAll);
	}
	
	if(isAll==DEFAULT_ALL)
	{
		/*flash default. should only hardcode hw setting, then go through using default setting to 
		  *overwrite current setting
		  */
		updateHardcodeMib(DEFAULT_HW);
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		updateHardcodeMib(DEFAULT_BLUETOOTHHW);
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		updateHardcodeMib(DEFAULT_CUSTOMERHW);
#endif
	}

	/*flash reset,default-sw*/
	flash_write(def_setting_data,CURRENT_SETTING_OFFSET,sizeof(def_setting_data));
	
	/*init first, then call reinit to load new value*/
	if ( !apmib_init()) {
		printf("Initialize AP MIB failed!\n");
	}
	ret=apmib_reinit();

	//printf("write default success!\n");
	return ret;
	

}
int updateHardcodeMib(WRITE_DEFAULT_TYPE_T isAll)
#else
static int writeDefault(WRITE_DEFAULT_TYPE_T isAll)
#endif
{
#ifdef HEADER_LEN_INT
	HW_PARAM_HEADER_T hwHeader;
#endif
	PARAM_HEADER_T header;
	APMIB_T mib;
	APMIB_Tp pMib=&mib;
	HW_SETTING_T hwmib;
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
	BLUETOOTH_HW_SETTING_T bluetoothHwMib;
	char bluetooth_hw_setting[6]={0};
	int comp_bluetooth_hw_setting = 0;
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
	CUSTOMER_HW_SETTING_T customerHwMib;
	char customer_hw_setting[6]={0};
	int comp_customer_hw_setting = 0;

#endif
	char *data=NULL;
	int status, fh, offset, i, idx,j;
	unsigned char checksum;
//	unsigned char buff[sizeof(APMIB_T)+sizeof(checksum)+1];
	unsigned char *buff;
#ifdef VLAN_CONFIG_SUPPORTED
	int vlan_entry=0;
#endif
#ifdef CONFIG_RTL_ETH_802DOT1X_SUPPORT
	int dot1x_entry=0;
#endif
#ifdef COMPRESS_MIB_SETTING
	//unsigned char* pContent = NULL;

	//COMPRESS_MIB_HEADER_T compHeader;
	//unsigned char *expPtr, *compPtr;
	//unsigned int expLen = header.len+sizeof(PARAM_HEADER_T);
	//unsigned int compLen;
	//unsigned int real_size = 0;
	//int zipRate=0;
#endif // #ifdef COMPRESS_MIB_SETTING

#ifdef MIB_TLV
	unsigned char *pfile = NULL;
	unsigned char *mib_tlv_data = NULL;
	unsigned int tlv_content_len = 0;
	unsigned int mib_tlv_max_len = 0;
#endif

	char hw_setting_start[6];	
	int comp_hw_setting = 0;
	int vlanIdx = 0;
#if defined(CONFIG_APP_APPLE_MFI_WAC)
	unsigned char tmp1[16];	
	//apmib_init();
#endif	

	//buff=calloc(1, 0x6000);
	buff=calloc(1, HW_SETTING_SECTOR_LEN + DEFAULT_SETTING_SECTOR_LEN + 
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		BLUETOOTH_HW_SETTING_SECTOR_LEN+
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		CUSTOMER_HW_SETTING_SECTOR_LEN+
#endif
		CURRENT_SETTING_SECTOR_LEN);
	if ( buff == NULL ) {
		printf("Allocate buffer failed!\n");
		return -1;
	}

#ifdef __mips__
#ifdef CONFIG_MTD_NAND
	fh = open(FLASH_DEVICE_SETTING, O_RDWR|O_CREAT);
#else
	fh = open(FLASH_DEVICE_NAME, O_RDWR);
#endif
#endif

#ifdef __i386__
	fh = open(FLASH_DEVICE_NAME, O_RDWR|O_CREAT|O_TRUNC);
	write(fh, buff, HW_SETTING_SECTOR_LEN + DEFAULT_SETTING_SECTOR_LEN +
		CURRENT_SETTING_SECTOR_LEN);
	//write(fh, buff, 0x6000);
#endif
	if ( fh == -1 ) {
		printf("create file failed!\n");
		return -1;
	}
	if (isAll==DEFAULT_ALL || isAll==DEFAULT_HW) {
		// write hw setting
#ifdef HEADER_LEN_INT
		sprintf((char *)hwHeader.signature, "%s%02d", HW_SETTING_HEADER_TAG, HW_SETTING_VER);
		hwHeader.len = sizeof(hwmib) + sizeof(checksum);
#else
		sprintf((char *)header.signature, "%s%02d", HW_SETTING_HEADER_TAG, HW_SETTING_VER);
		header.len = sizeof(hwmib) + sizeof(checksum);
#endif
		memset((char *)&hwmib, '\0', sizeof(hwmib));
		hwmib.boardVer = 1;
#if defined(CONFIG_RTL_8196B)
		memcpy(hwmib.nic0Addr, "\x0\xe0\x4c\x81\x96\xb1", 6);
		memcpy(hwmib.nic1Addr, "\x0\xe0\x4c\x81\x96", 6);
		hwmib.nic1Addr[5] = 0xb1 + NUM_WLAN_MULTIPLE_SSID;
#elif defined(CONFIG_RTL_8198C)||defined(CONFIG_RTL_8196C) || defined(CONFIG_RTL_8198) || defined(CONFIG_RTL_819XD) || defined(CONFIG_RTL_8196E) || defined(CONFIG_RTL_8198B) || defined(CONFIG_RTL_8197F)
	        memcpy(hwmib.nic0Addr, "\x0\xe0\x4c\x81\x96\xc1", 6);
		memcpy(hwmib.nic1Addr, "\x0\xe0\x4c\x81\x96", 6);
		hwmib.nic1Addr[5] = 0xc1 + NUM_WLAN_MULTIPLE_SSID;
#else
//!CONFIG_RTL_8196B =>rtl8651c+rtl8190
		memcpy(hwmib.nic0Addr, "\x0\xe0\x4c\x86\x51\xd1", 6);
		memcpy(hwmib.nic1Addr, "\x0\xe0\x4c\x86\x51", 6);
		hwmib.nic1Addr[5] = 0xd1 + NUM_WLAN_MULTIPLE_SSID;		
#endif	
		// set RF parameters
		for (idx=0; idx<NUM_WLAN_INTERFACE; idx++) {
#if defined(CONFIG_RTL_8196B)	
			memcpy(hwmib.wlan[idx].macAddr, "\x0\xe0\x4c\x81\x96", 5);		
			hwmib.wlan[idx].macAddr[5] = 0xb1 + idx;			
			
			memcpy(hwmib.wlan[idx].macAddr1, "\x0\xe0\x4c\x81\x96", 5);		
			hwmib.wlan[idx].macAddr1[5] = 0xb1 + 1;	
			
			memcpy(hwmib.wlan[idx].macAddr2, "\x0\xe0\x4c\x81\x96", 5);		
			hwmib.wlan[idx].macAddr2[5] = 0xb1 + 2;	
			
			memcpy(hwmib.wlan[idx].macAddr3, "\x0\xe0\x4c\x81\x96", 5);		
			hwmib.wlan[idx].macAddr3[5] = 0xb1 + 3;	
			
			memcpy(hwmib.wlan[idx].macAddr4, "\x0\xe0\x4c\x81\x96", 5);		
			hwmib.wlan[idx].macAddr4[5] = 0xb1 + 4;			

			memcpy(hwmib.wlan[idx].macAddr5, "\x0\xe0\x4c\x81\x96", 5);		
			hwmib.wlan[idx].macAddr5[5] = 0xb1 + 5;	
			
			memcpy(hwmib.wlan[idx].macAddr6, "\x0\xe0\x4c\x81\x96", 5);		
			hwmib.wlan[idx].macAddr6[5] = 0xb1 + 6;	
			
			memcpy(hwmib.wlan[idx].macAddr7, "\x0\xe0\x4c\x81\x96", 5);		
			hwmib.wlan[idx].macAddr7[5] = 0xb1 + 7;		
			hwmib.wlan[idx].regDomain = FCC;
			hwmib.wlan[idx].rfType = 10;
			hwmib.wlan[idx].xCap = 0;
			hwmib.wlan[idx].Ther = 0;
			for (i=0; i<MAX_CCK_CHAN_NUM; i++)
				hwmib.wlan[idx].txPowerCCK[i] = 0;
			for (i=0; i<MAX_OFDM_CHAN_NUM; i++)
				hwmib.wlan[idx].txPowerOFDM_HT_OFDM_1S[i] = 0;
			for (i=0; i<MAX_OFDM_CHAN_NUM; i++)
				hwmib.wlan[idx].txPowerOFDM_HT_OFDM_2S[i] = 0;
			hwmib.wlan[idx].LOFDMPwDiffA = 0;
			hwmib.wlan[idx].LOFDMPwDiffB = 0;
			hwmib.wlan[idx].TSSI1 = 0;
			hwmib.wlan[idx].TSSI2 = 0;
#elif defined(CONFIG_RTL_8198C)||defined(CONFIG_RTL_8196C) || defined(CONFIG_RTL_8198) || defined(CONFIG_RTL_819XD) || defined(CONFIG_RTL_8196E) || defined(CONFIG_RTL_8198B) || defined(CONFIG_RTL_8197F)
			memcpy(hwmib.wlan[idx].macAddr, "\x0\xe0\x4c\x81\x96", 5);		
			hwmib.wlan[idx].macAddr[5] = 0xc1 + (idx*16);			
			
			memcpy(hwmib.wlan[idx].macAddr1, "\x0\xe0\x4c\x81\x96", 5);		
			hwmib.wlan[idx].macAddr1[5] = hwmib.wlan[idx].macAddr[5] + 1;	
			
			memcpy(hwmib.wlan[idx].macAddr2, "\x0\xe0\x4c\x81\x96", 5);		
			hwmib.wlan[idx].macAddr2[5] = hwmib.wlan[idx].macAddr[5] + 2;	
			
			memcpy(hwmib.wlan[idx].macAddr3, "\x0\xe0\x4c\x81\x96", 5);		
			hwmib.wlan[idx].macAddr3[5] = hwmib.wlan[idx].macAddr[5] + 3;	
			
			memcpy(hwmib.wlan[idx].macAddr4, "\x0\xe0\x4c\x81\x96", 5);		
			hwmib.wlan[idx].macAddr4[5] = hwmib.wlan[idx].macAddr[5] + 4;			

			memcpy(hwmib.wlan[idx].macAddr5, "\x0\xe0\x4c\x81\x96", 5);		
			hwmib.wlan[idx].macAddr5[5] = hwmib.wlan[idx].macAddr[5] + 5;	
			
			memcpy(hwmib.wlan[idx].macAddr6, "\x0\xe0\x4c\x81\x96", 5);		
			hwmib.wlan[idx].macAddr6[5] = hwmib.wlan[idx].macAddr[5] + 6;	
			
			memcpy(hwmib.wlan[idx].macAddr7, "\x0\xe0\x4c\x81\x96", 5);		
			hwmib.wlan[idx].macAddr7[5] = hwmib.wlan[idx].macAddr[5] + 7;		
			hwmib.wlan[idx].regDomain = FCC;
			hwmib.wlan[idx].rfType = 10;
			hwmib.wlan[idx].xCap = 0;
			hwmib.wlan[idx].Ther = 0;
			#if defined(CONFIG_APP_APPLE_MFI_WAC)
			hwmib.wlan[idx].ledType = 7;
			#endif
/*
#if defined(CONFIG_RTL_8196C)
                        hwmib.wlan[idx].trswitch = 0;
#elif defined(CONFIG_RTL_8198)
                        hwmib.wlan[idx].trswitch = 1;
#endif	
*/
			for (i=0; i<MAX_2G_CHANNEL_NUM_MIB; i++)
				hwmib.wlan[idx].pwrlevelCCK_A[i] = 0;
				
			for (i=0; i<MAX_2G_CHANNEL_NUM_MIB; i++)
				hwmib.wlan[idx].pwrlevelCCK_B[i] = 0;
				
			for (i=0; i<MAX_2G_CHANNEL_NUM_MIB; i++)
				hwmib.wlan[idx].pwrlevelHT40_1S_A[i] = 0;	
				
			for (i=0; i<MAX_2G_CHANNEL_NUM_MIB; i++)
				hwmib.wlan[idx].pwrlevelHT40_1S_B[i] = 0;	
				
			for (i=0; i<MAX_2G_CHANNEL_NUM_MIB; i++)
				hwmib.wlan[idx].pwrdiffHT40_2S[i] = 0;	
				
			for (i=0; i<MAX_2G_CHANNEL_NUM_MIB; i++)
				hwmib.wlan[idx].pwrdiffHT20[i] = 0;	
				
			for (i=0; i<MAX_2G_CHANNEL_NUM_MIB; i++)
				hwmib.wlan[idx].pwrdiffOFDM[i] = 0;	
				
			hwmib.wlan[idx].TSSI1 = 0;
			hwmib.wlan[idx].TSSI2 = 0;
			
			for (i=0; i<MAX_5G_CHANNEL_NUM_MIB; i++)
				hwmib.wlan[idx].pwrlevel5GHT40_1S_A[i] = 0;
			for (i=0; i<MAX_5G_CHANNEL_NUM_MIB; i++)
				hwmib.wlan[idx].pwrlevel5GHT40_1S_B[i] = 0;
			for (i=0; i<MAX_5G_CHANNEL_NUM_MIB; i++)
				hwmib.wlan[idx].pwrdiff5GHT40_2S[i] = 0;		
			for (i=0; i<MAX_5G_CHANNEL_NUM_MIB; i++)
				hwmib.wlan[idx].pwrdiff5GHT20[i] = 0;
			for (i=0; i<MAX_5G_CHANNEL_NUM_MIB; i++)
				hwmib.wlan[idx].pwrdiff5GOFDM[i] = 0;	

#if defined(CONFIG_RTL_8812_SUPPORT) 
			// 5G

			for (i=0; i<MAX_5G_DIFF_NUM; i++)
				hwmib.wlan[idx].pwrdiff_5G_20BW1S_OFDM1T_A[i] = 0;
			for (i=0; i<MAX_5G_DIFF_NUM; i++)
				hwmib.wlan[idx].pwrdiff_5G_40BW2S_20BW2S_A[i] = 0;
			for (i=0; i<MAX_5G_DIFF_NUM; i++)
				hwmib.wlan[idx].pwrdiff_5G_80BW1S_160BW1S_A[i] = 0;		
			for (i=0; i<MAX_5G_DIFF_NUM; i++)
				hwmib.wlan[idx].pwrdiff_5G_80BW2S_160BW2S_A[i] = 0;

			for (i=0; i<MAX_5G_DIFF_NUM; i++)
				hwmib.wlan[idx].pwrdiff_5G_20BW1S_OFDM1T_B[i] = 0;
			for (i=0; i<MAX_5G_DIFF_NUM; i++)
				hwmib.wlan[idx].pwrdiff_5G_40BW2S_20BW2S_B[i] = 0;
			for (i=0; i<MAX_5G_DIFF_NUM; i++)
				hwmib.wlan[idx].pwrdiff_5G_80BW1S_160BW1S_B[i] = 0;		
			for (i=0; i<MAX_5G_DIFF_NUM; i++)
				hwmib.wlan[idx].pwrdiff_5G_80BW2S_160BW2S_B[i] = 0;

			// 2G

			for (i=0; i<MAX_2G_CHANNEL_NUM_MIB; i++)
				hwmib.wlan[idx].pwrdiff_20BW1S_OFDM1T_A[i] = 0;
			for (i=0; i<MAX_2G_CHANNEL_NUM_MIB; i++)
				hwmib.wlan[idx].pwrdiff_40BW2S_20BW2S_A[i] = 0;
			
			for (i=0; i<MAX_2G_CHANNEL_NUM_MIB; i++)
				hwmib.wlan[idx].pwrdiff_20BW1S_OFDM1T_B[i] = 0;
			for (i=0; i<MAX_2G_CHANNEL_NUM_MIB; i++)
				hwmib.wlan[idx].pwrdiff_40BW2S_20BW2S_B[i] = 0;
#endif


			hwmib.wlan[idx].trswpape_C9 = 0;//0xAA;
			hwmib.wlan[idx].trswpape_CC = 0;//0xAF;
			
			hwmib.wlan[idx].target_pwr = 0;
			hwmib.wlan[idx].pa_type = 0;
#else
//!CONFIG_RTL_8196B => rtl8651c+rtl8190
			memcpy(hwmib.wlan[idx].macAddr, "\x0\xe0\x4c\x86\x51", 5);		
			hwmib.wlan[idx].macAddr[5] = 0xd1 + idx;		
			memcpy(hwmib.wlan[idx].macAddr1, "\x0\xe0\x4c\x86\x51", 5);		
			hwmib.wlan[idx].macAddr1[5] = 0xd1 + 1;	
			memcpy(hwmib.wlan[idx].macAddr2, "\x0\xe0\x4c\x86\x51", 5);		
			hwmib.wlan[idx].macAddr2[5] = 0xd1 + 2;	
			memcpy(hwmib.wlan[idx].macAddr3, "\x0\xe0\x4c\x86\x51", 5);		
			hwmib.wlan[idx].macAddr3[5] = 0xd1 + 3;	
			memcpy(hwmib.wlan[idx].macAddr4, "\x0\xe0\x4c\x86\x51", 5);		
			hwmib.wlan[idx].macAddr4[5] = 0xd1 + 4;	
			hwmib.wlan[idx].regDomain = FCC;
			//hwmib.wlan[idx].rfType = (unsigned char)RF_ZEBRA;
			hwmib.wlan[idx].rfType = 10;
			hwmib.wlan[idx].ledType = 3;
							
			hwmib.wlan[idx].antDiversity = (unsigned char)0; // disabled
			hwmib.wlan[idx].txAnt = 0;
			hwmib.wlan[idx].initGain = 4;
			hwmib.wlan[idx].xCap = 0;
			hwmib.wlan[idx].LOFDMPwDiff = 0;
//			hwmib.wlan[idx].AntPwDiff_B = 0;
			hwmib.wlan[idx].AntPwDiff_C = 0;
//			hwmib.wlan[idx].AntPwDiff_D = 0;
			hwmib.wlan[idx].TherRFIC = 0;
			//strcpy(hwmib.wlan[idx].wscPin, "12345670"); //move to hw setting
			for (i=0; i<MAX_CCK_CHAN_NUM; i++)
				hwmib.wlan[idx].txPowerCCK[i] = 0;
			for (i=0; i<MAX_OFDM_CHAN_NUM; i++)
				hwmib.wlan[idx].txPowerOFDM[i] = 0;
#endif
			}

#ifdef _LITTLE_ENDIAN_
	swap_mib_value(&hwmib,HW_SETTING);
#endif

#ifdef MIB_TLV
#if 1	
	if ( flash_read(hw_setting_start, HW_SETTING_OFFSET, 6)==0 ) {
			printf("Read hw setting header failed!\n");					
	}
	else 
	{
		//printf("===,the start char from HW_SETTING_OFFSET is %c%c%c%c%c%c\n",
		//		hw_setting_start[0], hw_setting_start[1], hw_setting_start[2], 
		//		hw_setting_start[3], hw_setting_start[4], hw_setting_start[5]);

		if (!memcmp(hw_setting_start, HW_SETTING_HEADER_TAG, TAG_LEN) ||
			!memcmp(hw_setting_start, HW_SETTING_HEADER_FORCE_TAG, TAG_LEN) ||
			!memcmp(hw_setting_start, HW_SETTING_HEADER_UPGRADE_TAG, TAG_LEN))
		{
			comp_hw_setting = 0;
		}
		else
		{
			/*write HW setting uncompress even existed is compressed*/
			comp_hw_setting = 0;
		}
	}
#endif
 
		if(comp_hw_setting == 1)
		{
//mib_display_data_content(HW_SETTING, &hwmib, sizeof(HW_SETTING_T));	

		mib_tlv_max_len = mib_get_setting_len(HW_SETTING)*4;

		pfile = malloc(mib_tlv_max_len);
		if(pfile != NULL && mib_tlv_save(HW_SETTING, (void*)&hwmib, pfile, &tlv_content_len) == 1)
		{
 
			mib_tlv_data = malloc(tlv_content_len+1); // 1:checksum
			if(mib_tlv_data != NULL)
			{
				memcpy(mib_tlv_data, pfile, tlv_content_len);
			}
				
			free(pfile);

		}
		
 		if(mib_tlv_data != NULL)
		{
		
 #ifdef HEADER_LEN_INT
			hwHeader.len = tlv_content_len+1;
			data = (char *)mib_tlv_data;
			checksum = CHECKSUM((unsigned char *)data, hwHeader.len-1);		
#else
			header.len = tlv_content_len+1;
			data = (char *)mib_tlv_data;
			checksum = CHECKSUM((unsigned char *)data, header.len-1);
#endif
			data[tlv_content_len] = CHECKSUM((unsigned char *)data, tlv_content_len);
//mib_display_tlv_content(HW_SETTING, data, header.len);				
		}
		}
		else
		{
#endif //#ifdef MIB_TLV
		data = (char *)&hwmib;
 
#ifdef HEADER_LEN_INT
		checksum = CHECKSUM((unsigned char *)data, hwHeader.len-1);	
#else
		checksum = CHECKSUM((unsigned char *)data, header.len-1);
#endif
#ifdef MIB_TLV
		}
#endif
		lseek(fh, HW_SETTING_OFFSET, SEEK_SET);
#ifdef COMPRESS_MIB_SETTING
		if(comp_hw_setting == 1)
		{
#ifdef HEADER_LEN_INT
		if(flash_mib_compress_write(HW_SETTING, data, &hwHeader, &checksum) == 1) 
#else
		if(flash_mib_compress_write(HW_SETTING, data, &header, &checksum) == 1)
#endif
		{
			COMP_TRACE(stderr,"\r\n flash_mib_compress_write HW_SETTING DONE, __[%s-%u]", __FILE__,__LINE__);			
		}
		}
		else
		{

#endif //#ifdef COMPRESS_MIB_SETTING	
		
#ifdef _LITTLE_ENDIAN_
#ifdef HEADER_LEN_INT
		hwHeader.len=WORD_SWAP(hwHeader.len);
#else
		header.len=WORD_SWAP(hwHeader.len);
#endif
#endif
		
 #ifdef HEADER_LEN_INT
		if ( write(fh, (const void *)&hwHeader, sizeof(hwHeader))!=sizeof(hwHeader) )
#else
		if ( write(fh, (const void *)&header, sizeof(header))!=sizeof(header) ) 
#endif
		{
			printf("write hs header failed!\n");
			return -1;
		}
		if ( write(fh, (const void *)&hwmib, sizeof(hwmib))!=sizeof(hwmib) ) {
			printf("write hs MIB failed!\n");
			return -1;
		}
		if ( write(fh, (const void *)&checksum, sizeof(checksum))!=sizeof(checksum) ) {
			printf("write hs checksum failed!\n");
			return -1;
		}
#ifdef COMPRESS_MIB_SETTING
		}
#endif	
		
 #ifdef MIB_TLV
		if(comp_hw_setting == 1)
		{
		if(mib_tlv_data != NULL)
			free(mib_tlv_data);
		}
#endif
		close(fh);
		sync();
		
 		if(isAll==DEFAULT_HW)
		{
		
 			offset = HW_SETTING_OFFSET;
#ifdef COMPRESS_MIB_SETTING
			if(comp_hw_setting == 1)
			{

			if(flash_mib_checksum_ok(offset) == 1)
			{
				COMP_TRACE(stderr,"\r\n HW_SETTING hecksum_ok\n");
			}
			}
			else
			{
				if(comp_hw_setting == 1)
				{
				fprintf(stderr,"flash mib checksum error!\n");
				}
#endif // #ifdef COMPRESS_MIB_SETTING	
#ifdef HEADER_LEN_INT
			if ( flash_read((char *)&hwHeader, offset, sizeof(hwHeader)) == 0) 
			{
				printf("read hs header failed!\n");
				return -1;
			}
			offset += sizeof(hwHeader);
			hwHeader.len=WORD_SWAP(hwHeader.len);
			if ( flash_read((char *)buff, offset, hwHeader.len) == 0) {
				printf("read hs MIB failed!\n");
				return -1;
			}
			status = CHECKSUM_OK(buff, hwHeader.len);
			if ( !status) {
				printf("hs Checksum error!\n");
				return -1;
			}
#else
			if ( flash_read((char *)&header, offset, sizeof(header)) == 0) 
			{
				printf("read hs header failed!\n");
				return -1;
			}
			offset += sizeof(header);
			header.len=WORD_SWAP(header.len);
			if ( flash_read((char *)buff, offset, header.len) == 0) {
				printf("read hs MIB failed!\n");
				return -1;
			}
			status = CHECKSUM_OK(buff, header.len);
			if ( !status) {
				printf("hs Checksum error!\n");
				return -1;
			}
#endif

#ifdef COMPRESS_MIB_SETTING	
			}
#endif	
			free(buff);
#if CONFIG_APMIB_SHARED_MEMORY == 1	
			apmib_sem_lock();
			apmib_load_hwconf();			
			apmib_sem_unlock();
#endif		
			
 			return 0;

		}

		
 #ifdef CONFIG_MTD_NAND
		fh = open(FLASH_DEVICE_SETTING, O_RDWR|O_CREAT);
#else		
		fh = open(FLASH_DEVICE_NAME, O_RDWR);
#endif
	}

 
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
	 if(isAll==DEFAULT_ALL||isAll==DEFAULT_BLUETOOTHHW)
	 {		 
	 	sprintf((char *)hwHeader.signature, "%s%02d", BLUETOOTH_HW_SETTING_HEADER_TAG, BLUETOOTH_HW_SETTING_VER);
		hwHeader.len = sizeof(bluetoothHwMib) + sizeof(checksum);
		 bzero(&bluetoothHwMib,sizeof(BLUETOOTH_HW_SETTING_T));
		 memcpy(bluetoothHwMib.btAddr,"\x0\xe0\x4c\x81\x96",5);
		// memcpy(bluetoothHwMib.txPowerIdx,0,6);
		 bluetoothHwMib.thermalVal=0;
	 
#ifdef MIB_TLV
	 
			 if ( flash_read(bluetooth_hw_setting, BLUETOOTH_HW_SETTING_OFFSET, 6)==0 ) {
					 printf("Read bluetooth hw setting header failed!\n");					 
			 }
			 else 
			 {
				 //printf("===,the start char from HW_SETTING_OFFSET is %c%c%c%c%c%c\n",
				 // 	 hw_setting_start[0], hw_setting_start[1], hw_setting_start[2], 
				 // 	 hw_setting_start[3], hw_setting_start[4], hw_setting_start[5]);
 
				 if (memcmp(bluetooth_hw_setting, BLUETOOTH_HW_SETTING_HEADER_TAG, TAG_LEN)==0)
				 {
					 comp_bluetooth_hw_setting = 0;
				 }
				 else
				 {
					 
					 comp_bluetooth_hw_setting = 0;
				 }
			 }
 
				 if(comp_bluetooth_hw_setting == 1)
				 {
			 //mib_display_data_content(HW_SETTING, &hwmib, sizeof(HW_SETTING_T));	 
 
					 mib_tlv_max_len = mib_get_setting_len(BLUETOOTH_HW_SETTING)*4;
 
					 pfile = malloc(mib_tlv_max_len);
					 if(pfile != NULL && mib_tlv_save(BLUETOOTH_HW_SETTING, (void*)&bluetoothHwMib, pfile, &tlv_content_len) == 1)
					 {
 
						 mib_tlv_data = malloc(tlv_content_len+1); // 1:checksum
						 if(mib_tlv_data != NULL)
						 {
							 memcpy(mib_tlv_data, pfile, tlv_content_len);
						 }
							 
						 free(pfile);
 
					 }
					 
					 if(mib_tlv_data != NULL)
					 {
						 sprintf((char *)hwHeader.signature, "%s%02d", BLUETOOTH_HW_SETTING_HEADER_TAG, BLUETOOTH_HW_SETTING_VER);
						 hwHeader.len = tlv_content_len+1;
						 data = (char *)mib_tlv_data;
						 checksum = CHECKSUM((unsigned char *)data, hwHeader.len-1);
 
						 data[tlv_content_len] = CHECKSUM((unsigned char *)data, tlv_content_len);
			 //mib_display_tlv_content(HW_SETTING, data, header.len);				 
					 }
				 }
				 else
				 {
#endif //#ifdef MIB_TLV
					 data = (char *)&bluetoothHwMib;
					 checksum = CHECKSUM((unsigned char *)data, hwHeader.len-1);
 
#ifdef MIB_TLV
				 }
#endif
				 lseek(fh, BLUETOOTH_HW_SETTING_OFFSET, SEEK_SET);
#ifdef COMPRESS_MIB_SETTING
				 if(comp_bluetooth_hw_setting == 1)
				 {
					 if(flash_mib_compress_write(BLUETOOTH_HW_SETTING, data, &hwHeader, &checksum) == 1)
					 {
						 COMP_TRACE(stderr,"\r\n flash_mib_compress_write BLUETOOTH HW_SETTING DONE, header.len=%d sig=%s __[%s-%u]",hwHeader.len,hwHeader.signature, __FILE__,__LINE__);		 
					 }
				 }
				 else
				 {
 
#endif //#ifdef COMPRESS_MIB_SETTING

	
#ifdef _LITTLE_ENDIAN_
					hwHeader.len=WORD_SWAP(hwHeader.len);
#endif
					 if ( write(fh, (const void *)&hwHeader, sizeof(hwHeader))!=sizeof(hwHeader) ) 
					 {
						 printf("write bluetooth hs header failed!\n");
						 return -1;
					 }
					 if ( write(fh, (const void *)&bluetoothHwMib, sizeof(bluetoothHwMib))!=sizeof(bluetoothHwMib) ) {
						 printf("write bluetooth hs MIB failed!\n");
						 return -1;
					 }
					 if ( write(fh, (const void *)&checksum, sizeof(checksum))!=sizeof(checksum) ) {
						 printf("write bluetooth hs checksum failed!\n");
						 return -1;
					 }
#ifdef COMPRESS_MIB_SETTING
				 }
#endif	
 
#ifdef MIB_TLV
				 if(comp_bluetooth_hw_setting == 1)
				 {
					 if(mib_tlv_data != NULL)
						 free(mib_tlv_data);
				 }
#endif
				 close(fh);
				 sync();
	 
			 offset = BLUETOOTH_HW_SETTING_OFFSET;
#ifdef COMPRESS_MIB_SETTING
			 if(comp_bluetooth_hw_setting == 1)
			 {
				 if(flash_mib_checksum_ok(offset) == 1)
				 {
					 //COMP_TRACE(stderr,"\r\n bluetooth HW_SETTING checksum_ok\n");
				 }
			 }
			 else
			 {
				 if(comp_bluetooth_hw_setting == 1)
				 {
					 fprintf(stderr,"flash mib checksum error!\n");
				 }
#endif // #ifdef COMPRESS_MIB_SETTING	
	 
				 if ( flash_read((char *)&hwHeader, offset, sizeof(hwHeader)) == 0) 
				 {
					 printf("read hs header failed!\n");
					 return -1;
				 }
				 offset += sizeof(hwHeader);				
#ifdef _LITTLE_ENDIAN_
				hwHeader.len=WORD_SWAP(hwHeader.len);
#endif				
//				 fprintf(stderr,"%s:%d header.len=0x%x\n",__FUNCTION__,__LINE__,header.len);
				 if ( flash_read((char *)buff, offset, hwHeader.len) == 0) {
					 printf("read hs MIB failed!\n");
					 return -1;
				 }
				 status = CHECKSUM_OK(buff, hwHeader.len);
				 if ( !status) {
					 printf("hs Checksum error!\n");
					 return -1;
				 }
 
#ifdef COMPRESS_MIB_SETTING	
			 }
#endif	
			 if(isAll==DEFAULT_BLUETOOTHHW)
			 {
				 free(buff);
#if CONFIG_APMIB_SHARED_MEMORY == 1	
				 apmib_sem_lock();
				 apmib_load_bluetooth_hwconf();
				 apmib_sem_unlock();
#endif		
				 return 0;
			 }
					 
#ifdef CONFIG_MTD_NAND
		 fh = open(FLASH_DEVICE_SETTING, O_RDWR|O_CREAT);
#else		
		 fh = open(FLASH_DEVICE_NAME, O_RDWR);
#endif
			 
	 }
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
	if(isAll==DEFAULT_ALL||isAll==DEFAULT_CUSTOMERHW)
	{		
		bzero(&customerHwMib,sizeof(CUSTOMER_HW_SETTING_T));
		strcpy(customerHwMib.serialNum,"0000000000");
		//strcpy(customerHwMib.wlan0_wpaPSK,"wlan0_wpaPsk");
		//strcpy(customerHwMib.wlan1_wpaPSK,"wlan1_wpaPsk");
	
#ifdef MIB_TLV
	
			if ( flash_read(customer_hw_setting, CUSTOMER_HW_SETTING_OFFSET, 6)==0 ) {
					printf("Read customer hw setting header failed!\n");					
			}
			else 
			{
				//printf("===,the start char from HW_SETTING_OFFSET is %c%c%c%c%c%c\n",
				//		hw_setting_start[0], hw_setting_start[1], hw_setting_start[2], 
				//		hw_setting_start[3], hw_setting_start[4], hw_setting_start[5]);

				if (memcmp(customer_hw_setting, CUSTOMER_HW_SETTING_HEADER_TAG, TAG_LEN)==0)
				{
					comp_customer_hw_setting = 1;
				}
				else
				{
					
					comp_customer_hw_setting = 1;
				}
			}

				if(comp_customer_hw_setting == 1)
				{
			//mib_display_data_content(HW_SETTING, &hwmib, sizeof(HW_SETTING_T));	

					mib_tlv_max_len = mib_get_setting_len(CUSTOMER_HW_SETTING)*4;

					pfile = malloc(mib_tlv_max_len);
					if(pfile != NULL && mib_tlv_save(CUSTOMER_HW_SETTING, (void*)&customerHwMib, pfile, &tlv_content_len) == 1)
					{

						mib_tlv_data = malloc(tlv_content_len+1); // 1:checksum
						if(mib_tlv_data != NULL)
						{
							memcpy(mib_tlv_data, pfile, tlv_content_len);
						}
							
						free(pfile);

					}
					
					if(mib_tlv_data != NULL)
					{
						sprintf((char *)header.signature, "%s%02d", CUSTOMER_HW_SETTING_HEADER_TAG, CUSTOMER_HW_SETTING_VER);
						header.len = tlv_content_len+1;
						data = (char *)mib_tlv_data;
						checksum = CHECKSUM((unsigned char *)data, header.len-1);

						data[tlv_content_len] = CHECKSUM((unsigned char *)data, tlv_content_len);
			//mib_display_tlv_content(HW_SETTING, data, header.len);				
					}
				}
				else
				{
#endif //#ifdef MIB_TLV
					data = (char *)&customerHwMib;
					checksum = CHECKSUM((unsigned char *)data, header.len-1);

#ifdef MIB_TLV
				}
#endif
				lseek(fh, CUSTOMER_HW_SETTING_OFFSET, SEEK_SET);
#ifdef COMPRESS_MIB_SETTING
				if(comp_customer_hw_setting == 1)
				{
					if(flash_mib_compress_write(CUSTOMER_HW_SETTING, data, &header, &checksum) == 1)
					{
						COMP_TRACE(stderr,"\r\n flash_mib_compress_write CUSTOMER HW_SETTING DONE, header.len=%d sig=%s __[%s-%u]",header.len,header.signature, __FILE__,__LINE__);			
					}
				}
				else
				{

#endif //#ifdef COMPRESS_MIB_SETTING
					if ( write(fh, (const void *)&header, sizeof(header))!=sizeof(header) ) 
					{
						printf("write customer hs header failed!\n");
						return -1;
					}
					if ( write(fh, (const void *)&customerHwMib, sizeof(customerHwMib))!=sizeof(customerHwMib) ) {
						printf("write customer hs MIB failed!\n");
						return -1;
					}
					if ( write(fh, (const void *)&checksum, sizeof(checksum))!=sizeof(checksum) ) {
						printf("write customer hs checksum failed!\n");
						return -1;
					}
#ifdef COMPRESS_MIB_SETTING
				}
#endif	

#ifdef MIB_TLV
				if(comp_customer_hw_setting == 1)
				{
					if(mib_tlv_data != NULL)
						free(mib_tlv_data);
				}
#endif
				close(fh);
				sync();
	
			offset = CUSTOMER_HW_SETTING_OFFSET;
#ifdef COMPRESS_MIB_SETTING
			if(comp_customer_hw_setting == 1)
			{
				if(flash_mib_checksum_ok(offset) == 1)
				{
					//COMP_TRACE(stderr,"\r\n customer HW_SETTING checksum_ok\n");
				}
			}
			else
			{
				if(comp_customer_hw_setting == 1)
				{
					fprintf(stderr,"flash mib checksum error!\n");
				}
#endif // #ifdef COMPRESS_MIB_SETTING	
	
				if ( flash_read((char *)&header, offset, sizeof(header)) == 0) 
				{
					printf("read hs header failed!\n");
					return -1;
				}
				offset += sizeof(header);
				if ( flash_read((char *)buff, offset, header.len) == 0) {
					printf("read hs MIB failed!\n");
					return -1;
				}
				status = CHECKSUM_OK(buff, header.len);
				if ( !status) {
					printf("hs Checksum error!\n");
					return -1;
				}

#ifdef COMPRESS_MIB_SETTING	
			}
#endif	
			if(isAll==DEFAULT_CUSTOMERHW)
			{
				free(buff);
#if CONFIG_APMIB_SHARED_MEMORY == 1	
				apmib_sem_lock();
				apmib_load_customer_hwconf();
				apmib_sem_unlock();
#endif		
				return 0;
			}
					
#ifdef CONFIG_MTD_NAND
		fh = open(FLASH_DEVICE_SETTING, O_RDWR|O_CREAT);
#else		
		fh = open(FLASH_DEVICE_NAME, O_RDWR);
#endif
			
	}
#endif
	// write default & current setting

 
	memset(pMib, '\0', sizeof(APMIB_T));

	// give a initial value for testing purpose
/*
#if defined(CONFIG_RTL_8196B)	
	strcpy((char *)pMib->deviceName, "RTL8196b");
#elif defined(CONFIG_RTL_8196C) || defined(CONFIG_RTL_8198)
	strcpy((char *)pMib->deviceName, "RTL8196c");
#elif defined(CONFIG_RTL_819XD)
	strcpy((char *)pMib->deviceName, "RTL819xd");
#elif defined(CONFIG_RTL_8196E)
	strcpy((char *)pMib->deviceName, "RTL8196E");
#elif defined(CONFIG_RTL_8198B)
	strcpy((char *)pMib->deviceName, "RTL8198B");
#elif defined(CONFIG_RTL_8198C)
	strcpy((char *)pMib->deviceName, "RTL8198C");
#else
	strcpy((char *)pMib->deviceName, "RTL865x");
#endif
*/
	
	strcpy((char *)pMib->deviceName, "Realtek Wireless AP");

#if defined(CONFIG_RTL_ULINKER)
	strcpy((char *)pMib->domainName,"UL.Realtek");
#else
	strcpy((char *)pMib->domainName,"Realtek");
#endif

	for (idx=0; idx<NUM_WLAN_INTERFACE; idx++) {
		for (i=0; i<(NUM_VWLAN_INTERFACE+1); i++) {		


        //printf("\n\nsetting P2P default parameter[idx=[%d],i=[%d]]\n\n\n",idx,i);
		/*set default value*/
#if defined(CONFIG_RTL_P2P_SUPPORT)
		pMib->wlan[idx][i].p2p_type = P2P_DEVICE;
		pMib->wlan[idx][i].p2p_intent = 14; 			/*0 <= value <=15*/ 
		pMib->wlan[idx][i].p2p_listen_channel = 11; 	/*should be 1,6,11*/
		pMib->wlan[idx][i].p2p_op_channel = 11;		/*per channel that AP support */	
#endif

#if defined(CONFIG_RTL_92D_SUPPORT) && !defined(CONFIG_RTL_DUAL_PCIESLOT_BIWLAN_D)
			if (idx!=0)
				pMib->wlan[idx][i].wlanDisabled =1;
#endif
#ifdef MBSSID
                       if ((i > 0) && (i<=4))
                       {
                       	    if(0 == idx)
                               	sprintf((char *)pMib->wlan[idx][i].ssid, "RTK 11n AP VAP%d", i);
				    else
					sprintf((char *)pMib->wlan[idx][i].ssid, "RTK 11n AP %d VAP%d", idx,i);
                       }				   
                       else if(i == 0)
#endif			
			  {
#ifdef FOR_DUAL_BAND
					if(0 == idx)
                       	strcpy(pMib->wlan[idx][i].ssid, "RTK 11n AP 5G" );
#if !defined(CONFIG_RTL_TRI_BAND) 
					else if(1 == idx)
                       	strcpy(pMib->wlan[idx][i].ssid, "RTK 11n AP 2.4G" );
#else
					else if(1 == idx)
                       	strcpy(pMib->wlan[idx][i].ssid, "RTK 11n AP 5G 2" );
					else if(2 == idx)
                       	strcpy(pMib->wlan[idx][i].ssid, "RTK 11n AP 2.4G" );
#endif
					else
						sprintf(pMib->wlan[idx][i].ssid, "RTK 11n AP %d",idx);
#else
					if(0 == idx)
                               	strcpy((char *)pMib->wlan[idx][i].ssid, "RTK 11n AP" );
				    else
					sprintf((char *)pMib->wlan[idx][i].ssid, "RTK 11n AP %d",idx);
#endif
#if defined(CONFIG_APP_APPLE_MFI_WAC)
					if(isAll){
						sprintf(((char *)pMib->wlan[idx][i].ssid), "WAC_%02X%02X%02X", hwmib.wlan[idx].macAddr[3], hwmib.wlan[idx].macAddr[4], hwmib.wlan[idx].macAddr[5]);
					}else{
						if(flash_read(tmp1,HW_WLAN_SETTING_OFFSET + sizeof(struct hw_wlan_setting) * idx+HW_SETTING_OFFSET,6) != 0)
							sprintf((char *)pMib->wlan[idx][i].ssid, "WAC_%02X%02X%02X", tmp1[3], tmp1[4], tmp1[5]);
                        else{
							printf("[%s]:%d, get hw_nic0_addr error\n",__func__,__LINE__);
						}
					}
#endif
			  }				   

#ifdef UNIVERSAL_REPEATER
			if(i == NUM_VWLAN_INTERFACE) //rpt interface
			{
				pMib->wlan[idx][i].wlanMode = CLIENT_MODE; //assume root interface is AP
				sprintf((char *)pMib->wlan[idx][i].ssid, "RTK 11n AP RPT%d",idx);				
				if(idx == 0)
					sprintf((char *)pMib->repeaterSSID1, "RTK 11n AP RPT%d",idx);
#if !defined(CONFIG_RTL_TRI_BAND) 
				else
					sprintf((char *)pMib->repeaterSSID2, "RTK 11n AP RPT%d",idx);
#else
				else if(idx == 1)
					sprintf((char *)pMib->repeaterSSID2, "RTK 11n AP RPT%d",idx);
				else if(idx == 2)
					sprintf((char *)pMib->repeaterSSID3, "RTK 11n AP RPT%d",idx);
#endif				
			}
#endif

			if (i == 0)
			{	
#ifdef CONFIG_RTL_92D_SUPPORT
				if(wlanPhyBandDef[idx] == PHYBAND_5G) {
					pMib->wlan[idx][i].controlsideband = 1; // lower
					pMib->wlan[idx][i].channel = 44;
					pMib->wlan[idx][i].basicRates = TX_RATE_6M|TX_RATE_9M|TX_RATE_12M|TX_RATE_18M|TX_RATE_24M|
							TX_RATE_36M|TX_RATE_48M|TX_RATE_54M;				
					pMib->wlan[idx][i].supportedRates = TX_RATE_6M|TX_RATE_9M|TX_RATE_12M|TX_RATE_18M|TX_RATE_24M|
							TX_RATE_36M|TX_RATE_48M|TX_RATE_54M;
				} else
#endif
				{
					pMib->wlan[idx][i].channel = 11;
					pMib->wlan[idx][i].basicRates = TX_RATE_1M|TX_RATE_2M|TX_RATE_5M|TX_RATE_11M;				
					pMib->wlan[idx][i].supportedRates = TX_RATE_1M|TX_RATE_2M|TX_RATE_5M |TX_RATE_11M|
						TX_RATE_6M|TX_RATE_9M|TX_RATE_12M|TX_RATE_18M|TX_RATE_24M|
							TX_RATE_36M|TX_RATE_48M|TX_RATE_54M;
				}
				pMib->wlan[idx][i].wep = WEP_DISABLED;
				pMib->wlan[idx][i].beaconInterval = 100;
				pMib->wlan[idx][i].wepKeyType=1;
				pMib->wlan[idx][i].protectionDisabled=1;
				strcpy((char *)pMib->wlan[idx][i].wdsWepKey, "0000000000" );		
				pMib->wlan[idx][i].rtsThreshold = 2347;
				pMib->wlan[idx][i].aggregation = 1;
				pMib->wlan[idx][i].wpa2Cipher = (unsigned char)WPA_CIPHER_AES; //Keith
#ifdef CONFIG_IEEE80211W				
				pMib->wlan[idx][i].wpa11w = (unsigned char) NO_MGMT_FRAME_PROTECTION;
				pMib->wlan[idx][i].wpa2EnableSHA256 = 0;

#endif				
			}
			if(i <= 4)
			{
				pMib->wlan[idx][i].rateAdaptiveEnabled = 1;

#ifdef CONFIG_RTL_92D_SUPPORT				
				if(wlanPhyBandDef[idx] == PHYBAND_5G){
					pMib->wlan[idx][i].wlanBand = BAND_11A | BAND_11N;
#if defined(CONFIG_RTL_8812_SUPPORT) && !defined(CONFIG_RTL_8812AR_VN_SUPPORT)
					pMib->wlan[idx][i].wlanBand |= BAND_5G_11AC;			
#endif
				}
		        else
#endif
        			pMib->wlan[idx][i].wlanBand = BAND_11BG | BAND_11N;

				pMib->wlan[idx][i].wmmEnabled = 1;
			}
			pMib->wlan[idx][i].preambleType = LONG_PREAMBLE;
			
			//pMib->wlan[idx][i].fragThreshold = 2346;
			//pMib->wlan[idx][i].authType = AUTH_BOTH;
			//pMib->wlan[idx][i].inactivityTime = 30000; // 300 sec
			//pMib->wlan[idx][i].dtimPeriod = 1;
			if(i>4)
			{
				pMib->wlan[idx][i].wpaGroupRekeyTime = 86400;
				pMib->wlan[idx][i].wpaAuth = (unsigned char)WPA_AUTH_PSK;
				pMib->wlan[idx][i].wpaCipher = (unsigned char)WPA_CIPHER_TKIP; //Keith
				pMib->wlan[idx][i].wpa2Cipher = (unsigned char)WPA_CIPHER_AES; //Keith
#ifdef CONFIG_IEEE80211W				
				pMib->wlan[idx][i].wpa11w = (unsigned char) NO_MGMT_FRAME_PROTECTION;
				pMib->wlan[idx][i].wpa2EnableSHA256 = 0;
#endif				
			}
			pMib->wlan[idx][i].rsPort = 1812;
			pMib->wlan[idx][i].accountRsPort = 0;
			pMib->wlan[idx][i].accountRsUpdateDelay = 0;
			pMib->wlan[idx][i].rsMaxRetry = 3;
			pMib->wlan[idx][i].rsIntervalTime = 5;
			pMib->wlan[idx][i].accountRsMaxRetry = 0;
			pMib->wlan[idx][i].accountRsIntervalTime = 0;
#if defined(CONFIG_APP_APPLE_MFI_WAC)
		#ifdef UNIVERSAL_REPEATER
			if(i == NUM_VWLAN_INTERFACE) //rpt interface
			{
				#if defined(WAC_CONFIGURING_REPETER)
				pMib->wlan[idx][i].wlanDisabled = 0;
				#else
				pMib->wlan[idx][i].wlanDisabled = 1;
				#endif
			}else{
			if (i > 0)
				pMib->wlan[idx][i].wlanDisabled = 1;		
			}					
		#else
			if (i > 0)
				pMib->wlan[idx][i].wlanDisabled = 1;
		#endif	
#else		
			if (i > 0)
				pMib->wlan[idx][i].wlanDisabled = 1;		
#endif
#ifdef WLAN_EASY_CONFIG
			pMib->wlan[idx][i].acfMode = MODE_BUTTON;
			pMib->wlan[idx][i].acfAlgReq = ACF_ALGORITHM_WPA2_AES;
			pMib->wlan[idx][i].acfAlgSupp = ACF_ALGORITHM_WPA_TKIP  | ACF_ALGORITHM_WPA2_AES;
			strcpy(pMib->wlan[idx][i].acfScanSSID, "REALTEK_EASY_CONFIG");
#endif

#ifdef WIFI_SIMPLE_CONFIG
	
			//strcpy(pMib->wlan[idx][i].wscSsid, pMib->wlan[idx][i].ssid ); //must be the same as pMib->wlan[idx].ssid
#if !defined(CONFIG_APP_APPLE_MFI_WAC)
			pMib->wlan[idx][i].wscDisable = 0;
#else
			pMib->wlan[idx][i].wscDisable = 1;
#endif			
#endif
#if !defined(CONFIG_APP_APPLE_MFI_WAC)
			pMib->wlan[idx][i].iappDisabled = 0;
#else
			pMib->wlan[idx][i].iappDisabled = 1;
#endif
			// for 11n			
			if (i == 0)
			{
				pMib->wlan[idx][i].aggregation = 1;
#if defined(CONFIG_RTL_8812_SUPPORT)				
				if(wlanPhyBandDef[idx] == PHYBAND_5G)
					pMib->wlan[idx][i].channelbonding = 2;
				else
#endif
					pMib->wlan[idx][i].channelbonding = 1;
				pMib->wlan[idx][i].shortgiEnabled = 1;
				pMib->wlan[idx][i].fragThreshold=2346;	
				pMib->wlan[idx][i].authType = AUTH_BOTH;
				pMib->wlan[idx][i].inactivityTime = 30000; // 300 sec
				pMib->wlan[idx][i].dtimPeriod = 1;
				pMib->wlan[idx][i].wpaGroupRekeyTime = 86400;
				pMib->wlan[idx][i].wpaAuth = (unsigned char)WPA_AUTH_PSK;
				pMib->wlan[idx][i].wpaCipher = (unsigned char)WPA_CIPHER_TKIP; //Keith	
				pMib->wlan[idx][i].wscMethod = 3;
				//strcpy(pMib->wlan[idx].wscPin, "12345670"); //move to hw setting
				pMib->wlan[idx][i].wscAuth = WSC_AUTH_OPEN; //open
				pMib->wlan[idx][i].wscEnc = WSC_ENCRYPT_NONE; //open
				pMib->wlan[idx][i].wscUpnpEnabled = 1;
				pMib->wlan[idx][i].wscRegistrarEnabled = 1;
				#if defined(NEW_SCHEDULE_SUPPORT)
				pMib->wlan[idx][i].scheduleRuleNum = MAX_SCHEDULE_NUM;
				for(j=0;j<MAX_SCHEDULE_NUM;j++)
				{
					sprintf(pMib->wlan[idx][i].scheduleRuleArray[j].text,"rule_%d",j+1);
				}
				#endif
			}			
			pMib->wlan[idx][i].phyBandSelect = wlanPhyBandDef[idx];
#if defined(CONFIG_RTL_92D_SUPPORT) && defined(CONFIG_RTL_92D_DMDP) && !defined(CONFIG_RTL_DUAL_PCIESLOT_BIWLAN_D) 
			pMib->wlan[idx][i].macPhyMode = DMACDPHY;
#else	
			pMib->wlan[idx][i].macPhyMode = SMACSPHY;
#endif
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL8192E) || defined(CONFIG_RTL_8812_SUPPORT)
			pMib->wlan[idx][i].TxBeamforming = 1; 
#endif
			pMib->wlan[idx][i].LDPCEnabled = 1;
			pMib->wlan[idx][i].STBCEnabled = 1;
			pMib->wlan[idx][i].CoexistEnabled= 0;
			pMib->wlan[idx][i].dtimPeriod= 1;			
		}

#if defined(CONFIG_RTL_ULINKER)
	memcpy(&pMib->wlan[idx][ULINKER_AP_MIB], &pMib->wlan[idx][0], sizeof(CONFIG_WLAN_SETTING_T));
	memcpy(&pMib->wlan[idx][ULINKER_RPT_MIB], &pMib->wlan[idx][0], sizeof(CONFIG_WLAN_SETTING_T));

	memcpy(&pMib->wlan[idx][ULINKER_CL_MIB], &pMib->wlan[idx][0], sizeof(CONFIG_WLAN_SETTING_T));
	pMib->wlan[idx][ULINKER_CL_MIB].wlanMode = CLIENT_MODE;
	strcpy(pMib->wlan[idx][ULINKER_CL_MIB].ssid, "RTK_11n_UL_CL" );
	pMib->wlan[idx][ULINKER_CL_MIB].maccloneEnabled = 1;
#endif


		
	}

#ifdef UNIVERSAL_REPEATER
	//sync vap[NUM_VWLAN_INTERFACE] data to repeater
	strcpy(pMib->repeaterSSID1, pMib->wlan[0][NUM_VWLAN_INTERFACE].ssid);
	pMib->repeaterEnabled1 = (pMib->wlan[0][NUM_VWLAN_INTERFACE].wlanDisabled?0:1);
#if defined(CONFIG_RTL_92D_SUPPORT)
	strcpy(pMib->repeaterSSID2, pMib->wlan[1][NUM_VWLAN_INTERFACE].ssid);
	pMib->repeaterEnabled2= (pMib->wlan[1][NUM_VWLAN_INTERFACE].wlanDisabled?0:1);
#endif
#if defined(CONFIG_RTL_TRI_BAND)
	strcpy(pMib->repeaterSSID3, pMib->wlan[2][NUM_VWLAN_INTERFACE].ssid);
	pMib->repeaterEnabled3= (pMib->wlan[2][NUM_VWLAN_INTERFACE].wlanDisabled?0:1);	
#endif
#endif


#if defined(CONFIG_RTL_819X)
	pMib->wifiSpecific = 2; //Brad modify for 11n wifi test
#endif	
	pMib->ipAddr[0] = 192;
	pMib->ipAddr[1] = 168;
	pMib->ipAddr[2] = 1;
	pMib->ipAddr[3] = 254;

	pMib->subnetMask[0] = 255;
	pMib->subnetMask[1] = 255;
	pMib->subnetMask[2] = 255;
	pMib->subnetMask[3] = 0;

	pMib->dhcpClientStart[0] = 192;
	pMib->dhcpClientStart[1] = 168;
	pMib->dhcpClientStart[2] = 1;
	pMib->dhcpClientStart[3] = 100;

	pMib->dhcpClientEnd[0] = 192;
	pMib->dhcpClientEnd[1] = 168;
	pMib->dhcpClientEnd[2] = 1;
	pMib->dhcpClientEnd[3] = 200;
#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT)
	pMib->dhcp = DHCP_AUTO;

#elif defined(CONFIG_RTL_ULINKER)
	pMib->dhcp = DHCP_AUTO_WAN;
#else
	pMib->dhcp = DHCP_SERVER;
#endif
#ifdef SUPER_NAME_SUPPORT
	strcpy((char *)pMib->superName, "super");
	strcpy((char *)pMib->superPassword, "super");
#endif

	pMib->LanDhcpConfigurable=1;
	
#if defined(CONFIG_RTL_8198_AP_ROOT) || defined(CONFIG_RTL_8197D_AP)
	pMib->ntpEnabled=0;
	pMib->daylightsaveEnabled=0;
	strcpy(pMib->ntpTimeZone,"-8 4");
	pMib->ntpServerIp1[0]=0;
	pMib->ntpServerIp1[1]=0;
	pMib->ntpServerIp1[2]=0;
	pMib->ntpServerIp1[3]=0;
	pMib->ntpServerIp2[0]=0;
	pMib->ntpServerIp2[1]=0;
	pMib->ntpServerIp2[2]=0;
	pMib->ntpServerIp2[3]=0;
#endif
#ifdef HOME_GATEWAY
	pMib->qosAutoUplinkSpeed=1;
	pMib->qosManualUplinkSpeed=512;
	pMib->qosAutoDownLinkSpeed=1;
	pMib->qosManualDownLinkSpeed=512;
	
	pMib->vpnPassthruIPsecEnabled=1;
	pMib->vpnPassthruPPTPEnabled=1;
	pMib->vpnPassthruL2TPEnabled=1;

	pMib->wanIpAddr[0] = 172;
	pMib->wanIpAddr[1] = 1;
	pMib->wanIpAddr[2] = 1;
	pMib->wanIpAddr[3] = 1;

	pMib->wanSubnetMask[0] = 255;
	pMib->wanSubnetMask[1] = 255;
	pMib->wanSubnetMask[2] = 255;
	pMib->wanSubnetMask[3] = 0;

	pMib->wanDefaultGateway[0] = 172;
	pMib->wanDefaultGateway[1] = 1;
	pMib->wanDefaultGateway[2] = 1;
	pMib->wanDefaultGateway[3] = 254;

	pMib->wanDhcp = DHCP_CLIENT;
	pMib->dnsMode = DNS_AUTO;
	pMib->pppIdleTime = 300;
#ifdef MULTI_PPPOE	 

	pMib->pppConnectCount = 1;

	pMib->pppIdleTime2 =300;
	pMib->pppIdleTime3 =300;
	pMib->pppIdleTime4 =300;
	
	pMib->pppMtuSize2 = 1492;
	pMib->pppMtuSize3 = 1492;
	pMib->pppMtuSize4 = 1492;
	
#endif	
	pMib->pptpIdleTime = 300;
	pMib->l2tpIdleTime = 300;
	strcpy((char *)pMib->usb3g_idleTime, "300");
	strcpy((char *)pMib->usb3g_connType, "0");
	pMib->pppMtuSize = 1492;
	pMib->pptpIpAddr[0]=172;
	pMib->pptpIpAddr[1]=1;
	pMib->pptpIpAddr[2]=1;
	pMib->pptpIpAddr[3]=2;
	pMib->pptpSubnetMask[0]=255;
	pMib->pptpSubnetMask[1]=255;
	pMib->pptpSubnetMask[2]=255;
	pMib->pptpSubnetMask[3]=0;
	#if defined(CONFIG_DYNAMIC_WAN_IP)	
	pMib->pptpDefGw[0]=172;
	pMib->pptpDefGw[1]=1;
	pMib->pptpDefGw[2]=1;
	pMib->pptpDefGw[3]=254;
	#endif
	pMib->pptpServerIpAddr[0]=172;
	pMib->pptpServerIpAddr[1]=1;
	pMib->pptpServerIpAddr[2]=1;
	pMib->pptpServerIpAddr[3]=1;			
	pMib->pptpMtuSize = 1460;
	#if defined(CONFIG_DYNAMIC_WAN_IP)	
	pMib->pptpWanIPMode = DYNAMIC_IP;	//PPTP IP dynamic type, 0 - dynamic, 1-static
	#endif
	pMib->l2tpIpAddr[0]=172;
	pMib->l2tpIpAddr[1]=1;
	pMib->l2tpIpAddr[2]=1;
	pMib->l2tpIpAddr[3]=2;
	pMib->l2tpSubnetMask[0]=255;
	pMib->l2tpSubnetMask[1]=255;
	pMib->l2tpSubnetMask[2]=255;
	pMib->l2tpSubnetMask[3]=0;
	#if defined(CONFIG_DYNAMIC_WAN_IP)	
	pMib->l2tpDefGw[0]=172;
	pMib->l2tpDefGw[1]=1;
	pMib->l2tpDefGw[2]=1;
	pMib->l2tpDefGw[3]=254;
	#endif
	pMib->l2tpServerIpAddr[0]=172;
	pMib->l2tpServerIpAddr[1]=1;
	pMib->l2tpServerIpAddr[2]=1;
	pMib->l2tpServerIpAddr[3]=1;	
	pMib->l2tpMtuSize = 1460; /* keith: add l2tp support. 20080515 */
	#if defined(CONFIG_DYNAMIC_WAN_IP)
	pMib->L2tpwanIPMode = DYNAMIC_IP; /* set l2tp default wan ip type to dynamic */
	#else	
	pMib->L2tpwanIPMode = 1; /* keith: add l2tp support. 20080515 */
	#endif
	strcpy((char *)pMib->usb3g_mtuSize,"1490");
	pMib->fixedIpMtuSize = 1500;
	pMib->dhcpMtuSize = 1500;
	pMib->ntpEnabled=0;
	pMib->daylightsaveEnabled=0;
	strcpy((char *)pMib->ntpTimeZone,"-8 4");
	pMib->ntpServerIp1[0]=0;
	pMib->ntpServerIp1[1]=0;
	pMib->ntpServerIp1[2]=0;
	pMib->ntpServerIp1[3]=0;
	pMib->ntpServerIp2[0]=0;
	pMib->ntpServerIp2[1]=0;
	pMib->ntpServerIp2[2]=0;
	pMib->ntpServerIp2[3]=0;
	
	pMib->ddnsEnabled=0;
	pMib->ddnsType=0;
	strcpy((char *)pMib->ddnsDomainName,"host.dyndns.org");

#if defined(CONFIG_RTL_ULINKER)
	pMib->ulinker_cur_mode=255;
	pMib->ulinker_lst_mode=255;
	pMib->ulinker_cur_wl_mode=255;
	pMib->ulinker_lst_wl_mode=255;
	pMib->ulinker_auto=1;
#endif

#ifdef CONFIG_IPV6
	pMib->radvdCfgParam.enabled =0;
	strcpy(pMib->radvdCfgParam.interface.Name,"br0");
	pMib->radvdCfgParam.interface.MaxRtrAdvInterval=600;
	pMib->radvdCfgParam.interface.MinRtrAdvInterval=600*0.33;
	pMib->radvdCfgParam.interface.MinDelayBetweenRAs=3;
	pMib->radvdCfgParam.interface.AdvManagedFlag=0;
	pMib->radvdCfgParam.interface.AdvOtherConfigFlag=0;
	pMib->radvdCfgParam.interface.AdvLinkMTU=1500;
	pMib->radvdCfgParam.interface.AdvReachableTime=0;
	pMib->radvdCfgParam.interface.AdvRetransTimer=0;
	pMib->radvdCfgParam.interface.AdvCurHopLimit=64;
	pMib->radvdCfgParam.interface.AdvDefaultLifetime=600*3;
	strcpy(pMib->radvdCfgParam.interface.AdvDefaultPreference,"medium");
	pMib->radvdCfgParam.interface.AdvSourceLLAddress=1;
	pMib->radvdCfgParam.interface.UnicastOnly=0;
				/*prefix 1*/
	pMib->radvdCfgParam.interface.prefix[0].Prefix[0]=0x2001;
	pMib->radvdCfgParam.interface.prefix[0].Prefix[1]=0x0;
	pMib->radvdCfgParam.interface.prefix[0].Prefix[2]=0x0;
	pMib->radvdCfgParam.interface.prefix[0].Prefix[3]=0x0;
	pMib->radvdCfgParam.interface.prefix[0].Prefix[4]=0x0;
	pMib->radvdCfgParam.interface.prefix[0].Prefix[5]=0x0;
	pMib->radvdCfgParam.interface.prefix[0].Prefix[6]=0x0;
	pMib->radvdCfgParam.interface.prefix[0].Prefix[7]=0x0;
	pMib->radvdCfgParam.interface.prefix[0].PrefixLen=64;
	pMib->radvdCfgParam.interface.prefix[0].AdvOnLinkFlag=1;
	pMib->radvdCfgParam.interface.prefix[0].AdvAutonomousFlag=1;
	pMib->radvdCfgParam.interface.prefix[0].AdvValidLifetime=2592000;
	pMib->radvdCfgParam.interface.prefix[0].AdvPreferredLifetime=604800;
	pMib->radvdCfgParam.interface.prefix[0].AdvRouterAddr=0;
	strcpy(pMib->radvdCfgParam.interface.prefix[0].if6to4,"eth1");
	pMib->radvdCfgParam.interface.prefix[0].enabled=1;
				/*prefix 2*/
	pMib->radvdCfgParam.interface.prefix[1].Prefix[0]=0x2002;
	pMib->radvdCfgParam.interface.prefix[1].Prefix[1]=0;
	pMib->radvdCfgParam.interface.prefix[1].Prefix[2]=0;
	pMib->radvdCfgParam.interface.prefix[1].Prefix[3]=0;
	pMib->radvdCfgParam.interface.prefix[1].Prefix[4]=0;
	pMib->radvdCfgParam.interface.prefix[1].Prefix[5]=0;
	pMib->radvdCfgParam.interface.prefix[1].Prefix[6]=0;
	pMib->radvdCfgParam.interface.prefix[1].Prefix[7]=0;
	pMib->radvdCfgParam.interface.prefix[1].PrefixLen=64;
	pMib->radvdCfgParam.interface.prefix[1].AdvOnLinkFlag=1;
	pMib->radvdCfgParam.interface.prefix[1].AdvAutonomousFlag=1;
	pMib->radvdCfgParam.interface.prefix[1].AdvValidLifetime=2592000;
	pMib->radvdCfgParam.interface.prefix[1].AdvPreferredLifetime=604800;
	pMib->radvdCfgParam.interface.prefix[1].AdvRouterAddr=0;
//	pMib->radvdCfgParam.interface.prefix[1].if6to4="";
	pMib->radvdCfgParam.interface.prefix[1].enabled=1;
#ifdef CONFIG_APP_RADVD_WAN
	pMib->radvdCfgParam_wan.enabled =0;
	strcpy(pMib->radvdCfgParam_wan.interface.Name,"eth1");
	pMib->radvdCfgParam_wan.interface.MaxRtrAdvInterval=600;
	pMib->radvdCfgParam_wan.interface.MinRtrAdvInterval=600*0.33;
	pMib->radvdCfgParam_wan.interface.MinDelayBetweenRAs=3;
	pMib->radvdCfgParam_wan.interface.AdvManagedFlag=0;
	pMib->radvdCfgParam_wan.interface.AdvOtherConfigFlag=0;
	pMib->radvdCfgParam_wan.interface.AdvLinkMTU=1500;
	pMib->radvdCfgParam_wan.interface.AdvReachableTime=0;
	pMib->radvdCfgParam_wan.interface.AdvRetransTimer=0;
	pMib->radvdCfgParam_wan.interface.AdvCurHopLimit=64;
	pMib->radvdCfgParam_wan.interface.AdvDefaultLifetime=600*3;
	strcpy(pMib->radvdCfgParam_wan.interface.AdvDefaultPreference,"medium");
	pMib->radvdCfgParam_wan.interface.AdvSourceLLAddress=1;
	pMib->radvdCfgParam_wan.interface.UnicastOnly=0;
				/*prefix 1*/
	pMib->radvdCfgParam_wan.interface.prefix[0].Prefix[0]=0x2001;
	pMib->radvdCfgParam_wan.interface.prefix[0].Prefix[1]=0x0;
	pMib->radvdCfgParam_wan.interface.prefix[0].Prefix[2]=0x0;
	pMib->radvdCfgParam_wan.interface.prefix[0].Prefix[3]=0x0;
	pMib->radvdCfgParam_wan.interface.prefix[0].Prefix[4]=0x0;
	pMib->radvdCfgParam_wan.interface.prefix[0].Prefix[5]=0x0;
	pMib->radvdCfgParam_wan.interface.prefix[0].Prefix[6]=0x0;
	pMib->radvdCfgParam_wan.interface.prefix[0].Prefix[7]=0x0;
	pMib->radvdCfgParam_wan.interface.prefix[0].PrefixLen=64;
	pMib->radvdCfgParam_wan.interface.prefix[0].AdvOnLinkFlag=1;
	pMib->radvdCfgParam_wan.interface.prefix[0].AdvAutonomousFlag=1;
	pMib->radvdCfgParam_wan.interface.prefix[0].AdvValidLifetime=2592000;
	pMib->radvdCfgParam_wan.interface.prefix[0].AdvPreferredLifetime=604800;
	pMib->radvdCfgParam_wan.interface.prefix[0].AdvRouterAddr=0;
//	strcpy(pMib->radvdCfgParam.interface.prefix[0].if6to4,"eth1");
	pMib->radvdCfgParam_wan.interface.prefix[0].enabled=1;
				/*prefix 2*/
	pMib->radvdCfgParam_wan.interface.prefix[1].Prefix[0]=0x2002;
	pMib->radvdCfgParam_wan.interface.prefix[1].Prefix[1]=0;
	pMib->radvdCfgParam_wan.interface.prefix[1].Prefix[2]=0;
	pMib->radvdCfgParam_wan.interface.prefix[1].Prefix[3]=0;
	pMib->radvdCfgParam_wan.interface.prefix[1].Prefix[4]=0;
	pMib->radvdCfgParam_wan.interface.prefix[1].Prefix[5]=0;
	pMib->radvdCfgParam_wan.interface.prefix[1].Prefix[6]=0;
	pMib->radvdCfgParam_wan.interface.prefix[1].Prefix[7]=0;
	pMib->radvdCfgParam_wan.interface.prefix[1].PrefixLen=64;
	pMib->radvdCfgParam_wan.interface.prefix[1].AdvOnLinkFlag=1;
	pMib->radvdCfgParam_wan.interface.prefix[1].AdvAutonomousFlag=1;
	pMib->radvdCfgParam_wan.interface.prefix[1].AdvValidLifetime=2592000;
	pMib->radvdCfgParam_wan.interface.prefix[1].AdvPreferredLifetime=604800;
	pMib->radvdCfgParam_wan.interface.prefix[1].AdvRouterAddr=0;
//	pMib->radvdCfgParam.interface.prefix[1].if6to4="";
	pMib->radvdCfgParam_wan.interface.prefix[1].enabled=1;
#endif
	
//dnsv6
	pMib->dnsCfgParam.enabled=0;
	strcpy(pMib->dnsCfgParam.routerName,"router.my");
//dhcp6
	pMib->dhcp6sCfgParam.enabled=0;
	strcpy(pMib->dhcp6sCfgParam.DNSaddr6,"2001:db8::35");
	strcpy(pMib->dhcp6sCfgParam.addr6PoolS,"2001:db8:1:2::1000");
	strcpy(pMib->dhcp6sCfgParam.addr6PoolE,"2001:db8:1:2::2000");
	strcpy(pMib->dhcp6sCfgParam.interfaceNameds,"br0");
//ipv6 addr
	pMib->addrIPv6CfgParam.enabled=0;
	pMib->addrIPv6CfgParam.prefix_len[0]=0;
	pMib->addrIPv6CfgParam.prefix_len[1]=0;
	{
		int itemp;
		for(itemp=0;itemp<8;itemp++)
		{
			pMib->addrIPv6CfgParam.addrIPv6[0][itemp]=0;
			pMib->addrIPv6CfgParam.addrIPv6[1][itemp]=0;
		}
	}
//6to4 
	pMib->tunnelCfgParam.enabled=0;
#ifdef TR181_SUPPORT
	pMib->ipv6DhcpcReqAddr=1;
	pMib->ipv6DhcpcSendOptNum=IPV6_DHCPC_SENDOPT_NUM;
	for(i=0;i<IPV6_DHCPC_SENDOPT_NUM;i++)
	{
		pMib->ipv6DhcpcSendOptTbl[i].index=i;		
	}
#endif
#endif
#ifdef TR181_SUPPORT
	pMib->DnsClientEnable=1;
#endif
//#ifdef TR181_SUPPORT
#if 0
	pMib->dnsClientServerNum=DNS_CLIENT_SERVER_NUM;
	for(i=0; i<DNS_CLIENT_SERVER_NUM; i++)
	{
		pMib->dnsClientServerTbl[i].index=i;
	}

#endif

#ifdef RTK_CAPWAP
	pMib->capwapMode = CAPWAP_WTP_ENABLE | CAPWAP_AC_ENABLE;
#endif

#endif

// added by rock /////////////////////////////////////////
#ifdef VOIP_SUPPORT
	flash_voip_default(&pMib->voipCfgParam);
#endif

#ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
	pMib->vlan_wan_enable = 0;
	pMib->vlan_wan_tag = 10;
	pMib->vlan_wan_bridge_enable = 0; 
	pMib->vlan_wan_bridge_tag = 12; 	
	pMib->vlan_wan_bridge_port = 0;
	pMib->vlan_wan_bridge_multicast_enable = 0;
	pMib->vlan_wan_bridge_multicast_tag = 13;
	pMib->vlan_wan_host_enable = 0;
	pMib->vlan_wan_host_tag = 20;
	pMib->vlan_wan_host_pri = 7;	
	pMib->vlan_wan_wifi_root_enable = 0;
	pMib->vlan_wan_wifi_root_tag = 30;
	pMib->vlan_wan_wifi_root_pri = 0;
	pMib->vlan_wan_wifi_vap0_enable = 0;
	pMib->vlan_wan_wifi_vap0_tag = 30;
	pMib->vlan_wan_wifi_vap0_pri = 0;
	pMib->vlan_wan_wifi_vap1_enable = 0;	
	pMib->vlan_wan_wifi_vap1_tag = 30;
	pMib->vlan_wan_wifi_vap1_pri = 0;
	pMib->vlan_wan_wifi_vap2_enable = 0;
	pMib->vlan_wan_wifi_vap2_tag = 30;
	pMib->vlan_wan_wifi_vap2_pri = 0;
	pMib->vlan_wan_wifi_vap3_enable = 0;
	pMib->vlan_wan_wifi_vap3_tag = 30;
	pMib->vlan_wan_wifi_vap3_pri = 0;
#endif

	// SNMP, Forrest added, 2007.10.25.
#ifdef CONFIG_SNMP
	pMib->snmpEnabled = 1;
	sprintf(pMib->snmpName, "%s", "Realtek");
	sprintf(pMib->snmpLocation, "%s", "AP");
	sprintf(pMib->snmpContact, "%s", "Router");
	sprintf(pMib->snmpRWCommunity, "%s", "private");
	sprintf(pMib->snmpROCommunity, "%s", "public");
	pMib->snmpTrapReceiver1[0] = 0;
	pMib->snmpTrapReceiver1[1] = 0;
	pMib->snmpTrapReceiver1[2] = 0;
	pMib->snmpTrapReceiver1[3] = 0;
	pMib->snmpTrapReceiver2[0] = 0;
	pMib->snmpTrapReceiver2[1] = 0;
	pMib->snmpTrapReceiver2[2] = 0;
	pMib->snmpTrapReceiver2[3] = 0;
	pMib->snmpTrapReceiver3[0] = 0;
	pMib->snmpTrapReceiver3[1] = 0;
	pMib->snmpTrapReceiver3[2] = 0;
	pMib->snmpTrapReceiver3[3] = 0;
#endif

#ifdef CONFIG_RTK_MESH
	pMib->wlan[idx][0].meshEnabled = 0;
#ifdef CONFIG_NEW_MESH_UI
	pMib->wlan[idx][0].meshRootEnabled = 0;	
#else
	pMib->wlan[idx][0].meshRootEnabled = 0;		// if meshEnabled default value "1", Here "1" also
#endif
	strcpy(pMib->wlan[idx][0].meshID, "RTK-mesh");
	
#endif // CONFIG_RTK_MESH
#if defined(HOME_GATEWAY) && defined(ROUTE_SUPPORT)
	pMib->natEnabled = 1;
#endif


#ifdef HOME_GATEWAY
#if defined(CONFIG_APP_SAMBA)
	pMib->sambaEnabled = 1;
#else
	pMib->sambaEnabled = 0;
#endif

	pMib->pppSessionNum = 0;
#endif

#ifdef SNMP_SUPPORT
	strcpy(pMib->snmpROcommunity,"public");
	strcpy(pMib->snmpRWcommunity,"private");
#endif
#if defined(VLAN_CONFIG_SUPPORTED)
for(vlan_entry=0;vlan_entry<MAX_IFACE_VLAN_CONFIG;vlan_entry++){
	pMib->VlanConfigArray[vlan_entry].enabled=0;
	pMib->VlanConfigArray[vlan_entry].vlanId = 1;
	pMib->VlanConfigArray[vlan_entry].cfi = 0;
#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT) || defined(CONFIG_RTL_HW_VLAN_SUPPORT)
	pMib->VlanConfigArray[vlan_entry].forwarding_rule = 2;
#endif
}	
pMib->VlanConfigEnabled=0;
pMib->VlanConfigNum=MAX_IFACE_VLAN_CONFIG;
sprintf((char *)pMib->VlanConfigArray[vlanIdx++].netIface, "%s", "eth0");
sprintf((char *)pMib->VlanConfigArray[vlanIdx++].netIface, "%s", "eth2");
sprintf((char *)pMib->VlanConfigArray[vlanIdx++].netIface, "%s", "eth3");
sprintf((char *)pMib->VlanConfigArray[vlanIdx++].netIface, "%s", "eth4");
sprintf((char *)pMib->VlanConfigArray[vlanIdx++].netIface, "%s", "wlan0");
sprintf((char *)pMib->VlanConfigArray[vlanIdx++].netIface, "%s", "wlan0-va0");
sprintf((char *)pMib->VlanConfigArray[vlanIdx++].netIface, "%s", "wlan0-va1");
sprintf((char *)pMib->VlanConfigArray[vlanIdx++].netIface, "%s", "wlan0-va2");
sprintf((char *)pMib->VlanConfigArray[vlanIdx++].netIface, "%s", "wlan0-va3");
sprintf((char *)pMib->VlanConfigArray[vlanIdx++].netIface, "%s", "wlan1");
sprintf((char *)pMib->VlanConfigArray[vlanIdx++].netIface, "%s", "wlan1-va0");
sprintf((char *)pMib->VlanConfigArray[vlanIdx++].netIface, "%s", "wlan1-va1");
sprintf((char *)pMib->VlanConfigArray[vlanIdx++].netIface, "%s", "wlan1-va2");
sprintf((char *)pMib->VlanConfigArray[vlanIdx++].netIface, "%s", "wlan1-va3");
sprintf((char *)pMib->VlanConfigArray[vlanIdx++].netIface, "%s", "eth1");
#if defined(CONFIG_RTL_8198_AP_ROOT) && defined(GMII_ENABLED)
sprintf((char *)pMib->VlanConfigArray[vlanIdx++].netIface, "%s", "eth6");
#endif
#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT)
sprintf(pMib->VlanConfigArray[vlanIdx++].netIface, "%s", "eth7");
#endif

vlanIdx = 0;
pMib->VlanConfigArray[vlanIdx++].vlanId = 3022;
pMib->VlanConfigArray[vlanIdx].priority = 7;
pMib->VlanConfigArray[vlanIdx++].vlanId = 3030;
pMib->VlanConfigArray[vlanIdx].priority = 0;
pMib->VlanConfigArray[vlanIdx++].vlanId = 500;
pMib->VlanConfigArray[vlanIdx].priority = 3;
#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT) ||defined(CONFIG_RTL_HW_VLAN_SUPPORT)
pMib->VlanConfigArray[14].forwarding_rule = 2;
#endif
#endif
#if defined(SAMBA_WEB_SUPPORT)
pMib->StorageAnonEnable = 1; 
#endif

#if defined(CONFIG_RTL_WEB_WAN_ACCESS_PORT) && defined(HOME_GATEWAY)
pMib->wanAccessPort = 8080;
#endif

#if defined(CONFIG_RTL_ETH_802DOT1X_SUPPORT)
for(dot1x_entry=0;dot1x_entry<MAX_ELAN_DOT1X_PORTNUM;dot1x_entry++){
	pMib->ethdot1xArray[dot1x_entry].enabled=0;
	pMib->ethdot1xArray[dot1x_entry].portnum = dot1x_entry;

}	
pMib->enable1X=0;
pMib->entryNum=MAX_ELAN_DOT1X_PORTNUM;
#endif

#if defined(GW_QOS_ENGINE) || defined(QOS_BY_BANDWIDTH)
pMib->qosManualUplinkSpeed = 512;
pMib->qosManualDownLinkSpeed = 512;

#endif
#if defined(CONFIG_RTL_92D_SUPPORT)
  #if defined(CONFIG_RTL_92D_DMDP)
	pMib->wlanBand2G5GSelect = BANDMODEBOTH;
  #else
	pMib->wlanBand2G5GSelect = BANDMODE5G;
  #endif
#else
	pMib->wlanBand2G5GSelect = BANDMODE2G;
#endif
#if defined(CONFIG_RTL_8881A_SELECTIVE)
	pMib->wlanBand2G5GSelect = BANDMODESINGLE;
#endif

#if defined(MULTI_WAN_SUPPORT)
//pMib->WANIfaceEnabled = 1;
//WAN interface num is constant but default is ONE entry enabled ONLY,
//this is active entry number  NOT Total entry number
pMib->WANIfaceNum=WANIFACE_NUM;
{
	unsigned char i=0;
	for(i=0;i<WANIFACE_NUM;i++)
	{
		memset(&pMib->WANIfaceArray[i], 0x00, sizeof(WANIFACE_T));
		pMib->WANIfaceArray[i].ifIndex = 0x1ff05+i; 
		pMib->WANIfaceArray[i].cmode = IP_ROUTE; //wan attribute
		pMib->WANIfaceArray[i].AddressType = DHCP_CLIENT; //Dynamic get ip or not
		pMib->WANIfaceArray[i].brmode = BRIDGE_DISABLE; //default WIface does not work in bridge mode
		pMib->WANIfaceArray[i].mtu = 1500;

		pMib->WANIfaceArray[i].applicationtype =1;//TR069_INTERNET(0),INTERNET(1), TR069(2),  Other(3)
		pMib->WANIfaceArray[i].ServiceList =X_CT_SRV_INTERNET;
		pMib->WANIfaceArray[i].enableLanDhcp = 1;
		pMib->WANIfaceArray[i].multicastVlan = -1;
		pMib->WANIfaceArray[i].ipAddr[0] = 0;
		pMib->WANIfaceArray[i].ipAddr[1] = 0;
		pMib->WANIfaceArray[i].ipAddr[2] = 0;
		pMib->WANIfaceArray[i].ipAddr[3] = 0;
		pMib->WANIfaceArray[i].netMask[0] = 0;
		pMib->WANIfaceArray[i].netMask[1] = 0;
		pMib->WANIfaceArray[i].netMask[2] = 0;
		pMib->WANIfaceArray[i].netMask[3] = 0;
		sprintf((char *)pMib->WANIfaceArray[i].WanName, "%s", "TR069_INTERNET");
		pMib->WANIfaceArray[i].IpProtocol = IP_PROTOCOL_MODE_IPV4; 	
		pMib->WANIfaceArray[i].enableIpQos = 0;
		pMib->WANIfaceArray[i].ConDevInstNum = i+1;
		pMib->WANIfaceArray[i].ConIPInstNum = 0;
		pMib->WANIfaceArray[i].ConPPPInstNum = 0;
		pMib->WANIfaceArray[i].connDisable =1;
		pMib->WANIfaceArray[i].enable =0;
		pMib->WANIfaceArray[i].dhcpMtu =1492;
		pMib->WANIfaceArray[i].staticIpMtu =1500;
		pMib->WANIfaceArray[i].pppoeMtu =1492;
		pMib->WANIfaceArray[i].vlan = 1;
		pMib->WANIfaceArray[i].vlanid = i+1;
		pMib->WANIfaceArray[i].dnsAuto = 1;
#if defined(SINGLE_WAN_SUPPORT)		
		pMib->WANIfaceArray[i].l2tpMtuSize = 1460;
		pMib->WANIfaceArray[i].pptpMtuSize = 1460;
#endif
	}
	pMib->WANIfaceArray[0].vlan = 0;
}
#endif
#if defined(CONFIG_WLAN_HAL_8814AE)&&!defined(CONFIG_RTL_92D_SUPPORT)
	pMib->wlanBand2G5GSelect = BANDMODE2G;
#elif defined(CONFIG_RTL_8812_SUPPORT) && !defined(CONFIG_RTL_8881A_SELECTIVE)
	pMib->wlanBand2G5GSelect = BANDMODEBOTH;
#endif
#if defined(CONFIG_RTL_8198_AP_ROOT) || defined(CONFIG_RTL_8197D_AP) || defined(CONFIG_RTL_AP_PACKAGE)
	pMib->opMode=BRIDGE_MODE;
#endif


#if defined(WLAN_PROFILE)
	pMib->wlan_profile_enable1 = 0;
	pMib->wlan_profile_num1 = 0;
	pMib->wlan_profile_enable2 = 0;
	pMib->wlan_profile_num2 = 0;
#endif //#if defined(WLAN_PROFILE)
#if 0//def CONFIG_APP_TR069
//DEBUG ONLY, if formal release the setting should be modified
pMib->cwmp_enabled = 1;
sprintf((char *)pMib->cwmp_ACSURL, "%s", "http://172.21.146.44/cpe/?pd128");
sprintf((char *)pMib->cwmp_ACSUserName, "%s", "sd9_e8");
sprintf((char *)pMib->cwmp_ACSPassword, "%s", "1234");
sprintf((char *)pMib->cwmp_ConnReqUserName, "%s", "123456");
sprintf((char *)pMib->cwmp_ConnReqPassword, "%s", "123456");
pMib->cwmp_Flag = 97; //Turn On tr069 Debug message output
//pMib->cwmp_Flag = 96;//Turn OFF tr069 Debug message output
pMib->cwmp_ConnReqPort = 4567;
sprintf((char *)pMib->cwmp_ConnReqPath, "%s", "/tr069");

#if defined(CONFIG_APP_TR069) && defined(WLAN_SUPPORT)
//The Setting is "MUST"
Init_WlanConf(pMib);
#endif

pMib->cwmp_ipconn_instnum = 1; //default wan type is dhcp
pMib->cwmp_ipconn_created = 1; //default wan type is dhcp
//pMib->cwmp_pppconn_instnum = 1; //default wan type is pppoe
//pMib->cwmp_pppconn_created = 1; //default wan type is pppoe

#endif

#if defined(CONFIG_APP_TR069) && defined(WLAN_SUPPORT)
	//The Setting is "MUST"
	Init_WlanConf(pMib);
#endif

#if defined(CONFIG_APP_TR069)
	sprintf((char *)pMib->UIF_Cur_Lang, "%s", "en-us");

#ifdef _ALPHA_DUAL_WAN_SUPPORT_
	/* default enable TR069 */
	pMib->cwmp_Flag |= CWMP_FLAG_AUTORUN;
	pMib->cwmp_enabled = 1;
#endif
	
#endif

#if defined(CONFIG_APP_APPLE_MFI_WAC)
pMib->mibVer = 2;
pMib->opMode = BRIDGE_MODE;
#if defined(CONFIG_RTL_8196E) || defined(CONFIG_RTL_8871AM) || defined(CONFIG_RTL_8198C) || defined(CONFIG_RTL_8197F) || defined(CONFIG_RTL_8881A_SELECTIVE)
		// only support single band
		if(isAll)
			sprintf((char *)pMib->WACdeviceName, "Realtek_WAC_%02X%02X%02X",hwmib.wlan[0].macAddr[3], hwmib.wlan[0].macAddr[4], hwmib.wlan[0].macAddr[5]);
		else{
        	//apmib_get(MIB_HW_NIC0_ADDR, (void *)&tmp1);
        	if(flash_read(tmp1,HW_WLAN_SETTING_OFFSET + HW_SETTING_OFFSET,6) != 0)
				sprintf((char *)pMib->WACdeviceName, "Realtek_WAC_%02X%02X%02X",tmp1[3], tmp1[4], tmp1[5]);
            else{
				printf("[%s]:%d, get hw_nic0_addr error\n",__func__,__LINE__);
			}
        }       
        pMib->MfiWACBand = 2;
#else
		sprintf((char *)pMib->WACdeviceName, "%s", "Realtek");
#endif
#if defined(WAC_CONFIGURING_REPETER)
pMib-> repeaterEnabled1 =1;
#else
pMib-> repeaterEnabled1 =0;
#endif
#endif

	data = (char *)pMib;
#ifdef _LITTLE_ENDIAN_
	swap_mib_value(pMib,DEFAULT_SETTING);
#endif

	// write default setting
#ifdef MIB_TLV

//mib_display_data_content(DEFAULT_SETTING, pMib, sizeof(APMIB_T));	

	mib_tlv_max_len = mib_get_setting_len(DEFAULT_SETTING)*4;

	tlv_content_len = 0;

	pfile = malloc(mib_tlv_max_len);
	memset(pfile, 0x00, mib_tlv_max_len);
	
 	if( pfile != NULL && mib_tlv_save(DEFAULT_SETTING, (void*)pMib, pfile, &tlv_content_len) == 1)
	{
 
		mib_tlv_data = malloc(tlv_content_len+1); // 1:checksum
		if(mib_tlv_data != NULL)
		{
			memcpy(mib_tlv_data, pfile, tlv_content_len);
		}
			
		free(pfile);

	}
 	if(mib_tlv_data != NULL)
	{
		
		sprintf((char *)header.signature, "%s%02d", DEFAULT_SETTING_HEADER_TAG, DEFAULT_SETTING_VER);
		header.len = tlv_content_len+1;
		data = (char *)mib_tlv_data;
		checksum = CHECKSUM((unsigned char *)data, header.len-1);
		data[tlv_content_len] = CHECKSUM((unsigned char *)data, tlv_content_len);
//mib_display_tlv_content(DEFAULT_SETTING, data, header.len);
		
	}
	else
	{
#endif //#ifdef MIB_TLV
	data = (char *)pMib;	
	sprintf((char *)header.signature, "%s%02d", DEFAULT_SETTING_HEADER_TAG, DEFAULT_SETTING_VER);
	header.len = sizeof(APMIB_T) + sizeof(checksum);
	checksum = CHECKSUM((unsigned char *)data, header.len-1);

#ifdef MIB_TLV
	}
#endif

	lseek(fh, DEFAULT_SETTING_OFFSET, SEEK_SET);
#if CONFIG_APMIB_SHARED_MEMORY == 1	
    apmib_sem_lock();
#endif
#ifdef COMPRESS_MIB_SETTING

//fprintf(stderr,"\r\n header.len=%u, __[%s-%u]",header.len,__FILE__,__LINE__);

	if(flash_mib_compress_write(DEFAULT_SETTING, data, &header, &checksum) == 1)
	{
		COMP_TRACE(stderr,"\r\n flash_mib_compress_write DEFAULT_SETTING DONE, __[%s-%u]", __FILE__,__LINE__);			
	}
	else
	{

#endif //#ifdef COMPRESS_MIB_SETTING

#ifdef _LITTLE_ENDIAN_
	header.len=HEADER_SWAP(header.len);
#endif
	if ( write(fh, (const void *)&header, sizeof(header))!=sizeof(header) ) {
		printf("write ds header failed!\n");
		return -1;
	}
	if ( write(fh, (const void *)pMib, sizeof(mib))!=sizeof(mib) ) {
		printf("write ds MIB failed!\n");
		return -1;
	}
	if ( write(fh, (const void *)&checksum, sizeof(checksum))!=sizeof(checksum) ) {
		printf("write ds checksum failed!\n");
		return -1;
	}
#ifdef COMPRESS_MIB_SETTING
	}
#endif
 
	close(fh);
	sync();
#if CONFIG_APMIB_SHARED_MEMORY == 1	
    apmib_sem_unlock();
#endif
#ifdef CONFIG_MTD_NAND
	fh = open(FLASH_DEVICE_SETTING, O_RDWR|O_CREAT);
#else
	fh = open(FLASH_DEVICE_NAME, O_RDWR);
#endif

	// write current setting

#ifdef MIB_TLV
	if(mib_tlv_data != NULL)
	{
		sprintf((char *)header.signature, "%s%02d", CURRENT_SETTING_HEADER_TAG, CURRENT_SETTING_VER);
		header.len = tlv_content_len+1;
		checksum = CHECKSUM((unsigned char *)data, header.len-1);
//mib_display_tlv_content(CURRENT_SETTING, data, header.len);		
	}
	else
	{
#endif
	sprintf((char *)header.signature, "%s%02d", CURRENT_SETTING_HEADER_TAG, CURRENT_SETTING_VER);
	header.len = sizeof(APMIB_T) + sizeof(checksum);

#ifdef MIB_TLV
	}
#endif

	lseek(fh, CURRENT_SETTING_OFFSET, SEEK_SET);
#if CONFIG_APMIB_SHARED_MEMORY == 1	
    apmib_sem_lock();
#endif
#ifdef COMPRESS_MIB_SETTING

	if(flash_mib_compress_write(CURRENT_SETTING, data, &header, &checksum) == 1)
	{
		COMP_TRACE(stderr,"\r\n flash_mib_compress_write CURRENT_SETTING DONE, __[%s-%u]", __FILE__,__LINE__);			
	}
	else
	{

#endif //#ifdef COMPRESS_MIB_SETTING

#ifdef _LITTLE_ENDIAN_
		header.len=HEADER_SWAP(header.len);
#endif
	if ( write(fh, (const void *)&header, sizeof(header))!=sizeof(header) ) {
		printf("write cs header failed!\n");
		return -1;
	}
	if ( write(fh, (const void *)pMib, sizeof(mib))!=sizeof(mib) ) {
		printf("write cs MIB failed!\n");
		return -1;
	}
	if ( write(fh, (const void *)&checksum, sizeof(checksum))!=sizeof(checksum) ) {
		printf("write cs checksum failed!\n");
		return -1;
	}
#ifdef COMPRESS_MIB_SETTING	
	}
#endif

	close(fh);
	sync();
#if CONFIG_APMIB_SHARED_MEMORY == 1	
		apmib_sem_unlock();
#endif

	// check if hw, ds, cs checksum is ok
	offset = HW_SETTING_OFFSET;
#ifdef COMPRESS_MIB_SETTING
	if(comp_hw_setting == 1)
	{

	if(flash_mib_checksum_ok(offset) == 1)
	{
		COMP_TRACE(stderr,"\r\n HW_SETTING hecksum_ok\n");
	}
	}
	else
	{
		if(comp_hw_setting == 1)
		{
		fprintf(stderr,"flash mib checksum error!\n");
		}
#endif // #ifdef COMPRESS_MIB_SETTING	
#ifdef HEADER_LEN_INT
	if ( flash_read((char *)&hwHeader, offset, sizeof(hwHeader)) == 0) 
	{
		printf("read hs header failed!\n");
		return -1;
	}
	hwHeader.len=WORD_SWAP(hwHeader.len);
	offset += sizeof(hwHeader);
	if ( flash_read((char *)buff, offset, hwHeader.len) == 0) {
		printf("read hs MIB failed!\n");
		return -1;
	}
	status = CHECKSUM_OK(buff, hwHeader.len);
	if ( !status) {
		printf("hs Checksum error!\n");
		return -1;
	}
#else
	if ( flash_read((char *)&header, offset, sizeof(header)) == 0) 
	{
		printf("read hs header failed!\n");
		return -1;
	}
	header.len=WORD_SWAP(header.len);
	offset += sizeof(header);
	if ( flash_read((char *)buff, offset, header.len) == 0) {
		printf("read hs MIB failed!\n");
		return -1;
	}
	status = CHECKSUM_OK(buff, header.len);
	if ( !status) {
		printf("hs Checksum error!\n");
		return -1;
	}
#endif

#ifdef COMPRESS_MIB_SETTING	
	}
#endif	

	offset = DEFAULT_SETTING_OFFSET;
#ifdef COMPRESS_MIB_SETTING
	if(flash_mib_checksum_ok(offset) == 1)
	{
		COMP_TRACE(stderr,"\r\n DEFAULT_SETTING hecksum_ok\n");
	}
	else
	{
#endif // #ifdef COMPRESS_MIB_SETTING	
	if ( flash_read((char *)&header, offset, sizeof(header)) == 0) {
		printf("read ds header failed!\n");
		return -1;
	}
	
	header.len=HEADER_SWAP(header.len);
	
	offset += sizeof(header);
	if ( flash_read((char *)buff, offset, header.len) == 0) {
		printf("read ds MIB failed!\n");
		return -1;
	}
	status = CHECKSUM_OK(buff, header.len);
	if ( !status) {
		printf("ds Checksum error!\n");
		return -1;
	}
#ifdef COMPRESS_MIB_SETTING
	}
#endif

	offset = CURRENT_SETTING_OFFSET;
#ifdef COMPRESS_MIB_SETTING
	if(flash_mib_checksum_ok(offset) == 1)
	{
		COMP_TRACE(stderr,"\r\n CURRENT_SETTING hecksum_ok\n");
	}
	else
	{
#endif // #ifdef COMPRESS_MIB_SETTING	
	if ( flash_read((char *)&header, offset, sizeof(header)) == 0) {
		printf("read cs header failed!\n");
		return -1;
	}
	
	header.len=HEADER_SWAP(header.len);
	
	offset += sizeof(header);
	if ( flash_read((char *)buff, offset, header.len) == 0) {
		printf("read cs MIB failed!\n");
		return -1;
	}
	status = CHECKSUM_OK(buff, header.len);

	if ( !status) {
		printf("cs Checksum error!\n");
		return -1;
	}
#ifdef COMPRESS_MIB_SETTING
	}
#endif	
	free(buff);

#if CONFIG_APMIB_SHARED_MEMORY == 1	
    apmib_sem_lock();
	fprintf(stderr,"\r\n  __[%s-%u]", __FILE__,__LINE__);
    apmib_load_hwconf();
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
	apmib_load_bluetooth_hwconf();
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
	apmib_load_customer_hwconf();
#endif
    apmib_load_dsconf();
    apmib_load_csconf();
    apmib_sem_unlock();
#endif
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
/* flash_read only used to read mib
/* flash_read_web used to read web|linux|cert for nand flash*/
static int flash_read_web(char *buf, int offset, int len)
{
	int fh;
	int ok=1;
#ifdef MTD_NAME_MAPPING
	char webpageMtd[16];
#endif

#ifdef CONFIG_MTD_NAND
	if(offset < NAND_BOOT_SETTING_SIZE){
		printf("webpage offset in nand flash platform < 8M\n");
		return 0;		
	}else
		offset = offset - NAND_BOOT_SETTING_SIZE;
#endif




#ifdef MTD_NAME_MAPPING
	if(rtl_name_to_mtdblock(FLASH_WEBPAGE_NAME,webpageMtd) == 0){
		return 0;
	}
#ifdef __mips__
	fh = open(webpageMtd, O_RDWR);
#endif

#ifdef __i386__
	fh = open(webpageMtd, O_RDONLY);
#endif
#else
#ifdef __mips__
	fh = open(FLASH_DEVICE_NAME, O_RDWR);
#endif

#ifdef __i386__
	fh = open(FLASH_DEVICE_NAME, O_RDONLY);
#endif
#endif

	if ( fh == -1 )
		return 0;

	lseek(fh, offset, SEEK_SET);

	if ( read(fh, buf, len) != len)
		ok = 0;

	close(fh);

	return ok;

}



static int flash_read(char *buf, int offset, int len)
{
	int fh;
	int ok=1;

#ifdef __mips__
#ifdef CONFIG_MTD_NAND
	if (offset == CURRENT_SETTING_OFFSET)
	{
		fh = open("/config/current_setting.bin", O_RDWR | O_CREAT);
	}
	else
	{
		fh = open(FLASH_DEVICE_SETTING, O_RDWR | O_CREAT);
	}
#else
	fh = open(FLASH_DEVICE_NAME, O_RDWR);
#endif
#endif

#ifdef __i386__
	fh = open(FLASH_DEVICE_NAME, O_RDONLY);
#endif
	if ( fh == -1 )
		return 0;

	lseek(fh, offset, SEEK_SET);

	if ( read(fh, buf, len) != len)
		ok = 0;

	close(fh);

	return ok;
}

#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE)
#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_WEB_BACKUP_ENABLE)
static int get_actvie_bank()
{
	FILE *fp;
	char buffer[2];
	int bootbank;
	fp = fopen("/proc/bootbank", "r");
	
	if (!fp) {
		fprintf(stderr,"%s\n","Read /proc/bootbank failed!\n");
	}else
	{
			//fgets(bootbank, sizeof(bootbank), fp);
			fgets(buffer, sizeof(buffer), fp);
			fclose(fp);
	}
	bootbank = buffer[0] - 0x30;	
	if ( bootbank ==1 || bootbank ==2)
		return bootbank;
	else
		return 1;	
}


static void get_bank_info(int *active)
{
	int bootbank=0,backup_bank;
	
	bootbank = get_actvie_bank();	

	*active = bootbank;
}

static int flash_read_webpage(char *buf, int offset, int len)
{
	int fh;
	int ok=1;
#ifdef MTD_NAME_MAPPING
	char webpageMtd[16];
#endif
	
	int boot_bak;
	get_bank_info(&boot_bak);

#ifdef CONFIG_MTD_NAND
	if(offset < NAND_BOOT_SETTING_SIZE){
		printf("webpage offset in nand flash platform < 8M\n");
		return 0;		
	}else
		offset = offset - NAND_BOOT_SETTING_SIZE;
#endif

#ifdef MTD_NAME_MAPPING
	if(boot_bak == 1)
		if(rtl_name_to_mtdblock(FLASH_WEBPAGE_NAME,webpageMtd) == 0){
			return 0;
		}
	else if(boot_bak == 2)
		if(rtl_name_to_mtdblock(FLASH_WEBPAGE_NAME2,webpageMtd) == 0){
			return 0;
		}
#ifdef __mips__
	fh = open(webpageMtd,O_RDWR);
#endif

#ifdef __i386__
	fh = open(webpageMtd, O_RDONLY);
#endif
	
#else
#ifdef __mips__
	fh = open(Web_dev_name[boot_bak-1],O_RDWR);
#endif

#ifdef __i386__
	fh = open(FLASH_DEVICE_NAME, O_RDONLY);
#endif
#endif
	if ( fh == -1 )
		return 0;

	lseek(fh, offset, SEEK_SET);

	if ( read(fh, buf, len) != len)
		ok = 0;

	close(fh);

	return ok;

}

#endif
#endif

#ifdef __i386__
/* This API saves uncompressed raw file for debug */
static int flash_write_file(char *buf, int offset, int len , char * filename)
{
	int fh;
	int ok=1;


	printf("%s(%d)write file %s, offset=0x%08x, len=%d\n",
			__FUNCTION__,__LINE__,filename,offset,len);

	if (offset != 0x8000)
		return ok;

	fh = open(filename, O_RDWR|O_CREAT|O_TRUNC,0644);

	if ( fh == -1 )
		return 0;

	if ( write(fh, buf, len) != len)
		ok = 0;

	close(fh);
	sync();

	return ok;
}
#endif






///////////////////////////////////////////////////////////////////////////////
static int searchMIB(char *token)
{
	int idx = 0;
	char tmpBuf[100];
	int desired_config=0;

	if (!memcmp(token, "HW_", 3)) {
		config_area = HW_MIB_AREA;
		if (!memcmp(&token[3], "WLAN", 4) && token[8] == '_') {
			wlan_idx = token[7] - '0';
			if (wlan_idx >= NUM_WLAN_INTERFACE)
				return -1;
			strcpy(tmpBuf, &token[9]);
			desired_config = config_area+1;
		}
		else
			strcpy(tmpBuf, &token[3]);
	}
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		else if (!memcmp(token, "BLUETOOTH_HW_", 13)) {
			config_area = BLUETOOTH_HW_AREA; 	
			strcpy(tmpBuf, &token[13]);
//			printf("%s:%d tmpBuf=%s\n",__FUNCTION__,__LINE__,tmpBuf);
		}
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
	else if (!memcmp(token, "CUSTOMER_HW_", 12)) {
		config_area = CUSTOMER_HW_AREA;		
		strcpy(tmpBuf, &token[12]);
		//printf("%s:%d tmpBuf=%s\n",__FUNCTION__,__LINE__,tmpBuf);
	}
#endif
	else if (!memcmp(token, "DEF_", 4)) {
		config_area = DEF_MIB_AREA;

		if (!memcmp(&token[4], "WLAN", 4) && token[9] == '_') {
			wlan_idx = token[8] - '0';
			if (wlan_idx >= NUM_WLAN_INTERFACE)
				return -1;
#ifdef MBSSID
			if (!memcmp(&token[10], "VAP", 3) && token[14] == '_') {
				vwlan_idx = token[13] - '0';
				if (vwlan_idx >= NUM_VWLAN_INTERFACE) {
					vwlan_idx = 0;
					return -1;
				}
				vwlan_idx += 1;
				strcpy(tmpBuf, &token[15]);
			}
			else
#endif
			strcpy(tmpBuf, &token[10]);
			desired_config = config_area+1;
		}
		else
#ifdef MULTI_WAN_SUPPORT
		if(!memcmp(&token[4], "WAN", 3) && (token[8] == '_'))
		{
			int idx = token[7] - '0';
			if(idx>WANIFACE_NUM)
				return -1;
			apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&idx);
			strcpy(tmpBuf, &token[9]);
			desired_config = config_area+2;
//			fprintf(stderr,"%s:%d desired_config=%d\n",__FUNCTION__,__LINE__,desired_config);
		}else
#endif
			strcpy(tmpBuf, &token[4]);
	}
	else {
		config_area = MIB_AREA;

		if (!memcmp(&token[0], "WLAN", 4) && token[5] == '_') {
			wlan_idx = token[4] - '0';
			if (wlan_idx >= NUM_WLAN_INTERFACE)
				return -1;
#ifdef MBSSID
			if (!memcmp(&token[6], "VAP", 3) && token[10] == '_') {
				vwlan_idx = token[9] - '0';
				if (vwlan_idx >= NUM_VWLAN_INTERFACE) {
					vwlan_idx = 0;
					return -1;
				}
				vwlan_idx += 1;
				strcpy(tmpBuf, &token[11]);
			}
#ifdef UNIVERSAL_REPEATER
			else if (!memcmp(&token[6], "VXD", 3) && token[9] == '_') {
				vwlan_idx = NUM_VWLAN_INTERFACE;
				strcpy(tmpBuf, &token[10]);
			}
#endif
			else
#endif
			strcpy(tmpBuf, &token[6]);
			desired_config = config_area+1;
		}
#ifdef MULTI_WAN_SUPPORT
		else if(!memcmp(&token[0], "WAN", 3) && (token[4] == '_'))
		{
			int idx = token[3] - '0';
			if(idx>WANIFACE_NUM)
				return -1;
			apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&idx);
			strcpy(tmpBuf, &token[5]);
			desired_config = config_area+2;
		}
#endif
		else
			strcpy(tmpBuf, &token[0]);
	}

	if ( config_area == HW_MIB_AREA ) {
		while (hwmib_table[idx].id) {
			if ( !strcmp(hwmib_table[idx].name, tmpBuf)) {
				if (desired_config && config_area != desired_config)
					return -1;
				return idx;
			}
			idx++;
		}
		idx=0;
		while (hwmib_wlan_table[idx].id) {
			if ( !strcmp(hwmib_wlan_table[idx].name, tmpBuf)) {
				config_area++;
				if (desired_config && config_area != desired_config)
					return -1;
				return idx;
			}
			idx++;
		}
		return -1;
	}
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
	else if(config_area == BLUETOOTH_HW_AREA){
		while (bluetooth_hwmib_table[idx].id) {
//			printf("%s:%d tmpBuf=%s name=%s\n",__FUNCTION__,__LINE__,tmpBuf,bluetooth_hwmib_table[idx].name);
			if ( !strcmp(bluetooth_hwmib_table[idx].name, tmpBuf)) {
				if (desired_config && config_area != desired_config)
					return -1;
				return idx;
			}
			idx++;
		}
		return -1;
	}
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
	else if(config_area == CUSTOMER_HW_AREA){
		while (customer_hwmib_table[idx].id) {
//			printf("%s:%d tmpBuf=%s name=%s\n",__FUNCTION__,__LINE__,tmpBuf,customer_hwmib_table[idx].name);
			if ( !strcmp(customer_hwmib_table[idx].name, tmpBuf)) {
				if (desired_config && config_area != desired_config)
					return -1;
				return idx;
			}
			idx++;
		}
		return -1;
	}
#endif
	else {
		while (mib_table[idx].id) {
			if ( !strcmp(mib_table[idx].name, tmpBuf)) {
				if (desired_config && config_area != desired_config)
					return -1;
				return idx;
			}
			idx++;
		}
		idx=0;
		while (mib_wlan_table[idx].id) {
			if ( !strcmp(mib_wlan_table[idx].name, tmpBuf)) {
				config_area++;
				if (desired_config && config_area != desired_config)
					return -1;
				return idx;
			}
			idx++;
		}
#ifdef MULTI_WAN_SUPPORT
		idx = 0;
		while (mib_waniface_tbl[idx].id) {
			if ( !strcmp(mib_waniface_tbl[idx].name, tmpBuf)) {
				config_area+=2;
				if (desired_config && config_area != desired_config)
					return -1;
					
					//fprintf(stderr,"%s:%dconfig_area=%d\n",__FUNCTION__,__LINE__,config_area);
				return idx;
			}
			idx++;
		}
#endif
		return -1;
	}
}

///////////////////////////////////////////////////////////////////////////////
static void getMIB(char *name, int id, TYPE_T type, int num, int array_separate, char **val)
{
	unsigned char array_val[4096];
	struct in_addr ia_val;
	void *value;
	unsigned char tmpBuf[4096]={0}, *format=NULL, *buf, tmp1[400];
	int int_val, i;
	unsigned int uint_val=0;
	int index=1, tbl=0;
	char mibName[100]={0};

	if (num ==0)
		goto ret;

	strcat(mibName, name);

#if 0
	if(id == MIB_CWMP_NOTIFY_LIST)
	{
		printf("%s's value, please cat \"/var/CWMPNotify.txt\" \n", mibName);
		return;
	}
#endif

getval:
	buf = &tmpBuf[strlen((char *)tmpBuf)];
	switch (type) {
	case BYTE_T:
		value = (void *)&int_val;
		format = (unsigned char *)DEC_FORMAT;
		break;
	case WORD_T:
		value = (void *)&int_val;
		format = (unsigned char *)DEC_FORMAT;
		break;
	case IA_T:
		value = (void *)&ia_val;
		format = (unsigned char *)STR_FORMAT;
		break;
	case BYTE5_T:
		value = (void *)array_val;
		format = (unsigned char *)BYTE5_FORMAT;
		break;
	case BYTE6_T:
		value = (void *)array_val;
		format = (unsigned char *)BYTE6_FORMAT;
		break;
	case BYTE13_T:
		value = (void *)array_val;
		format = (unsigned char *)BYTE13_FORMAT;
		break;

	case STRING_T:
		value = (void *)array_val;
		format = (unsigned char *)STR_FORMAT;
		break;

	case BYTE_ARRAY_T:
		value = (void *)array_val;
		break;

	case DWORD_T:
		value = (void *)&uint_val;
		format = (unsigned char *)UDEC_FORMAT;
		break;

	case WLAC_ARRAY_T:
//#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_)
	case MESH_ACL_ARRAY_T:
//#endif
	case WDS_ARRAY_T:
#ifdef TR181_SUPPORT
#ifdef CONFIG_IPV6
	case DHCPV6C_SENDOPT_ARRAY_T:
#endif
	case DNS_CLIENT_SERVER_ARRAY_T:
#endif
	case DHCPRSVDIP_ARRY_T:	
	case SCHEDULE_ARRAY_T:

#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	case CAP_PORTAL_ALLOW_ARRAY_T:
#endif
#ifdef HOME_GATEWAY
	case PORTFW_ARRAY_T:
#if defined(CONFIG_APP_TR069) && defined(WLAN_SUPPORT)
	case CWMP_WLANCONF_ARRAY_T:
#endif		
	case IPFILTER_ARRAY_T:
	case PORTFILTER_ARRAY_T:
	case MACFILTER_ARRAY_T:
	case URLFILTER_ARRAY_T:
	case TRIGGERPORT_ARRAY_T:

#if defined(GW_QOS_ENGINE) || defined(QOS_BY_BANDWIDTH)
	case QOS_ARRAY_T:
#endif

//added by shirley  2014/5/21
#ifdef QOS_OF_TR069
    case QOS_CLASS_ARRAY_T:
    case QOS_QUEUE_ARRAY_T:
	case TR098_APPCONF_ARRAY_T:	
	case TR098_FLOWCONF_ARRAY_T:
case QOS_POLICER_ARRAY_T:		//mark_qos
   case QOS_QUEUESTATS_ARRAY_T:
#endif
#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
	case DHCP_SERVER_OPTION_ARRAY_T:
	case DHCP_CLIENT_OPTION_ARRAY_T:
	case DHCPS_SERVING_POOL_ARRAY_T:
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */
#ifdef ROUTE_SUPPORT
	case STATICROUTE_ARRAY_T:
#endif
#ifdef VPN_SUPPORT
	case IPSECTUNNEL_ARRAY_T:
#endif
#ifdef CONFIG_IPV6
	case RADVDPREFIX_T:
	case DNSV6_T:
	case DHCPV6S_T:
	case DHCPV6C_T:
	case ADDR6_T:
	case ADDRV6_T:
	case TUNNEL6_T:
#endif
#endif
#ifdef TLS_CLIENT
	case CERTROOT_ARRAY_T:
	case CERTUSER_ARRAY_T:
#endif

#if defined(VLAN_CONFIG_SUPPORTED)
	case VLANCONFIG_ARRAY_T:
#endif		
#if defined(SAMBA_WEB_SUPPORT)
	case STORAGE_USER_ARRAY_T:
	case STORAGE_SHAREINFO_ARRAY_T:
#endif		
#if defined(CONFIG_RTL_ETH_802DOT1X_SUPPORT)
	case ETHDOT1X_ARRAY_T:
#endif

#if defined(CONFIG_8021Q_VLAN_SUPPORTED)
	case VLAN_ARRAY_T:
#endif

#ifdef WLAN_PROFILE
	case PROFILE_ARRAY_T:
#endif

#ifdef RTK_CAPWAP
	case CAPWAP_WTP_CONFIG_ARRAY_T:
#endif

#ifdef CONFIG_RTL_AIRTIME
	case AIRTIME_ARRAY_T:
#endif
		tbl = 1;
		value = (void *)array_val;
		array_val[0] = index++;
		break;
	default: printf("invalid mib!\n"); return;
	}

	if ( !APMIB_GET(id, value)) {
		printf("Get MIB failed!\n");
		return;
	}

	if ( type == IA_T )
		value = inet_ntoa(ia_val);

	if (type == BYTE_T || type == WORD_T)
		sprintf((char *)buf, (char *)format, int_val);
	else if ( type == IA_T || type == STRING_T ) {
		sprintf((char *)buf, (char *)format, value);
		
		if (type == STRING_T ) {
			unsigned char tmpBuf1[1024];
			int srcIdx, dstIdx;
			for (srcIdx=0, dstIdx=0; buf[srcIdx]; srcIdx++, dstIdx++) {
				if ( buf[srcIdx] == '"' || buf[srcIdx] == '\\' || buf[srcIdx] == '$' || buf[srcIdx] == '`' || buf[srcIdx] == ' ' )
					tmpBuf1[dstIdx++] = '\\';

				tmpBuf1[dstIdx] = buf[srcIdx];
			}
			if (dstIdx != srcIdx) {
				memcpy(buf, tmpBuf1, dstIdx);
				buf[dstIdx] ='\0';
			}
		}
	}
	else if (type == BYTE5_T) {
		sprintf((char *)buf, (char *)format, array_val[0],array_val[1],array_val[2],
			array_val[3],array_val[4],array_val[5]);
		convert_lower((char *)buf);
	}
	else if (type == BYTE6_T ) {
		sprintf((char *)buf, (char *)format, array_val[0],array_val[1],array_val[2],
			array_val[3],array_val[4],array_val[5],array_val[6]);
		convert_lower((char *)buf);
	}
	else if (type == BYTE13_T) {
		sprintf((char *)buf, (char *)format, array_val[0],array_val[1],array_val[2],
			array_val[3],array_val[4],array_val[5],array_val[6],
			array_val[7],array_val[8],array_val[9],array_val[10],
			array_val[11],array_val[12]);
		convert_lower((char *)buf);
	}
	else if(type == BYTE_ARRAY_T ) {
		int max_chan_num=MAX_2G_CHANNEL_NUM_MIB;
		int chan;
#if defined(CONFIG_RTL_8196B)
		max_chan_num = (id == MIB_HW_TX_POWER_CCK)? MAX_CCK_CHAN_NUM : MAX_OFDM_CHAN_NUM;
		if(val == NULL || val[0] == NULL){
			for(i=0 ;i< max_chan_num ;i++){
				sprintf(tmp1, "%02x", array_val[i]);
				strcat(buf, tmp1);
			}
			convert_lower(buf);
		}
		else{
			chan = atoi(val[0]);
			if(chan < 1 || chan > max_chan_num){
				if((chan<1) || (id==MIB_HW_TX_POWER_CCK) || ((id==MIB_HW_TX_POWER_OFDM) && (chan>216))){
					printf("invalid channel number\n");
					return;
				}
				else{
					if((chan >= 163) && (chan <= 181))
						chan -= 148;
					else // 182 ~ 216
						chan -= 117;
				}
			}
			sprintf(buf, "%d", *(((unsigned char *)value)+chan-1) );
		}
#else
		if((id >= MIB_HW_TX_POWER_CCK_A &&  id <=MIB_HW_TX_POWER_DIFF_OFDM)			
#if defined(CONFIG_WLAN_HAL_8814AE)
			||(id >= MIB_HW_TX_POWER_CCK_C &&  id <=MIB_HW_TX_POWER_CCK_D)
			||(id >= MIB_HW_TX_POWER_HT40_1S_C &&  id <=MIB_HW_TX_POWER_HT40_1S_D)
#endif
			)
			max_chan_num = MAX_2G_CHANNEL_NUM_MIB;
		else if((id >= MIB_HW_TX_POWER_5G_HT40_1S_A &&  id <=MIB_HW_TX_POWER_DIFF_5G_OFDM)
#if defined(CONFIG_WLAN_HAL_8814AE)
				||(id >= MIB_HW_TX_POWER_5G_HT40_1S_C &&  id <=MIB_HW_TX_POWER_5G_HT40_1S_D)
#endif
			)
			max_chan_num = MAX_5G_CHANNEL_NUM_MIB;
		else if(id == MIB_L2TP_PAYLOAD)
			max_chan_num = MAX_L2TP_BUFF_LEN;
		
#if defined(CONFIG_RTL_8812_SUPPORT)
		if(((id >= MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_A) && (id <= MIB_HW_TX_POWER_DIFF_OFDM4T_CCK4T_A)) 
			|| ((id >= MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_B) && (id <= MIB_HW_TX_POWER_DIFF_OFDM4T_CCK4T_B)) )
			max_chan_num = MAX_2G_CHANNEL_NUM_MIB;
		
		if(((id >= MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_A) && (id <= MIB_HW_TX_POWER_DIFF_5G_80BW4S_160BW4S_A)) 
			|| ((id >= MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_B) && (id <= MIB_HW_TX_POWER_DIFF_5G_80BW4S_160BW4S_B)) )
			max_chan_num = MAX_5G_DIFF_NUM;
#endif

		
		if(val == NULL || val[0] == NULL){
			for(i=0;i< max_chan_num;i++){
				sprintf((char *)tmp1, "%02x", array_val[i]);
				strcat((char *)buf, (char *)tmp1);
			}
			convert_lower((char *)buf);
		}
		else{
			chan = atoi(val[0]);
			if(chan < 1 || chan > max_chan_num){
				printf("invalid channel number\n");
				return;
			}
			sprintf((char *)buf, "%d", *(((unsigned char *)value)+chan-1) );
		}
#endif
	}
	else if (type == DWORD_T)
		sprintf((char *)buf, (char *)format, uint_val);
#ifdef TR181_SUPPORT
#ifdef CONFIG_IPV6
	else if (type == DHCPV6C_SENDOPT_ARRAY_T) {		
		DHCPV6C_SENDOPT_Tp pEntry=(DHCPV6C_SENDOPT_Tp)array_val;
		sprintf((char *)buf, "%d %d %d %s", pEntry->index,			
			pEntry->enable,pEntry->tag,pEntry->value);
	}
#endif
	else if(type == DNS_CLIENT_SERVER_ARRAY_T) {
		char *dnsType, *dnsStatus;
		DNS_CLIENT_SERVER_Tp pEntry=(DNS_CLIENT_SERVER_Tp)array_val;
		
		if(pEntry->status == 1)
			dnsStatus = "Enable";
		else
			dnsStatus = "Disable";

		if(pEntry->type == 1)
			dnsType = "DHCPv4";
		else if(pEntry->type == 2)
			dnsType = "DHCPv6";
		else if(pEntry->type == 3)
			dnsType = "RouterAdvertisement";
		else if(pEntry->type == 4)
			dnsType = "IPCP";
		else if(pEntry->type == 5)
			dnsType = "Static";
		else
			dnsType = "Unknown";
		
//		sprintf((char*)buf, "%d %d %s %s %s %s %s", pEntry->index,
//			pEntry->enable, dnsStatus, pEntry->alias, pEntry->ipAddr,
//			pEntry->interface, dnsType);
		sprintf((char*)buf, "%d %d %s %s %s", pEntry->index,
			pEntry->enable, dnsStatus, pEntry->ipAddr, dnsType);
	}
#endif
	else if (type == DHCPRSVDIP_ARRY_T) {		
		DHCPRSVDIP_Tp pEntry=(DHCPRSVDIP_Tp)array_val;
		sprintf((char *)buf, DHCPRSVDIP_FORMAT, 
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
			pEntry->dhcpRsvdIpEntryEnabled,
#endif
			pEntry->macAddr[0],pEntry->macAddr[1],pEntry->macAddr[2],
			pEntry->macAddr[3],pEntry->macAddr[4],pEntry->macAddr[5],
			inet_ntoa(*((struct in_addr*)pEntry->ipAddr)), pEntry->hostName);
	}	
#if defined(VLAN_CONFIG_SUPPORTED)	
	else if (type == VLANCONFIG_ARRAY_T) 
	{
		OPMODE_T opmode=-1;
		int isLan=1;
    #ifdef RTK_USB3G_PORT5_LAN
        DHCP_T wan_dhcp = -1;
        apmib_get( MIB_DHCP, (void *)&wan_dhcp);
    #endif
		apmib_get( MIB_OP_MODE, (void *)&opmode);
		
		VLAN_CONFIG_Tp pEntry=(VLAN_CONFIG_Tp)array_val;

		if(strncmp((char *)pEntry->netIface,"eth1",strlen("eth1")) == 0)
		{			
    #ifdef RTK_USB3G_PORT5_LAN		
            if(opmode == WISP_MODE || opmode == BRIDGE_MODE || wan_dhcp == USB3G)
    #else
			if(opmode == WISP_MODE || opmode == BRIDGE_MODE)
    #endif
				isLan=1;
			else
				isLan=0;
		}
		else if(strncmp("wlan0",(char *)pEntry->netIface, strlen((char *)pEntry->netIface)) == 0)
		{						
			if(opmode == WISP_MODE)
				isLan=0;
			else
				isLan=1;
		}
		else
		{						
			isLan=1;
		}

		sprintf((char *)buf, VLANCONFIG_FORMAT, 			
			pEntry->netIface,pEntry->enabled,pEntry->tagged,pEntry->priority,pEntry->cfi, pEntry->vlanId,isLan
#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT) || defined(CONFIG_RTL_HW_VLAN_SUPPORT)
			,pEntry->forwarding_rule
#endif
		);
	}

#endif
#if defined(SAMBA_WEB_SUPPORT)	
		else if (type == STORAGE_USER_ARRAY_T) 
		{
			STORAGE_USER_Tp pEntry=(STORAGE_USER_Tp)array_val;
			sprintf((char *)buf, "%s %s", 			
				pEntry->storage_user_name,pEntry->storage_user_password
			);
		}		
		else if (type == STORAGE_SHAREINFO_ARRAY_T) 
		{
			STORAGE_SHAREINFO_Tp pEntry=(STORAGE_SHAREINFO_Tp)array_val;
			sprintf((char *)buf, "%s %s %s %d", 			
				pEntry->storage_sharefolder_name,pEntry->storage_sharefolder_path,
				pEntry->storage_account, pEntry->storage_permission
			);
		}		
#endif	
#if defined(CONFIG_RTL_ETH_802DOT1X_SUPPORT)	
		else if (type == ETHDOT1X_ARRAY_T) 
		{
			ETHDOT1X_Tp pEntry=(ETHDOT1X_Tp)array_val;
			sprintf((char *)buf, "%d %d", 			
				pEntry->enabled,pEntry->portnum
			);
		}		
#endif	
#if defined(CONFIG_8021Q_VLAN_SUPPORTED)
	else if (type == VLAN_ARRAY_T) 
	{
		VLAN_CONFIG_Tp pEntry=(VLAN_CONFIG_Tp)array_val;

		sprintf((char *)buf, "\nVlanType:%d  VlanId:%d  Priority:%d  Member/Tagged:%08x/%08x\n", 		
			pEntry->VlanType, pEntry->VlanId, pEntry->VlanPriority, 
			pEntry->MemberPortMask, pEntry->TaggedPortMask);
	}
#endif

	else if (type == SCHEDULE_ARRAY_T) 
	{
		SCHEDULE_Tp pEntry=(SCHEDULE_Tp)array_val;
		sprintf((char *)buf, SCHEDULE_FORMAT, pEntry->eco, pEntry->day, pEntry->fTime, pEntry->tTime);
#ifndef NEW_SCHEDULE_SUPPORT
		if ( strlen((char *)pEntry->text) ) {
			sprintf((char *)tmp1, ",%s", pEntry->text);
			strcat((char *)buf, (char *)tmp1);
		}
#endif
	}

#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	else if (type == CAP_PORTAL_ALLOW_ARRAY_T) {
		CAP_PORTAL_Tp pEntry=(CAP_PORTAL_Tp)array_val;
		sprintf((char *)buf, "%s",pEntry->ipAddr);
	}
#endif

#ifdef HOME_GATEWAY
	else if (type == PORTFW_ARRAY_T) {
		PORTFW_Tp pEntry=(PORTFW_Tp)array_val;
		#if defined(CONFIG_RTL_PORTFW_EXTEND)
		unsigned char ip_buff[32] = {0};		
		unsigned char rmt_ip_buff[32] = {0};
		sprintf(ip_buff, "%s", inet_ntoa(*((struct in_addr*)pEntry->ipAddr)));
		sprintf(rmt_ip_buff, "%s", inet_ntoa(*((struct in_addr*)pEntry->rmtipAddr)));
		sprintf((char *)buf, "%s, %d, %d, %d, %s, %d", ip_buff,
			 pEntry->fromPort, pEntry->externelFromPort, pEntry->protoType,  rmt_ip_buff, pEntry->enabled);
		#else
		sprintf((char *)buf, PORTFW_FORMAT, inet_ntoa(*((struct in_addr*)pEntry->ipAddr)),
			 pEntry->fromPort, pEntry->toPort, pEntry->protoType, pEntry->InstanceNum);
		#endif
		if ( strlen((char *)pEntry->comment) ) {
			sprintf((char *)tmp1, ", %s", pEntry->comment);
			strcat((char *)buf, (char *)tmp1);
		}
	}
#if defined(CONFIG_APP_TR069) && defined(WLAN_SUPPORT)
	else if (type == CWMP_WLANCONF_ARRAY_T) {
		CWMP_WLANCONF_Tp pEntry=(CWMP_WLANCONF_Tp)array_val;
		//InstanceNum RootIdx VWlanIdx IsConfigured 
		sprintf((char *)buf, CWMP_WLANCONF_FMT, pEntry->InstanceNum, pEntry->RootIdx, pEntry->VWlanIdx, pEntry->IsConfigured, pEntry->RfBandAvailable);
		
	}
#endif	
	else if (type == PORTFILTER_ARRAY_T) {
		PORTFILTER_Tp pEntry=(PORTFILTER_Tp)array_val;
		sprintf((char *)buf, PORTFILTER_FORMAT,
			 pEntry->fromPort, pEntry->toPort, pEntry->protoType);
		if ( strlen((char *)pEntry->comment) ) {
			sprintf((char *)tmp1, ", %s", pEntry->comment);
			strcat((char *)buf, (char *)tmp1);
		}
	}
	else if (type == IPFILTER_ARRAY_T) {
		IPFILTER_Tp pEntry=(IPFILTER_Tp)array_val;
		sprintf((char *)buf, IPFILTER_FORMAT, inet_ntoa(*((struct in_addr*)pEntry->ipAddr)),
			 pEntry->protoType);
		if ( strlen((char *)pEntry->comment) ) {
			sprintf((char *)tmp1, ", %s", pEntry->comment);
			strcat((char *)buf, (char *)tmp1);
		}
#ifdef CONFIG_IPV6
		sprintf((char *)tmp1, ", %s, %d", pEntry->ip6Addr,pEntry->ipVer);
		strcat((char *)buf, (char *)tmp1);	
#endif
	}
	else if (type == MACFILTER_ARRAY_T) {
		MACFILTER_Tp pEntry=(MACFILTER_Tp)array_val;
		sprintf((char *)buf, MACFILTER_COLON_FORMAT, pEntry->macAddr[0],pEntry->macAddr[1],pEntry->macAddr[2],
			 pEntry->macAddr[3],pEntry->macAddr[4],pEntry->macAddr[5]);
		if ( strlen((char *)pEntry->comment) ) {
			sprintf((char *)tmp1, ", %s", pEntry->comment);
			strcat((char *)buf, (char *)tmp1);
		}
	}
	else if (type == URLFILTER_ARRAY_T) {
		URLFILTER_Tp pEntry=(URLFILTER_Tp)array_val;
		if(!strncmp((char *)pEntry->urlAddr,"http://",7))				
			sprintf((char *)buf, STR_FORMAT, pEntry->urlAddr+7);
		else
			sprintf((char *)buf, STR_FORMAT, pEntry->urlAddr);
		//if ( strlen(pEntry->comment) ) {
		//	sprintf(tmp1, ", %s", pEntry->comment);
		//	strcat(buf, tmp1);
		//}
		sprintf(tmp1, ", %d", pEntry->ruleMode);
		strcat(buf, tmp1);
	}
	else if (type == TRIGGERPORT_ARRAY_T) {
		TRIGGERPORT_Tp pEntry=(TRIGGERPORT_Tp)array_val;
		sprintf((char *)buf, TRIGGERPORT_FORMAT,
			 pEntry->tri_fromPort, pEntry->tri_toPort, pEntry->tri_protoType,
			 pEntry->inc_fromPort, pEntry->inc_toPort, pEntry->inc_protoType);
		if ( strlen((char *)pEntry->comment) ) {
			sprintf((char *)tmp1, ", %s", pEntry->comment);
			strcat((char *)buf, (char *)tmp1);
		}
	}
#ifdef GW_QOS_ENGINE
	else if (type == QOS_ARRAY_T) {
		QOS_Tp pEntry=(QOS_Tp)array_val;
            strcpy(tmp1, inet_ntoa(*((struct in_addr*)pEntry->local_ip_start)));
            strcpy(&tmp1[20], inet_ntoa(*((struct in_addr*)pEntry->local_ip_end)));
            strcpy(&tmp1[40], inet_ntoa(*((struct in_addr*)pEntry->remote_ip_start)));
            strcpy(&tmp1[60], inet_ntoa(*((struct in_addr*)pEntry->remote_ip_end)));
		sprintf(buf, QOS_FORMAT, pEntry->enabled,
                      pEntry->priority, pEntry->protocol, 
                      tmp1, &tmp1[20], 
                      pEntry->local_port_start, pEntry->local_port_end, 
                      &tmp1[40], &tmp1[60],
			   pEntry->remote_port_start, pEntry->remote_port_end, pEntry->entry_name);
	}
#endif

#ifdef QOS_BY_BANDWIDTH
	else if (type == QOS_ARRAY_T) {
		IPQOS_Tp pEntry=(IPQOS_Tp)array_val;
            strcpy((char *)tmp1, inet_ntoa(*((struct in_addr*)pEntry->local_ip_start)));
            strcpy((char *)&tmp1[20], inet_ntoa(*((struct in_addr*)pEntry->local_ip_end)));
		sprintf((char *)buf, QOS_FORMAT, pEntry->enabled,
                      pEntry->mac[0],pEntry->mac[1],pEntry->mac[2],pEntry->mac[3],pEntry->mac[4],pEntry->mac[5],
                      pEntry->mode, tmp1, &tmp1[20], 
                      (int)pEntry->bandwidth, (int)pEntry->bandwidth_downlink,
			   pEntry->entry_name);
	}
#endif

//added by shirley  2014/5/21
#ifdef QOS_OF_TR069
else if (type == QOS_CLASS_ARRAY_T) {

		QOSCLASS_Tp pEntry=(QOSCLASS_Tp)array_val;
        char strdIP[20]={0}, strdmask[20]={0}, strsIP[20]={0}, strsmask[20]={0};
        strcpy(strdIP, inet_ntoa(*((struct in_addr*)pEntry->DestIP)));
        strcpy(strdmask, inet_ntoa(*((struct in_addr*)pEntry->DestMask)));
        strcpy(strsIP, inet_ntoa(*((struct in_addr*)pEntry->SourceIP)));
        strcpy(strsmask, inet_ntoa(*((struct in_addr*)pEntry->SourceMask)));
        sprintf(buf, QOSCLASS_FORMAT, pEntry->ClassInstanceNum,
                    pEntry->ClassificationKey,
                    pEntry->ClassificationEnable,
                    pEntry->ClassificationStatus,
                    pEntry->Alias,
                    pEntry->ClassificationOrder,
                    pEntry->ClassInterface,
                    strdIP, strdmask, pEntry->DestIPExclude,
                    strsIP, strsmask, pEntry->SourceIPExclude,
                    pEntry->Protocol, pEntry->ProtocolExclude, 
                    pEntry->DestPort, pEntry->DestPortRangeMax, pEntry->DestPortExclude,
                    pEntry->SourcePort, pEntry->SourcePortRangeMax, pEntry->SourcePortExclude,
                    pEntry->SourceMACAddress[0],
                    pEntry->SourceMACAddress[1],
                    pEntry->SourceMACAddress[2],
                    pEntry->SourceMACAddress[3],
                    pEntry->SourceMACAddress[4],
                    pEntry->SourceMACAddress[5],
		            pEntry->SourceMACMask[0],
                    pEntry->SourceMACMask[1],
                    pEntry->SourceMACMask[2],
                    pEntry->SourceMACMask[3],
                    pEntry->SourceMACMask[4],
                    pEntry->SourceMACMask[5],
                    pEntry->SourceMACExclude,
                    pEntry->DestMACAddress[0],
                    pEntry->DestMACAddress[1],
                    pEntry->DestMACAddress[2],
                    pEntry->DestMACAddress[3],
                    pEntry->DestMACAddress[4],
                    pEntry->DestMACAddress[5],
		            pEntry->DestMACMask[0],
                    pEntry->DestMACMask[1],
                    pEntry->DestMACMask[2],
                    pEntry->DestMACMask[3],
                    pEntry->DestMACMask[4],
                    pEntry->DestMACMask[5],
                    pEntry->DestMACExclude,
                    pEntry->Ethertype, pEntry->EthertypeExclude,
                    pEntry->SSAP, pEntry->SSAPExclude,
                    pEntry->DSAP, pEntry->DSAPExclude,
                    pEntry->LLCControl, pEntry->LLCControlExclude,
                    pEntry->SNAPOUI, pEntry->SNAPOUIExclude,
                    pEntry->SourceVendorClassID, 
                    pEntry->SourceVendorClassIDExclude, 
                    pEntry->SourceVendorClassIDMode,
                    pEntry->DestVendorClassID,
                    pEntry->DestVendorClassIDExclude,
                    pEntry->DestVendorClassIDMode,
                    pEntry->SourceClientID, pEntry->SourceClientIDExclude,
                    pEntry->DestClientID, pEntry->DestClientIDExclude,
                    pEntry->SourceUserClassID, pEntry->SourceUserClassIDExclude,
                    pEntry->DestUserClassID, pEntry->DestUserClassIDExclude,
                    pEntry->SourceVendorSpecificInfo,
                    pEntry->SourceVendorSpecificInfoExclude,
                    pEntry->SourceVendorSpecificInfoEnterprise,
                    pEntry->SourceVendorSpecificInfoSubOption,
                    pEntry->SourceVendorSpecificInfoMode,
                    pEntry->DestVendorSpecificInfo,
                    pEntry->DestVendorSpecificInfoExclude,
                    pEntry->DestVendorSpecificInfoEnterprise,
                    pEntry->DestVendorSpecificInfoSubOption,
                    pEntry->DestVendorSpecificInfoMode,
                    pEntry->TCPACK, pEntry->TCPACKExclude,
                    pEntry->IPLengthMin, pEntry->IPLengthMax, pEntry->IPLengthExclude,
		            pEntry->DSCPCheck, pEntry->DSCPExclude, pEntry->DSCPMark,
		            pEntry->EthernetPriorityCheck, pEntry->EthernetPriorityExclude, pEntry->EthernetPriorityMark,
		            pEntry->VLANIDCheck, pEntry->VLANIDExclude,
                    pEntry->OutOfBandInfo,
                    pEntry->ForwardingPolicy,
                    pEntry->TrafficClass,
                    pEntry->ClassPolicer,
                    pEntry->ClassQueue,
                    pEntry->ClassApp);
       // fprintf(stderr,"%s:%d buf=%s\n",__FUNCTION__,__LINE__,buf);
    }

    else if (type == QOS_QUEUE_ARRAY_T) {
		QOSQUEUE_Tp pEntry=(QOSQUEUE_Tp)array_val;
        sprintf(buf, QOSQUEUE_FORMAT, pEntry->QueueInstanceNum,
                    pEntry->QueueKey,pEntry->QueueEnable, pEntry->Alias, pEntry->TrafficClasses,
                    pEntry->QueueInterface,pEntry->QueueBufferLength,pEntry->QueueWeight, pEntry->QueuePrecedence,
                    pEntry->REDThreshold,pEntry->REDPercentage,pEntry->DropAlgorithm,
                    pEntry->SchedulerAlgorithm, pEntry->ShapingRate, pEntry->ShapingBurstSize);
    }
	else if (type == TR098_APPCONF_ARRAY_T) {
		TR098_APPCONF_Tp pTr098QosApp=(TR098_APPCONF_Tp)array_val;
		
		sprintf(buf, QOSAPP_FORMAT, pTr098QosApp->app_key, pTr098QosApp->app_enable, 
		pTr098QosApp->app_status, pTr098QosApp->app_alias, pTr098QosApp->protocol_identify, pTr098QosApp->app_name, 
		pTr098QosApp->default_policy,  pTr098QosApp->default_class, pTr098QosApp->default_policer, 
		pTr098QosApp->default_queue, pTr098QosApp->default_dscp_mark, pTr098QosApp->default_8021p_mark, pTr098QosApp->instanceNum);


			
			
	}
	else if (type == TR098_FLOWCONF_ARRAY_T) {
		TR098_FLOWCONF_Tp pTr098QosFlow=(TR098_FLOWCONF_Tp)array_val;
		
		sprintf(buf, QOSFLOW_FORMAT, pTr098QosFlow->flow_key,pTr098QosFlow->flow_enable,
						pTr098QosFlow->flow_status, pTr098QosFlow->flow_alias,pTr098QosFlow->flow_type,
						pTr098QosFlow->flow_type_para,pTr098QosFlow->flow_name,pTr098QosFlow->app_identify,
						pTr098QosFlow->qos_policy, pTr098QosFlow->flow_class,pTr098QosFlow->flow_policer,
						pTr098QosFlow->flow_queue,pTr098QosFlow->flow_dscp_mark,pTr098QosFlow->flow_8021p_mark,pTr098QosFlow->instanceNum);

	}
else if( type == QOS_POLICER_ARRAY_T){ //mark_qos
		QOSPOLICER_Tp pEntry=(QOSPOLICER_Tp)array_val;
		sprintf(buf, QOSPOLICER_FORMAT ,pEntry->InstanceNum, pEntry->PolicerKey,pEntry->PolicerEnable,
                         pEntry->Alias, pEntry->CommittedRate, pEntry->CommittedBurstSize, pEntry->ExcessBurstSize,
                         pEntry->PeakRate,pEntry->PeakBurstSize,pEntry->MeterType,pEntry->PossibleMeterTypes,
                         pEntry->ConformingAction,pEntry->PartialConformingAction,pEntry->NonConformingAction);	
    }		
    else if (type == QOS_QUEUESTATS_ARRAY_T) {
		QOSQUEUESTATS_Tp pEntry=(QOSQUEUESTATS_Tp)array_val;
		sprintf(buf, QOSQUEUESTATS_FORMAT, pEntry->InstanceNum,pEntry->Enable,
                         pEntry->Alias, pEntry->Queue, pEntry->Interface);

    }	
	
#endif

#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
	else if (type == DHCP_SERVER_OPTION_ARRAY_T || type == DHCP_CLIENT_OPTION_ARRAY_T) {
		MIB_CE_DHCP_OPTION_Tp pEntry=(MIB_CE_DHCP_OPTION_Tp)array_val;
		sprintf((char *)buf, DHCP_OPTION_FORMAT, 
			pEntry->enable, pEntry->usedFor, pEntry->order,
			pEntry->tag, pEntry->len, pEntry->value, 
			pEntry->ifIndex, pEntry->dhcpOptInstNum, pEntry->dhcpConSPInstNum);
	}
	//else if (type == DHCPS_SERVING_POOL_ARRAY_T || type == DHCP_CLIENT_OPTION_ARRAY_T) {
	else if (type == DHCPS_SERVING_POOL_ARRAY_T) {
		DHCPS_SERVING_POOL_Tp pEntry=(DHCPS_SERVING_POOL_Tp)array_val;
		sprintf((char *)buf, DHCPS_SERVING_POOL_FORMAT,
			pEntry->enable, pEntry->poolorder, pEntry->poolname,
			pEntry->deviceType, pEntry->rsvOptCode, pEntry->sourceinterface, 
			pEntry->vendorclass, pEntry->vendorclassflag, pEntry->vendorclassmode, 
			pEntry->clientid, pEntry->clientidflag, pEntry->userclass, 
			pEntry->userclassflag, pEntry->chaddr, pEntry->chaddrmask, 
			pEntry->chaddrflag, pEntry->localserved, pEntry->startaddr, 
			pEntry->endaddr, pEntry->subnetmask, pEntry->iprouter, 
			pEntry->dnsserver1, pEntry->dnsserver2, pEntry->dnsserver3, 
			pEntry->domainname, pEntry->leasetime, pEntry->dhcprelayip, 
			pEntry->dnsservermode, pEntry->InstanceNum);
	}
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */
#ifdef ROUTE_SUPPORT
	else if (type == STATICROUTE_ARRAY_T) {
		char strIp[20], strMask[20], strGw[20];
		STATICROUTE_Tp pEntry=(STATICROUTE_Tp)array_val;
		strcpy(strIp, inet_ntoa(*((struct in_addr*)pEntry->dstAddr)));
                strcpy(strMask, inet_ntoa(*((struct in_addr*)pEntry->netmask)));
                strcpy(strGw, inet_ntoa(*((struct in_addr*)pEntry->gateway)));
		sprintf((char *)buf, "%s, %s, %s, %d, %d",strIp, strMask, strGw, pEntry->interface, pEntry->metric);
	}
#endif //ROUTE
#ifdef CONFIG_RTL_AIRTIME
	else if (type == AIRTIME_ARRAY_T) {
		AIRTIME_Tp pEntry=(AIRTIME_Tp)array_val;
		sprintf((char *)buf, AIRTIME_FORMAT, 
			inet_ntoa(*((struct in_addr*)pEntry->ipAddr)),
			pEntry->macAddr[0],pEntry->macAddr[1],pEntry->macAddr[2],
			pEntry->macAddr[3],pEntry->macAddr[4],pEntry->macAddr[5],
			pEntry->atm_time);
		if ( strlen((char *)pEntry->comment) ) {
			sprintf((char *)tmp1, ", %s", pEntry->comment);
			strcat((char *)buf, (char *)tmp1);
		}
	}
#endif /* CONFIG_RTL_AIRTIME */
#endif
#ifdef HOME_GATEWAY
#ifdef VPN_SUPPORT
	else if (type == IPSECTUNNEL_ARRAY_T) {
		IPSECTUNNEL_Tp pEntry=(IPSECTUNNEL_Tp)array_val;
		char strLcIp[20], strRtIp[20], strRtGw[20];
		strcpy(strLcIp, inet_ntoa(*((struct in_addr*)pEntry->lc_ipAddr)));
		strcpy(strRtIp, inet_ntoa(*((struct in_addr*)pEntry->rt_ipAddr)));
		strcpy(strRtGw, inet_ntoa(*((struct in_addr*)pEntry->rt_gwAddr)));

		sprintf(buf, IPSECTUNNEL_FORMAT, pEntry->tunnelId, pEntry->enable,  pEntry->connName, 
		pEntry->lcType, strLcIp, pEntry->lc_maskLen, 
		pEntry->rtType, strRtIp, pEntry->rt_maskLen, 
		strRtGw, pEntry->keyMode, 
		pEntry->conType, pEntry->espEncr, pEntry->espAuth, 
		pEntry->psKey, pEntry->ikeEncr, pEntry->ikeAuth, pEntry->ikeKeyGroup, pEntry->ikeLifeTime, 
		pEntry->ipsecLifeTime, pEntry->ipsecPfs, pEntry->spi, pEntry->encrKey, pEntry->authKey, pEntry->authType, pEntry->lcId, pEntry->rtId, pEntry->lcIdType, pEntry->rtIdType, pEntry->rsaKey
		);
	}
#endif
#ifdef CONFIG_IPV6
	else if(type == RADVDPREFIX_T)
		{
			radvdCfgParam_Tp pEntry=(radvdCfgParam_Tp)array_val;
			sprintf(buf,RADVD_FORMAT,
				/*enabled*/
				pEntry->enabled,
				/*interface*/
				pEntry->interface.Name,pEntry->interface.MaxRtrAdvInterval,pEntry->interface.MinRtrAdvInterval,
				pEntry->interface.MinDelayBetweenRAs,pEntry->interface.AdvManagedFlag,pEntry->interface.AdvOtherConfigFlag,pEntry->interface.AdvLinkMTU,
				pEntry->interface.AdvReachableTime,pEntry->interface.AdvRetransTimer,pEntry->interface.AdvCurHopLimit,pEntry->interface.AdvDefaultLifetime,
				pEntry->interface.AdvDefaultPreference,pEntry->interface.AdvSourceLLAddress,pEntry->interface.UnicastOnly,
				/*prefix 1*/
				pEntry->interface.prefix[0].Prefix[0],pEntry->interface.prefix[0].Prefix[1],pEntry->interface.prefix[0].Prefix[2],pEntry->interface.prefix[0].Prefix[3],
				pEntry->interface.prefix[0].Prefix[4],pEntry->interface.prefix[0].Prefix[5],pEntry->interface.prefix[0].Prefix[6],pEntry->interface.prefix[0].Prefix[7],
				pEntry->interface.prefix[0].PrefixLen,pEntry->interface.prefix[0].AdvOnLinkFlag,pEntry->interface.prefix[0].AdvAutonomousFlag,pEntry->interface.prefix[0].AdvValidLifetime,
				pEntry->interface.prefix[0].AdvPreferredLifetime,pEntry->interface.prefix[0].AdvRouterAddr,pEntry->interface.prefix[0].if6to4,pEntry->interface.prefix[0].enabled,
				/*prefix 2*/
				pEntry->interface.prefix[1].Prefix[0],pEntry->interface.prefix[1].Prefix[1],pEntry->interface.prefix[1].Prefix[2],pEntry->interface.prefix[1].Prefix[3],
				pEntry->interface.prefix[1].Prefix[4],pEntry->interface.prefix[1].Prefix[5],pEntry->interface.prefix[1].Prefix[6],pEntry->interface.prefix[1].Prefix[7],
				pEntry->interface.prefix[1].PrefixLen,pEntry->interface.prefix[1].AdvOnLinkFlag,pEntry->interface.prefix[1].AdvAutonomousFlag,pEntry->interface.prefix[1].AdvValidLifetime,
				pEntry->interface.prefix[1].AdvPreferredLifetime,pEntry->interface.prefix[1].AdvRouterAddr,pEntry->interface.prefix[1].if6to4,pEntry->interface.prefix[1].enabled);
		}
	else if(type == DNSV6_T)
		{
			dnsv6CfgParam_Tp pEntry=(dnsv6CfgParam_Tp)array_val;
			sprintf(buf,DNSV6_FORMAT,pEntry->enabled,pEntry->routerName);
		}
	else if(type == DHCPV6S_T)
		{
			dhcp6sCfgParam_Tp pEntry=(dhcp6sCfgParam_Tp)array_val;
			sprintf(buf,DHCPV6S_FORMAT,pEntry->enabled,pEntry->DNSaddr6,pEntry->addr6PoolS,pEntry->addr6PoolE,pEntry->interfaceNameds);
		}	
	else if(type == DHCPV6C_T)
	{
		dhcp6cCfgParam_Tp pEntry=(dhcp6cCfgParam_Tp)array_val;
		sprintf(buf,DHCPV6C_FORMAT,pEntry->enabled,pEntry->ifName,
			pEntry->dhcp6pd.sla_len,pEntry->dhcp6pd.sla_id,pEntry->dhcp6pd.ifName);
			
	}
	else if(type == ADDR6_T)
		{
			daddrIPv6CfgParam_Tp pEntry=(daddrIPv6CfgParam_Tp)array_val;
			sprintf(buf,ADDR6_FORMAT,pEntry->enabled,pEntry->prefix_len[0],pEntry->prefix_len[1],
				pEntry->addrIPv6[0][0],pEntry->addrIPv6[0][1],pEntry->addrIPv6[0][2],pEntry->addrIPv6[0][3],
				pEntry->addrIPv6[0][4],pEntry->addrIPv6[0][5],pEntry->addrIPv6[0][6],pEntry->addrIPv6[0][7],
				pEntry->addrIPv6[1][0],pEntry->addrIPv6[1][1],pEntry->addrIPv6[1][2],pEntry->addrIPv6[1][3],
				pEntry->addrIPv6[1][4],pEntry->addrIPv6[1][5],pEntry->addrIPv6[1][6],pEntry->addrIPv6[1][7]);
		}
	else if(type == ADDRV6_T)
	{
			addr6CfgParam_Tp pEntry=(addr6CfgParam_Tp)array_val;
			sprintf(buf,ADDRV6_FORMAT,pEntry->prefix_len,
				pEntry->addrIPv6[0],pEntry->addrIPv6[1],pEntry->addrIPv6[2],pEntry->addrIPv6[3],
				pEntry->addrIPv6[4],pEntry->addrIPv6[5],pEntry->addrIPv6[6],pEntry->addrIPv6[7]
			);
	}
	else if(type == TUNNEL6_T)
		{
			tunnelCfgParam_Tp pEntry=(tunnelCfgParam_Tp)array_val;
			sprintf(buf,"%d",pEntry->enabled);
		}
#endif
#endif
#ifdef TLS_CLIENT
	else if (type == CERTROOT_ARRAY_T) {
		CERTROOT_Tp pEntry=(CERTROOT_Tp)array_val;
		sprintf(buf, "%s", pEntry->comment);
	}
	else if (type == CERTUSER_ARRAY_T) {
		CERTUSER_Tp pEntry=(CERTUSER_Tp)array_val;
		sprintf(buf, "%s,%s", pEntry->comment, pEntry->pass);
	}	
#endif
	else if (type == WLAC_ARRAY_T) {
		MACFILTER_Tp pEntry=(MACFILTER_Tp)array_val;
		sprintf((char *)buf, MACFILTER_FORMAT, pEntry->macAddr[0],pEntry->macAddr[1],pEntry->macAddr[2],
			 pEntry->macAddr[3],pEntry->macAddr[4],pEntry->macAddr[5]);
		if ( strlen((char *)pEntry->comment) ) {
			sprintf((char *)tmp1, ", %s", pEntry->comment);
			strcat((char *)buf, (char *)tmp1);
		}
	}
	
//#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_) // below code copy above ACL code
	else if (type == MESH_ACL_ARRAY_T) {
		MACFILTER_Tp pEntry=(MACFILTER_Tp)array_val;
		sprintf((char *)buf, MACFILTER_FORMAT, pEntry->macAddr[0],pEntry->macAddr[1],pEntry->macAddr[2],
			 pEntry->macAddr[3],pEntry->macAddr[4],pEntry->macAddr[5]);
		if ( strlen((char *)pEntry->comment) ) {
			sprintf((char *)tmp1, ", %s", pEntry->comment);
			strcat((char *)buf, (char *)tmp1);
		}
	}
//#endif

	else if (type == WDS_ARRAY_T) {
		WDS_Tp pEntry=(WDS_Tp)array_val;
		sprintf((char *)buf, WDS_FORMAT, pEntry->macAddr[0],pEntry->macAddr[1],pEntry->macAddr[2],
			 pEntry->macAddr[3],pEntry->macAddr[4],pEntry->macAddr[5], pEntry->fixedTxRate);
		if ( strlen((char *)pEntry->comment) ) {
			sprintf((char *)tmp1, ", %s", pEntry->comment);
			strcat((char *)buf, (char *)tmp1);
		}
	}
#ifdef WLAN_PROFILE
	else if (type == PROFILE_ARRAY_T) {		
		WLAN_PROFILE_Tp pEntry=(WLAN_PROFILE_Tp)array_val;
		sprintf((char *)buf, "%s,enc=%d,auth=%d", pEntry->ssid,pEntry->encryption,pEntry->auth);
		if (pEntry->encryption == 1 || pEntry->encryption == 2) {			
			if (pEntry->encryption == WEP64)			
				sprintf(tmp1,",id=%d,key1=%02x%02x%02x%02x%02x,key2=%02x%02x%02x%02x%02x,key3=%02x%02x%02x%02x%02x,key4=%02x%02x%02x%02x%02x", 
					pEntry->wep_default_key,
					pEntry->wepKey1[0],pEntry->wepKey1[1],pEntry->wepKey1[2],pEntry->wepKey1[3],pEntry->wepKey1[4],
					pEntry->wepKey2[0],pEntry->wepKey2[1],pEntry->wepKey2[2],pEntry->wepKey2[3],pEntry->wepKey2[4],
					pEntry->wepKey3[0],pEntry->wepKey3[1],pEntry->wepKey3[2],pEntry->wepKey3[3],pEntry->wepKey3[4],
					pEntry->wepKey4[0],pEntry->wepKey4[1],pEntry->wepKey4[2],pEntry->wepKey4[3],pEntry->wepKey4[4]);
			else
				sprintf(tmp1,",id=%d,key1=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x,key2=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x,key3=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x,key4=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
					pEntry->wep_default_key,
					pEntry->wepKey1[0],pEntry->wepKey1[1],pEntry->wepKey1[2],pEntry->wepKey1[3],pEntry->wepKey1[4],pEntry->wepKey1[5],pEntry->wepKey1[6],pEntry->wepKey1[7],pEntry->wepKey1[8],
					pEntry->wepKey1[9],pEntry->wepKey1[10],pEntry->wepKey1[11],pEntry->wepKey1[12],
					pEntry->wepKey2[0],pEntry->wepKey2[1],pEntry->wepKey2[2],pEntry->wepKey2[3],pEntry->wepKey2[4],pEntry->wepKey2[5],pEntry->wepKey2[6],pEntry->wepKey2[7],pEntry->wepKey2[8],
					pEntry->wepKey2[9],pEntry->wepKey2[10],pEntry->wepKey2[11],pEntry->wepKey2[12],
					pEntry->wepKey3[0],pEntry->wepKey3[1],pEntry->wepKey3[2],pEntry->wepKey3[3],pEntry->wepKey3[4],pEntry->wepKey3[5],pEntry->wepKey3[6],pEntry->wepKey3[7],pEntry->wepKey3[8],
					pEntry->wepKey3[9],pEntry->wepKey3[10],pEntry->wepKey3[11],pEntry->wepKey3[12],
					pEntry->wepKey4[0],pEntry->wepKey4[1],pEntry->wepKey4[2],pEntry->wepKey4[3],pEntry->wepKey4[4],pEntry->wepKey4[5],pEntry->wepKey4[6],pEntry->wepKey4[7],pEntry->wepKey4[8],
					pEntry->wepKey4[9],pEntry->wepKey4[10],pEntry->wepKey4[11],pEntry->wepKey4[12]);		
			strcat(buf, tmp1);			
		}
		
		else if (pEntry->encryption == 3 || pEntry->encryption == 4 || pEntry->encryption == 6) {
			sprintf(tmp1, ",wpa_cipher=%d,psk=%s", pEntry->wpa_cipher, pEntry->wpaPSK);			
			strcat(buf, tmp1);						
		}		
	}
#endif	
	
#ifdef VOIP_SUPPORT 
	else if (type == VOIP_T) {
		// use flash voip get xxx to replace
	}
#endif
#ifdef RTK_CAPWAP
	else if (type == CAPWAP_WTP_CONFIG_ARRAY_T) {
		CAPWAP_WTP_CONFIG_Tp pEntry=(CAPWAP_WTP_CONFIG_Tp)array_val;		
		int wtpId = pEntry->wtpId;
		int radioNum = pEntry->radioNum;
		int wlanNum = pEntry->wlanNum;
		sprintf((char *)buf, "{wtpId=%d, radioNum=%d, wlanNum=%d}", wtpId, radioNum, wlanNum);
		//if(wtpId>0) 
		{
			int i, j;
			char tmp_str[1024];
			// print radio information
	                for (i=0; i<radioNum; i++) {
	                //for (i=0; i<MAX_CAPWAP_RADIO_NUM; i++) {
				sprintf(tmp_str, "{radio-%d: powerScale=%d, channel=%d}", 
					i, (int)pEntry->powerScale[i], (int)pEntry->channel[i]);				
				strcat((char *)buf, tmp_str);
			}

			// print wlan information
	                for (i=0; i<radioNum; i++) {
	                        for (j=0; j<wlanNum; j++)
	                        {
	                                if (!pEntry->wlanConfig[i][j].enable) continue;
					sprintf(tmp_str,	
						"{wlan-%d-%d: enable=%d, keyType=%d, pskFormat=%d, "
						"key=%s, ssid=%s, bssid=%02x%02x%02x%02x%02x%02x}\n", 
						i,
						j, 
						(int)pEntry->wlanConfig[i][j].enable, 
						(int)pEntry->wlanConfig[i][j].keyType, 
						(int)pEntry->wlanConfig[i][j].pskFormat, 
						pEntry->wlanConfig[i][j].key, 
						pEntry->wlanConfig[i][j].ssid, 
						pEntry->wlanConfig[i][j].bssid[0], 
						pEntry->wlanConfig[i][j].bssid[1], 
						pEntry->wlanConfig[i][j].bssid[2], 
						pEntry->wlanConfig[i][j].bssid[3], 
						pEntry->wlanConfig[i][j].bssid[4], 
						pEntry->wlanConfig[i][j].bssid[5] );
					strcat((char *)buf, tmp_str);
				}
			}
	                sprintf(tmp_str, "\n");
		}
	}	
	else if (type == CAPWAP_ALL_WLANS_CONFIG_T) {
		CAPWAP_WLAN_CONFIG_Tp pEntry=(CAPWAP_WLAN_CONFIG_Tp)array_val;
		char tmp_str[1024];

		sprintf((char *)buf,	
			"\tenable=%d, keyType=%d, pskFormat=%d, "
			"key=%s, ssid=%s, bssid=%02x:%02x:%02x:%02x:%02x:%02x\n", 
			(int)pEntry->enable, 
			(int)pEntry->keyType, 
			(int)pEntry->pskFormat, 
			pEntry->key, 
			pEntry->ssid, 
			pEntry->bssid[0], 
			pEntry->bssid[1], 
			pEntry->bssid[2], 
			pEntry->bssid[3], 
			pEntry->bssid[4], 
			pEntry->bssid[5] );

		pEntry++;

		sprintf(tmp_str,	
			"\tenable=%d, keyType=%d, pskFormat=%d, "
			"key=%s, ssid=%s, bssid=%02x:%02x:%02x:%02x:%02x:%02x\n", 
			(int)pEntry->enable, 
			(int)pEntry->keyType, 
			(int)pEntry->pskFormat, 
			pEntry->key, 
			pEntry->ssid, 
			pEntry->bssid[0], 
			pEntry->bssid[1], 
			pEntry->bssid[2], 
			pEntry->bssid[3], 
			pEntry->bssid[4], 
			pEntry->bssid[5] );		
		strcat((char *)buf, tmp_str);
	}
#endif

	if (--num > 0) {
		if (!array_separate)
			strcat((char *)tmpBuf, " ");
		else {
			if (tbl){
				if(type == STRING_T)
					printf("%s%d=\"%s\"\n", mibName, index-1, tmpBuf);
				else
					printf("%s%d=%s\n", mibName, index-1, tmpBuf);
			}
			else{
				if(type == STRING_T)
					printf("%s=\"%s\"\n", mibName, tmpBuf);
				else
					printf("%s=%s\n", mibName, tmpBuf);
			}
			tmpBuf[0] = '\0';
		}
		goto getval;
	}
ret:
	if (tbl) {
		if(type == STRING_T)
			printf("%s%d=\"%s\"\n", mibName, index-1, tmpBuf);
		else
			printf("%s%d=%s\n", mibName, index-1, tmpBuf);
	}
	else{
		if(type == STRING_T)
                                printf("%s=\"%s\"\n", mibName, tmpBuf);
		else
			printf("%s=%s\n", mibName, tmpBuf);
	}
}

///////////////////////////////////////////////////////////////////////////////
static void setMIB(char *name, int id, TYPE_T type, int len, int valNum, char **val)
{
	unsigned char key[200];
	struct in_addr ia_val;
	void *value=NULL;
	int int_val, i;
	unsigned int uint_val=0;
	MACFILTER_T wlAc;	// Use with MESH_ACL
	WDS_T wds;

#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	CAP_PORTAL_T capPortalAllowbl;
#endif
#ifdef HOME_GATEWAY
	PORTFW_T portFw;
	PORTFILTER_T portFilter;
	IPFILTER_T ipFilter;
	MACFILTER_T macFilter;
	URLFILTER_T urlFilter;
	TRIGGERPORT_T triggerPort;

#ifdef GW_QOS_ENGINE
	QOS_T qos;
#endif

#ifdef QOS_BY_BANDWIDTH
	IPQOS_T qos;
#endif

//added by shirley 2014/5/21
#ifdef QOS_OF_TR069
    QOSCLASS_T qosclass, qosclass_temp[2] = {0};
    QOSQUEUE_T qosqueue, qosqueue_temp[2] = {0};
	TR098_APPCONF_T tr098AppConf, tr098AppConf_temp[2] = {0};
	TR098_FLOWCONF_T tr098FlowConf, tr098FlowConf_temp[2] = {0};
	struct in_addr inAddr;
    QOSPOLICER_T qospolicer, qospolicer_temp[2] = {0};	//mark_qos
    QOSQUEUESTATS_T qosqueuestats, qosqueuestats_temp[2] = {0};
#endif
#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
	MIB_CE_DHCP_OPTION_T dhcpdOpt;
	MIB_CE_DHCP_OPTION_T dhcpcOpt;
	DHCPS_SERVING_POOL_T servingPool;
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */

#ifdef CONFIG_RTL_AIRTIME
	AIRTIME_T airTime;
#endif

#ifdef ROUTE_SUPPORT
	STATICROUTE_T staticRoute;
#endif
#endif

#ifdef HOME_GATEWAY
#ifdef VPN_SUPPORT
	IPSECTUNNEL_T ipsecTunnel;
#endif

#ifdef CONFIG_IPV6
	char tmp[5];
	char *pstart ,*pend;
	int size;
	radvdCfgParam_t radvdCfgParam;
	dnsv6CfgParam_t dnsv6CfgParam;
	dhcp6sCfgParam_t dhcp6sCfgParam;
	dhcp6cCfgParam_t dhcp6cCfgParam;
	addrIPv6CfgParam_t addrIPv6CfgParam;
	addr6CfgParam_t addr6CfgParam;
	tunnelCfgParam_t tunnelCfgParam;
#ifdef TR181_SUPPORT
	DHCPV6C_SENDOPT_T dhcp6cSendOpt;
#endif
#endif
#ifdef TR181_SUPPORT
	DNS_CLIENT_SERVER_T dnsClientServer;
#endif
#endif
#ifdef TLS_CLIENT
	CERTROOT_T certRoot;
	CERTUSER_T certUser;
#endif
	DHCPRSVDIP_T dhcpRsvd;
	
#if defined(VLAN_CONFIG_SUPPORTED)	
	VLAN_CONFIG_T vlanConfig_entry;
#endif		
#if defined(SAMBA_WEB_SUPPORT)	
	STORAGE_USER_T user_entry;
	STORAGE_SHAREINFO_T shareinfo_entry;
	int k;
#endif		
#if defined(CONFIG_RTL_ETH_802DOT1X_SUPPORT)	
	ETHDOT1X_T ethdot1x_entry;
#endif		
#if defined(CONFIG_8021Q_VLAN_SUPPORTED)	
	VLAN_CONFIG_T vlan_entry;
#endif	

#ifdef WLAN_PROFILE
	WLAN_PROFILE_T profile;
#endif
	SCHEDULE_T wlschedule[2]={0};
#ifdef RTK_CAPWAP
	CAPWAP_WTP_CONFIG_T wtpConfig; 			
#endif

	int entryNum;
	int max_chan_num=0, tx_power_cnt=0;

	switch (type) {
	case BYTE_T:
	case WORD_T:
		int_val = atoi(val[0]);
		value = (void *)&int_val;
		break;

	case IA_T:
		if ( !inet_aton(val[0], &ia_val) ) {
			printf("invalid internet address!\n");
			return;
		}
		value = (void *)&ia_val;
		break;

	case BYTE5_T:
		if ( strlen(val[0])!=10 || !string_to_hex(val[0], key, 10)) {
			printf("invalid value!\n");
			return;
		}
		value = (void *)key;
		break;

	case BYTE6_T:
		if ( strlen(val[0])!=12 || !string_to_hex(val[0], key, 12)) {
			printf("invalid value!\n");
			return;
		}
		value = (void *)key;
		break;

	case BYTE_ARRAY_T:
		#if defined(CONFIG_RTL_8196B)
		if ( (id!=MIB_HW_TX_POWER_CCK) && (id!=MIB_HW_TX_POWER_OFDM_1S) && (id!=MIB_HW_TX_POWER_OFDM_2S)) {		
			printf("invalid mib!\n");
			return;
		}
			max_chan_num = (id==MIB_HW_TX_POWER_CCK)? MAX_CCK_CHAN_NUM : MAX_OFDM_CHAN_NUM ;
		
			for (i=0; i<max_chan_num ; i++) {
				if(val[i] == NULL) break;
				if ( !sscanf(val[i], "%d", &int_val) ) {
					printf("invalid value!\n");
					return;
				}
				key[i] = (unsigned char)int_val;
				tx_power_cnt ++;
			}		
			if(tx_power_cnt != 1 && tx_power_cnt !=2 && tx_power_cnt != max_chan_num){
				unsigned char key_tmp[170];
				memcpy(key_tmp, key, tx_power_cnt);		
				APMIB_GET(id, key);
				memcpy(key, key_tmp, tx_power_cnt);
			}
			if(tx_power_cnt == 1){
				for(i=1 ; i < max_chan_num; i++) {
					key[i] = key[0];
				}
			}
			else if(tx_power_cnt ==2){
				//key[0] is channel number to set
				//key[1] is tx power value
				if(key[0] < 1 || key[0] > max_chan_num){
					if((key[0]<1) || (id==MIB_HW_TX_POWER_CCK) || ((id==MIB_HW_TX_POWER_OFDM) && (key[0]>216))){
						printf("invalid channel number\n");
						return;
					}
					else{
						if ((key[0] >= 163) && (key[0] <= 181))
							key[0] -= 148;
						else // 182 ~ 216
							key[0] -= 117;
					}
				}
				key[2] = 0xff ;
			}
#elif defined(CONFIG_RTL_8198C)||defined(CONFIG_RTL_8196C) || defined(CONFIG_RTL_8198) || defined(CONFIG_RTL_819XD) || defined(CONFIG_RTL_8196E) || defined(CONFIG_RTL_8198B) || defined(CONFIG_RTL_8197F)
		if(!(id >= MIB_HW_TX_POWER_CCK_A &&  id <=MIB_HW_TX_POWER_DIFF_OFDM) &&
			!(id >= MIB_HW_TX_POWER_5G_HT40_1S_A &&  id <=MIB_HW_TX_POWER_DIFF_5G_OFDM)
#if defined(CONFIG_RTL_8812_SUPPORT) || defined(CONFIG_WLAN_HAL_8822BE)
			&& !(id >= MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_A &&  id <=MIB_HW_TX_POWER_DIFF_5G_80BW4S_160BW4S_B)
#endif
#if defined(CONFIG_WLAN_HAL_8814AE)
        && !(id >= MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_C &&  id <=MIB_HW_TX_POWER_5G_HT40_1S_D)
#endif
			&& (id != MIB_L2TP_PAYLOAD)){
				printf("invalid mib!\n");
				return;
		}
		if((id >= MIB_HW_TX_POWER_CCK_A &&  id <=MIB_HW_TX_POWER_DIFF_OFDM)
#if defined(CONFIG_WLAN_HAL_8814AE)
			||(id >= MIB_HW_TX_POWER_CCK_C &&  id <=MIB_HW_TX_POWER_CCK_D)
			||(id >= MIB_HW_TX_POWER_HT40_1S_C &&  id <=MIB_HW_TX_POWER_HT40_1S_D)
#endif
			)
			max_chan_num = MAX_2G_CHANNEL_NUM_MIB;
		else if((id >= MIB_HW_TX_POWER_5G_HT40_1S_A &&  id <=MIB_HW_TX_POWER_DIFF_5G_OFDM)
#if defined(CONFIG_WLAN_HAL_8814AE)
			||(id >= MIB_HW_TX_POWER_5G_HT40_1S_C &&  id <=MIB_HW_TX_POWER_5G_HT40_1S_D)
#endif
			)
			max_chan_num = MAX_5G_CHANNEL_NUM_MIB;

#if defined(CONFIG_RTL_8812_SUPPORT) || defined(CONFIG_WLAN_HAL_8822BE)
		if(((id >= MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_A) && (id <= MIB_HW_TX_POWER_DIFF_OFDM4T_CCK4T_A)) 
			|| ((id >= MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_B) && (id <= MIB_HW_TX_POWER_DIFF_OFDM4T_CCK4T_B)) )
			max_chan_num = MAX_2G_CHANNEL_NUM_MIB;

		if(((id >= MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_A) && (id <= MIB_HW_TX_POWER_DIFF_5G_80BW4S_160BW4S_A)) 
			|| ((id >= MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_B) && (id <= MIB_HW_TX_POWER_DIFF_5G_80BW4S_160BW4S_B)) )
			max_chan_num = MAX_5G_DIFF_NUM;
#endif

#if defined(CONFIG_WLAN_HAL_8814AE)
		if(((id >= MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_C) && (id <= MIB_HW_TX_POWER_DIFF_OFDM4T_CCK4T_C)) 
			|| ((id >= MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_D) && (id <= MIB_HW_TX_POWER_DIFF_OFDM4T_CCK4T_D)) )
			max_chan_num = MAX_2G_CHANNEL_NUM_MIB;

		if(((id >= MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_C) && (id <= MIB_HW_TX_POWER_DIFF_5G_80BW4S_160BW4S_C)) 
			|| ((id >= MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_D) && (id <= MIB_HW_TX_POWER_DIFF_5G_80BW4S_160BW4S_D)) )
			max_chan_num = MAX_5G_DIFF_NUM;
#endif
		
#if defined(RTL_L2TP_POWEROFF_PATCH)
	if(id == MIB_L2TP_PAYLOAD)
	{
		if ( strlen(val[0])!=108 || !string_to_hex(val[0], key, 108)) {
		printf("\ninvalid value!!!!!!!!!!!!!!! strlen(val[0])=%d\n", strlen(val[0]));
		return;
		}
		value = (void *)key;
		break;
	}
#endif
			
			for (i=0; i<max_chan_num ; i++) {
				if(val[i] == NULL) break;
				if ( !sscanf(val[i], "%d", &int_val) ) {
					printf("invalid value!\n");
					return;
				}
				key[i+1] = (unsigned char)int_val;
				tx_power_cnt ++;
			}	
			if(tx_power_cnt != 1 && tx_power_cnt !=2 && tx_power_cnt != max_chan_num){
				unsigned char key_tmp[200];
				memcpy(key_tmp, key+1, tx_power_cnt);		
				APMIB_GET(id, key+1);
				memcpy(key+1, key_tmp, tx_power_cnt);
			}
			if(tx_power_cnt == 1){
				for(i=1 ; i <= max_chan_num; i++) {
					key[i] = key[1];
				}
			}
		else if(tx_power_cnt ==2){
			//key[1] is channel number to set
			//key[2] is tx power value
	                //key[3] is tx power key for check set mode
			if(key[1] < 1 || key[1] > max_chan_num){
				if((key[1]<1) || ((id >= MIB_HW_TX_POWER_CCK_A &&  id <=MIB_HW_TX_POWER_DIFF_OFDM)) ||
					 ((id >= MIB_HW_TX_POWER_5G_HT40_1S_A &&  id <=MIB_HW_TX_POWER_DIFF_5G_OFDM) && (key[1]>216))){
					printf("invalid channel number\n");
					return;
				}
			}
			key[3] = 0xff ;
		}
		key[0] = tx_power_cnt;
		#else
		//!CONFIG_RTL_8196B => rtl8651c+rtl8190
		if ( (id!=MIB_HW_TX_POWER_CCK) && (id!=MIB_HW_TX_POWER_OFDM)) {		
			printf("invalid mib!\n");
			return;
		}
		max_chan_num = (id==MIB_HW_TX_POWER_CCK)? MAX_CCK_CHAN_NUM : MAX_OFDM_CHAN_NUM ;
		for (i=0; i<max_chan_num ; i++) {
			if(val[i] == NULL) break;
			if ( !sscanf(val[i], "%d", &int_val) ) {
				printf("invalid value!\n");
				return;
			}
			key[i] = (unsigned char)int_val;
			tx_power_cnt ++;
		}		
		if(tx_power_cnt != 1 && tx_power_cnt !=2 && tx_power_cnt != max_chan_num){
			unsigned char key_tmp[170];
			memcpy(key_tmp, key, tx_power_cnt);		
			APMIB_GET(id, key);
			memcpy(key, key_tmp, tx_power_cnt);
		}
		if(tx_power_cnt == 1){
			for(i=1 ; i < max_chan_num; i++) {
				key[i] = key[0];
			}
		}
		else if(tx_power_cnt ==2){
			//key[0] is channel number to set
			//key[1] is tx power value
			if(key[0] < 1 || key[0] > max_chan_num){
				if((key[0]<1) || (id==MIB_HW_TX_POWER_CCK) || ((id==MIB_HW_TX_POWER_OFDM) && (key[0]>216))){
					printf("invalid channel number\n");
					return;
				}
				else{
					if ((key[0] >= 163) && (key[0] <= 181))
						key[0] -= 148;
					else // 182 ~ 216
						key[0] -= 117;
				}
			}
			key[2] = 0xff ;
		}
		#endif
		value = (void *)key;
		break;
	case DWORD_T:
		//printf("%s:%d uval=%s\n",__FUNCTION__,__LINE__,val[0]);
		uint_val = strtoul(val[0],0,10);//atoi(val[0]);
		//printf("%s:%d uval=%u %d\n",__FUNCTION__,__LINE__,atoi(val[0]),atoi(val[0]));
		value = (void *)&uint_val;
		break;
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	case CAP_PORTAL_ALLOW_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_CAP_PORTAL_ALLOW_TBL_ADD;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			strcpy(capPortalAllowbl.ipAddr,val[1]);
		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_CAP_PORTAL_ALLOW_TBL_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_CAP_PORTAL_ALLOW_TBL_NUM, (void *)&entryNum)) {
				printf("Get capPortalAllowtbl entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&capPortalAllowbl) = (char)int_val;
			if ( !APMIB_GET(MIB_CAP_PORTAL_ALLOW_TBL, (void *)&capPortalAllowbl)) {
				printf("Get table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_CAP_PORTAL_ALLOW_TBL_DELALL;
		value = (void *)&capPortalAllowbl;
		break;
#endif

#ifdef HOME_GATEWAY
	case PORTFW_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_PORTFW_ADD;
			if ( valNum < 6 ) {
				printf("input argument is not enough!\n");
				return;
			}
			if ( !inet_aton(val[1], (struct in_addr *)&portFw.ipAddr)) {
				printf("invalid internet address!\n");
				return;
			}
			#if defined(CONFIG_RTL_PORTFW_EXTEND)
			portFw.toPort = portFw.fromPort = atoi(val[2]);
			portFw.externeltoPort = portFw.externelFromPort = atoi(val[3]);
			portFw.protoType = atoi(val[4]);
			inet_aton(val[5], (struct in_addr *)&portFw.rmtipAddr);
			if ( valNum > 7)
				portFw.enabled = atoi(val[7]);
			else
				portFw.enabled = 1;
			#else
			portFw.fromPort = atoi(val[2]);
			portFw.toPort = atoi(val[3]);
			portFw.protoType = atoi(val[4]);
			portFw.InstanceNum = atoi(val[5]);
			#endif
			if ( valNum > 6)
				strcpy((char *)portFw.comment, val[6]);
			else
				portFw.comment[0] = '\0';

		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_PORTFW_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_PORTFW_TBL_NUM, (void *)&entryNum)) {
				printf("Get port forwarding entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&portFw) = (char)int_val;
			if ( !APMIB_GET(MIB_PORTFW_TBL, (void *)&portFw)) {
				printf("Get table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_PORTFW_DELALL;

		value = (void *)&portFw;
		break;

	case PORTFILTER_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_PORTFILTER_ADD;
			if ( valNum < 4 ) {
				printf("input argument is not enough!\n");
				return;
			}
			portFilter.fromPort = atoi(val[1]);
			portFilter.toPort = atoi(val[2]);
			portFilter.protoType = atoi(val[3]);
			if ( valNum > 4)
				strcpy((char *)portFilter.comment, val[4]);
			else
				portFilter.comment[0] = '\0';
#ifdef CONFIG_IPV6
			portFilter.ipVer = atoi(val[5]);
#endif

		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_PORTFILTER_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_PORTFILTER_TBL_NUM, (void *)&entryNum)) {
				printf("Get port filter entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&portFilter) = (char)int_val;
			if ( !APMIB_GET(MIB_PORTFILTER_TBL, (void *)&portFilter)) {
				printf("Get table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_PORTFILTER_DELALL;

		value = (void *)&portFilter;
		break;

	case IPFILTER_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_IPFILTER_ADD;
			if ( valNum < 3 ) {
				printf("input argument is not enough!\n");
				return;
			}
			if ( !inet_aton(val[1], (struct in_addr *)&ipFilter.ipAddr)) {
				printf("invalid internet address!\n");
				return;
			}
			ipFilter.protoType = atoi(val[2]);
			if ( valNum > 3)
				strcpy((char *)ipFilter.comment, val[3]);
			else
				ipFilter.comment[0] = '\0';
#ifdef CONFIG_IPV6
			if ( valNum > 4)
				strcpy(ipFilter.ip6Addr, val[4]);
			else
				memset(ipFilter.ip6Addr,0,48);

			if(valNum > 5)
				ipFilter.ipVer = atoi(val[5]);
			else
				ipFilter.ipVer = 0;
#endif

		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_IPFILTER_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_IPFILTER_TBL_NUM, (void *)&entryNum)) {
				printf("Get port forwarding entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&ipFilter) = (char)int_val;
			if ( !APMIB_GET(MIB_IPFILTER_TBL, (void *)&ipFilter)) {
				printf("Get table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_IPFILTER_DELALL;
		
		value = (void *)&ipFilter;
		break;

	case MACFILTER_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_MACFILTER_ADD;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			if ( strlen(val[1])!=12 || !string_to_hex(val[1], macFilter.macAddr, 12)) {
				printf("invalid value!\n");
				return;
			}

			if ( valNum > 2)
				strcpy((char *)macFilter.comment, val[2]);
			else
				macFilter.comment[0] = '\0';

		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_MACFILTER_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_MACFILTER_TBL_NUM, (void *)&entryNum)) {
				printf("Get port forwarding entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&macFilter) = (char)int_val;
			if ( !APMIB_GET(MIB_MACFILTER_TBL, (void *)&macFilter)) {
				printf("Get table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_MACFILTER_DELALL;
		value = (void *)&macFilter;
		break;

	case URLFILTER_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_URLFILTER_ADD;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			//strcpy(urlFilter.urlAddr, val[1]);
			//if ( strlen(val[1])!=12 || !string_to_hex(val[1], wlAc.macAddr, 12)) {
			//	printf("invalid value!\n");
			//	return;
			//}

			//if ( valNum > 2)
			//	strcpy(urlFilter.comment, val[2]);
			//else
			//	uslFilter.comment[0] = '\0';
			if(!strncmp(val[1],"http://",7))				
				memcpy(urlFilter.urlAddr, (&val[1][0])+7, 20);
			else
				memcpy(urlFilter.urlAddr, val[1], 20);

			if (val[2])
				urlFilter.ruleMode=atoi(val[2]);

		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_URLFILTER_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_URLFILTER_TBL_NUM, (void *)&entryNum)) {
				printf("Get port forwarding entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&urlFilter) = (char)int_val;
			if ( !APMIB_GET(MIB_URLFILTER_TBL, (void *)&urlFilter)) {
				printf("Get table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_URLFILTER_DELALL;
		value = (void *)&urlFilter;
		break;
		
	case TRIGGERPORT_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_TRIGGERPORT_ADD;
			if ( valNum < 7 ) {
				printf("input argument is not enough!\n");
				return;
			}
			triggerPort.tri_fromPort = atoi(val[1]);
			triggerPort.tri_toPort = atoi(val[2]);
			triggerPort.tri_protoType = atoi(val[3]);
			triggerPort.inc_fromPort = atoi(val[4]);
			triggerPort.inc_toPort = atoi(val[5]);
			triggerPort.inc_protoType = atoi(val[6]);

			if ( valNum > 7)
				strcpy((char *)triggerPort.comment, val[7]);
			else
				triggerPort.comment[0] = '\0';

		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_TRIGGERPORT_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_TRIGGERPORT_TBL_NUM, (void *)&entryNum)) {
				printf("Get trigger-port entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&triggerPort) = (char)int_val;
			if ( !APMIB_GET(MIB_TRIGGERPORT_TBL, (void *)&triggerPort)) {
				printf("Get trigger-port table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_TRIGGERPORT_DELALL;

		value = (void *)&triggerPort;
		break;
#ifdef GW_QOS_ENGINE
	case QOS_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_QOS_ADD;
			if ( valNum < 13 ) {
				printf("input argument is not enough!\n");
				return;
			}
			qos.enabled = atoi(val[1]);
			if ( !inet_aton(val[4], (struct in_addr *)&qos.local_ip_start) ||
                        !inet_aton(val[5], (struct in_addr *)&qos.local_ip_end) ||
                        !inet_aton(val[8], (struct in_addr *)&qos.remote_ip_start) ||
                        !inet_aton(val[9], (struct in_addr *)&qos.remote_ip_end) 
                      ) {
				printf("invalid internet address!\n");
				return;
			}
			qos.priority = atoi(val[2]);
			qos.protocol = atoi(val[3]);
			qos.local_port_start = atoi(val[6]);
			qos.local_port_end = atoi(val[7]);
			qos.remote_port_start = atoi(val[10]);
			qos.remote_port_end = atoi(val[11]);
			strcpy(qos.entry_name, val[12]);

		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_QOS_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum)) {
				printf("Get QoS entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&qos) = (char)int_val;
			if ( !APMIB_GET(MIB_QOS_RULE_TBL, (void *)&qos)) {
				printf("Get table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_QOS_DELALL;

		value = (void *)&qos;
		break;
#endif

#ifdef QOS_BY_BANDWIDTH
	case QOS_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_QOS_ADD;
			if ( valNum < 9 ) {
				printf("input argument is not enough!\n");
				return;
			}
			qos.enabled = atoi(val[1]);
			if ( !inet_aton(val[4], (struct in_addr *)&qos.local_ip_start) ||
                        !inet_aton(val[5], (struct in_addr *)&qos.local_ip_end) 
                      ) {
				printf("invalid internet address!\n");
				return;
			}
			//strcpy(qos.mac, val[2]);
			if (strlen(val[2])!=12 || !string_to_hex(val[2], qos.mac, 12))  {
				printf("invalid MAC address!\n");
				return;			
			}
			qos.mode = atoi(val[3]);
			qos.bandwidth = atoi(val[6]);
			strcpy((char *)qos.entry_name, val[7]);
			qos.bandwidth_downlink = atoi(val[8]);

		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_QOS_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum)) {
				printf("Get QoS entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&qos) = (char)int_val;
			if ( !APMIB_GET(MIB_QOS_RULE_TBL, (void *)&qos)) {
				printf("Get table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_QOS_DELALL;

		value = (void *)&qos;
		break;
#endif

//added by shirley
#ifdef QOS_OF_TR069
        case QOS_CLASS_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_QOS_CLASS_ADD;
			if ( valNum < 81 ) {
				printf("input argument is not enough!\n");
				return;
			}

            
            qosclass.ClassInstanceNum = (unsigned int)atoi(val[1]);
            qosclass.ClassificationKey = (unsigned short)atoi(val[2]);
            qosclass.ClassificationEnable = (unsigned char)atoi(val[3]);
            strcpy(qosclass.ClassificationStatus, val[4]);
            strcpy(qosclass.Alias, val[5]);
            qosclass.ClassificationOrder = (unsigned int)atoi(val[6]);
            strcpy( qosclass.ClassInterface, val[7]);
            
            if(strlen(val[8])){
                if ( !inet_aton(val[8], (struct in_addr *)&qosclass.DestIP)){
                    printf("invalid Dest IP address!\n");
     				return;
			    }
            }else{
            strcpy(qosclass.DestIP, "");
                }
            fprintf(stderr, "%s:%d, val[8] = %s, DestIP = %s\n", __FUNCTION__, __LINE__, val[8], inet_ntoa(*((struct in_addr*)&qosclass.DestIP)));
            if(strlen(val[9])){
                if ( !inet_aton(val[9], (struct in_addr *)&qosclass.DestMask)){
                    printf("invalid Dest Mask address!\n");
     				return;
			    }
            }else{
            strcpy(qosclass.DestMask, "");
                }
            if(strlen(val[11])){
                if ( !inet_aton(val[11], (struct in_addr *)&qosclass.SourceIP)){
                    printf("invalid Source IP address!\n");
     				return;
			    }
            }else{
            strcpy(qosclass.SourceIP, "");
                }
            if(strlen(val[12])){
                if ( !inet_aton(val[12], (struct in_addr *)&qosclass.SourceMask)){
                    printf("invalid Source Mask address!\n");
     				return;
			    }
            }else{
            strcpy(qosclass.SourceMask, "");
                }

            qosclass.DestIPExclude = (unsigned char)atoi(val[10])  ;
            qosclass.SourceIPExclude = (unsigned char)atoi(val[13]);
            qosclass.Protocol = atoi(val[14]) ;
            qosclass.ProtocolExclude = (unsigned char)atoi(val[15]);
            qosclass.DestPort = atoi(val[16]);
            qosclass.DestPortRangeMax = atoi(val[17]);
            qosclass.DestPortExclude = (unsigned char)atoi(val[18]);
            qosclass.SourcePort = atoi(val[19]);
            qosclass.SourcePortRangeMax = atoi(val[20]);
            qosclass.SourcePortExclude = (unsigned char)atoi(val[21]);
            if(strlen(val[22])){
            if (strlen(val[22])!=12 || !string_to_hex(val[22], qosclass.SourceMACAddress, 12)) 
                {
				printf("invalid Source MAC address!\n");
				return;
			}
                }else{
                strcpy(qosclass.SourceMACAddress, "");
            }

            if(strlen(val[25])){
            if (strlen(val[25])!=12 || !string_to_hex(val[25], qosclass.DestMACAddress, 12)) 
                {
				printf("invalid Dest MAC address!\n");
				return;
			}
                }else{
                strcpy(qosclass.DestMACAddress, "");
            }

            if(strlen(val[23])){
                if (strlen(val[23])!=12 || !string_to_hex(val[23], qosclass.SourceMACMask, 12)) 
                    {
                    printf("invalid Source MAC Mask!\n");
                    return;
                }
            }else{
                    strcpy(qosclass.SourceMACMask, "");
            }
                
            if(strlen(val[26])){
                if (strlen(val[26])!=12 || !string_to_hex(val[26], qosclass.DestMACMask, 12)) 
                    {
                    printf("invalid Dest MAC Mask!\n");
                    return;
                }
            }else{
                    strcpy(qosclass.DestMACMask, "");
            }


            qosclass.SourceMACExclude = (unsigned char)atoi(val[24]);
            qosclass.DestMACExclude = (unsigned char)atoi(val[27]);          
            qosclass.Ethertype = atoi(val[28]);
            qosclass.EthertypeExclude = (unsigned char)atoi(val[29]);
            qosclass.SSAP = atoi(val[30]);
            qosclass.SSAPExclude = (unsigned char)atoi(val[31]);
            qosclass.DSAP = atoi(val[32]);
            qosclass.DSAPExclude = (unsigned char)atoi(val[33]);
            qosclass.LLCControl = atoi(val[34]);
            qosclass.LLCControlExclude = (unsigned char)atoi(val[35]);
            qosclass.SNAPOUI = atoi(val[36]);
            qosclass.SNAPOUIExclude = (unsigned char)atoi(val[37]);
            
            strcpy(qosclass.SourceVendorClassID, val[38]);
            qosclass.SourceVendorClassIDExclude = (unsigned char)atoi(val[39]);
            strcpy(qosclass.SourceVendorClassIDMode, val[40]);
            
            strcpy(qosclass.DestVendorClassID, val[41]);
            qosclass.DestVendorClassIDExclude = (unsigned char)atoi(val[42]);
            strcpy(qosclass.DestVendorClassIDMode, val[43]);
            
            strcpy(qosclass.SourceClientID, val[44]);
            qosclass.SourceClientIDExclude = (unsigned char)atoi(val[45]);
            strcpy(qosclass.DestClientID, val[46]);
            qosclass.DestClientIDExclude = (unsigned char)atoi(val[47]);
            
            strcpy(qosclass.SourceUserClassID, val[48]);
            qosclass.SourceUserClassIDExclude = (unsigned char)atoi(val[49]);
            strcpy(qosclass.DestUserClassID, val[50]);
            qosclass.DestUserClassIDExclude = (unsigned char)atoi(val[51]);
            
            strcpy(qosclass.SourceVendorSpecificInfo, val[52]);
            qosclass.SourceVendorSpecificInfoExclude = (unsigned char)atoi(val[53]);
            qosclass.SourceVendorSpecificInfoEnterprise = (unsigned int)atoi(val[54]);
            qosclass.SourceVendorSpecificInfoSubOption = (unsigned short)atoi(val[55]);
            strcpy(qosclass.SourceVendorSpecificInfoMode, val[56]);
            strcpy(qosclass.DestVendorSpecificInfo, val[57]);
            qosclass.DestVendorSpecificInfoExclude = (unsigned char)atoi(val[58]);
            qosclass.DestVendorSpecificInfoEnterprise = (unsigned int)atoi(val[59]);
            qosclass.DestVendorSpecificInfoSubOption = (unsigned short)atoi(val[60]);
            strcpy(qosclass.DestVendorSpecificInfoMode, val[61]);
            
            qosclass.TCPACK = (unsigned char)atoi(val[62]);
            qosclass.TCPACKExclude = (unsigned char)atoi(val[63]);
            qosclass.IPLengthMin = atoi(val[64]);
            qosclass.IPLengthMax = atoi(val[65]);
            qosclass.IPLengthExclude = (unsigned char)atoi(val[66]);
            qosclass.DSCPCheck = atoi(val[67]);
            qosclass.DSCPExclude = (unsigned char)atoi(val[68]);
            qosclass.DSCPMark = atoi(val[69]);
            qosclass.EthernetPriorityCheck = atoi(val[70]);
            qosclass.EthernetPriorityExclude = (unsigned char)atoi(val[71]);
            qosclass.EthernetPriorityMark = atoi(val[72]);
            qosclass.VLANIDCheck = atoi(val[73]);
            qosclass.VLANIDExclude = (unsigned char)atoi(val[74]);
            qosclass.OutOfBandInfo = atoi(val[75]);
            qosclass.ForwardingPolicy = (unsigned int)atoi(val[76]);
            qosclass.TrafficClass = atoi(val[77]);
            qosclass.ClassPolicer = atoi(val[78]);
            qosclass.ClassQueue = atoi(val[79]);
            qosclass.ClassApp = atoi(val[80]);
            value = (void *)&qosclass;

		}
        else if ( !strcmp(val[0], "mod")){

			id=MIB_QOS_CLASS_MOD;
			if ( valNum < 82 ) {
				printf("input argument is not enough!\n");
				return;
            }
			
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_QOS_CLASS_TBL_NUM, (void *)&entryNum)) {
				printf("Get QOS_CLASS_TBL total entry number error!");
				return;
			}
			if ( int_val > entryNum || int_val<1) {
				printf("Put wrong entry number!\n");
				return;
			}
			*((char *)&(qosclass_temp[0])) = (char)int_val;
			if ( !APMIB_GET(MIB_QOS_CLASS_TBL, (void *)&(qosclass_temp[0]))) {
				printf("Get table entry error!");
				return;
			}

            qosclass_temp[1].ClassInstanceNum = (unsigned int)atoi(val[2]);
            qosclass_temp[1].ClassificationKey = (unsigned char)atoi(val[3]);
            qosclass_temp[1].ClassificationEnable = (unsigned char)atoi(val[4]);
            strcpy(qosclass_temp[1].ClassificationStatus, val[5]);
            strcpy(qosclass_temp[1].Alias, val[6]);
            qosclass_temp[1].ClassificationOrder = (unsigned int)atoi(val[7]);
            strcpy( qosclass_temp[1].ClassInterface, val[8]);
            
            if(strlen(val[9])){
                if ( !inet_aton(val[9], (struct in_addr *)&qosclass_temp[1].DestIP)){
                    printf("invalid Dest IP address!\n");
     				return;
			    }
            }else{
            strcpy(qosclass_temp[1].DestIP, "");
                }

            if(strlen(val[10])){
                if ( !inet_aton(val[10], (struct in_addr *)&qosclass_temp[1].DestMask)){
                    printf("invalid Dest Mask address!\n");
     				return;
			    }
            }else{
            strcpy(qosclass_temp[1].DestMask, "");
                }
            if(strlen(val[12])){
                if ( !inet_aton(val[12], (struct in_addr *)&qosclass_temp[1].SourceIP)){
                    printf("invalid Source IP address!\n");
     				return;
			    }
            }else{
            strcpy(qosclass_temp[1].SourceIP, "");
                }
            if(strlen(val[13])){
                if ( !inet_aton(val[13], (struct in_addr *)&qosclass_temp[1].SourceMask)){
                    printf("invalid Source Mask address!\n");
     				return;
			    }
            }else{
            strcpy(qosclass_temp[1].SourceMask, "");
                }

            qosclass_temp[1].DestIPExclude = (unsigned char)atoi(val[11])  ;
            qosclass_temp[1].SourceIPExclude = (unsigned char)atoi(val[14]);
            qosclass_temp[1].Protocol = atoi(val[15]) ;
            qosclass_temp[1].ProtocolExclude = (unsigned char)atoi(val[16]);
            qosclass_temp[1].DestPort = atoi(val[17]);
            qosclass_temp[1].DestPortRangeMax = atoi(val[18]);
            qosclass_temp[1].DestPortExclude = (unsigned char)atoi(val[19]);
            qosclass_temp[1].SourcePort = atoi(val[20]);
            qosclass_temp[1].SourcePortRangeMax = atoi(val[21]);
            qosclass_temp[1].SourcePortExclude = (unsigned char)atoi(val[22]);
            
            if(strlen(val[23])){
            if (strlen(val[23])!=12 || !string_to_hex(val[23], qosclass_temp[1].SourceMACAddress, 12)) 
                {
				printf("invalid Source MAC address!\n");
				return;
			}
                }else{
                strcpy(qosclass_temp[1].SourceMACAddress, "");
            }

            if(strlen(val[26])){
            if (strlen(val[26])!=12 || !string_to_hex(val[26], qosclass_temp[1].DestMACAddress, 12)) 
                {
				printf("invalid Dest MAC address!\n");
				return;
			}
                }else{
                strcpy(qosclass_temp[1].DestMACAddress, "");
            }

            if(strlen(val[24])){
                if (strlen(val[24])!=12 || !string_to_hex(val[24], qosclass_temp[1].SourceMACMask, 12)) 
                    {
                    printf("invalid Source MAC Mask!\n");
                    return;
                }
            }else{
                    strcpy(qosclass_temp[1].SourceMACMask, "");
            }
                
            if(strlen(val[27])){
                if (strlen(val[27])!=12 || !string_to_hex(val[27], qosclass_temp[1].DestMACMask, 12)) 
                    {
                    printf("invalid Dest MAC Mask!\n");
                    return;
                }
            }else{
                    strcpy(qosclass_temp[1].DestMACMask, "");
            }


            qosclass_temp[1].SourceMACExclude = (unsigned char)atoi(val[25]);
            qosclass_temp[1].DestMACExclude = (unsigned char)atoi(val[28]);          
            qosclass_temp[1].Ethertype = atoi(val[29]);
            qosclass_temp[1].EthertypeExclude = (unsigned char)atoi(val[30]);
            qosclass_temp[1].SSAP = atoi(val[31]);
            qosclass_temp[1].SSAPExclude = (unsigned char)atoi(val[32]);
            qosclass_temp[1].DSAP = atoi(val[33]);
            qosclass_temp[1].DSAPExclude = (unsigned char)atoi(val[34]);
            qosclass_temp[1].LLCControl = atoi(val[35]);
            qosclass_temp[1].LLCControlExclude = (unsigned char)atoi(val[36]);
            qosclass_temp[1].SNAPOUI = atoi(val[37]);
            qosclass_temp[1].SNAPOUIExclude = (unsigned char)atoi(val[38]);
            
            strcpy(qosclass_temp[1].SourceVendorClassID, val[39]);
            qosclass_temp[1].SourceVendorClassIDExclude = (unsigned char)atoi(val[40]);
            strcpy(qosclass_temp[1].SourceVendorClassIDMode, val[41]);
            
            strcpy(qosclass_temp[1].DestVendorClassID, val[42]);
            qosclass_temp[1].DestVendorClassIDExclude = (unsigned char)atoi(val[43]);
            strcpy(qosclass_temp[1].DestVendorClassIDMode, val[44]);
            
            strcpy(qosclass_temp[1].SourceClientID, val[45]);
            qosclass_temp[1].SourceClientIDExclude = (unsigned char)atoi(val[46]);
            strcpy(qosclass_temp[1].DestClientID, val[47]);
            qosclass_temp[1].DestClientIDExclude = (unsigned char)atoi(val[48]);
            
            strcpy(qosclass_temp[1].SourceUserClassID, val[49]);
            qosclass_temp[1].SourceUserClassIDExclude = (unsigned char)atoi(val[50]);
            strcpy(qosclass_temp[1].DestUserClassID, val[51]);
            qosclass_temp[1].DestUserClassIDExclude = (unsigned char)atoi(val[52]);
            
            strcpy(qosclass_temp[1].SourceVendorSpecificInfo, val[53]);
            qosclass_temp[1].SourceVendorSpecificInfoExclude = (unsigned char)atoi(val[54]);
            qosclass_temp[1].SourceVendorSpecificInfoEnterprise = (unsigned int)atoi(val[55]);
            qosclass_temp[1].SourceVendorSpecificInfoSubOption = (unsigned short)atoi(val[56]);
            strcpy(qosclass_temp[1].SourceVendorSpecificInfoMode, val[57]);
            strcpy(qosclass_temp[1].DestVendorSpecificInfo, val[58]);
            qosclass_temp[1].DestVendorSpecificInfoExclude = (unsigned char)atoi(val[59]);
            qosclass_temp[1].DestVendorSpecificInfoEnterprise = (unsigned int)atoi(val[60]);
            qosclass_temp[1].DestVendorSpecificInfoSubOption = (unsigned short)atoi(val[61]);
            strcpy(qosclass_temp[1].DestVendorSpecificInfoMode, val[62]);
            
            qosclass_temp[1].TCPACK = (unsigned char)atoi(val[63]);
            qosclass_temp[1].TCPACKExclude = (unsigned char)atoi(val[64]);
            qosclass_temp[1].IPLengthMin = atoi(val[65]);
            qosclass_temp[1].IPLengthMax = atoi(val[66]);
            qosclass_temp[1].IPLengthExclude = (unsigned char)atoi(val[67]);
            qosclass_temp[1].DSCPCheck = atoi(val[68]);
            qosclass_temp[1].DSCPExclude = (unsigned char)atoi(val[69]);
            qosclass_temp[1].DSCPMark = atoi(val[70]);
            qosclass_temp[1].EthernetPriorityCheck = atoi(val[71]);
            qosclass_temp[1].EthernetPriorityExclude = (unsigned char)atoi(val[72]);
            qosclass_temp[1].EthernetPriorityMark = atoi(val[73]);
            qosclass_temp[1].VLANIDCheck = atoi(val[74]);
            qosclass_temp[1].VLANIDExclude = (unsigned char)atoi(val[75]);
            qosclass_temp[1].OutOfBandInfo = atoi(val[76]);
            qosclass_temp[1].ForwardingPolicy = (unsigned int)atoi(val[77]);
            qosclass_temp[1].TrafficClass = atoi(val[78]);
            qosclass_temp[1].ClassPolicer = atoi(val[79]);
            qosclass_temp[1].ClassQueue = atoi(val[80]);
            qosclass_temp[1].ClassApp = atoi(val[81]);
            value = (void *)qosclass_temp;
        }
		else if ( !strcmp(val[0], "del")) {
			id = MIB_QOS_CLASS_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_QOS_CLASS_TBL_NUM, (void *)&entryNum)) {
				printf("Get QoS Class entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&qosclass) = (char)int_val;
			if ( !APMIB_GET(MIB_QOS_CLASS_TBL, (void *)&qosclass)) {
				printf("Get QOS Class table entry error!");
				return;
			}
            value = (void *)&qosclass;
		}
		else if ( !strcmp(val[0], "delall"))
		{
		    id = MIB_QOS_CLASS_DELALL;
            value = (void *)&qosclass;
        }
		break;


        case QOS_QUEUE_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_QOS_QUEUE_ADD;
			if ( valNum < 16 ) {
				printf("input argument is not enough %d!\n", valNum);
				return;
			}

            qosqueue.QueueInstanceNum = (unsigned int)atoi(val[1]);
            qosqueue.QueueKey = (unsigned char)atoi(val[2]);			
            qosqueue.QueueEnable = (unsigned char)atoi(val[3]);
            strcpy(qosqueue.Alias, val[4]);
            strcpy(qosqueue.TrafficClasses, val[5]);
		strcpy(qosqueue.QueueInterface, val[6]);
		qosqueue.QueueBufferLength = atoi(val[7]);
		qosqueue.QueueWeight = atoi(val[8]);
 		qosqueue.QueuePrecedence = atoi(val[9]);

		qosqueue.REDThreshold = atoi(val[10]);
		qosqueue.REDPercentage = atoi(val[11]);
		 strcpy(qosqueue.DropAlgorithm, val[12]);
            strcpy(qosqueue.SchedulerAlgorithm, val[13]);
            qosqueue.ShapingRate = atoi(val[14]);
            qosqueue.ShapingBurstSize = atoi(val[15]);
            value = (void *)&qosqueue;

		}
        else if ( !strcmp(val[0], "mod")){

			id=MIB_QOS_QUEUE_MOD;
			if ( valNum < 17) {
				printf("input argument is not enough!\n");
				return;
			}
            
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_QOS_QUEUE_TBL_NUM, (void *)&entryNum)) {
				printf("Get QOS_QUEUE_TBL total entry number error!");
				return;
			}
			if ( int_val > entryNum || int_val<1) {
				printf("Get wrong entry number!\n");
				return;
			}
			*((char *)&(qosqueue_temp[0])) = (char)int_val;
			if ( !APMIB_GET(MIB_QOS_QUEUE_TBL, (void *)&(qosqueue_temp[0]))) {
				printf("Get table entry error!");
				return;
			}
			
            qosqueue_temp[1].QueueInstanceNum = (unsigned int)atoi(val[2]);
            qosqueue_temp[1].QueueKey = (unsigned char)atoi(val[3]);
            qosqueue_temp[1].QueueEnable = (unsigned char)atoi(val[4]);
            strcpy(qosqueue_temp[1].Alias, val[5]);
            strcpy(qosqueue_temp[1].TrafficClasses, val[6]);
		strcpy(qosqueue_temp[1].QueueInterface, val[7]);
		qosqueue_temp[1].QueueBufferLength = atoi(val[8]);
		qosqueue_temp[1].QueueWeight = atoi(val[9]);
		qosqueue_temp[1].QueuePrecedence = atoi(val[10]);

		qosqueue_temp[1].REDThreshold = atoi(val[11]);
		qosqueue_temp[1].REDPercentage = atoi(val[12]);
		 strcpy(qosqueue_temp[1].DropAlgorithm, val[13]);			
			
            strcpy(qosqueue_temp[1].SchedulerAlgorithm, val[14]);
            qosqueue_temp[1].ShapingRate = atoi(val[15]);
            qosqueue_temp[1].ShapingBurstSize = atoi(val[16]);
            value = (void *)qosqueue_temp;

		}

		else if ( !strcmp(val[0], "del")) {
			id = MIB_QOS_QUEUE_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_QOS_QUEUE_TBL_NUM, (void *)&entryNum)) {
				printf("Get QoS Queue entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&qosqueue) = (char)int_val;
			if ( !APMIB_GET(MIB_QOS_QUEUE_TBL, (void *)&qosqueue)) {
				printf("Get QOS Queue table entry error!");
				return;
			}
            value = (void *)&qosqueue;
		}
		else if ( !strcmp(val[0], "delall"))
		{
			id = MIB_QOS_QUEUE_DELALL;
            value = (void *)&qosqueue;
        }
		break;
		
	case TR098_APPCONF_ARRAY_T:
		#if 1
	
		if ( !strcmp(val[0], "add")) {
			id = MIB_TR098_QOS_APP_ADD;
			printf("MIB_TR098_QOS_APP_ADD\n");
			if ( valNum < 14 ) {
				printf("input argument is not enough!\n");
				return;
			}
            tr098AppConf.app_key= (unsigned short)atoi(val[1]);
			tr098AppConf.app_enable = (unsigned char)atoi(val[2]);
			strcpy( tr098AppConf.app_status, (val[3]) );
			strcpy( tr098AppConf.app_alias, (val[4]) );
			strcpy( tr098AppConf.protocol_identify, (val[5]) );
			strcpy( tr098AppConf.app_name, (val[6]) );
			tr098AppConf.default_policy =(unsigned short)atoi(val[7]);
			tr098AppConf.default_class =(short)atoi(val[8]);
			tr098AppConf.default_policer =(short)atoi(val[9]);
			tr098AppConf.default_queue =(short)atoi(val[10]);
			tr098AppConf.default_dscp_mark =(short)atoi(val[11]);
			tr098AppConf.default_8021p_mark =(short)atoi(val[12]);
                tr098AppConf.instanceNum = atoi(val[13]);
                value = (void *)&tr098AppConf;
		}
        else if(!strcmp(val[0], "mod")){
            id = MIB_TR098_QOS_APP_MOD;
            if(valNum < 15){
				printf("input argument is not enough!\n");
				return;
			}
            int_val = atoi(val[1]);
            if ( !APMIB_GET(MIB_TR098_QOS_APP_TBL_NUM, (void *)&entryNum)) {
				printf("Get QoS app entry number error!");
				return;
			}
            if(int_val>entryNum||int_val<1){
                printf("Input wrong entry number");
                return;
                }
            *((char *)&tr098AppConf_temp[0]) = (char)int_val;
            if( !APMIB_GET(MIB_TR098_QOS_APP_TBL, (void *)&(tr098AppConf_temp[0]))){
             	printf("Get table entry error!");
				return;
			}   
            tr098AppConf_temp[1].app_key= (unsigned short)atoi(val[2]);
			tr098AppConf_temp[1].app_enable = (unsigned char)atoi(val[3]);
			strcpy( tr098AppConf_temp[1].app_status, (val[4]) );
			strcpy( tr098AppConf_temp[1].app_alias, (val[5]) );
			strcpy( tr098AppConf_temp[1].protocol_identify, (val[6]) );
			strcpy( tr098AppConf_temp[1].app_name, (val[7]) );
			tr098AppConf_temp[1].default_policy =(unsigned short)atoi(val[8]);
			tr098AppConf_temp[1].default_class =(short)atoi(val[9]);
			tr098AppConf_temp[1].default_policer =(short)atoi(val[10]);
			tr098AppConf_temp[1].default_queue =(short)atoi(val[11]);
			tr098AppConf_temp[1].default_dscp_mark =(short)atoi(val[12]);
			tr098AppConf_temp[1].default_8021p_mark =(short)atoi(val[13]);
            tr098AppConf_temp[1].instanceNum =atoi(val[14]);
	        value = (void *)&tr098AppConf_temp;
	

		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_TR098_QOS_APP_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_TR098_QOS_APP_TBL_NUM, (void *)&entryNum)) {
				printf("Get QoS App entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&tr098AppConf) = (char)int_val;
			if ( !APMIB_GET(MIB_TR098_QOS_APP_TBL, (void *)&tr098AppConf)) {
				printf("Get tr098 QOS App table entry error!");
				return;
			}
            value = (void *)&tr098AppConf;
		}
		else if ( !strcmp(val[0], "delall")){
			id = MIB_TR098_QOS_APP_DELALL;

		value = (void *)&tr098AppConf;
		}
		#endif
		break;	
	case TR098_FLOWCONF_ARRAY_T:		
		
		if ( !strcmp(val[0], "add")) {
			id = MIB_TR098_QOS_FLOW_ADD;
			
			printf("MIB_TR098_QOS_FLOW_ADD\n");
			if ( valNum < 16 ) {
				printf("input argument is not enough!\n");
				return;
			}

			tr098FlowConf.flow_key= (unsigned char)atoi(val[1]);
			tr098FlowConf.flow_enable = (unsigned char)atoi(val[2]);
			strcpy( tr098FlowConf.flow_status, (val[3]) );
			strcpy( tr098FlowConf.flow_alias, (val[4]) );
			strcpy( tr098FlowConf.flow_type, (val[5]));
			strcpy( tr098FlowConf.flow_type_para, (val[6]));
			strcpy( tr098FlowConf.flow_name, (val[7]) );
		    tr098FlowConf.app_identify = (short)atoi(val[8]);
			tr098FlowConf.qos_policy =atoi(val[9]);
			tr098FlowConf.flow_class =atoi(val[10]);
			tr098FlowConf.flow_policer =atoi(val[11]);
			tr098FlowConf.flow_queue =atoi(val[12]);
			tr098FlowConf.flow_dscp_mark =atoi(val[13]);
			tr098FlowConf.flow_8021p_mark =atoi(val[14]);
            tr098FlowConf.instanceNum = atoi(val[15]);
            value = (void *)&tr098FlowConf;
			
       		}
        else if ( !strcmp(val[0], "mod")){
            id = MIB_TR098_QOS_FLOW_MOD;
            if ( valNum < 17 ) {
				printf("input argument is not enough!\n");
				return;
			}
            int_val = atoi(val[1]);
            if ( !APMIB_GET(MIB_TR098_QOS_FLOW_TBL_NUM, (void *)&entryNum)){
                printf("Get QoS Flow entry number error! ");
                return;
            }
            if (int_val > entryNum || int_val < 1){
                printf("Input entry number error!");
                return;
                }

            *((char *)&tr098FlowConf_temp[0]) = (char)int_val;
            if ( !APMIB_GET(MIB_TR098_QOS_FLOW_TBL, (void *)&(tr098FlowConf_temp[0]))){
                printf("Get QoS flow entry error !");
                return;
                }
            tr098FlowConf_temp[1].flow_key= (unsigned char)atoi(val[2]);
			tr098FlowConf_temp[1].flow_enable = (unsigned char)atoi(val[3]);
			strcpy( tr098FlowConf_temp[1].flow_status, (val[4]) );
			strcpy( tr098FlowConf_temp[1].flow_alias, (val[5]) );
			strcpy( tr098FlowConf_temp[1].flow_type, (val[6]));
			strcpy( tr098FlowConf_temp[1].flow_type_para, (val[7]));
			strcpy( tr098FlowConf_temp[1].flow_name, (val[8]) );
			tr098FlowConf_temp[1].app_identify = (short)atoi(val[9]) ;
		
			tr098FlowConf_temp[1].qos_policy =atoi(val[10]);
			tr098FlowConf_temp[1].flow_class =atoi(val[11]);
			tr098FlowConf_temp[1].flow_policer =atoi(val[12]);
			tr098FlowConf_temp[1].flow_queue =atoi(val[13]);
			tr098FlowConf_temp[1].flow_dscp_mark =atoi(val[14]);
			tr098FlowConf_temp[1].flow_8021p_mark =atoi(val[15]);
            tr098FlowConf_temp[1].instanceNum =atoi(val[16]);

            value = (void *)&tr098FlowConf_temp;
            
            }
		else if ( !strcmp(val[0], "del")) {
			id = MIB_TR098_QOS_FLOW_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_TR098_QOS_FLOW_TBL_NUM, (void *)&entryNum)) {
				printf("Get QoS Flow entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&tr098FlowConf) = (char)int_val;
			if ( !APMIB_GET(MIB_TR098_QOS_FLOW_TBL, (void *)&tr098FlowConf)) {
				printf("Get tr098 QOS Queue table entry error!");
				return;
			}
            value = (void *)&tr098FlowConf;
		}
		else if ( !strcmp(val[0], "delall")){
			id = MIB_TR098_QOS_FLOW_DELALL;
            value = (void *)&tr098FlowConf;
          }
		break;	
	  //mark_qos
		case QOS_POLICER_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_QOS_POLICER_ADD;
			if ( valNum < 15 ) {
				printf("input argument is not enough!\n");
				return;
			}  
	     qospolicer.InstanceNum = atoi(val[1]);
            qospolicer.PolicerKey = atoi(val[2]);
	     qospolicer.PolicerEnable = atoi(val[3]);
            strcpy( qospolicer.Alias, val[4]);
      	     qospolicer.CommittedRate = atoi(val[5]);			
      	     qospolicer.CommittedBurstSize = atoi(val[6]);			
      	     qospolicer.ExcessBurstSize = atoi(val[7]);			
      	     qospolicer.PeakRate = atoi(val[8]);			
		qospolicer.PeakBurstSize = atoi(val[9]);			
		 strcpy( qospolicer.MeterType, val[10]);
		 strcpy( qospolicer.PossibleMeterTypes, val[11]);
 		 strcpy( qospolicer.ConformingAction, val[12]);
 		 strcpy( qospolicer.PartialConformingAction, val[13]);
 		 strcpy( qospolicer.NonConformingAction, val[14]);
         value = (void *)&qospolicer;

		}
        else if (!strcmp(val[0], "mod")){
            id = MIB_QOS_POLICER_MOD;
            if (valNum < 16){
                printf("Input argument is not enough !");
                return;
                }
            int_val = atoi(val[1]);
            if (! APMIB_GET(MIB_QOS_POLICER_TBL_NUM, (void *)&entryNum)){
                printf("Get QoS policer number error !");
                return;
                }
            if (int_val > entryNum || int_val < 1){
                printf("Input mod entry number error!");
                return;
                }
            *((char *)&qospolicer_temp[0]) = (char)int_val;
            if( !APMIB_GET(MIB_QOS_POLICER_TBL, (void *)&qospolicer_temp[0])){
                printf("Get QoS policer entry error !");
                return;
                }
            qospolicer_temp[1].InstanceNum = atoi(val[2]);
            qospolicer_temp[1].PolicerKey = atoi(val[3]);
	        qospolicer_temp[1].PolicerEnable = atoi(val[4]);
            strcpy( qospolicer_temp[1].Alias, val[5]);
      	    qospolicer_temp[1].CommittedRate = atoi(val[6]);
      	    qospolicer_temp[1].CommittedBurstSize = atoi(val[7]);
      	    qospolicer_temp[1].ExcessBurstSize = atoi(val[8]);
      	    qospolicer_temp[1].PeakRate = atoi(val[9]);	
		    qospolicer_temp[1].PeakBurstSize = atoi(val[10]);
		    strcpy( qospolicer_temp[1].MeterType, val[11]);
		    strcpy( qospolicer_temp[1].PossibleMeterTypes, val[12]);
 		    strcpy( qospolicer_temp[1].ConformingAction, val[13]);
 		    strcpy( qospolicer_temp[1].PartialConformingAction, val[14]);
 		    strcpy( qospolicer_temp[1].NonConformingAction, val[15]);
            value = (void *)&qospolicer_temp; 
        }
		else if ( !strcmp(val[0], "del")) {
			id = MIB_QOS_POLICER_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_QOS_POLICER_TBL_NUM, (void *)&entryNum)) {
				printf("Get QoS Queue entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&qospolicer) = (char)int_val;
			if ( !APMIB_GET(MIB_QOS_POLICER_TBL, (void *)&qospolicer)) {
				printf("Get QOS Queue table entry error!");
				return;
			}
            value = (void *)&qospolicer;
		}
		else if ( !strcmp(val[0], "delall")){
			id = MIB_QOS_POLICER_DELALL;
            value = (void *)&qospolicer;
          }
		break;

		case QOS_QUEUESTATS_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_QOS_QUEUESTATS_ADD;
			if ( valNum < 6 ) {
				printf("input argument is not enough!\n");
				return;
			}
            		qosqueuestats.InstanceNum = atoi(val[1]);
            		qosqueuestats.Enable = atoi(val[2]);
			strcpy(qosqueuestats.Alias, val[3]);
			qosqueuestats.Queue = atoi(val[4]);
			strcpy(qosqueuestats.Interface, val[5]);
            value = (void *)&qosqueuestats;

		}
        else if( !strcmp(val[0], "mod")){
            id = MIB_QOS_QUEUESTATS_MOD;
            if(valNum < 7){
                printf("Input argument number is not enough !");
                return;
                }
            int_val = atoi(val[1]);
            if ( !APMIB_GET(MIB_QOS_QUEUESTATS_TBL_NUM, (void *)&entryNum)){
                printf("Get QoS Stats number error!");
                return;
                }
            if ( int_val > entryNum || int_val < 1){
                printf("Input Qos Stats number error !");
                return;
                }
            *((char *)&qosqueuestats_temp[0]) = (char)int_val;
            if ( !APMIB_GET(MIB_QOS_QUEUESTATS_TBL, (void *)&qosqueuestats_temp[0])){
                printf("Get QoS stats entry error !");
                return;
                }
            qosqueuestats_temp[1].InstanceNum = atoi(val[2]);
            qosqueuestats_temp[1].Enable = atoi(val[3]);
			strcpy(qosqueuestats_temp[1].Alias, val[4]);
			qosqueuestats_temp[1].Queue = atoi(val[5]);
			strcpy(qosqueuestats_temp[1].Interface, val[6]);

            value = (void *)&qosqueuestats_temp;
            }
		else if ( !strcmp(val[0], "del")) {
			id = MIB_QOS_QUEUESTATS_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_QOS_QUEUESTATS_TBL_NUM, (void *)&entryNum)) {
				printf("Get QoS Queue entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&qosqueuestats) = (char)int_val;
			if ( !APMIB_GET(MIB_QOS_QUEUESTATS_TBL, (void *)&qosqueuestats)) {
				printf("Get QOS Queue table entry error!");
				return;
			}
            value = (void *)&qosqueuestats;
		}
		else if ( !strcmp(val[0], "delall")){
			id = MIB_QOS_QUEUESTATS_DELALL;
		    value = (void *)&qosqueuestats;
		}
		break;
#endif

#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
	case DHCP_SERVER_OPTION_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_DHCP_SERVER_OPTION_ADD;
			if ( valNum < 10 ) {
				printf("input argument is not enough!\n");
				return;
			}
			dhcpdOpt.enable  = atoi(val[2]);
			dhcpdOpt.usedFor = atoi(val[3]);
			dhcpdOpt.order   = atoi(val[4]);
			dhcpdOpt.tag     = atoi(val[5]);
			dhcpdOpt.len     = atoi(val[6]);
			strcpy((char *)dhcpdOpt.value, val[7]);
			dhcpdOpt.ifIndex = atoi(val[8]);
			dhcpdOpt.dhcpOptInstNum   = atoi(val[9]);
			dhcpdOpt.dhcpConSPInstNum = atoi(val[10]);		
		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_DHCP_SERVER_OPTION_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_DHCP_SERVER_OPTION_TBL_NUM, (void *)&entryNum)) {
				printf("Get port forwarding entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&dhcpdOpt) = (char)int_val;
			if ( !APMIB_GET(MIB_DHCP_SERVER_OPTION_TBL, (void *)&dhcpdOpt)) {
				printf("Get table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_DHCP_SERVER_OPTION_DELALL;

		value = (void *)&dhcpdOpt;
		break;


	case DHCP_CLIENT_OPTION_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_DHCP_CLIENT_OPTION_ADD;
			if ( valNum < 10 ) {
				printf("input argument is not enough!\n");
				return;
			}
			dhcpcOpt.enable  = atoi(val[2]);
			dhcpcOpt.usedFor = atoi(val[3]);
			dhcpcOpt.order	 = atoi(val[4]);
			dhcpcOpt.tag	 = atoi(val[5]);
			dhcpcOpt.len	 = atoi(val[6]);
			strcpy((char *)dhcpcOpt.value, val[7]);
			dhcpcOpt.ifIndex = atoi(val[8]);
			dhcpcOpt.dhcpOptInstNum   = atoi(val[9]);
			dhcpcOpt.dhcpConSPInstNum = atoi(val[10]);		
		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_DHCP_CLIENT_OPTION_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_DHCP_CLIENT_OPTION_TBL_NUM, (void *)&entryNum)) {
				printf("Get port forwarding entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&dhcpcOpt) = (char)int_val;
			if ( !APMIB_GET(MIB_DHCP_CLIENT_OPTION_TBL, (void *)&dhcpcOpt)) {
				printf("Get table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_DHCP_CLIENT_OPTION_DELALL;

		value = (void *)&dhcpcOpt;
		break;


	case DHCPS_SERVING_POOL_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_DHCPS_SERVING_POOL_ADD;
			if ( valNum < 31 ) {
				printf("input argument is not enough!\n");
				return;
			}
			servingPool.enable  = atoi(val[2]);
			servingPool.poolorder  = atoi(val[3]);
			strcpy((char *)servingPool.poolname, val[4]);
			servingPool.deviceType  = atoi(val[5]);
			servingPool.rsvOptCode  = atoi(val[6]);
			servingPool.sourceinterface  = atoi(val[7]);
			strcpy((char *)servingPool.vendorclass, val[8]);
			servingPool.vendorclassflag  = atoi(val[9]);
			strcpy((char *)servingPool.vendorclassmode, val[10]);
			strcpy((char *)servingPool.clientid, val[11]);
			servingPool.clientidflag  = atoi(val[12]);
			strcpy((char *)servingPool.userclass, val[13]);
			servingPool.userclassflag  = atoi(val[14]);
			strcpy((char *)servingPool.chaddr, val[15]);
			strcpy((char *)servingPool.chaddrmask, val[16]);
			servingPool.chaddrflag  = atoi(val[17]);
			servingPool.localserved  = atoi(val[18]);
			strcpy((char *)servingPool.startaddr, val[19]);
			strcpy((char *)servingPool.endaddr, val[20]);
			strcpy((char *)servingPool.subnetmask, val[21]);
			strcpy((char *)servingPool.iprouter, val[22]);
			strcpy((char *)servingPool.dnsserver1, val[23]);
			strcpy((char *)servingPool.dnsserver2, val[24]);
			strcpy((char *)servingPool.dnsserver3, val[25]);
			strcpy((char *)servingPool.domainname, val[26]);
			servingPool.leasetime  = atoi(val[27]);
			strcpy((char *)servingPool.dhcprelayip, val[29]);
			servingPool.dnsservermode  = atoi(val[30]);
			servingPool.InstanceNum  = atoi(val[31]);
		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_DHCPS_SERVING_POOL_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_DHCPS_SERVING_POOL_TBL_NUM, (void *)&entryNum)) {
				printf("Get port forwarding entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&servingPool) = (char)int_val;
			if ( !APMIB_GET(MIB_DHCPS_SERVING_POOL_TBL, (void *)&servingPool)) {
				printf("Get table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_DHCPS_SERVING_POOL_DELALL;

		value = (void *)&servingPool;
		break;
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */

#ifdef ROUTE_SUPPORT
		case STATICROUTE_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_STATICROUTE_ADD;
			if ( valNum < 3 ) {
				printf("input argument is not enough!\n");
				return;
			}
			if ( !inet_aton(val[1], (struct in_addr *)&staticRoute.dstAddr)) {
                                printf("invalid destination IP address!\n");
                                return;
                        }
			if ( !inet_aton(val[2], (struct in_addr *)&staticRoute.netmask)) {
                                printf("invalid netmask !\n");
                                return;
                        }
			if ( !inet_aton(val[3], (struct in_addr *)&staticRoute.gateway)) {
                                printf("invalid gateway address!\n");
                                return;
                        }
		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_STATICROUTE_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_STATICROUTE_TBL_NUM, (void *)&entryNum)) {
				printf("Get trigger-port entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&staticRoute) = (char)int_val;
			if ( !APMIB_GET(MIB_STATICROUTE_TBL, (void *)&staticRoute)) {
				printf("Get trigger-port table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_STATICROUTE_DELALL;

		value = (void *)&staticRoute;
		break;
#endif //ROUTE

#ifdef CONFIG_RTL_AIRTIME
	case AIRTIME_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = (wlan_idx==0?MIB_AIRTIME_ADD:MIB_AIRTIME2_ADD);
			if ( valNum < 4 ) {
				printf("input argument is not enough!\n");
				return;
			}
			if ( !inet_aton(val[1], (struct in_addr *)&airTime.ipAddr)) {
				printf("invalid internet address!\n");
				return;
			}
			if ( strlen(val[2])!=12 || !string_to_hex(val[2], airTime.macAddr, 12)) {
				printf("invalid value!\n");
				return;
			}
			airTime.atm_time = atoi(val[3]);
			if ( valNum > 4)
				strcpy((char *)airTime.comment, val[4]);
			else
				airTime.comment[0] = '\0';
		}
		else if ( !strcmp(val[0], "del")) {
			id = (wlan_idx==0?MIB_AIRTIME_DEL:MIB_AIRTIME2_DEL);
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET((wlan_idx==0?MIB_AIRTIME_TBL_NUM:MIB_AIRTIME2_TBL_NUM), (void *)&entryNum)) {
				printf("Get port forwarding entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&airTime) = (char)int_val;
			if ( !APMIB_GET((wlan_idx==0?MIB_AIRTIME_TBL:MIB_AIRTIME2_TBL), (void *)&airTime)) {
				printf("Get table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = (wlan_idx==0?MIB_AIRTIME_DELALL:MIB_AIRTIME2_DELALL);
		
		value = (void *)&airTime;
		break;
#endif /* CONFIG_RTL_AIRTIME */

#endif

	case SCHEDULE_ARRAY_T:
	#ifdef NEW_SCHEDULE_SUPPORT

		if ( !strcmp(val[0], "mod")) {

			id=MIB_WLAN_SCHEDULE_MOD;
			if ( valNum < 5 ) {
				printf("input argument is not enough!\n");
				return;
			}
			
			int_val = atoi(val[1]);
			//printf("%d int_val=%d\n",__LINE__,int_val);
			if ( !APMIB_GET(MIB_WLAN_SCHEDULE_TBL_NUM, (void *)&entryNum)) {
				printf("Get WLAN_SCHEDULE entry number error!");
				return;
			}
			if ( int_val > entryNum || int_val<1) {
				printf("Element number must be 1-10!\n");
				return;
			}
			//printf("%d entryNum=%d\n",__LINE__,entryNum);
			*((char *)&(wlschedule[0])) = (char)int_val;
			if ( !APMIB_GET(MIB_WLAN_SCHEDULE_TBL, (void *)&(wlschedule[0]))) {
				printf("Get table entry error!");
				return;
			}
			//printf("%d ec0=%d day=%d ft=%d tt=%d\n",__LINE__,wlschedule[0].eco,wlschedule[0].day,wlschedule[0].fTime,wlschedule[0].tTime);
			wlschedule[1]=wlschedule[0];
			wlschedule[1].eco=atoi(val[2]);
			wlschedule[1].day=atoi(val[3]);
			wlschedule[1].fTime=atoi(val[4]);
			wlschedule[1].tTime=atoi(val[5]);
			if((wlschedule[1].eco==0 ||wlschedule[1].eco==1)
				&&(wlschedule[1].day>=0 && wlschedule[1].day<=7)
				&&(wlschedule[1].fTime>=0 &&wlschedule[1].fTime<1440)
				&&(wlschedule[1].tTime>=0 &&wlschedule[1].tTime<1440)
				&&(wlschedule[1].fTime<=wlschedule[1].tTime)
				)
				{
					value = (void *)wlschedule;
					break;
				}
		}

			printf("cmd not support! try\n flash set [MIB] mod [index(1-%d)] [Enable(0/1)] [day(0-7)] [formTime(min,0-1440)] [toTime(min,0-1440)]\n",MAX_SCHEDULE_NUM);
			return;

		
#endif
		break;
#ifdef TR181_SUPPORT
#ifdef CONFIG_IPV6
	case DHCPV6C_SENDOPT_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			//may not used
		}
		else if ( !strcmp(val[0], "del")) {
			//may not used
		}
		else if ( !strcmp(val[0], "delall")){
			//may not used
		}
		break;
#endif
	case DNS_CLIENT_SERVER_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			//may not used
		}
		else if ( !strcmp(val[0], "del")) {
			//may not used
		}
		else if ( !strcmp(val[0], "delall")){
			//may not used
		}
		break;
#endif
	case DHCPRSVDIP_ARRY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_DHCPRSVDIP_ADD;
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
		int_val=4; 
#else
		int_val=3;
#endif
			if ( valNum < int_val ) {
				printf("input argument is not enough!\n");
				return;
			}
			
			i=1;
			
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_		
			dhcpRsvd.dhcpRsvdIpEntryEnabled=atoi(val[i++]);
#endif

			if ( strlen(val[i])!=12 || !string_to_hex(val[i], dhcpRsvd.macAddr, 12)) {
				printf("invalid value!\n");
				return;
			}
			if (!inet_aton(val[++i], (struct in_addr *)&dhcpRsvd.ipAddr)) {
				printf("invalid internet address!\n");
				return;
			}		
			if ( valNum > int_val )
				strcpy((char *)dhcpRsvd.hostName, val[++i]);			
		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_DHCPRSVDIP_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_DHCPRSVDIP_TBL_NUM, (void *)&entryNum)) {
				printf("Get DHCP resvd IP entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&dhcpRsvd) = (char)int_val;
			if ( !APMIB_GET(MIB_DHCPRSVDIP_TBL, (void *)&dhcpRsvd)) {
				printf("Get table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_DHCPRSVDIP_DELALL;
		value = (void *)&dhcpRsvd;
		break;
#if defined(VLAN_CONFIG_SUPPORTED)	
		case VLANCONFIG_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_VLANCONFIG_ADD;
#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT) ||defined(CONFIG_RTL_HW_VLAN_SUPPORT)
			if ( valNum < 7 )
#else
			if ( valNum < 6 )
#endif
			{
				printf("input argument is not enough!\n");
				return;
			}
			vlanConfig_entry.enabled=(unsigned char)atoi(val[1]);
			sprintf((char *)vlanConfig_entry.netIface, "%s", val[2]);
			vlanConfig_entry.tagged = (unsigned char)atoi(val[3]);
			vlanConfig_entry.priority = (unsigned char)atoi(val[4]);
			vlanConfig_entry.cfi = (unsigned char)atoi(val[5]);
			vlanConfig_entry.vlanId = (unsigned short)atoi(val[6]);
#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT) ||defined(CONFIG_RTL_HW_VLAN_SUPPORT)
			vlanConfig_entry.forwarding_rule = (unsigned char)atoi(val[7]);
#endif
		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_VLANCONFIG_DEL;
			if ( valNum < 8 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]); //index of entry
			if ( !APMIB_GET(MIB_VLANCONFIG_TBL_NUM, (void *)&entryNum)) {
				printf("Get VLAN config entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&vlanConfig_entry) = (char)int_val;
			if ( !APMIB_GET(MIB_VLANCONFIG_TBL, (void *)&vlanConfig_entry)) {
				printf("Get table entry error!");
				return;
			}
			vlanConfig_entry.enabled=0;

		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_VLANCONFIG_DELALL;
		value = (void *)&vlanConfig_entry;
		break;
#endif
#if defined(SAMBA_WEB_SUPPORT)	
		case STORAGE_USER_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_STORAGE_USER_ADD;
			if ( valNum < 2 )
			{
				printf("input argument is not enough!\n");
				return;
			}
			strcpy(user_entry.storage_user_name, val[1]);
			strcpy(user_entry.storage_user_password, val[2]);
		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_STORAGE_USER_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			#if 0
			int_val = atoi(val[1]); //index of entry
			if ( !APMIB_GET(MIB_STORAGE_USER_TBL_NUM, (void *)&entryNum)) {
				printf("Get eth dot1x config entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&user_entry) = (char)int_val;
			if ( !APMIB_GET(MIB_STORAGE_USER_TBL, (void *)&user_entry)) {
				printf("Get table entry error!");
				return;
			}
			#else
			//del by username
			if ( !APMIB_GET(MIB_STORAGE_USER_TBL_NUM, (void *)&entryNum)) {
				printf("Get samba account entry number error!");
				return;
			}
			for (k = entryNum; k > 0; k--)
			{
				memset((void *)&user_entry, '\0', sizeof(user_entry));
				*((char *)&user_entry) = (char)k;
				if ( !APMIB_GET(MIB_STORAGE_USER_TBL, (void *)&user_entry)) {
					printf("Get table entry error!");
					return;
				}
				if (!strcmp(user_entry.storage_user_name, val[1]))
					break;
			}
			if (k == 0)
			{
				printf("cannot find username=%s !\n", val[1]);
				return;
			}
			#endif

		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_STORAGE_USER_DELALL;
		value = (void *)&user_entry;
		break;
		case STORAGE_SHAREINFO_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_STORAGE_SHAREINFO_ADD;
			if ( valNum < 4 )
			{
				printf("input argument is not enough!\n");
				return;
			}
			strcpy(shareinfo_entry.storage_sharefolder_name, val[1]);
			strcpy(shareinfo_entry.storage_sharefolder_path, val[2]);
			strcpy(shareinfo_entry.storage_account, val[3]);
			shareinfo_entry.storage_permission = atoi(val[4]);
		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_STORAGE_SHAREINFO_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
	#if 0
			int_val = atoi(val[1]); //index of entry
			if ( !APMIB_GET(MIB_STORAGE_USER_TBL_NUM, (void *)&entryNum)) {
				printf("Get eth dot1x config entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&user_entry) = (char)int_val;
			if ( !APMIB_GET(MIB_STORAGE_USER_TBL, (void *)&user_entry)) {
				printf("Get table entry error!");
				return;
			}
	#else
			//del by share folder name
			if ( !APMIB_GET(MIB_STORAGE_SHAREINFO_TBL_NUM, (void *)&entryNum)) {
				printf("Get samba share info entry number error!");
				return;
			}
			for (k = entryNum; k > 0; k--)
			{
				memset((void *)&shareinfo_entry, '\0', sizeof(shareinfo_entry));
				*((char *)&shareinfo_entry) = (char)k;
				if ( !APMIB_GET(MIB_STORAGE_SHAREINFO_TBL, (void *)&shareinfo_entry)) {
					printf("Get table entry error!");
					return;
				}
				if (!strcmp(shareinfo_entry.storage_sharefolder_name, val[1]))
					break;
			}
			if (k == 0)
			{
				printf("cannot find share folder username=%s !\n", val[1]);
				return;
			}
	#endif

		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_STORAGE_SHAREINFO_DELALL;
		value = (void *)&shareinfo_entry;
		break;
#endif
#if defined(CONFIG_RTL_ETH_802DOT1X_SUPPORT)	
			case ETHDOT1X_ARRAY_T:
			if ( !strcmp(val[0], "add")) {
				id = MIB_ELAN_DOT1X_ADD;
				if ( valNum < 2 )
				{
					printf("input argument is not enough!\n");
					return;
				}
				ethdot1x_entry.enabled=(unsigned char)atoi(val[1]);
				ethdot1x_entry.portnum = (unsigned char)atoi(val[2]);
			}
			else if ( !strcmp(val[0], "del")) {
				id = MIB_ELAN_DOT1X_DEL;
				if ( valNum < 2 ) {
					printf("input argument is not enough!\n");
					return;
				}
				int_val = atoi(val[1]); //index of entry
				if ( !APMIB_GET(MIB_ELAN_DOT1X_TBL_NUM, (void *)&entryNum)) {
					printf("Get eth dot1x config entry number error!");
					return;
				}
				if ( int_val > entryNum ) {
					printf("Element number is too large!\n");
					return;
				}
				*((char *)&ethdot1x_entry) = (char)int_val;
				if ( !APMIB_GET(MIB_ELAN_DOT1X_TBL, (void *)&ethdot1x_entry)) {
					printf("Get table entry error!");
					return;
				}
				ethdot1x_entry.enabled=0;
	
			}
			else if ( !strcmp(val[0], "delall"))
				id = MIB_ELAN_DOT1X_DELALL;
			value = (void *)&ethdot1x_entry;
			break;
#endif
#if defined(CONFIG_8021Q_VLAN_SUPPORTED)
		case VLAN_ARRAY_T:
			if ( !strcmp(val[0], "delall")){
				id = MIB_VLAN_DELALL;
			}
			value = (void *)&vlan_entry;
			break;
#endif

	case WLAC_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_WLAN_AC_ADDR_ADD;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			if ( strlen(val[1])!=12 || !string_to_hex(val[1], wlAc.macAddr, 12)) {
				printf("invalid value!\n");
				return;
			}

			if ( valNum > 2)
				strcpy((char *)wlAc.comment, val[2]);
			else
				wlAc.comment[0] = '\0';
		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_WLAN_AC_ADDR_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_WLAN_MACAC_NUM, (void *)&entryNum)) {
				printf("Get port forwarding entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&wlAc) = (char)int_val;
			if ( !APMIB_GET(MIB_WLAN_MACAC_ADDR, (void *)&wlAc)) {
				printf("Get table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_WLAN_AC_ADDR_DELALL;
		value = (void *)&wlAc;
		break;

//#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_) // below code copy above ACL code
	case MESH_ACL_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_WLAN_MESH_ACL_ADDR_ADD;
			if ( valNum < 2 ) {
				printf("Mesh Acl Addr input argument is not enough!\n");
				return;
			}
			if ( strlen(val[1])!=12 || !string_to_hex(val[1], wlAc.macAddr, 12)) {
				printf("Mesh Acl Addr invalid value!\n");
				return;
			}

			if ( valNum > 2)
				strcpy((char *)wlAc.comment, val[2]);
			else
				wlAc.comment[0] = '\0';
		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_WLAN_MESH_ACL_ADDR_DEL;
			if ( valNum < 2 ) {
				printf("Mesh Acl Addr input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_WLAN_MESH_ACL_NUM, (void *)&entryNum)) {
				printf("Mesh Acl Addr get port forwarding entry number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Mesh Acl Addr element number is too large!\n");
				return;
			}
			*((char *)&wlAc) = (char)int_val;
			if ( !APMIB_GET(MIB_WLAN_MESH_ACL_ADDR, (void *)&wlAc)) {
				printf("Mesh Acl Addr get table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_WLAN_MESH_ACL_ADDR_DELALL;
		value = (void *)&wlAc;
		break;
//#endif	// CONFIG_RTK_MESH && _MESH_ACL_ENABLE_

	case WDS_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_WLAN_WDS_ADD;
			if ( valNum < 3 ) {
				printf("input argument is not enough!\n");
				return;
			}
			if ( strlen(val[1])!=12 || !string_to_hex(val[1], wds.macAddr, 12)) {
				printf("invalid value!\n");
				return;
			}

			if ( valNum > 2)
				strcpy((char *)wds.comment, val[2]);
			else
				wds.comment[0] = '\0';
				
			wds.fixedTxRate = strtoul(val[3],0,10);//atoi(val[3]);	
		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_WLAN_WDS_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_WLAN_WDS_NUM, (void *)&entryNum)) {
				printf("Get wds number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&wds) = (char)int_val;
			if ( !APMIB_GET(MIB_WLAN_WDS, (void *)&wds)) {
				printf("Get table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_WLAN_WDS_DELALL;
		value = (void *)&wds;
		break;

#ifdef HOME_GATEWAY
#ifdef VPN_SUPPORT
	case IPSECTUNNEL_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			id = MIB_IPSECTUNNEL_ADD;
			if ( valNum < 27 ) {
				printf("input argument is not enough!\n");
				return;
			}
			if ( !inet_aton(val[5], (struct in_addr *)&ipsecTunnel.lc_ipAddr)) {
				printf("invalid local IP address!\n");
				return;
			}
			
			if ( !inet_aton(val[8], (struct in_addr *)&ipsecTunnel.rt_ipAddr)) {
				printf("invalid remote IP address!\n");
				return;
			}
			if ( !inet_aton(val[10], (struct in_addr *)&ipsecTunnel.rt_gwAddr)) {
				printf("invalid remote gateway address!\n");
				return;
			}
			ipsecTunnel.tunnelId =  atoi(val[1]); 
			ipsecTunnel.enable = atoi(val[2]);
			ipsecTunnel.lcType = atoi(val[4]);

			if(strlen(val[3]) > (MAX_NAME_LEN-1)){
				printf("Connection Name too long !\n");
				return;
			}else
				strcpy(ipsecTunnel.connName, val[3]); 
			ipsecTunnel.lc_maskLen = atoi(val[6]);
			ipsecTunnel.rt_maskLen  = atoi(val[9]);
			ipsecTunnel.keyMode= atoi(val[11]);
			ipsecTunnel.conType = atoi(val[12]);
			ipsecTunnel.espEncr = atoi(val[13]);
			ipsecTunnel.espAuth = atoi(val[14]);
			if(strlen(val[15]) >  (MAX_NAME_LEN-1)){
				printf("Preshared Key too long !\n");
				return;
			}else
				strcpy(ipsecTunnel.psKey, val[15]); 

			ipsecTunnel.ikeEncr = atoi(val[16]);
			ipsecTunnel.ikeAuth = atoi(val[17]);
			ipsecTunnel.ikeKeyGroup = atoi(val[18]);
			ipsecTunnel.ikeLifeTime= strtol(val[19], (char **)NULL, 10);

			ipsecTunnel.ipsecLifeTime= strtol(val[20], (char **)NULL, 10);
			ipsecTunnel.ipsecPfs= atoi(val[21]);
			if(strlen(val[22]) >  (MAX_SPI_LEN-1)){
				printf("SPI too long !\n");
				return;
			}else
				strcpy(ipsecTunnel.spi, val[22]); 

			if(strlen(val[23]) >  (MAX_ENCRKEY_LEN-1)){
				printf("Encryption key too long !\n");
				return;
			}else
				strcpy(ipsecTunnel.encrKey, val[23]); 

			if(strlen(val[24]) >  (MAX_AUTHKEY_LEN-1)){
				printf("Authentication key too long !\n");
				return;
			}else
				strcpy(ipsecTunnel.authKey, val[24]); 


		}
		else if ( !strcmp(val[0], "del")) {
			id = MIB_IPSECTUNNEL_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_IPSECTUNNEL_TBL_NUM, (void *)&entryNum)) {
				printf("Get ipsec tunnel number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&ipsecTunnel) = (char)int_val;
			if ( !APMIB_GET(MIB_IPSECTUNNEL_TBL, (void *)&ipsecTunnel)) {
				printf("Get table entry error!");
				return;
			}
		}
		else if ( !strcmp(val[0], "delall"))
			id = MIB_IPSECTUNNEL_DELALL;
		value = (void *)&ipsecTunnel;
		break;

#endif

#ifdef CONFIG_IPV6
	case RADVDPREFIX_T:
		if(valNum != 24 && valNum != 33)
		{
			printf("valNum=%d input argumentis not enough should be like \
				1 br0 600 200 3 0 0 1500 0 0 64 1800 medium 1 0 2001:0:0:0:0:0:0:0 64 1 1 2592000 604800 0 eth1 1\
				or\
				1 br0 600 200 3 0 0 1500 0 0 64 1800 medium 1 0 2001:0:0:0:0:0:0:0 64 1 1 2592000 604800 0 eth1 1 2002:0:0:0:0:0:0:0 64 1 1 2592000 604800 0 eth1 1",valNum);			
			return;
		}
		
		radvdCfgParam.enabled=atoi(val[0]);
		strcpy(radvdCfgParam.interface.Name,val[1]);
		radvdCfgParam.interface.MaxRtrAdvInterval=atoi(val[2]);
		radvdCfgParam.interface.MinRtrAdvInterval=atoi(val[3]);
		radvdCfgParam.interface.MinDelayBetweenRAs=atoi(val[4]);
		radvdCfgParam.interface.AdvManagedFlag=atoi(val[5]);
		radvdCfgParam.interface.AdvOtherConfigFlag=atoi(val[6]);
		radvdCfgParam.interface.AdvLinkMTU=atoi(val[7]);
		/*replace atoi by strtoul to support max value test of ipv6 phase 2 test*/
		radvdCfgParam.interface.AdvReachableTime=strtoul(val[8],NULL,10);
		radvdCfgParam.interface.AdvRetransTimer=strtoul(val[9],NULL,10);
		radvdCfgParam.interface.AdvCurHopLimit=atoi(val[10]);
		radvdCfgParam.interface.AdvDefaultLifetime=atoi(val[11]);
		strcpy(radvdCfgParam.interface.AdvDefaultPreference,val[12]);
		radvdCfgParam.interface.AdvSourceLLAddress=atoi(val[13]);
		radvdCfgParam.interface.UnicastOnly=atoi(val[14]);

		/*prefix1*/
		if(inet_pton(AF_INET6,val[15],(void*)(radvdCfgParam.interface.prefix[0].Prefix))!=1)
		{
			printf("invalid ipv6 address!\n");
			return;
		}
		
		radvdCfgParam.interface.prefix[0].PrefixLen=atoi(val[16]);
		radvdCfgParam.interface.prefix[0].AdvOnLinkFlag=atoi(val[17]);
		radvdCfgParam.interface.prefix[0].AdvAutonomousFlag=atoi(val[18]);
		/*replace atoi by strtoul to support max value test of ipv6 phase 2 test*/
		radvdCfgParam.interface.prefix[0].AdvValidLifetime=strtoul(val[19],NULL,10);
		radvdCfgParam.interface.prefix[0].AdvPreferredLifetime=strtoul(val[20],NULL,10);
		radvdCfgParam.interface.prefix[0].AdvRouterAddr=atoi(val[21]);
		strcpy(radvdCfgParam.interface.prefix[0].if6to4,val[22]);
		radvdCfgParam.interface.prefix[0].enabled=atoi(val[23]);
		if(valNum==33)
		{
			/*prefix2*/
			if(inet_pton(AF_INET6,val[24],(void*)(radvdCfgParam.interface.prefix[1].Prefix))!=1)
			{
				printf("invalid ipv6 address!\n");
				return;
			}
			radvdCfgParam.interface.prefix[1].PrefixLen=atoi(val[25]);
			radvdCfgParam.interface.prefix[1].AdvOnLinkFlag=atoi(val[26]);
			radvdCfgParam.interface.prefix[1].AdvAutonomousFlag=atoi(val[26]);
			/*replace atoi by strtoul to support max value test of ipv6 phase 2 test*/
			radvdCfgParam.interface.prefix[1].AdvValidLifetime=strtoul(val[28],NULL,10);
			radvdCfgParam.interface.prefix[1].AdvPreferredLifetime=strtoul(val[29],NULL,10);
			radvdCfgParam.interface.prefix[1].AdvRouterAddr=atoi(val[30]);
			strcpy(radvdCfgParam.interface.prefix[1].if6to4,val[31]);
			radvdCfgParam.interface.prefix[1].enabled=atoi(val[32]);
		}
		value=(void *)&radvdCfgParam;
		break;
	case DNSV6_T:
		if(valNum < 2)
			printf("input argumentis not enough");
		dnsv6CfgParam.enabled=atoi(val[0]);
		strcpy(dnsv6CfgParam.routerName,val[1]);
		value=(void *)&dnsv6CfgParam;
		break;
	case DHCPV6S_T:
		if(valNum < 5)
			printf("input argumentis not enough");
		dhcp6sCfgParam.enabled=atoi(val[0]);
		strcpy(dhcp6sCfgParam.DNSaddr6,val[1]);
		strcpy(dhcp6sCfgParam.addr6PoolS,val[2]);
		strcpy(dhcp6sCfgParam.addr6PoolE,val[3]);
		strcpy(dhcp6sCfgParam.interfaceNameds,val[4]);
		value=(void *)&dhcp6sCfgParam;
		break;
	case DHCPV6C_T:
		if(valNum < 5)
			printf("input argumentis not enough");
		dhcp6cCfgParam.enabled=atoi(val[0]);
		strncpy(&dhcp6cCfgParam.ifName,val[1],NAMSIZE);
		dhcp6cCfgParam.dhcp6pd.sla_len=atoi(val[2]);
		dhcp6cCfgParam.dhcp6pd.sla_id=atoi(val[3]);
		strncpy(&dhcp6cCfgParam.dhcp6pd.ifName,val[4],NAMSIZE);
		value=(void *)&dhcp6cCfgParam;
		break;
	case ADDR6_T:
		if(valNum < 19)
			printf("input argumentis not enough");
		addrIPv6CfgParam.enabled=atoi(val[0]);
		addrIPv6CfgParam.prefix_len[0]=atoi(val[1]);
		addrIPv6CfgParam.prefix_len[1]=atoi(val[2]);

		addrIPv6CfgParam.addrIPv6[0][0]=atoi(val[3]);
		addrIPv6CfgParam.addrIPv6[0][1]=atoi(val[4]);
		addrIPv6CfgParam.addrIPv6[0][2]=atoi(val[5]);
		addrIPv6CfgParam.addrIPv6[0][3]=atoi(val[6]);
		addrIPv6CfgParam.addrIPv6[0][4]=atoi(val[7]);
		addrIPv6CfgParam.addrIPv6[0][5]=atoi(val[8]);
		addrIPv6CfgParam.addrIPv6[0][6]=atoi(val[9]);
		addrIPv6CfgParam.addrIPv6[0][7]=atoi(val[10]);

		addrIPv6CfgParam.addrIPv6[1][0]=atoi(val[11]);
		addrIPv6CfgParam.addrIPv6[1][1]=atoi(val[12]);
		addrIPv6CfgParam.addrIPv6[1][2]=atoi(val[13]);
		addrIPv6CfgParam.addrIPv6[1][3]=atoi(val[14]);
		addrIPv6CfgParam.addrIPv6[1][4]=atoi(val[15]);
		addrIPv6CfgParam.addrIPv6[1][5]=atoi(val[16]);
		addrIPv6CfgParam.addrIPv6[1][6]=atoi(val[17]);
		addrIPv6CfgParam.addrIPv6[1][7]=atoi(val[18]);
		value=(void *)&addrIPv6CfgParam;
		break;
		
	case ADDRV6_T:
		if(valNum < 9)
			printf("input argumentis not enough");
		addr6CfgParam.prefix_len=atoi(val[0]);
		addr6CfgParam.addrIPv6[0]=strtoul(val[1],NULL,16);
		addr6CfgParam.addrIPv6[1]=strtoul(val[2],NULL,16);
		addr6CfgParam.addrIPv6[2]=strtoul(val[3],NULL,16);
		addr6CfgParam.addrIPv6[3]=strtoul(val[4],NULL,16);
		addr6CfgParam.addrIPv6[4]=strtoul(val[5],NULL,16);
		addr6CfgParam.addrIPv6[5]=strtoul(val[6],NULL,16);
		addr6CfgParam.addrIPv6[6]=strtoul(val[7],NULL,16);
		addr6CfgParam.addrIPv6[7]=strtoul(val[8],NULL,16);
		value=(void *)&addr6CfgParam;
		break;

	case TUNNEL6_T:
		tunnelCfgParam.enabled=atoi(val[0]);
		value=(void *)&tunnelCfgParam;
		break;
#endif
#endif
#ifdef TLS_CLIENT
	case CERTROOT_ARRAY_T:
	if ( !strcmp(val[0], "add")) {
		id = MIB_CERTROOT_ADD;
		strcpy(certRoot.comment, val[1]);
	}
	else if ( !strcmp(val[0], "del")) {
			id = MIB_CERTROOT_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_CERTROOT_TBL_NUM, (void *)&entryNum)) {
				printf("Get cert ca number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&certRoot) = (char)int_val;
			if ( !APMIB_GET(MIB_CERTROOT_TBL, (void *)&certRoot)) {
				printf("Get table entry error!");
				return;
			}			
	}
	else if ( !strcmp(val[0], "delall"))
			id = MIB_CERTROOT_DELALL;
	value = (void *)&certRoot;
	break;
	case CERTUSER_ARRAY_T:
	if ( !strcmp(val[0], "add")) {
		id = MIB_CERTUSER_ADD;
		strcpy(certUser.comment, val[1]);
		strcpy(certUser.pass , val[2]);
	}
	else if ( !strcmp(val[0], "del")) {
			id = MIB_CERTUSER_DEL;
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);
			if ( !APMIB_GET(MIB_CERTUSER_TBL_NUM, (void *)&entryNum)) {
				printf("Get cert ca number error!");
				return;
			}
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&certUser) = (char)int_val;
			if ( !APMIB_GET(MIB_CERTUSER_TBL, (void *)&certUser)) {
				printf("Get table entry error!");
				return;
			}			
	}
	else if ( !strcmp(val[0], "delall"))
			id = MIB_CERTUSER_DELALL;
	value = (void *)&certUser;
	break;	
#endif

#ifdef WLAN_PROFILE
	case PROFILE_ARRAY_T:
		if ( !strcmp(val[0], "add")) {
			if (!strcmp(name, "PROFILE_TBL1"))							
				id = MIB_PROFILE_ADD1;
#if !defined(CONFIG_RTL_TRI_BAND) 
			else
				id = MIB_PROFILE_ADD2;
#else
			else if(!strcmp(name, "PROFILE_TBL2"))
				id = MIB_PROFILE_ADD2;				
			else if(!strcmp(name, "PROFILE_TBL3"))
				id = MIB_PROFILE_ADD3;	
#endif
			strcpy(profile.ssid, val[1]);
			profile.encryption = atoi(val[2]);
			profile.auth = atoi(val[3]);

			if (profile.encryption == 1 || profile.encryption == 2) {
				profile.wep_default_key = atoi(val[4]);
				if (profile.encryption == 1) {
					string_to_hex(val[5], profile.wepKey1, 10);
					string_to_hex(val[6], profile.wepKey2, 10);
					string_to_hex(val[7], profile.wepKey3, 10);
					string_to_hex(val[8], profile.wepKey4, 10);					
				}
				else {
					string_to_hex(val[5], profile.wepKey1, 26);
					string_to_hex(val[6], profile.wepKey2, 26);
					string_to_hex(val[7], profile.wepKey3, 26);
					string_to_hex(val[8], profile.wepKey4, 26); 					
				}				
			}
			else if (profile.encryption == 3 || profile.encryption == 4) {
				profile.wpa_cipher = atoi(val[4]);
				strcpy(profile.wpaPSK, val[5]);	
			}	
		}
		else if ( !strcmp(val[0], "del")) {		
			if (!strcmp(name, "PROFILE_TBL1"))										
				id = MIB_PROFILE_DEL1;
#if !defined(CONFIG_RTL_TRI_BAND) 
			else
				id = MIB_PROFILE_DEL2;
#else
			else if(!strcmp(name, "PROFILE_TBL2"))
				id = MIB_PROFILE_DEL2;				
			else if(!strcmp(name, "PROFILE_TBL3"))
				id = MIB_PROFILE_DEL3;	
#endif
			if ( valNum < 2 ) {
				printf("input argument is not enough!\n");
				return;
			}
			int_val = atoi(val[1]);			
			if (!strcmp(name, "PROFILE_TBL1"))				
				APMIB_GET(MIB_PROFILE_NUM1, (void *)&entryNum);
#if !defined(CONFIG_RTL_TRI_BAND) 
			else
				APMIB_GET(MIB_PROFILE_NUM2, (void *)&entryNum);
#else
			else if (!strcmp(name, "PROFILE_TBL2"))	
				APMIB_GET(MIB_PROFILE_NUM2, (void *)&entryNum);
			else if (!strcmp(name, "PROFILE_TBL3"))	
				APMIB_GET(MIB_PROFILE_NUM3, (void *)&entryNum);
#endif
				
			if ( int_val > entryNum ) {
				printf("Element number is too large!\n");
				return;
			}
			*((char *)&profile) = (char)int_val;

			if (!strcmp(name, "PROFILE_TBL1"))			
				APMIB_GET(MIB_PROFILE_TBL1, (void *)&profile);
#if !defined(CONFIG_RTL_TRI_BAND) 
			else
				APMIB_GET(MIB_PROFILE_TBL2, (void *)&profile);
#else
			else if (!strcmp(name, "PROFILE_TBL2"))
				APMIB_GET(MIB_PROFILE_TBL2, (void *)&profile);
			else if (!strcmp(name, "PROFILE_TBL3"))
				APMIB_GET(MIB_PROFILE_TBL3, (void *)&profile);
#endif
		}
		else if ( !strcmp(val[0], "delall")) {
			if (!strcmp(name, "PROFILE_TBL1"))			
				id = MIB_PROFILE_DELALL1;
#if !defined(CONFIG_RTL_TRI_BAND)
			else
				id = MIB_PROFILE_DELALL2;
#else
			else if (!strcmp(name, "PROFILE_TBL2"))	
				id = MIB_PROFILE_DELALL2;				
			else if (!strcmp(name, "PROFILE_TBL3"))	
				id = MIB_PROFILE_DELALL3;
#endif
		}
		value = (void *)&profile;
		break;
#endif // WLAN_PROFILE

	case BYTE13_T:
		if ( strlen(val[0])!=26 || !string_to_hex(val[0], key, 26)) {
			printf("invalid value!\n");
			return;
		}
		value = (void *)key;
		break;

	case STRING_T:
		if ( strlen(val[0]) > len) {
			printf("string value too long!\n");
			return;
		}
		value = (void *)val[0];
		break;
#ifdef VOIP_SUPPORT 
	case VOIP_T:
		// use flash voip set xxx to replace
		break;
#endif
#ifdef RTK_CAPWAP
	case CAPWAP_WTP_CONFIG_ARRAY_T:
		if (val[0] == NULL) {				
                        showCapwapSetWtpHelp();
			return;
		}
		if ( strcmp(val[0],"set-wtp")==0 ) {
			int wtpNum, i;
                        int wtp_id;
			CAPWAP_WTP_CONFIG_T wtpConfig_old;
                        unsigned char base_bssid[6];

                        if ( val[1]==NULL || (wtp_id=atoi(val[1]))==0 || wtp_id<=0 || wtp_id>=200) { // check wtp_id
                                printf("Please input wtp-id: 0 < wtp-id <= 200.\n");
                                return;
                        }
                        if ( val[2]==NULL || strlen(val[2])!=12 || !string_to_hex(val[2], base_bssid, 12)) { showCapwapSetWtpHelp(); return;} // bssid

			memset(&wtpConfig, 0, sizeof(CAPWAP_WTP_CONFIG_T));
			APMIB_GET(MIB_CAPWAP_WTP_CONFIG_TBL_NUM, &wtpNum);
			for (i=0; i<wtpNum; i++) {
				((unsigned char *)&wtpConfig_old)[0] = i+1;
				APMIB_GET(MIB_CAPWAP_WTP_CONFIG_TBL, &wtpConfig_old);
				if (wtpConfig_old.wtpId == wtp_id) {
					memcpy(&wtpConfig, &wtpConfig_old, sizeof(CAPWAP_WTP_CONFIG_T));
					APMIB_SET(MIB_CAPWAP_WTP_CONFIG_DEL, &wtpConfig_old);
					break;
				}
			}
                        if (i==wtpNum) {        // not found
				wtpConfig.wtpId = wtp_id;							// wtp_id
			}
                        memcpy(wtpConfig.wlanConfig[0][0].bssid, base_bssid, 6);        // bssid
				if (val[3]!=NULL && strlen(val[3])!=0) {
                                wtpConfig.channel[0] = atoi(val[3]);
                                if (val[4]!=NULL && strlen(val[4])!=0) {
                                        wtpConfig.channel[1] = atoi(val[4]);
					wtpConfig.radioNum = 2;
				} else {			
					wtpConfig.radioNum = 1;
				}
			} else {
				wtpConfig.radioNum = 0;
			}
			id = MIB_CAPWAP_WTP_CONFIG_ADD;
		} else if ( strcmp(val[0],"del-wtp")==0 ) {
			int wtpNum, i;
			unsigned char wtp_id;

                        if ( val[1]==NULL || (wtp_id=atoi(val[1]))==0 || wtp_id<=0 || wtp_id>=200) { // check wtp_id
                                printf("Please input wtp-id: 0 < wtp-id <= 200.\n");
                                return;
                        }

			APMIB_GET(MIB_CAPWAP_WTP_CONFIG_TBL_NUM, &wtpNum);
			for (i=0; i<wtpNum; i++) {	
				((unsigned char *)&wtpConfig)[0] = i+1;
				APMIB_GET(MIB_CAPWAP_WTP_CONFIG_TBL, &wtpConfig);
				if (wtpConfig.wtpId == wtp_id) {
					id = MIB_CAPWAP_WTP_CONFIG_DEL; 	
					break;
				}
			}
			if (i == wtpNum) {// not found
				printf("wtpId not exist!\n");
				return;
			}
		} else if ( strcmp(val[0],"delall-wtp")==0 ) {		
			id = MIB_CAPWAP_WTP_CONFIG_DELALL;
                } else if ( strcmp(val[0],"set-wlan")==0 ) {
                        int wtpNum, i;
                        unsigned char wtp_id, radio_id, wlan_id;
                        CAPWAP_WTP_CONFIG_T wtpConfig_old;
                        if ( val[1]==NULL || (wtp_id=atoi(val[1]))==0 || wtp_id<=0 || wtp_id>=200) { // check wtp_id
                                printf("Please input wtp-id: 0 < wtp-id <= 200.\n");
                                return;
                        }
                        if ( val[2]==NULL || (radio_id=atoi(val[2]))>=MAX_CAPWAP_RADIO_NUM) { showCapwapSetWtpHelp(); return;} // radio id
                        if ( val[3]==NULL || (wlan_id=atoi(val[3]))>=MAX_CAPWAP_WLAN_NUM) { showCapwapSetWtpHelp(); return;} // wlan id
                        if ( val[4]==NULL || strlen(val[4])<=0 ) { showCapwapSetWtpHelp(); return;} // wlan ssid

                        memset(&wtpConfig, 0, sizeof(CAPWAP_WTP_CONFIG_T));
                        APMIB_GET(MIB_CAPWAP_WTP_CONFIG_TBL_NUM, &wtpNum);
                        for (i=0; i<wtpNum; i++) {
                                ((unsigned char *)&wtpConfig_old)[0] = i+1;
                                APMIB_GET(MIB_CAPWAP_WTP_CONFIG_TBL, &wtpConfig_old);
                                if (wtpConfig_old.wtpId == wtp_id) {
                                        memcpy(&wtpConfig, &wtpConfig_old, sizeof(CAPWAP_WTP_CONFIG_T));
                                        APMIB_SET(MIB_CAPWAP_WTP_CONFIG_DEL, &wtpConfig_old);
                                        break;
                                }
                        }
                        if (i==wtpNum) {
                                printf("not existed wtpId\n");
                                return;
                        }
                        wtpConfig.wlanNum = MAX_CAPWAP_WLAN_NUM;
                        wtpConfig.wlanConfig[radio_id][wlan_id].enable = 1;                             // wlan  enable
                        strcpy(wtpConfig.wlanConfig[radio_id][wlan_id].ssid, val[4]);   // wlan  ssid

                        if (val[5]!=NULL && strlen(val[5])>0) {                         // key
                                wtpConfig.wlanConfig[radio_id][wlan_id].keyType = CAPWAP_KEY_TYPE_SHARED_WPA2_AES;
                                wtpConfig.wlanConfig[radio_id][wlan_id].pskFormat = PSK_FORMAT_PASSPHRASE;
                                strcpy(wtpConfig.wlanConfig[radio_id][wlan_id].key, val[5]);
		} else {
                                wtpConfig.wlanConfig[radio_id][wlan_id].keyType = CAPWAP_KEY_TYPE_NONE;
                        }

                        memcpy(wtpConfig.wlanConfig[radio_id][wlan_id].bssid, wtpConfig.wlanConfig[0][0].bssid, 6);
                        wtpConfig.wlanConfig[radio_id][wlan_id].bssid[5] += radio_id*MAX_CAPWAP_WLAN_NUM+wlan_id;
                        id = MIB_CAPWAP_WTP_CONFIG_ADD;
                } else if ( strcmp(val[0],"del-wlan")==0 ) {
                        int wtpNum, i;
                        unsigned char wtp_id, radio_id, wlan_id;
                        CAPWAP_WTP_CONFIG_T wtpConfig_old;
                        if ( val[1]==NULL || (wtp_id=atoi(val[1]))==0 || wtp_id<=0 || wtp_id>=200) { // check wtp_id
                                printf("Please input wtp-id: 0 < wtp-id <= 200.\n");
                                return;
                        }
                        if ( val[2]==NULL || (radio_id=atoi(val[2]))>=MAX_CAPWAP_RADIO_NUM) { showCapwapSetWtpHelp(); return;} // radio id
                        if ( val[3]==NULL || (wlan_id=atoi(val[3]))>=MAX_CAPWAP_WLAN_NUM) { showCapwapSetWtpHelp(); return;} // wlan id

                        memset(&wtpConfig, 0, sizeof(CAPWAP_WTP_CONFIG_T));
                        APMIB_GET(MIB_CAPWAP_WTP_CONFIG_TBL_NUM, &wtpNum);
                        for (i=0; i<wtpNum; i++) {
                                ((unsigned char *)&wtpConfig_old)[0] = i+1;
                                APMIB_GET(MIB_CAPWAP_WTP_CONFIG_TBL, &wtpConfig_old);
                                if (wtpConfig_old.wtpId == wtp_id && wtpConfig_old.wlanConfig[radio_id][wlan_id].enable) {
                                        memcpy(&wtpConfig, &wtpConfig_old, sizeof(CAPWAP_WTP_CONFIG_T));
                                        APMIB_SET(MIB_CAPWAP_WTP_CONFIG_DEL, &wtpConfig_old);
                                        break;
                                }
                        }
                        if (i==wtpNum) {
                                printf("not existed wlan\n");
                                return;
                        }
                        wtpConfig.wlanConfig[radio_id][wlan_id].enable = 0;
                        id = MIB_CAPWAP_WTP_CONFIG_ADD;
		} else {
                        showCapwapSetWtpHelp();
			return;
		}
		value = &wtpConfig;
		break;
#endif
	default: printf("invalid mib!\n"); return;
	}

	if ( !APMIB_SET(id, value))
		printf("set MIB failed!\n");
	printf("setMIB end...\n");
	if(config_area
#if defined(SET_MIB_FROM_FILE)
	&& (set_mib_from_file==0)
#endif
	) { 
		if (config_area == HW_MIB_AREA || config_area == HW_MIB_WLAN_AREA)
			apmib_update(HW_SETTING);
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		else if(config_area == BLUETOOTH_HW_AREA)
			apmib_update(BLUETOOTH_HW_SETTING);
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		else if(config_area == CUSTOMER_HW_AREA)
			apmib_update(CUSTOMER_HW_SETTING);
#endif
		else if (config_area == DEF_MIB_AREA || config_area == DEF_MIB_WLAN_AREA)
			apmib_update(DEFAULT_SETTING);
		else
			apmib_update(CURRENT_SETTING);
	}
}
static void dumpAllHW(void)
{
	int idx=0, num;
	mib_table_entry_T *pTbl=NULL;

	if ( !apmib_init_HW()) {
		printf("Initialize AP MIB failed!\n");
		return;
	}
#ifdef MBSSID
	vwlan_idx=0;
#endif
	 config_area=0;

next_tbl:
	if (++config_area ==DEF_MIB_AREA)
	 	return;
	if (config_area == HW_MIB_AREA)
		pTbl = hwmib_table;
	else if (config_area == HW_MIB_WLAN_AREA)
		pTbl = hwmib_wlan_table;
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
	else if (config_area == BLUETOOTH_HW_AREA)
		pTbl = bluetooth_hwmib_table;
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
	else if (config_area == CUSTOMER_HW_AREA)
		pTbl = customer_hwmib_table;
#endif

next_wlan:
	while (pTbl[idx].id) {
			num = 1;
		if (num >0) {
#ifdef MIB_TLV
			if(pTbl[idx].type == TABLE_LIST_T) // ignore table root entry. keith
			{
				idx++;
				continue;
			}
#endif // #ifdef MIB_TLV			
			if ( config_area == HW_MIB_AREA ||
				config_area == HW_MIB_WLAN_AREA)
			{
				printf("HW_");
				if (config_area == HW_MIB_WLAN_AREA) {
					printf("WLAN%d_", wlan_idx);
				}				
			}
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
			else if(config_area == BLUETOOTH_HW_AREA)
				printf("BLUETOOTH_HW_");
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
			else if(config_area == CUSTOMER_HW_AREA)
				printf("CUSTOMER_HW_");
#endif
			getMIB(pTbl[idx].name, pTbl[idx].id, pTbl[idx].type, num, 1 , NULL);
		}
		idx++;
	}
	idx = 0;

	if (config_area == HW_MIB_WLAN_AREA ) {
		if (++wlan_idx < NUM_WLAN_INTERFACE) 
			goto next_wlan;
		else
			wlan_idx = 0;		
	}
	
	goto next_tbl;
}



////////////////////////////////////////////////////////////////////////////////
static void dumpAll(void)
{
	int idx=0, num;
	mib_table_entry_T *pTbl=NULL;

	if ( !apmib_init()) {
		printf("Initialize AP MIB failed!\n");
		return;
	}

#ifdef MBSSID
	vwlan_idx=0;
#endif
	 config_area=0;
#ifdef MULTI_WAN_SUPPORT
wan_idx = 1;
#endif

next_tbl:
	if (++config_area >= MIB_AREA_NUM)
	 	return;
	if (config_area == HW_MIB_AREA)
		pTbl = hwmib_table;
	else if (config_area == HW_MIB_WLAN_AREA)
		pTbl = hwmib_wlan_table;
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
	else if (config_area == BLUETOOTH_HW_AREA)
		pTbl = bluetooth_hwmib_table;
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
	else if (config_area == CUSTOMER_HW_AREA)
		pTbl = customer_hwmib_table;
#endif
	else if (config_area == DEF_MIB_AREA || config_area == MIB_AREA)
		pTbl = mib_table;
	else if (config_area == DEF_MIB_WLAN_AREA || config_area == MIB_WLAN_AREA)
		pTbl = mib_wlan_table;
#ifdef MULTI_WAN_SUPPORT
	else if(config_area == DEF_MIB_WAN_AREA || config_area == MIB_WAN_AREA)
		pTbl = mib_waniface_tbl;
#endif

next_wlan:
	while (pTbl[idx].id) {
		if ( pTbl[idx].id == MIB_WLAN_MACAC_ADDR)
			APMIB_GET(MIB_WLAN_MACAC_NUM, (void *)&num);		
#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_) // below code copy above ACL code
		else if ( pTbl[idx].id == MIB_WLAN_MESH_ACL_ADDR)
			APMIB_GET(MIB_WLAN_MESH_ACL_NUM, (void *)&num);
#endif
#if defined(WLAN_PROFILE)
		else if ( pTbl[idx].id == MIB_PROFILE_TBL1)
			APMIB_GET(MIB_PROFILE_NUM1, (void *)&num);
		else if ( pTbl[idx].id == MIB_PROFILE_TBL2)
			APMIB_GET(MIB_PROFILE_NUM2, (void *)&num);
#if defined(CONFIG_RTL_TRI_BAND)
		else if ( pTbl[idx].id == MIB_PROFILE_TBL3)
			APMIB_GET(MIB_PROFILE_NUM3, (void *)&num);
#endif
#ifdef HOME_GATEWAY
		else if ( pTbl[idx].id == MIB_PORTFW_TBL)
			APMIB_GET(MIB_PORTFW_TBL_NUM, (void *)&num);
#endif
#endif		
		else if ( pTbl[idx].id == MIB_WLAN_WDS)
			APMIB_GET(MIB_WLAN_WDS_NUM, (void *)&num);
		else if ( pTbl[idx].id == MIB_WLAN_SCHEDULE_TBL)
			APMIB_GET(MIB_WLAN_SCHEDULE_TBL_NUM, (void *)&num);		
			
#if defined(VLAN_CONFIG_SUPPORTED)				
		else if ( pTbl[idx].id == MIB_VLANCONFIG_TBL){
			APMIB_GET(MIB_VLANCONFIG_TBL_NUM, (void *)&num);
		}
#endif
#if defined(CONFIG_8021Q_VLAN_SUPPORTED)				
		else if ( pTbl[idx].id == MIB_VLAN_TBL){
			APMIB_GET(MIB_VLAN_TBL_NUM, (void *)&num);
		}
#endif
#if defined(CONFIG_RTL_ETH_802DOT1X_SUPPORT)				
		else if ( pTbl[idx].id == MIB_ELAN_DOT1X_TBL){
			APMIB_GET(MIB_ELAN_DOT1X_TBL, (void *)&num);
		}
#endif		

#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
		else if ( pTbl[idx].id == MIB_CAP_PORTAL_ALLOW_TBL)
			APMIB_GET(MIB_CAP_PORTAL_ALLOW_TBL_NUM, (void *)&num);
#endif/*_SUPPORT_CAPTIVEPORTAL_PROFILE_*/

#ifdef HOME_GATEWAY
#if defined(CONFIG_APP_TR069) && defined(WLAN_SUPPORT)
		else if ( pTbl[idx].id == MIB_CWMP_WLANCONF_TBL){
			APMIB_GET(MIB_CWMP_WLANCONF_TBL_NUM, (void *)&num);
		}
#endif//#if defined(CONFIG_APP_TR069) && defined(WLAN_SUPPORT)
		else if ( pTbl[idx].id == MIB_PORTFW_TBL)
			APMIB_GET(MIB_PORTFW_TBL_NUM, (void *)&num);
		else if ( pTbl[idx].id == MIB_PORTFILTER_TBL)
			APMIB_GET(MIB_PORTFILTER_TBL_NUM, (void *)&num);
		else if ( pTbl[idx].id == MIB_IPFILTER_TBL)
			APMIB_GET(MIB_IPFILTER_TBL_NUM, (void *)&num);
		else if ( pTbl[idx].id == MIB_MACFILTER_TBL)
			APMIB_GET(MIB_MACFILTER_TBL_NUM, (void *)&num);
		else if ( pTbl[idx].id == MIB_URLFILTER_TBL)
			APMIB_GET(MIB_URLFILTER_TBL_NUM, (void *)&num);
		else if ( pTbl[idx].id == MIB_TRIGGERPORT_TBL)
			APMIB_GET(MIB_TRIGGERPORT_TBL_NUM, (void *)&num);
#if defined(GW_QOS_ENGINE) || defined(QOS_BY_BANDWIDTH)
		else if ( pTbl[idx].id == MIB_QOS_RULE_TBL)
			APMIB_GET(MIB_QOS_RULE_TBL_NUM, (void *)&num);
#endif


#ifdef QOS_OF_TR069
        else if ( pTbl[idx].id == MIB_QOS_CLASS_TBL)
            APMIB_GET(MIB_QOS_CLASS_TBL_NUM, (void *)&num);
        else if ( pTbl[idx].id == MIB_QOS_QUEUE_TBL)
            APMIB_GET(MIB_QOS_QUEUE_TBL_NUM, (void *)&num);
		else if ( pTbl[idx].id == MIB_TR098_QOS_APP_TBL)
            APMIB_GET(MIB_TR098_QOS_APP_TBL_NUM, (void *)&num);
		else if ( pTbl[idx].id == MIB_TR098_QOS_FLOW_TBL)
            APMIB_GET(MIB_TR098_QOS_FLOW_TBL_NUM, (void *)&num);
	  else if ( pTbl[idx].id == MIB_QOS_POLICER_TBL) //mark_qos
            APMIB_GET(MIB_QOS_POLICER_TBL_NUM, (void *)&num);	
	  else if ( pTbl[idx].id == MIB_QOS_QUEUESTATS_TBL)
            APMIB_GET(MIB_QOS_QUEUESTATS_TBL_NUM, (void *)&num);
#endif 
#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
		else if ( pTbl[idx].id == MIB_DHCP_SERVER_OPTION_TBL)
			APMIB_GET(MIB_DHCP_SERVER_OPTION_TBL_NUM, (void *)&num);
		else if ( pTbl[idx].id == MIB_DHCP_CLIENT_OPTION_TBL)
			APMIB_GET(MIB_DHCP_CLIENT_OPTION_TBL_NUM, (void *)&num);
		else if ( pTbl[idx].id == MIB_DHCPS_SERVING_POOL_TBL)
			APMIB_GET(MIB_DHCPS_SERVING_POOL_TBL_NUM, (void *)&num);
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */

#ifdef ROUTE_SUPPORT
		else if ( pTbl[idx].id == MIB_STATICROUTE_TBL)
			APMIB_GET(MIB_STATICROUTE_TBL_NUM, (void *)&num);
#endif //ROUTE

#ifdef CONFIG_RTL_AIRTIME
		else if ( pTbl[idx].id == MIB_AIRTIME_TBL)
			APMIB_GET(MIB_AIRTIME_TBL_NUM, (void *)&num);
		else if ( pTbl[idx].id == MIB_AIRTIME2_TBL)
			APMIB_GET(MIB_AIRTIME2_TBL_NUM, (void *)&num);
#endif /* CONFIG_RTL_AIRTIME */

#ifdef VPN_SUPPORT
		else if ( pTbl[idx].id == MIB_IPSECTUNNEL_TBL)
			APMIB_GET(MIB_IPSECTUNNEL_TBL_NUM, (void *)&num);
#endif
#endif // #ifdef HOME_GATEWAY
#ifdef TLS_CLIENT
		else if ( pTbl[idx].id == MIB_CERTROOT_TBL)
			APMIB_GET(MIB_CERTROOT_TBL_NUM, (void *)&num);
		else if ( pTbl[idx].id == MIB_CERTUSER_TBL)
			APMIB_GET(MIB_CERTUSER_TBL_NUM, (void *)&num);			
#endif
#ifdef TR181_SUPPORT
#ifdef CONFIG_IPV6
		else if(pTbl[idx].id == MIB_IPV6_DHCPC_SENDOPT_TBL)
		{
			num=IPV6_DHCPC_SENDOPT_NUM;
		}
#endif
		else if(pTbl[idx].id == MIB_DNS_CLIENT_SERVER_TBL)
			num = DNS_CLIENT_SERVER_NUM;
#endif
		else if ( pTbl[idx].id == MIB_DHCPRSVDIP_TBL)
		{
			APMIB_GET(MIB_DHCPRSVDIP_TBL_NUM, (void *)&num);			
		}
#ifdef RTK_CAPWAP
		else if ( pTbl[idx].id == MIB_CAPWAP_WTP_CONFIG_TBL)
		{
			APMIB_GET(MIB_CAPWAP_WTP_CONFIG_TBL_NUM, (void *)&num);			
		}
#endif
		else
		{
			num = 1;
		}

		if (num >0) {			
#ifdef MIB_TLV
			/*skip printf HW_ or DEF_ prefix when table root entry*/
			if(pTbl[idx].type == TABLE_LIST_T
#ifdef MULTI_WAN_SUPPORT
			 || pTbl[idx].type == WANIFACE_ARRAY_T
#endif
			) // ignore table root entry. keith
			{
				idx++;
				continue;
			}
#endif // #ifdef MIB_TLV
			if ( config_area == HW_MIB_AREA ||
				config_area == HW_MIB_WLAN_AREA)
			{
				printf("HW_");
			}
			else if (config_area == DEF_MIB_AREA || config_area == DEF_MIB_WLAN_AREA
#ifdef MULTI_WAN_SUPPORT
			|| config_area == DEF_MIB_WAN_AREA
#endif
			)
				printf("DEF_");
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
			else if(config_area == BLUETOOTH_HW_AREA)
				printf("BLUETOOTH_HW_");
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
			else if(config_area == CUSTOMER_HW_AREA)
				printf("CUSTOMER_HW_");
#endif
			if (config_area == HW_MIB_WLAN_AREA || config_area == DEF_MIB_WLAN_AREA || config_area == MIB_WLAN_AREA) {
#ifdef MBSSID
				if ((config_area == DEF_MIB_WLAN_AREA || config_area == MIB_WLAN_AREA) && vwlan_idx > 0)
					printf("WLAN%d_VAP%d_", wlan_idx, vwlan_idx-1);
				else
#endif
				printf("WLAN%d_", wlan_idx);
			}
#ifdef MULTI_WAN_SUPPORT
			else if(config_area == DEF_MIB_WAN_AREA || config_area == MIB_WAN_AREA)
				printf("WAN%d_", wan_idx);
#endif
			getMIB(pTbl[idx].name, pTbl[idx].id,
						pTbl[idx].type, num, 1 , NULL);
		}
		idx++;
	}
	idx = 0;

	if (config_area == HW_MIB_WLAN_AREA || config_area == DEF_MIB_WLAN_AREA || config_area == MIB_WLAN_AREA) {
#ifdef MBSSID
		if (config_area == DEF_MIB_WLAN_AREA || config_area == MIB_WLAN_AREA) {
				
			if (++vwlan_idx <= NUM_VWLAN_INTERFACE) 
				goto next_wlan;
			else
				vwlan_idx = 0;
		}
#endif
		if (++wlan_idx < NUM_WLAN_INTERFACE) 
			goto next_wlan;
		else
			wlan_idx = 0;		
	}
#ifdef MULTI_WAN_SUPPORT
	if(config_area == DEF_MIB_WAN_AREA || config_area == MIB_WAN_AREA)
	{
		if(++wan_idx<=WANIFACE_NUM)
			goto next_wlan;
		else
			wan_idx = 1;
	}
#endif
	goto next_tbl;
}

//////////////////////////////////////////////////////////////////////////////////
static void showHelp(void)
{
	printf("Usage: flash cmd\n");
	printf("option:\n");
	printf("cmd:\n");
	printf("      default -- write all flash parameters from hard code.\n");
#ifdef VOIP_SUPPORT
	printf("      default-sw -- write flash default/current setting from hard code.\n");
#endif
	printf("      get [wlan interface-index] mib-name -- get a specific mib from flash\n");
	printf("          memory.\n");
	printf("      set [wlan interface-index] mib-name mib-value -- set a specific mib into\n");
	printf("          flash memory.\n");
	printf("      all -- dump all flash parameters.\n");
	printf("      gethw hw-mib-name -- get a specific mib from flash\n");
	printf("          memory.\n");
	printf("      sethw hw-mib-name mib-value -- set a specific mib into\n");
	printf("          flash memory.\n");
	printf("      allhw -- dump all hw flash parameters.\n");
#ifdef VOIP_SUPPORT
	printf("      voip -- dump all voip mib parameters.\n");
	printf("      voip get mib-name -- get a specific VoIP mib value.\n");
	printf("      voip set mib-name mib-vale -- set a specific VoIP mib value.\n");
#endif
	printf("      reset -- reset current setting to default.\n");
#ifdef WLAN_FAST_INIT
	printf("      set_mib -- get mib from flash and set to wlan interface.\n");
#endif
#if defined(CONFIG_APP_BLUEZ)
	printf("      dump_btconfig -- dump BT configuration file.\n");
#endif
	printf("\n");
}

//////////////////////////////////////////////////////////////////////////////////
static void showAllHWMibName(void)
{
	int idx;
	mib_table_entry_T *pTbl;

	config_area = 0;
	while (config_area++ < 7) {
		idx = 0;
		if (config_area == HW_MIB_AREA || config_area == HW_MIB_WLAN_AREA) {
			if (config_area == HW_MIB_AREA)
				pTbl = hwmib_table;
			else
				pTbl = hwmib_wlan_table;
			while (pTbl[idx].id) {
				printf("HW_%s\n", pTbl[idx].name);
				idx++;
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////
static void showAllMibName(void)
{
	int idx;
	mib_table_entry_T *pTbl;

	config_area = 0;
	while (config_area++ < MIB_WLAN_AREA+1) {
		idx = 0;
		if (config_area == HW_MIB_AREA || config_area == HW_MIB_WLAN_AREA) {
			if (config_area == HW_MIB_AREA)
				pTbl = hwmib_table;
			else
				pTbl = hwmib_wlan_table;
			while (pTbl[idx].id) {
				printf("HW_%s\n", pTbl[idx].name);
				idx++;
			}
		}
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		else if(config_area == BLUETOOTH_HW_AREA)
		{
			pTbl = bluetooth_hwmib_table;
			while (pTbl[idx].id) {
				printf("BLUETOOTH_HW_%s\n", pTbl[idx].name);
				idx++;
			}
		}
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		else if(config_area == CUSTOMER_HW_AREA)
		{
			pTbl = customer_hwmib_table;
			while (pTbl[idx].id) {
				printf("CUSTOMER_HW_%s\n", pTbl[idx].name);
				idx++;
			}
		}
#endif
		else {
			if (config_area == DEF_MIB_AREA || config_area == MIB_AREA)
				pTbl = mib_table;
			else
				pTbl = mib_wlan_table;

			if (config_area == DEF_MIB_AREA || config_area == DEF_MIB_WLAN_AREA)
				printf("DEF_");

			while (pTbl[idx].id) {
				printf("%s\n", pTbl[idx].name);
				idx++;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////
static void showSetACHelp(void)
{
#if 0
	printf("flash set MACAC_ADDR cmd\n");
	printf("cmd:\n");
	printf("      add mac-addr comment -- append a filter mac address.\n");
	printf("      del entry-number -- delete a filter entry.\n");
	printf("      delall -- delete all filter mac address.\n");
#endif	
}

#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_) // below code copy above ACL code
static void showSetMeshACLHelp(void)
{
#if 0
	printf("flash set MESH_ACL_ADDR cmd\n");
	printf("cmd:\n");
	printf("      add mac-addr comment -- append a filter mac address.\n");
	printf("      del entry-number -- delete a filter entry.\n");
	printf("      delall -- delete all filter mac address.\n");
#endif	
}
#endif

#ifdef VLAN_CONFIG_SUPPORTED
////////////////////////////////////////////////////////////////////////////////////
static void showSetVlanConfigHelp(void)
{
#if 0
	printf("flash set VLAN CONFIG  cmd\n");
	printf("cmd:\n");
	printf("      add enable iface -- update vlan config for specific iface.\n");
	printf("      del entry-number -- delete a vlan config entry.\n");
	printf("      delall -- delete all vlan config entry\n");
#endif		
}
#endif
#if defined(SAMBA_WEB_SUPPORT)
////////////////////////////////////////////////////////////////////////////////////
static void showSetSambaAccountHelp(void)
{
#if 0
	printf("flash set STORAGE_USER_TBL cmd\n");
	printf("cmd:\n");
	printf("      add username password -- add a samba account.\n");
	printf("      del username -- delete a samba account entry by username.\n");
	printf("      delall -- delete all samba account entry\n");
#endif		
}
////////////////////////////////////////////////////////////////////////////////////
static void showSetSambaShareinfoHelp(void)
{
#if 0
	printf("flash set STORAGE_SHAREINFO_TBL cmd\n");
	printf("cmd:\n");
	printf("      add sharefoldername sharefolderpath owner permission -- add a samba share folder.\n");
	printf("      del sharefoldername -- delete a samba entry by sharefoldername.\n");
	printf("      delall -- delete all samba shareinfo entry\n");
#endif		
}
#endif

#ifdef CONFIG_RTL_ETH_802DOT1X_SUPPORT
////////////////////////////////////////////////////////////////////////////////////
static void showSetEthDot1xConfigHelp(void)
{
#if 0
	printf("flash set ETH DOT1X CONFIG  cmd\n");
	printf("cmd:\n");
	printf("      add port_number -- update eth dot1x config for specific port.\n");
	printf("      del entry-number -- delete a eth dot1x config entry.\n");
	printf("      delall -- delete all vlan config entry\n");
#endif		
}
#endif


///////////////////////////////////////////////////////////////////////////////////
static void showSetWdsHelp(void)
{
#if 0
	printf("flash set WDS cmd\n");
	printf("cmd:\n");
	printf("      add mac-addr comment -- append a WDS mac address.\n");
	printf("      del entry-number -- delete a WDS entry.\n");
	printf("      delall -- delete all WDS mac address.\n");
#endif	
}

#ifdef HOME_GATEWAY
///////////////////////////////////////////////////////////////////////////////////
static void showSetPortFwHelp(void)
{
#if 0
	printf("flash set PORTFW_TBL cmd\n");
	printf("cmd:\n");
	printf("      add ip from-port to-port protocol comment -- add a filter.\n");
	printf("      del entry-number -- delete a filter.\n");
	printf("      delall -- delete all filter.\n");
#endif	
}


///////////////////////////////////////////////////////////////////////////////////
static void showSetPortFilterHelp(void)
{
#if 0
	printf("flash set PORTFILTER_TBL cmd\n");
	printf("cmd:\n");
	printf("      add from-port to-port protocol comment -- add a filter.\n");
	printf("      del entry-number -- delete a filter.\n");
	printf("      delall -- delete all filter.\n");
#endif	
}

///////////////////////////////////////////////////////////////////////////////////
static void showSetIpFilterHelp(void)
{
#if 0
	printf("flash set IPFILTER_TBL cmd\n");
	printf("cmd:\n");
	printf("      add ip protocol comment -- add a filter.\n");
	printf("      del entry-number -- delete a filter.\n");
	printf("      delall -- delete all filter.\n");
#endif	
}

///////////////////////////////////////////////////////////////////////////////////
static void showSetMacFilterHelp(void)
{
#if 0
	printf("flash set MACFILTER_TBL cmd\n");
	printf("cmd:\n");
	printf("      add mac-addr comment -- add a filter.\n");
	printf("      del entry-number -- delete a filter.\n");
	printf("      delall -- delete all filter.\n");
#endif	
}
///////////////////////////////////////////////////////////////////////////////////
static void showSetUrlFilterHelp(void)
{
#if 0
	printf("flash set URLFILTER_TBL cmd\n");
	printf("cmd:\n");
	printf("      add url-addr -- add a filter.\n");
	printf("      del entry-number -- delete a filter.\n");
	printf("      delall -- delete all filter.\n");
#endif	
}
///////////////////////////////////////////////////////////////////////////////////
static void showSetTriggerPortHelp(void)
{
#if 0
	printf("flash set TRIGGER_PORT cmd\n");
	printf("cmd:\n");
	printf("   add trigger-from trigger-to trigger-proto incoming-from incoming-to incoming-proto comment -- add a trigger-port.\n");
	printf("   del entry-number -- delete a trigger-port.\n");
	printf("   delall -- delete all trigger-port.\n");
#endif	
}

#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
static void showSetDhcpServerOptionHelp(void)
{
	//nothing
}

static void showSetDhcpClientOptionHelp(void)
{
	//nothing
}

static void showSetDhcpsServingPoolHelp(void)
{
	//nothing
}
#endif

///////////////////////////////////////////////////////////////////////////////////
#ifdef CONFIG_RTL_AIRTIME
static void showSetAirTimeHelp(void)
{
#if 0
	printf("flash set AIRTIME_TBL cmd\n");
	printf("cmd:\n");
	printf("      add ip mac sta_time comment -- add a station.\n");
	printf("      del entry-number -- delete a v.\n");
	printf("      delall -- delete all station.\n");
#endif	
}
#endif /* CONFIG_RTL_AIRTIME */

#ifdef GW_QOS_ENGINE
///////////////////////////////////////////////////////////////////////////////////
//static void showSetQosHelp(void) {}
#endif
///////////////////////////////////////////////////////////////////////////////////
#ifdef ROUTE_SUPPORT
static void showSetStaticRouteHelp(void)
{
	printf("flash set STATICROUTE_TBL cmd\n");
	printf("cmd:\n");
	printf("   add dest_ip netmask gateway  -- add a static route.\n");
	printf("   del entry-number -- delete a static route.\n");
	printf("   delall -- delete all static route.\n");


}
#endif //ROUTE
#endif

#ifdef HOME_GATEWAY
#ifdef VPN_SUPPORT
static void  showSetIpsecTunnelHelp(void)
{
        printf("flash set IPSECTUNNEL_TBL cmd\n");
        printf("cmd:\n");
        printf("   add tunnel_id enable name local_type local_ip local_mask_len remote_type remote_ip remote_mask_len remote_gw keymode connectType espEncr espAuth psKey ike_encr ike_auth ike_keygroup ike_lifetime ipsec_lifetime ipsec_pfs spi encrKey authKey -- add a ipsec manual tunnel.\n");
        printf("   del entry-number -- delete a vpn tunnel.\n");
        printf("   delall -- delete all tunnel.\n");
}
#endif
#endif

#ifdef TLS_CLIENT
static void  showSetCertRootHelp(void)
{
        printf("flash set CERTROOT_TBL cmd\n");
        printf("cmd:\n");
        printf("   add comment.\n");
        printf("   del entry-number -- delete a certca .\n");
        printf("   delall -- delete all certca.\n");
}
static void  showSetCertUserHelp(void)
{
        printf("flash set CERTUSER_TBL cmd\n");
        printf("cmd:\n");
        printf("   add comment password.\n");
        printf("   del entry-number -- delete a certca .\n");
        printf("   delall -- delete all certca.\n");
}
#endif
#ifdef RTK_CAPWAP
static void showCapwapSetWtpHelp()
{
	printf("set-wtp <wtp-id> <base-bssid> [channel-0] [channel-1] | del-wtp <wtp-id> | delall-wtp | "
	                "set-wlan <wtp-id> <radio-id> <wlan-id> <ssid> [key] | del-wlan <wtp-id> <radio-id> <wlan-id>\n");
}
#endif

#ifdef PARSE_TXT_FILE
////////////////////////////////////////////////////////////////////////////////
static int parseTxtConfig(char *filename, APMIB_Tp pConfig)
{
	char line[300], value[300];
	FILE *fp;
	int id;

	fp = fopen(filename, "r");
	if ( fp == NULL )
		return -1;

	acNum = 0;
	
#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_)
	meshAclNum = 0;
#endif

	wdsNum = 0;

#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	capPortalAllowNum = 0;
#endif
#ifdef HOME_GATEWAY
	portFilterNum = ipFilterNum = macFilterNum = portFwNum = staticRouteNum=0;
	urlFilterNum = 0;

#if defined(GW_QOS_ENGINE) || defined(QOS_BY_BANDWIDTH)
	qosRuleNum = 0;
#endif

//added by shirley  2014/5/21
#ifdef QOS_OF_TR069
    QosClassNum = 0;
    QosQueueNum = 0;
	tr098_appconf_num =0;
	tr098_flowconf_num =0;
	QosPolicerNum = 0; //mark_qos
	 QosQueueStatsNum = 0;
#endif

#endif
#ifdef TLS_CLIENT
	certRootNum =  certUserNum = 0;
#endif

#if defined(VLAN_CONFIG_SUPPORTED)
 vlanConfigNum=0;	
#endif

#ifdef CONFIG_RTL_AIRTIME
	airTimeNum = 0;
#endif

	while ( fgets(line, 100, fp) ) {
		id = getToken(line, value);
		if ( id == 0 )
			continue;
		if ( set_mib(pConfig, id, value) < 0) {
			printf("Parse MIB [%d] error!\n", id );
			fclose(fp);
			return -1;
		}
	}

	fclose(fp);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
static int getToken(char *line, char *value)
{
	char *ptr=line, *p1;
	char token[300]={0};
	int len=0, idx;

	if ( *ptr == ';' )	// comments
		return 0;

	// get token
	while (*ptr && *ptr!=EOL) {
		if ( *ptr == '=' ) {
			memcpy(token, line, len);

			// delete ending space
			for (idx=len-1; idx>=0; idx--) {
				if (token[idx]!= SPACE )
					break;
			}
			token[idx+1] = '\0';
			ptr++;
			break;
		}
		ptr++;
		len++;
	}
	if ( !token[0] )
		return 0;

	// get value
	len=0;
	while (*ptr == SPACE ) ptr++; // delete space

	p1 = ptr;
	while ( *ptr && *ptr!=EOL) {
		ptr++;
		len++;
	}
	memcpy(value, p1, len );
	value[len] = '\0';

	idx = 0;
	while (mib_table[idx].id) {
		if (!strcmp(mib_table[idx].name, token))
			return mib_table[idx].id;
		idx++;
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
static int set_mib(APMIB_Tp pMib, int id, void *value)
{
	unsigned char key[100];
#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
	char *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10;
	char *p11, *p12, *p13, *p14, *p15, *p16, *p17, *p18, *p19, *p20;
	char *p21, *p22, *p23, *p24, *p25, *p26, *p27, *p28, *p29;
#else
	char *p1, *p2;
#ifdef HOME_GATEWAY
	char *p3, *p4, *p5;
#if defined(GW_QOS_ENGINE) || defined(VLAN_CONFIG_SUPPORTED) || defined(QOS_OF_TR069) && !defined(QOS_BY_BANDWIDTH)
	char *p6, *p7, *p8, *p9, *p10, *p11, *p12;
#endif

#ifdef QOS_OF_TR069
    char *p13, *p14, *p15, *p16, *p17, *p18, *p19, *p20; 
    char *p21, *p22, *p23, *p24, *p25, *p26, *p27, *p28, *p29, *p30;
    char *p31, *p32, *p33, *p34, *p35, *p36, *p37, *p38, *p39, *p40; 
    char *p41, *p42, *p43, *p44, *p45, *p46, *p47, *p48, *p49, *p50;
    char *p51, *p52, *p53, *p54, *p55, *p56, *p57, *p58, *p59, *p60;
    char *p61, *p62, *p63, *p64, *p65, *p66, *p67, *p68, *p69, *p70;
    char *p71, *p72, *p73, *p74, *p75, *p76, *p77, *p78, *p79, *p80;
#endif

#if defined(QOS_BY_BANDWIDTH) && !defined(GW_QOS_ENGINE)

#if defined(VLAN_CONFIG_SUPPORTED)
	char *p6, *p7, *p8;
#else
	char *p6, *p7;
#endif

#endif

#else

#if defined(VLAN_CONFIG_SUPPORTED)	
	char *p3, *p4, *p5, *p6, *p7, *p8;
#endif

#endif
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */
	struct in_addr inAddr;
	int i;
	MACFILTER_Tp pWlAc;
	WDS_Tp pWds;

#ifdef HOME_GATEWAY
	PORTFW_Tp pPortFw;
	PORTFILTER_Tp pPortFilter;
	IPFILTER_Tp pIpFilter;
	MACFILTER_Tp pMacFilter;
	URLFILTER_Tp pUrlFilter;

#ifdef GW_QOS_ENGINE
	QOS_Tp pQos;    
#endif

#ifdef QOS_BY_BANDWIDTH
	IPQOS_Tp pQos;    
#endif

//added by shirley 2014/5/21
#ifdef QOS_OF_TR069
    QOSCLASS_Tp pQosClass;
    QOSQUEUE_Tp pQosQueue;
	TR098_APPCONF_Tp pTr098QosApp;
	TR098_FLOWCONF_Tp pTr098QosFlow;
	QOSPOLICER_Tp pQosPolicer;
	QOSQUEUESTATS_Tp pQosQueueStats;
#endif
#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
	MIB_CE_DHCP_OPTION_Tp pDhcpdOpt;
	MIB_CE_DHCP_OPTION_Tp pDhcpcOpt;
	DHCPS_SERVING_POOL_Tp pServingPool;
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */

#ifdef CONFIG_RTL_AIRTIME
	AIRTIME_Tp pAirTime;
#endif /* CONFIG_RTL_AIRTIME */

#endif

#ifdef TLS_CLIENT
	CERTROOT_Tp pCertRoot;
	CERTUSER_Tp pCertUser;
#endif
	DHCPRSVDIP_Tp pDhcpRsvd;
#ifdef TR181_SUPPORT
#ifdef CONFIG_IPV6
	DHCPV6C_SENDOPT_T pDhcp6cSendOpt;
#endif
	DNS_CLIENT_SERVER_T pDnsClientServer;
#endif
#if defined(VLAN_CONFIG_SUPPORTED)	
	VLAN_CONFIG_Tp pVlanConfig;
	int j;
#endif
#if defined(SAMBA_WEB_SUPPORT)	
	STORAGE_USER_Tp pUserConfig;
	STORAGE_SHAREINFO_Tp pshareInfoConfig;
#endif
#if defined(CONFIG_RTL_ETH_802DOT1X_SUPPORT)	
	ETHDOT1X_Tp pethdot1xConfig;
	int k;
#endif	

	for (i=0; mib_table[i].id; i++) {
		if ( mib_table[i].id == id )
			break;
	}
	if ( mib_table[i].id == 0 )
		return -1;

	switch (mib_table[i].type) {
	case BYTE_T:
		*((unsigned char *)(((long)pMib) + mib_table[i].offset)) = (unsigned char)atoi(value);
		break;

	case WORD_T:
		*((unsigned short *)(((long)pMib) + mib_table[i].offset)) = (unsigned short)atoi(value);
		break;

	case STRING_T:
		if ( strlen(value)+1 > mib_table[i].size )
			return 0;
		strcpy((char *)(((long)pMib) + mib_table[i].offset), (char *)value);
		break;

	case BYTE5_T:
		if (strlen(value)!=10 || !string_to_hex(value, key, 10))
			return -1;
		memcpy((unsigned char *)(((long)pMib) + mib_table[i].offset), key, 5);
		break;

	case BYTE6_T:
		if (strlen(value)!=12 || !string_to_hex(value, key, 12))
			return -1;
		memcpy((unsigned char *)(((long)pMib) + mib_table[i].offset), key, 6);
		break;

	case BYTE13_T:
		if (strlen(value)!=26 || !string_to_hex(value, key, 26))
			return -1;
		memcpy((unsigned char *)(((long)pMib) + mib_table[i].offset), key, 13);
		break;
	
	case DWORD_T:
		*((unsigned long *)(((long)pMib) + mib_table[i].offset)) = (unsigned long)atoi(value);
		break;

	case IA_T:
		if ( !inet_aton(value, &inAddr) )
			return -1;
		memcpy((unsigned char *)(((long)pMib) + mib_table[i].offset), (unsigned char *)&inAddr,  4);
		break;

	// CONFIG_RTK_MESH Note: The statement haven't use maybe, Because mib_table haven't WLAC_ARRAY_T
	case WLAC_ARRAY_T:
		getVal2((char *)value, &p1, &p2);
		if (p1 == NULL) {
			printf("Invalid WLAC in argument!\n");
			break;
		}
		if (strlen(p1)!=12 || !string_to_hex(p1, key, 12))
			return -1;

		pWlAc = (MACFILTER_Tp)(((long)pMib)+mib_table[i].offset+acNum*sizeof(MACFILTER_T));
		memcpy(pWlAc->macAddr, key, 6);
		if (p2 != NULL )
			strcpy(pWlAc->comment, p2);
		acNum++;
		break;

//#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_) // below code copy above ACL code
	case MESH_ACL_ARRAY_T:
		getVal2((char *)value, &p1, &p2);
		if (p1 == NULL) {
			printf("Invalid Mesh Acl in argument!\n");
			break;
		}
		if (strlen(p1)!=12 || !string_to_hex(p1, key, 12))
			return -1;

		pWlAc = (MACFILTER_Tp)(((long)pMib)+mib_table[i].offset+meshAclNum*sizeof(MACFILTER_T));
		memcpy(pWlAc->macAddr, key, 6);
		if (p2 != NULL )
			strcpy(pWlAc->comment, p2);
		meshAclNum++;
		break;
//#endif

	case WDS_ARRAY_T:
		getVal3((char *)value, &p1, &p2, &p3);
		if (p1 == NULL) {
			printf("Invalid WDS in argument!\n");
			break;
		}
		if (strlen(p1)!=12 || !string_to_hex(p1, key, 12))
			return -1;

		pWds = (WDS_Tp)(((long)pMib)+mib_table[i].offset+wdsNum*sizeof(WDS_T));
		memcpy(pWds->macAddr, key, 6);
		if (p2 != NULL )
			strcpy(pWds->comment, p2);
		pWds->fixedTxRate = strtoul(p3,0,10);//(unsigned int)atoi(p3);	
		wdsNum++;
		break;

#ifdef HOME_GATEWAY
	case MACFILTER_ARRAY_T:
		getVal2((char *)value, &p1, &p2);
		if (p1 == NULL) {
			printf("Invalid MACFILTER in argument!\n");
			break;
		}
		if (strlen(p1)!=12 || !string_to_hex(p1, key, 12))
			return -1;

		pMacFilter = (MACFILTER_Tp)(((long)pMib)+mib_table[i].offset+macFilterNum*sizeof(MACFILTER_T));
		memcpy(pMacFilter->macAddr, key, 6);
		if (p2 != NULL )
			strcpy(pMacFilter->comment, p2);
		macFilterNum++;
		break;

	case URLFILTER_ARRAY_T:
		getVal2((char *)value, &p1, &p2);
		if (p1 == NULL) {
			printf("Invalid URLFILTER in argument!\n");
			break;
		}
		//if (strlen(p1)!=12 || !string_to_hex(p1, key, 12))
		//	return -1;

		pUrlFilter = (URLFILTER_Tp)(((long)pMib)+mib_table[i].offset+urlFilterNum*sizeof(URLFILTER_T));
		if(!strncmp(p1,"http://",7))				
			memcpy(pUrlFilter->urlAddr, p1+7, 20);
		else
			memcpy(pUrlFilter->urlAddr, p1, 20);
		
		if (p2 != NULL )
			pUrlFilter->ruleMode=atoi(p2);
		urlFilterNum++;
		break;

	case PORTFW_ARRAY_T:
		#if defined(CONFIG_RTL_PORTFW_EXTEND)		
		getVal7((char *)value, &p1, &p2, &p3, &p4, &p5, &p6, &p7);
		#else
		getVal5((char *)value, &p1, &p2, &p3, &p4, &p5);
		#endif
		if (p1 == NULL || p2 == NULL || p3 == NULL || p4 == NULL ) {
			printf("Invalid PORTFW arguments!\n");
			break;
		}
		if ( !inet_aton(p1, &inAddr) )
			return -1;

		pPortFw = (PORTFW_Tp)(((long)pMib)+mib_table[i].offset+portFwNum*sizeof(PORTFW_T));
		memcpy(pPortFw->ipAddr, (unsigned char *)&inAddr, 4);
		
		#if defined(CONFIG_RTL_PORTFW_EXTEND)		
		pPortFw->toPort = pPortFw->fromPort = (unsigned short)atoi(p2);
		pPortFw->externeltoPort = pPortFw->externelFromPort = (unsigned short)atoi(p3);
		pPortFw->protoType = (unsigned char)atoi(p4);

		inet_aton(p5, &inAddr);
		memcpy(pPortFw->rmtipAddr, (unsigned char *)&inAddr, 4);
		if ( p6 )
			strcpy( pPortFw->comment, p6 );
		if ( p7 )
			pPortFw->enabled = (unsigned char)atoi(p7);
		else
			pPortFw->enabled= 1;
		#else
		pPortFw->fromPort = (unsigned short)atoi(p2);
		pPortFw->toPort = (unsigned short)atoi(p3);
		pPortFw->protoType = (unsigned char)atoi(p4);
		if ( p5 )
			strcpy( pPortFw->comment, p5 );
		#endif
		portFwNum++;
		break;

	case IPFILTER_ARRAY_T:
		getVal5((char *)value, &p1, &p2, &p3,&p4,&p5);
		if (p1 == NULL || p2 == NULL
#ifdef CONFIG_IPV6
			p4 == NULL || p5 == NULL
#endif
		) {
			printf("Invalid IPFILTER arguments!\n");
			break;
		}
		if ( !inet_aton(p1, &inAddr) )
			return -1;
		pIpFilter = (IPFILTER_Tp)(((long)pMib)+mib_table[i].offset+ipFilterNum*sizeof(IPFILTER_T));
		memcpy(pIpFilter->ipAddr, (unsigned char *)&inAddr, 4);
		pIpFilter->protoType = (unsigned char)atoi(p2);
		if ( p3 )
			strcpy( pIpFilter->comment, p3 );
#ifdef CONFIG_IPV6
		strcpy(pIpFilter->ip6Addr,p4);
		pIpFilter->ipVer = atoi(p5);
#endif
		ipFilterNum++;
		break;

	case PORTFILTER_ARRAY_T:
		getVal5((char *)value, &p1, &p2, &p3, &p4, &p5);
		if (p1 == NULL || p2 == NULL || p3 == NULL) {
			printf("Invalid PORTFILTER arguments!\n");
			break;
		}
		if ( !inet_aton(p1, &inAddr) )
			return -1;
		pPortFilter = (PORTFILTER_Tp)(((long)pMib)+mib_table[i].offset+portFilterNum*sizeof(PORTFILTER_T));
		pPortFilter->fromPort = (unsigned short)atoi(p1);
		pPortFilter->toPort = (unsigned short)atoi(p2);
		pPortFilter->protoType = (unsigned char)atoi(p3);
		if ( p4 )
			strcpy( pPortFilter->comment, p4 );
#ifdef CONFIG_IPV6
		if(p5)
			pPortFilter->ipVer = atoi(p5);
#endif
		portFilterNum++;
		break;
#ifdef GW_QOS_ENGINE
	case QOS_ARRAY_T:
		getVal12((char *)value, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10, &p11, &p12);
		if (p1 == NULL || p2 == NULL || p3 == NULL || p4 == NULL || p5 == NULL || p6 == NULL || p7 == NULL ||
		    p8 == NULL || p9 == NULL || p10 == NULL || p11 == NULL || p12 == NULL ) {
			printf("Invalid QoS arguments!\n");
			break;
		}
		pQos = (QOS_Tp)(((long)pMib)+mib_table[i].offset+qosRuleNum*sizeof(QOS_T));
		pQos->enabled = (unsigned char)atoi(p1);
		pQos->priority = (unsigned char)atoi(p2);
		pQos->protocol = (unsigned short)atoi(p3);
		if ( !inet_aton(p4, &inAddr) )
			return -1;
		memcpy(pQos->local_ip_start, (unsigned char *)&inAddr, 4);
		if ( !inet_aton(p5, &inAddr) )
			return -1;
		memcpy(pQos->local_ip_end, (unsigned char *)&inAddr, 4);
        
		pQos->local_port_start = (unsigned short)atoi(p6);
		pQos->local_port_end = (unsigned short)atoi(p7);
		if ( !inet_aton(p8, &inAddr) )
			return -1;
		memcpy(pQos->remote_ip_start, (unsigned char *)&inAddr, 4);
		if ( !inet_aton(p9, &inAddr) )
			return -1;
		memcpy(pQos->remote_ip_end, (unsigned char *)&inAddr, 4);

		pQos->remote_port_start = (unsigned short)atoi(p10);
		pQos->remote_port_end = (unsigned short)atoi(p11);
        	strcpy( pQos->entry_name, p12 );
		qosRuleNum++;        
		break;
#endif

#ifdef QOS_BY_BANDWIDTH
	case QOS_ARRAY_T:
		getVal7((char *)value, &p1, &p2, &p3, &p4, &p5, &p6, &p7);
		if (p1 == NULL || p2 == NULL || p3 == NULL || p4 == NULL || p5 == NULL || p6 == NULL || p7 == NULL) {
			printf("Invalid QoS arguments!\n");
			break;
		}
		pQos = (IPQOS_Tp)(((long)pMib)+mib_table[i].offset+qosRuleNum*sizeof(IPQOS_T));
		pQos->enabled = (unsigned char)atoi(p1);
        	//strcpy( pQos->mac, p2 );
		if (strlen(p2)!=12 || !string_to_hex(p2, pQos->mac, 12)) 
			return -1;		
		pQos->mode = (unsigned char)atoi(p3);
		if ( !inet_aton(p4, &inAddr) )
			return -1;
		memcpy(pQos->local_ip_start, (unsigned char *)&inAddr, 4);
		if ( !inet_aton(p5, &inAddr) )
			return -1;
		memcpy(pQos->local_ip_end, (unsigned char *)&inAddr, 4);
        
		pQos->bandwidth = (unsigned long)atoi(p6);

        	strcpy( pQos->entry_name, p7 );
		qosRuleNum++;        
		break;
#endif


#ifdef QOS_OF_TR069
        case QOS_CLASS_ARRAY_T:
		getVal80((char *)value, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10, &p11, &p12, &p13
		, &p14, &p15, &p16, &p17, &p18, &p19, &p20, &p21, &p22, &p23, &p24, &p25, &p26, &p27, &p28
		, &p29, &p30, &p31, &p32, &p33, &p34, &p35, &p36, &p37, &p38, &p39, &p40, &p41, &p42, &p43
		, &p44, &p45, &p46, &p47, &p48, &p49, &p50, &p51, &p52, &p53, &p54, &p55, &p56, &p57, &p58
		, &p59, &p60, &p61, &p62, &p63, &p64, &p65, &p66, &p67, &p68, &p69, &p70, &p71, &p72, &p73
		, &p74, &p75, &p76, &p77, &p78, &p79, &p80);
		if (p1 == NULL || p2 == NULL || p3 == NULL || p4 == NULL || p5 == NULL || p6 == NULL || p7 == NULL || p8 == NULL\
		||p9 == NULL || p10 == NULL || p11 == NULL || p12 == NULL || p13 == NULL || p14 == NULL || p15 == NULL\
		|| p16 == NULL || p17 == NULL || p18 == NULL || p19 == NULL || p20 == NULL || p21 == NULL || p22 == NULL\
		|| p23 == NULL || p24 == NULL || p25 == NULL || p26 == NULL || p27 == NULL || p28 == NULL || p29 == NULL\
		|| p30 == NULL || p31 == NULL || p32 == NULL || p33 == NULL || p34 == NULL || p35 == NULL || p36 == NULL\
		|| p37 == NULL || p38 == NULL || p39 == NULL || p40 == NULL || p41 == NULL || p42 == NULL || p43 == NULL\
		|| p44 == NULL || p45 == NULL || p46 == NULL || p47 == NULL || p48 == NULL || p49 == NULL || p50 == NULL\
		|| p51 == NULL || p52 == NULL || p53 == NULL || p54 == NULL || p55 == NULL || p56 == NULL || p57 == NULL\
		|| p58 == NULL || p59 == NULL || p60 == NULL || p61 == NULL || p62 == NULL || p63 == NULL || p64 == NULL\
		|| p65 == NULL || p66 == NULL || p67 == NULL || p68 == NULL || p69 == NULL || p70 == NULL || p71 == NULL\
		|| p72 == NULL || p73 == NULL || p74 == NULL || p75 == NULL || p76 == NULL || p77 == NULL || p78 == NULL \
		|| p79 == NULL || p80 == NULL)
		{
			printf("Invalid QoS Classification arguments!\n");
			break;
		} 

		pQosClass = (QOSCLASS_Tp)(((long)pMib)+mib_table[i].offset+QosClassNum*sizeof(QOSCLASS_T));
        pQosClass->ClassInstanceNum = (unsigned int)atoi(p1);
             pQosClass->ClassificationKey = (unsigned short)atoi(p2);
             pQosClass->ClassificationEnable = (unsigned char)atoi(p3);
             strcpy(pQosClass->ClassificationStatus, p4);
             strcpy(pQosClass->Alias, p5);
             pQosClass ->ClassificationOrder = (unsigned int)atoi(p6);
             strcpy( pQosClass->ClassInterface, p7);
             if ( !inet_aton(p8, &inAddr) )
                 return -1;
             memcpy(pQosClass->DestIP, (unsigned char *)&inAddr, 4);
             if ( !inet_aton(p9, &inAddr) )
                 return -1;
             memcpy(pQosClass->DestMask, (unsigned char *)&inAddr, 4);
             pQosClass->DestIPExclude = (unsigned char)atoi(p10) ;
             if ( !inet_aton(p11, &inAddr) )
                 return -1;
             memcpy(pQosClass->SourceIP, (unsigned char *)&inAddr, 4);
             if ( !inet_aton(p12, &inAddr) )
                 return -1;
             memcpy(pQosClass->SourceMask, (unsigned char *)&inAddr, 4);
             pQosClass->SourceIPExclude = (unsigned char)atoi(p13);
             pQosClass->Protocol = atoi(p14) ;
             pQosClass->ProtocolExclude = (unsigned char)atoi(p15);
             pQosClass->DestPort = atoi(p16);
             pQosClass->DestPortRangeMax = atoi(p17);
             pQosClass->DestPortExclude = (unsigned char)atoi(p18);
             pQosClass->SourcePort = atoi(p19);
             pQosClass->SourcePortRangeMax = atoi(p20);
             pQosClass->SourcePortExclude = (unsigned char)atoi(p21);
             if (strlen(p22)!=12 || !string_to_hex(p22, pQosClass->SourceMACAddress, 12)) 
                 return -1;
             memset(pQosClass->SourceMACAddress, 0, MAC_ADDR_LEN);
             strncpy( pQosClass->SourceMACAddress, p22, MAC_ADDR_LEN);
             
             if (strlen(p23)!=12 || !string_to_hex(p23, pQosClass->SourceMACMask, 12)) 
                 return -1;
             memset(pQosClass->SourceMACMask, 0, MAC_ADDR_LEN);
             strncpy( pQosClass->SourceMACMask, p23, MAC_ADDR_LEN);
        
             pQosClass->SourceMACExclude = (unsigned char)atoi(p24);
         
             if (strlen(p25)!=12 || !string_to_hex(p25, pQosClass->DestMACAddress, 12)) 
                 return -1;
             memset(pQosClass->DestMACAddress, 0, MAC_ADDR_LEN);
             strncpy( pQosClass->DestMACAddress, p25, MAC_ADDR_LEN);
        
             if (strlen(p26)!=12 || !string_to_hex(p26, pQosClass->DestMACMask, 12)) 
                 return -1;
             memset(pQosClass->DestMACMask, 0, MAC_ADDR_LEN);
             strncpy( pQosClass->DestMACMask, p26, MAC_ADDR_LEN);
             pQosClass->DestMACExclude = (unsigned char)atoi(p27);
        
             pQosClass->Ethertype = atoi(p28);
             pQosClass->EthertypeExclude = (unsigned char)atoi(p29);
             pQosClass->SSAP = atoi(p30);
             pQosClass->SSAPExclude = (unsigned char)atoi(p31);
             pQosClass->DSAP = atoi(p32);
             pQosClass->DSAPExclude = (unsigned char)atoi(p33);
             pQosClass->LLCControl = atoi(p34);
             pQosClass->LLCControlExclude = (unsigned char)atoi(p35);
             pQosClass->SNAPOUI = atoi(p36);
             pQosClass->SNAPOUIExclude = (unsigned char)atoi(p37);
        
             strcpy(pQosClass->SourceVendorClassID, p38);
             pQosClass->SourceVendorClassIDExclude = (unsigned char)atoi(p39);
             strcpy(pQosClass->SourceVendorClassIDMode, p40);
             
             strcpy(pQosClass->DestVendorClassID, p41);
             pQosClass->DestVendorClassIDExclude = (unsigned char)atoi(p42);
             strcpy(pQosClass->DestVendorClassIDMode, p43);
        
             strcpy(pQosClass->SourceClientID, p44);
             pQosClass->SourceClientIDExclude = (unsigned char)atoi(p45);
             strcpy(pQosClass->DestClientID, p46);
             pQosClass->DestClientIDExclude = (unsigned char)atoi(p47);
        
             strcpy(pQosClass->SourceUserClassID, p48);
             pQosClass->SourceUserClassIDExclude = (unsigned char)atoi(p49);
             strcpy(pQosClass->DestUserClassID, p50);
             pQosClass->DestUserClassIDExclude = (unsigned char)atoi(p51);
        
             strcpy(pQosClass->SourceVendorSpecificInfo, p52);
             pQosClass->SourceVendorSpecificInfoExclude = (unsigned char)atoi(p53);
             pQosClass->SourceVendorSpecificInfoEnterprise = (unsigned int)atoi(p54);
             pQosClass->SourceVendorSpecificInfoSubOption = (short int)atoi(p55);
             strcpy(pQosClass->SourceVendorSpecificInfoMode, p56);
             strcpy(pQosClass->DestVendorSpecificInfo, p57);
             pQosClass->DestVendorSpecificInfoExclude = (unsigned char)atoi(p58);
             pQosClass->DestVendorSpecificInfoEnterprise = (unsigned int)atoi(p59);
             pQosClass->DestVendorSpecificInfoSubOption = (short int)atoi(p60);
             strcpy(pQosClass->DestVendorSpecificInfoMode, p61);
        
             pQosClass->TCPACK = (unsigned char)atoi(p62);
             pQosClass->TCPACKExclude = (unsigned char)atoi(p63);
             pQosClass->IPLengthMin = atoi(p64);
             pQosClass->IPLengthMax = atoi(p65);
             pQosClass->IPLengthExclude = (unsigned char)atoi(p66);
             pQosClass->DSCPCheck = atoi(p67);
             pQosClass->DSCPExclude = (unsigned char)atoi(p68);
             pQosClass->DSCPMark = atoi(p69);
             pQosClass->EthernetPriorityCheck = atoi(p70);
             pQosClass->EthernetPriorityExclude = (unsigned char)atoi(p71);
             pQosClass->EthernetPriorityMark = atoi(p72);
             pQosClass->VLANIDCheck = atoi(p73);
             pQosClass->VLANIDExclude = (unsigned char)atoi(p74);
             pQosClass->OutOfBandInfo = atoi(p75);
             pQosClass->ForwardingPolicy = (unsigned int)atoi(p76);
             pQosClass->TrafficClass = atoi(p77);
             pQosClass->ClassPolicer = atoi(p78);
             pQosClass->ClassQueue = atoi(p70);
             pQosClass->ClassApp = atoi(p80);
             
             QosClassNum++;
             break;

        case QOS_QUEUE_ARRAY_T:
		 getVal24((char *)value, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10, &p11, &p12, &p13
		, &p14, &p15, &p16, &p17, &p18, &p19, &p20, &p21 , &p22, &p23, &p24);
		if (p1 == NULL || p2 == NULL || p3 == NULL || p4 == NULL || p5 == NULL || p6 == NULL || p7 == NULL ||\
		p9 == NULL || p10 == NULL || p11 == NULL || p12 == NULL || p13 == NULL || p14 == NULL || p15 == NULL )
		{
			printf("Invalid QoS Queue arguments!\n");
			break;
		}        
	 pQosQueue = (QOSQUEUE_Tp)(((long)pMib)+mib_table[i].offset+QosQueueNum*sizeof(QOSQUEUE_T));
        pQosQueue->QueueInstanceNum = (unsigned int)atoi(p1);
	pQosQueue->QueueKey = (unsigned int)atoi(p2);	
        pQosQueue->QueueEnable = (unsigned char)atoi(p3);
        //strcpy(pQosQueue->QueueStatus, p3);
        strcpy(pQosQueue->Alias, p4);
        strcpy(pQosQueue->TrafficClasses, p5);
        strcpy( pQosQueue->QueueInterface, p6);
	pQosQueue->QueueBufferLength = (unsigned int)atoi(p7);	
        pQosQueue->QueueWeight = (unsigned int)atoi(p8);
        pQosQueue->QueuePrecedence = (unsigned int)atoi(p9);
	 pQosQueue->REDThreshold = (unsigned int)atoi(p10);
	pQosQueue->REDPercentage = (unsigned int)atoi(p11);	 
	strcpy( pQosQueue->DropAlgorithm, p12);
        strcpy( pQosQueue->SchedulerAlgorithm, p13);
        pQosQueue->ShapingRate = atoi(p14);
        pQosQueue->ShapingBurstSize = (unsigned int)atoi(p15);
 
		QosQueueNum++;
		break;	

        case TR098_APPCONF_ARRAY_T:

		getVal14((char *)value, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10, &p11, &p12,&p13,&p14);
		if (p1 == NULL || p2 == NULL || p3 == NULL || p4 == NULL || p5 == NULL || p6 == NULL || p7 == NULL ||
		    p8 == NULL || p9 == NULL || p10 == NULL || p11 == NULL || p12 == NULL|| p13 == NULL ) {
			printf("Invalid QoS app config arguments!\n");
			break;
		}
		pTr098QosApp = (TR098_APPCONF_Tp)(((long)pMib)+pTbl[i].offset+tr098_appconf_num*sizeof(TR098_APPCONF_T));

        pTr098QosApp->app_key = (unsigned short)atoi(p1);
        pTr098QosApp->app_enable = (unsigned char)atoi(p2);
        strcpy(pTr098QosApp->app_status, p3);
        strcpy(pTr098QosApp->app_alias, p4);
        strcpy(pTr098QosApp->protocol_identify, p5);
        strcpy(pTr098QosApp->app_name, p6);
        pTr098QosApp->default_policy = (unsigned short)atoi(p7);
        pTr098QosApp->default_class = (short)atoi(p8);
        pTr098QosApp->default_policer = (short)atoi(p9);
        pTr098QosApp->default_queue = (short)atoi(p10);
        pTr098QosApp->default_decp_mark = (short)atoi(p11);
        pTr098QosApp->default_8021p_mark = (short)atoi(p12);
        pTr098QosApp->instanceNum = atoi(p13);

		tr098_appconf_num++;
		break;
    case TR098_FLOWCONF_ARRAY_T:

		getVal24((char *)value, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10, &p11, &p12,&p13,&p14
		, &p15, &p16, &p17, &p18, &p19, &p20, &p21 , &p22, &p23, &p24);
		if (p1 == NULL || p2 == NULL || p3 == NULL || p4 == NULL || p5 == NULL || p6 == NULL || p7 == NULL ||
		    p8 == NULL || p9 == NULL || p10 == NULL || p11 == NULL || p12 == NULL|| p13 == NULL || p14 == NULL  || p15 == NULL ) {
			printf("Invalid QoS flow config arguments!\n");
			break;
		}
		pTr098QosFlow = (TR098_FLOWCONF_Tp)(((long)pMib)+pTbl[i].offset+tr098_flowconf_num*sizeof(TR098_FLOWCONF_T));

		pTr098QosFlow->flow_key= (unsigned short)atoi(p1);
		pTr098QosFlow->flow_enable = (unsigned char)atoi(p2);
		strcpy( pTr098QosFlow->flow_status, p3 );
		strcpy( pTr098QosFlow->flow_alias, p4 );
		strcpy( pTr098QosFlow->flow_type, p5 );
		strcpy( pTr098QosFlow->flow_type_para, p6 );
		strcpy( pTr098QosFlow->flow_name, p7 );
		strcpy( pTr098QosFlow->app_identify, p8 );
	
		pTr098QosFlow->qos_policy =(unsigned short)atoi(p9);
		pTr098QosFlow->flow_class =(short)atoi(p10);
		pTr098QosFlow->flow_policer =(short)atoi(p11);
		pTr098QosFlow->flow_queue =(short)atoi(p12);
		pTr098QosFlow->flow_dscp_mark = (short)atoi(p13);
		pTr098QosFlow->flow_8021p_mark = (short)atoi(p14);
		pTr098QosFlow->instanceNum = atoi(p15);
		tr098_flowconf_num++;
		break;	
	//mark_qos
	 case QOS_POLICER_ARRAY_T: // Policer have 13 parameter
	    getVal24((char *)value, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10, &p11, &p12, &p13
		, &p14, &p15, &p16, &p17, &p18, &p19, &p20, &p21 , &p22, &p23, &p24);
		if (p1 == NULL || p2 == NULL || p3 == NULL || p4 == NULL || p5 == NULL || p6 == NULL || p7 == NULL ||\
		p9 == NULL || p10 == NULL || p11 == NULL || p12 == NULL || p13 == NULL || p14 == NULL )
		{
			printf("Invalid QoS Policer arguments!\n");
			break;
		}
	    pQosPolicer = (QOSPOLICER_Tp)(((long)pMib)+mib_table[i].offset+QosPolicerNum*sizeof(QOSPOLICER_T));

           pQosPolicer->InstanceNum = (unsigned int)atoi(p1);
	   pQosPolicer->PolicerKey = (unsigned int)atoi(p2);
	   pQosPolicer->PolicerEnable = (unsigned int)atoi(p3);
          strcpy( pQosPolicer->Alias, p4);
            pQosPolicer->CommittedRate = (unsigned int)atoi(p5);
		pQosPolicer->CommittedBurstSize = (unsigned int)atoi(p6);
		pQosPolicer->ExcessBurstSize = (unsigned int)atoi(p7);
		pQosPolicer->PeakRate = (unsigned int)atoi(p8);
		pQosPolicer->PeakBurstSize = (unsigned int)atoi(p9);

		strcpy( pQosPolicer->MeterType, p10);
		strcpy( pQosPolicer->PossibleMeterTypes, p11);
		strcpy( pQosPolicer->ConformingAction, p12);
		strcpy( pQosPolicer->PartialConformingAction, p13);
		strcpy( pQosPolicer->NonConformingAction, p14);
     
		QosPolicerNum++;
   	  break;	

	   case QOS_QUEUESTATS_ARRAY_T:
		getVal6((char *)value, &p1, &p2, &p3, &p4, &p5, &p6);
		if (p1 == NULL || p2 == NULL || p3 == NULL || p4 == NULL || p5 == NULL )
		{
			printf("Invalid QoS Queue arguments!\n");
			break;
		}
	  pQosQueueStats = (QOSQUEUESTATS_Tp)(((long)pMib)+mib_table[i].offset+QosQueueStatsNum*sizeof(QOSQUEUESTATS_T));
            pQosQueueStats->InstanceNum = (unsigned int)atoi(p1);
  	   pQosQueueStats->Enable = (unsigned int)atoi(p2);
 	   strcpy( pQosQueueStats->Alias, p3);
	  pQosQueueStats->Queue = (unsigned int)atoi(p4);
	   strcpy( pQosQueueStats->Interface, p5);
		QosQueueStatsNum++;
		break;        
#endif
#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
	case DHCP_SERVER_OPTION_ARRAY_T:
		getVal9((char *)value, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9);
		if (p1 == NULL || p2 == NULL || p3 == NULL || p4 == NULL 
			|| p5 == NULL || p6 == NULL || p7 == NULL || p8 == NULL || p9 == NULL) {
			printf("Invalid DHCP_SERVER_OPTION arguments!\n");
			break;
		}
		pDhcpdOpt = (MIB_CE_DHCP_OPTION_Tp)(((long)pMib)+mib_table[i].offset+dhcpdOptNum*sizeof(MIB_CE_DHCP_OPTION_T));
		pDhcpdOpt->enable  = (unsigned short)atoi(p1);
		pDhcpdOpt->usedFor = (unsigned short)atoi(p2);
		pDhcpdOpt->order   = (unsigned short)atoi(p3);
		pDhcpdOpt->tag     = (unsigned short)atoi(p4);
		pDhcpdOpt->len     = (unsigned short)atoi(p5);
		memcpy(pDhcpdOpt->value, p6, DHCP_OPT_VAL_LEN);
		pDhcpdOpt->ifIndex = (unsigned short)atoi(p7);
		pDhcpdOpt->dhcpOptInstNum   = (unsigned short)atoi(p8);
		pDhcpdOpt->dhcpConSPInstNum = (unsigned short)atoi(p9);
		dhcpdOptNum++;
		break;

	case DHCP_CLIENT_OPTION_ARRAY_T:
		getVal9((char *)value, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9);
		if (p1 == NULL || p2 == NULL || p3 == NULL || p4 == NULL 
			|| p5 == NULL || p6 == NULL || p7 == NULL || p8 == NULL || p9 == NULL) {
			printf("Invalid DHCP_CLIENT_OPTION arguments!\n");
			break;
		}
		pDhcpcOpt = (MIB_CE_DHCP_OPTION_Tp)(((long)pMib)+mib_table[i].offset+dhcpcOptNum*sizeof(MIB_CE_DHCP_OPTION_T));
		pDhcpcOpt->enable  = (unsigned short)atoi(p1);
		pDhcpcOpt->usedFor = (unsigned short)atoi(p2);
		pDhcpcOpt->order   = (unsigned short)atoi(p3);
		pDhcpcOpt->tag     = (unsigned short)atoi(p4);
		pDhcpcOpt->len     = (unsigned short)atoi(p5);
		memcpy(pDhcpcOpt->value, p6, DHCP_OPT_VAL_LEN);
		pDhcpcOpt->ifIndex = (unsigned short)atoi(p7);
		pDhcpcOpt->dhcpOptInstNum   = (unsigned short)atoi(p8);
		pDhcpcOpt->dhcpConSPInstNum = (unsigned short)atoi(p9);
		dhcpcOptNum++;
		break;

	case DHCPS_SERVING_POOL_ARRAY_T:
		getVal29((char *)value, 
			&p1, &p2, &p3, &p4, &p5,
			&p6, &p7, &p8, &p9, &p10,
			&p11, &p12, &p13, &p14, &p15,
			&p16, &p17, &p18, &p19, &p20,
			&p21, &p22, &p23, &p24, &p25,
			&p26, &p27, &p28, &p29);
		if (p1 == NULL || p2 == NULL || p3 == NULL || p4 == NULL  || p5 == NULL || 
			p6 == NULL || p7 == NULL || p8 == NULL || p9 == NULL  || p10 == NULL || 
			p11 == NULL || p12 == NULL || p13 == NULL || p14 == NULL  || p15 == NULL || 
			p16 == NULL || p17 == NULL || p18 == NULL || p19 == NULL  || p20 == NULL || 
			p21 == NULL || p22 == NULL || p23 == NULL || p24 == NULL  || p25 == NULL || 
			p26 == NULL || p27 == NULL || p28 == NULL || p29 == NULL) {
			printf("Invalid DHCPS_SERVING_POOL arguments!\n");
			break;
		}
		pServingPool = (DHCPS_SERVING_POOL_Tp)(((long)pMib)+mib_table[i].offset+servingPoolNum*sizeof(DHCPS_SERVING_POOL_T));
		pServingPool->enable	 = (unsigned short)atoi(p1);
		pServingPool->poolorder  = (unsigned int)atoi(p2);
		memcpy(pServingPool->poolname, p3, MAX_NAME_LEN);
		pServingPool->deviceType = (unsigned short)atoi(p4);
		pServingPool->rsvOptCode = (unsigned short)atoi(p5);
		pServingPool->sourceinterface = (unsigned short)atoi(p6);
		memcpy(pServingPool->vendorclass, p7, OPTION_60_LEN+1);
		pServingPool->vendorclassflag = (unsigned short)atoi(p8);
		memcpy(pServingPool->vendorclassmode, p9, MODE_LEN);
		memcpy(pServingPool->clientid, p10, OPTION_LEN);
		pServingPool->clientidflag= (unsigned short)atoi(p11);
		memcpy(pServingPool->userclass, p12, LEN);
		pServingPool->userclassflag = (unsigned short)atoi(p13);
		memcpy(pServingPool->chaddr, p14, MAC_ADDR_LEN);
		memcpy(pServingPool->chaddrmask, p15, MAC_ADDR_LEN);
		pServingPool->chaddrflag  = (unsigned short)atoi(p16);
		pServingPool->localserved = (unsigned short)atoi(p17);
		memcpy(pServingPool->startaddr, p18, IP_ADDR_LEN);
		memcpy(pServingPool->endaddr, p19, IP_ADDR_LEN);
		memcpy(pServingPool->subnetmask, p20, IP_ADDR_LEN);
		memcpy(pServingPool->iprouter, p21, IP_ADDR_LEN);
		memcpy(pServingPool->dnsserver1, p22, IP_ADDR_LEN);
		memcpy(pServingPool->dnsserver2, p23, IP_ADDR_LEN);
		memcpy(pServingPool->dnsserver3, p24, IP_ADDR_LEN);
		memcpy(pServingPool->domainname, p25, GENERAL_LEN);
		pServingPool->leasetime 	= (unsigned int)atoi(p26);
		memcpy(pServingPool->dhcprelayip, p27, IP_ADDR_LEN);
		pServingPool->dnsservermode = (unsigned short)atoi(p28);
		pServingPool->InstanceNum	= (unsigned int)atoi(p29);
		servingPoolNum++;
		break;
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */

#ifdef ROUTE_SUPPORT
	case STATICROUTE_ARRAY_T:
		getVal5((char *)value, &p1, &p2, &p3, &p4, &p5);
		if (p1 == NULL || p2 == NULL || p3 == NULL) {
			printf("Invalid PORTFILTER arguments!\n");
			break;
		}
		if ( !inet_aton(p1, &inAddr) )
			return -1;
		pStaticRoute = (STATICROUTE_Tp)(((long)pMib)+mib_table[i].offset+staticRouteNum*sizeof(STATICROUTE_T));
		if( !inet_aton(p1, &pStaticRoute->destAddr))
			return -1 ;
		if( !inet_aton(p2, &pStaticRoute->netmask))
			return -1 ;
		if( !inet_aton(p3, &pStaticRoute->gateway))
			return -1 ;
		pStaticRoute->interface=(unsigned char)atoi(p4);
		pStaticRoute->metric=(unsigned char)atoi(p5);
			
		staticRouteNum++;
		break;
#endif // ROUTE_SUPPORT

#ifdef CONFIG_RTL_AIRTIME
	case AIRTIME_ARRAY_T:
		getVal4((char *)value, &p1, &p2, &p3, &p4);
		if (p1 == NULL || p2 == NULL || p3 == NULL) {
			printf("Invalid AIRTIME arguments!\n");
			break;
		}
		if ( !inet_aton(p1, &inAddr) )
			return -1;
		pAirTime = (AIRTIME_Tp)(((long)pMib)+mib_table[i].offset+airTimeNum*sizeof(AIRTIME_T));
		memcpy(pAirTime->ipAddr, (unsigned char *)&inAddr, 4);

		if (strlen(p2)!=12 || !string_to_hex(p2, key, 12))
			return -1;
		memcpy(pAirTime->macAddr, key, 6);

		pAirTime->atm_time = (unsigned char)atoi(p3);
		if ( p4 )
			strcpy( pAirTime->comment, p4 );
		airTimeNum++;
		break;
#endif /* CONFIG_RTL_AIRTIME */

#endif
#ifdef TLS_CLIENT
	case CERTROOT_ARRAY_T:
		getVal1((char *)value, &p1);
		if (p1 == NULL ) {
			printf("Invalid CERTCA arguments!\n");
			break;
		}
		pCertRoot = (CERTROOT_Tp)(((long)pMib)+mib_table[i].offset+certRootNum*sizeof(CERTROOT_T));
		strcpy( pCertRoot->comment, p1 );
		certRootNum++;
		break;
	case CERTUSER_ARRAY_T:
		getVal2((char *)value,&p1, &p2);
		if (p1 == NULL || p2 = NULL) {
			printf("Invalid CERTPR arguments!\n");
			break;
		}
		pCertUser = (CERTUSER_Tp)(((long)pMib)+mib_table[i].offset+certUserNum*sizeof(CERTUSER_T));
		strcpy( pCertUser->pass, p1 );
		strcpy( pCertUser->comment, p2 );
		certUserNum++;
		break;		
#endif
#ifdef TR181_SUPPORT
#ifdef CONFIG_IPV6
	case DHCPV6C_SENDOPT_ARRAY_T:
		getVal4((char *)value, &p1, &p2, &p3,&p4);
		if (p1 == NULL || p2 == NULL || p3 == NULL||p4==NULL) {
			printf("Invalid ipv6DhcpcSendOpt in argument!\n");
			break;
		}	
		
		pDhcp6cSendOpt= (DHCPV6C_SENDOPT_T)(((long)pMib)+mib_table[i].offset+ipv6DhcpcSendOptNum*sizeof(DHCPV6C_SENDOPT_T));
		pDhcp6cSendOpt->index=(unsigned char)atoi(p1);
		pDhcp6cSendOpt->enable=(unsigned char)atoi(p2);
		pDhcp6cSendOpt->tag=(unsigned char)atoi(p3);
		strcpy(pDhcp6cSendOpt->value,p4);
		ipv6DhcpcSendOptNum++;
		break;
#endif
	case DNS_CLIENT_SERVER_ARRAY_T:
		getVal7((char *)value,&p1 ,&p2, &p3, &p4, &p5, &p6, &p7);
		if (p1 == NULL || p2 == NULL || p3 == NULL||p4==NULL || p5 == NULL || p6 == NULL||p7==NULL) {
			printf("Invalid dnsClientServer in argument!\n");
			break;
		}
		pDnsClientServer= (DHCPV6C_SENDOPT_T)(((long)pMib)+mib_table[i].offset+dnsClientServerNum*sizeof(DNS_CLIENT_SERVER_T));
		pDnsClientServer->index=(unsigned char)atoi(p1);
		pDnsClientServer->enable=(unsigned char)atoi(p2);
		pDnsClientServer->status=(unsigned char)atoi(p3);
		strcpy(pDnsClientServer->alias,p4);
		strcpy(pDnsClientServer->ipAddr,p5);
		strcpy(pDnsClientServer->interface,p6);
		pDnsClientServer->type=(unsigned char)atoi(p7);
		dnsClientServerNum++;
		break;
#endif
	case DHCPRSVDIP_ARRY_T:
		getVal3((char *)value, &p1, &p2, &p3);
		if (p1 == NULL || p2 == NULL || p3 == NULL) {
			printf("Invalid DHCPRSVDIP in argument!\n");
			break;
		}	
		if (strlen(p2)!=12 || !string_to_hex(p2, key, 12))
			return -1;
		pDhcpRsvd= (DHCPRSVDIP_Tp)(((long)pMib)+mib_table[i].offset+dhcpRsvdIpNum*sizeof(DHCPRSVDIP_T));
		strcpy(pDhcpRsvd->hostName, p1);		
		memcpy(pDhcpRsvd->macAddr, key, 6);
		if( !inet_aton(p3, &pDhcpRsvd->ipAddr))
			return -1;
		dhcpRsvdIpNum++;
		break;
	case SCHEDULE_ARRAY_T:
	//may not used
		break;
#if defined(VLAN_CONFIG_SUPPORTED)		
	case VLANCONFIG_ARRAY_T:
	getVal6((char *)value, &p1, &p2, &p3, &p4, &p5, &p6);
		if (p1 == NULL || p2 == NULL || p3 == NULL || p4 == NULL || p5 == NULL || p6 == NULL ) {
			printf("Invalid VLAN Config arguments!\n");
			break;
		}	
		if (p2){
			pVlanConfig = (VLAN_CONFIG_Tp)(((long)pMib)+mib_table[i].offset);
			for(j=0;j<vlanConfigNum;j++){
			if(!strcmp((pVlanConfig+(j*sizeof(VLAN_CONFIG_T)))->netIface, p2){
				pVlanConfig =  (VLAN_CONFIG_Tp)(((long)pMib)+mib_table[i].offset+(j*sizeof(VLAN_CONFIG_T)));
				pVlanConfig->enabled = (unsigned char)atoi(p1);
				pVlanConfig->tagged = (unsigned char)atoi(p3);
				pVlanConfig->priority = (unsigned char)atoi(p4);
				pVlanConfig->cfi = (unsigned char)atoi(p5);
				pVlanConfig->vlanId = (unsigned short)atoi(p6);
	        	}
	        }
        	}
	break;
#endif	
#if defined(SAMBA_WEB_SUPPORT)		
	case STORAGE_USER_ARRAY_T:
	getVal2((char *)value, &p1, &p2);
		if (p1 == NULL || p2 == NULL  ) {
			printf("Invalid samba Config arguments!\n");
			break;
		}	
		pUserConfig = (STORAGE_USER_Tp)(((long)pMib)+mib_table[i].offset+sambaAccountConfigNum*sizeof(STORAGE_USER_T));
		strcpy(pUserConfig->storage_user_name, p1);		
		strcpy(pUserConfig->storage_user_password, p2);
		sambaAccountConfigNum++;
	break;
	case STORAGE_SHAREINFO_ARRAY_T:
	getVal4((char *)value, &p1, &p2, &p3, &p4);
		if (p1 == NULL || p2 == NULL || p3 == NULL || p4 == NULL) {
			printf("Invalid samba share info arguments!\n");
			break;
		}	
		pshareInfoConfig = (STORAGE_SHAREINFO_Tp)(((long)pMib)+mib_table[i].offset+sambaShareInfoNum*sizeof(STORAGE_SHAREINFO_T));
		strcpy(pshareInfoConfig->storage_sharefolder_name, p1);		
		strcpy(pshareInfoConfig->storage_sharefolder_path, p2);
		strcpy(pshareInfoConfig->storage_account, p3);
		pshareInfoConfig->storage_permission = atoi(p4);
		sambaShareInfoNum++;
	break;
#endif		
#if defined(CONFIG_RTL_ETH_802DOT1X_SUPPORT)		
	case ETHDOT1X_ARRAY_T:
	getVal2((char *)value, &p1, &p2);
		if (p1 == NULL || p2 == NULL  ) {
			printf("Invalid dot1x Config arguments!\n");
			break;
		}	
		if (p1)
		{
			pethdot1xConfig = (ETHDOT1X_Tp)(((long)pMib)+mib_table[i].offset);
			for(k=0;k<ethdot1xConfigNum;k++)
			{
				if((pethdot1xConfig+(k*sizeof(ETHDOT1X_T)))->portnum==(unsigned char)atoi(p1))
				{
					pethdot1xConfig =  (ETHDOT1X_Tp)(((long)pMib)+mib_table[i].offset+(k*sizeof(ETHDOT1X_T)));
					pethdot1xConfig->enabled = (unsigned char)atoi(p2);
				}
			}
		}
	break;
#endif		

	default:
		return -1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
static void getVal2(char *value, char **p1, char **p2)
{
	value = getVal(value, p1);
	if ( value )
		getVal(value, p2);
	else
		*p2 = NULL;
}

#ifdef HOME_GATEWAY
////////////////////////////////////////////////////////////////////////////////
static void getVal3(char *value, char **p1, char **p2, char **p3)
{
	*p1 = *p2 = *p3 = NULL;

	value = getVal(value, p1);
	if ( !value )
		return;
	value = getVal(value, p2);
	if ( !value )
		return;
	getVal(value, p3);
}

////////////////////////////////////////////////////////////////////////////////
static void getVal4(char *value, char **p1, char **p2, char **p3, char **p4)
{
	*p1 = *p2 = *p3 = *p4 = NULL;

	value = getVal(value, p1);
	if ( !value )
		return;
	value = getVal(value, p2);
	if ( !value )
		return;
	value = getVal(value, p3);
	if ( !value )
		return;
	getVal(value, p4);
}

////////////////////////////////////////////////////////////////////////////////
static void getVal5(char *value, char **p1, char **p2, char **p3, char **p4, char **p5)
{
	*p1 = *p2 = *p3 = *p4 = *p5 = NULL;

	value = getVal(value, p1);
	if ( !value )
		return;
	value = getVal(value, p2);
	if ( !value )
		return;
	value = getVal(value, p3);
	if ( !value )
		return;
	value = getVal(value, p4);
	if ( !value )
		return;
	getVal(value, p5);
}

#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
static void getVal9(char *value, char **p1, char **p2, char **p3, char **p4, 
	char **p5, char **p6, char **p7,char **p8,char **p9)
{
	*p1 = *p2 = *p3 = *p4 = *p5 = *p6 = *p7 = NULL;

	value = getVal(value, p1);
	if ( !value )
		return;
	value = getVal(value, p2);
	if ( !value )
		return;
	value = getVal(value, p3);
	if ( !value )
		return;
	value = getVal(value, p4);
	if ( !value )
		return;
	value = getVal(value, p5);
	if ( !value )
		return;
	value = getVal(value, p6);
	if ( !value )
		return;
	value = getVal(value, p7);
	if ( !value )
		return;
	value = getVal(value, p8);
	if ( !value )
		return;
	value = getVal(value, p9);
}

static void getVal29(char *value, char **p1, char **p2, char **p3, char **p4, char **p5, char **p6, char **p7,\
	char **p8, char **p9, char **p10, char **p11, char **p12, char **p13, char **p14, char **p15, char **p16,\
	char **p17, char **p18, char **p19, char **p20, char **p21, char **p22, char **p23, char **p24, char **p25,\
	char **p26, char **p27, char **p28, char **p29)
{
	*p1 = *p2 = *p3 = *p4 = *p5 = *p6 = *p7 = *p8 = *p9 = *p10 = *p11 = *p12\
	= *p13 = *p14 = *p15 = *p16 = *p17 = *p18 = *p19 = *p20 = *p21 = *p22 = *p23 = *p24  = *p25\
	=*p26=*p27=*p28=*p29=NULL;

	value = getVal(value, p1);
	if ( !value )
		return;
	value = getVal(value, p2);
	if ( !value )
		return;
	value = getVal(value, p3);
	if ( !value )
		return;
	value = getVal(value, p4);
	if ( !value )
		return;
	value = getVal(value, p5);
	if ( !value )
		return;
	value = getVal(value, p6);
	if ( !value )
		return;
	value = getVal(value, p7);
	if ( !value )
		return;
	value = getVal(value, p8);
	if ( !value )
		return;
	value = getVal(value, p9);
	if ( !value )
		return;
	value = getVal(value, p10);
	if ( !value )
		return;
	value = getVal(value, p11);
	if ( !value )
		return;
	value = getVal(value, p12);
	if ( !value )
		return;
	value = getVal(value, p13);
	if ( !value )
		return;
	value = getVal(value, p14);
	if ( !value )
		return;
	value = getVal(value, p15);
	if ( !value )
		return;
	value = getVal(value, p16);
	if ( !value )
		return;
	value = getVal(value, p17);
	if ( !value )
		return;
	value = getVal(value, p18);
	if ( !value )
		return;
	value = getVal(value, p19);
	if ( !value )
		return;
	value = getVal(value, p20);
	if ( !value )
		return;
	value = getVal(value, p21);
	if ( !value )
		return;
	value = getVal(value, p22);
	if ( !value )
		return;
	value = getVal(value, p23);
	if ( !value )
		return;
	value = getVal(value, p24);
	if ( !value )
		return;
	value = getVal(value, p25);
	if ( !value )
		return;
	value = getVal(value, p26);
	if ( !value )
		return;
	value = getVal(value, p27);
	if ( !value )
		return;
	value = getVal(value, p28);
	if ( !value )
		return;
	value = getVal(value, p29);
	if ( !value )
		return;
}
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */

#endif // HOME_GATEWAY

#endif // PARSE_TXT_FILE

////////////////////////////////////////////////////////////////////////////////
static int getdir(char *fullname, char *path, int loop)
{
	char tmpBuf[100], *p, *p1;

	strcpy(tmpBuf, fullname);
	path[0] = '\0';

	p1 = tmpBuf;
	while (1) {
		if ((p=strchr(p1, '/'))) {
			if (--loop == 0) {
				*p = '\0';
				strcpy(path, tmpBuf);
				return 0;
			}
			p1 = ++p;
		}
		else
			break;
	}
	return -1;
}

////////////////////////////////////////////////////////////////////////////////
static int read_flash_webpage(char *prefix, char *webfile)
{
	WEB_HEADER_T header;
	char *buf, tmpFile[100], tmpFile1[100], tmpBuf[100];
	int fh=0, i, loop, size;
	FILE_ENTRY_Tp pEntry;
	struct stat sbuf;
	char *file;
	if (webfile[0])
		file = webfile;
	else
		file = NULL;

	if (!file) {
#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE)
#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_WEB_BACKUP_ENABLE)
		if ( flash_read_webpage((char *)&header, WEB_PAGE_OFFSET, sizeof(header)) == 0) {
#else
		if ( flash_read_web((char *)&header, WEB_PAGE_OFFSET, sizeof(header)) == 0) {
#endif
#else
		if ( flash_read_web((char *)&header, WEB_PAGE_OFFSET, sizeof(header)) == 0) {
#endif
			printf("Read web header failed!\n");
			return -1;
		}
	}
	else {
		if ((fh = open(file, O_RDONLY)) < 0) {
			printf("Can't open file %s\n", file);
			return -1;
		}
		lseek(fh, 0L, SEEK_SET);
		if (read(fh, &header, sizeof(header)) != sizeof(header)) {
			printf("Read web header failed %s!\n", file);
			close(fh);
			return -1;
		}
	}
//#ifndef __mips__
	header.len = DWORD_SWAP(header.len);
//#endif

	if (memcmp(header.signature, WEB_HEADER, SIGNATURE_LEN)) {
		printf("Invalid web image! Expect %s\n",WEB_HEADER);
		return -1;
	}

// for debug
//printf("web size=%ld\n", header.len);
	buf = malloc(header.len);
	if (buf == NULL) {
		sprintf(tmpBuf, "Allocate buffer failed %d!\n", header.len);
		printf(tmpBuf);
		return -1;
	}

	if (!file) {
#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE)
#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_WEB_BACKUP_ENABLE)
		if ( flash_read_webpage(buf, WEB_PAGE_OFFSET+sizeof(header), header.len) == 0) {
#else
		if ( flash_read_web(buf, WEB_PAGE_OFFSET+sizeof(header), header.len) == 0) {
#endif
#else
		if ( flash_read_web(buf, WEB_PAGE_OFFSET+sizeof(header), header.len) == 0) {
#endif
			printf("Read web image failed!\n");
			free(buf);
			return -1;
		}
	}
	else {
		if (read(fh, buf, header.len) != header.len) {
			printf("Read web image failed!\n");
			free(buf);
			return -1;
		}
		close(fh);
	}

	if ( !CHECKSUM_OK((unsigned char *)buf, header.len) ) {
		printf("Web image invalid!\n");
		free(buf);
		return -1;
	}
// for debug
//printf("checksum ok!\n");

	// save to a file
	strcpy(tmpFile, "flashweb.bz2");
	fh = open(tmpFile, O_RDWR|O_CREAT|O_TRUNC);
	if (fh == -1) {
		printf("Create output file error %s!\n", tmpFile );
		free(buf);
		return -1;
	}
	if ( write(fh, buf, header.len-1) != header.len -1) {
		printf("write file error %s!\n", tmpFile);
		free(buf);
		close(fh);
		return -1;
	}
	close(fh);
	free(buf);
	sync();

	// decompress file
	sprintf(tmpFile1, "%sXXXXXX", tmpFile);
	mkstemp(tmpFile1);

	sprintf(tmpBuf, "bunzip2 -c %s > %s", tmpFile, tmpFile1);
	system(tmpBuf);

	unlink(tmpFile);
	sync();

	if (stat(tmpFile1, &sbuf) != 0) {
		printf("Stat file error %s!\n", tmpFile1);
		return -1;
	}
	if (sbuf.st_size < sizeof(FILE_ENTRY_T) ) {
		sprintf(tmpBuf, "Invalid decompress file size %ld!\n", sbuf.st_size);
		printf(tmpBuf);
		unlink(tmpFile1);
		return -1;
	}
// for debug
//printf("decompress size=%ld\n", sbuf.st_size);

	buf = malloc(sbuf.st_size);
	if (buf == NULL) {
		sprintf(tmpBuf,"Allocate buffer failed %ld!\n", sbuf.st_size);
		printf(tmpBuf);
		return -1;
	}
	if ((fh = open(tmpFile1, O_RDONLY)) < 0) {
		printf("Can't open file %s\n", tmpFile1);
		free(buf);
		return -1;
	}
	lseek(fh, 0L, SEEK_SET);
	if ( read(fh, buf, sbuf.st_size) != sbuf.st_size) {
		printf("Read file error %ld!\n", sbuf.st_size);
		free(buf);
		close(fh);
		return -1;
	}
	close(fh);
	unlink(tmpFile1);
	sync();
	size = sbuf.st_size;
	for (i=0; i<size; ) {
		pEntry = (FILE_ENTRY_Tp)&buf[i];

//#ifndef __mips__
		pEntry->size = DWORD_SWAP(pEntry->size);
//#endif

		strcpy(tmpFile, prefix);
		strcat(tmpFile, "/");
		strcat(tmpFile, pEntry->name);

		loop = 0;
		while (1) {
			if (getdir(tmpFile, tmpBuf, ++loop) < 0)
				break;
			if (tmpBuf[0] && stat(tmpBuf, &sbuf) < 0) { // not exist
 				if ( mkdir(tmpBuf, S_IREAD|S_IWRITE) < 0) {
					printf("Create directory %s failed!\n", tmpBuf);
					return -1;
				}
			}
		}
// for debug
//printf("write file %s, size=%ld\n", tmpFile, pEntry->size);

		fh = open(tmpFile, O_RDWR|O_CREAT|O_TRUNC);
		if (fh == -1) {
			printf("Create output file error %s!\n", tmpFile );
			free(buf);
			return -1;
		}
// for debug
//if ( (i+sizeof(FILE_ENTRY_T)+pEntry->size) > size ) {
//printf("error in size, %ld !\n", pEntry->size);
//}
		if ( write(fh, &buf[i+sizeof(FILE_ENTRY_T)], pEntry->size) != pEntry->size ) {
			printf("Write file error %s, len=%d!\n", tmpFile, pEntry->size);
			free(buf);
			close(fh);
			return -1;
		}
		close(fh);
		// always set execuatble for script file
//		chmod(tmpFile,  S_IXUSR);

		i += (pEntry->size + sizeof(FILE_ENTRY_T));
	}
	free(buf);
	return 0;
}

#ifdef VPN_SUPPORT
static int read_flash_rsa(char *outputFile)
{
	int fh;
	char *rsaBuf;

	if ( !apmib_init()) {
		printf("Initialize AP MIB failed!\n");
		return -1;
	}

	fh = open(outputFile,  O_RDWR|O_CREAT);
	if (fh == -1) {
		printf("Create WPA config file error!\n");
		return -1;
	}
	rsaBuf = malloc(sizeof(unsigned char) * MAX_RSA_FILE_LEN);
	apmib_get( MIB_IPSEC_RSA_FILE, (void *)rsaBuf);
	write(fh, rsaBuf, MAX_RSA_FILE_LEN);
	close(fh);
	free(rsaBuf);
	chmod(outputFile,  DEFFILEMODE);
	return 0;
}
#endif
#ifdef TLS_CLIENT
////////////////////////////////////////////////////////////////////////////////
static int read_flash_cert(char *prefix, char *certfile)
{
	CERT_HEADER_T header;
	char *buf, tmpFile[100], tmpFile1[100], tmpBuf[100];
	int fh=0, i, loop, size;
	FILE_ENTRY_Tp pEntry;
	struct stat sbuf;
	char *file;

	if (certfile[0])
		file = certfile;
	else
		file = NULL;

	if (!file) {
		if ( flash_read_web((char *)&header, CERT_PAGE_OFFSET, sizeof(header)) == 0) {
			printf("Read web header failed!\n");
			return -1;
		}
	}
	else {
		if ((fh = open(file, O_RDONLY)) < 0) {
			printf("Can't open file %s\n", file);
			return -1;
		}
		lseek(fh, 0L, SEEK_SET);
		if (read(fh, &header, sizeof(header)) != sizeof(header)) {
			printf("Read web header failed %s!\n", file);
			close(fh);
			return -1;
		}
	}
//#ifndef __mips__
	header.len = DWORD_SWAP(header.len);
//#endif
	
	if (memcmp(header.signature, CERT_HEADER, SIGNATURE_LEN)) {
		printf("Invalid cert image!\n");
		return -1;
	}

// for debug
//printf("web size=%ld\n", header.len);
	buf = malloc(header.len);
	if (buf == NULL) {
		sprintf(tmpBuf, "Allocate buffer failed %ld!\n", header.len);
		printf(tmpBuf);
		return -1;
	}

	if (!file) {
		if ( flash_read_web(buf, CERT_PAGE_OFFSET+sizeof(header), header.len) == 0) {
			printf("Read web image failed!\n");
			return -1;
		}
	}
	else {
		if (read(fh, buf, header.len) != header.len) {
			printf("Read web image failed!\n");
			return -1;
		}
		close(fh);
	}

	if ( !CHECKSUM_OK(buf, header.len) ) {
		printf("Web image invalid!\n");
		free(buf);
		return -1;
	}
// for debug
//printf("checksum ok!\n");

	// save to a file
	strcpy(tmpFile, "/tmp/cert.tmp");
	fh = open(tmpFile, O_RDWR|O_CREAT|O_TRUNC);
	if (fh == -1) {
		printf("Create output file error %s!\n", tmpFile );
		return -1;
	}
	if ( write(fh, buf, header.len-1) != header.len -1) {
		printf("write file error %s!\n", tmpFile);
		return -1;
	}
	close(fh);
	free(buf);
	sync();

	// decompress file
	sprintf(tmpFile1, "%sXXXXXX", tmpFile);
	mkstemp(tmpFile1);

	//sprintf(tmpBuf, "bunzip2 -c %s > %s", tmpFile, tmpFile1);
	sprintf(tmpBuf, "cat %s  > %s", tmpFile, tmpFile1);
	system(tmpBuf);

	unlink(tmpFile);
	sync();

	if (stat(tmpFile1, &sbuf) != 0) {
		printf("Stat file error %s!\n", tmpFile1);
		return -1;
	}
	if (sbuf.st_size < sizeof(FILE_ENTRY_T) ) {
		sprintf(tmpBuf, "Invalid decompress file size %ld!\n", sbuf.st_size);
		printf(tmpBuf);
		unlink(tmpFile1);
		return -1;
	}
// for debug
//printf("decompress size=%ld\n", sbuf.st_size);

	buf = malloc(sbuf.st_size);
	if (buf == NULL) {
		sprintf(tmpBuf,"Allocate buffer failed %ld!\n", sbuf.st_size);
		printf(tmpBuf);
		return -1;
	}
	if ((fh = open(tmpFile1, O_RDONLY)) < 0) {
		printf("Can't open file %s\n", tmpFile1);
		return -1;
	}
	lseek(fh, 0L, SEEK_SET);
	if ( read(fh, buf, sbuf.st_size) != sbuf.st_size) {
		printf("Read file error %ld!\n", sbuf.st_size);
		return -1;
	}
	close(fh);
	unlink(tmpFile1);
	sync();
	size = sbuf.st_size;
	for (i=0; i<size; ) {
		pEntry = (FILE_ENTRY_Tp)&buf[i];

//#ifndef __mips__
		pEntry->size = DWORD_SWAP(pEntry->size);
//#endif

		strcpy(tmpFile, prefix);
		strcat(tmpFile, "/");
		strcat(tmpFile, pEntry->name);
		if(!strcmp(pEntry->name , ""))
			break;
		//printf("name = %s\n", pEntry->name);	
		loop = 0;
		while (1) {
			if (getdir(tmpFile, tmpBuf, ++loop) < 0)
				break;
			if (tmpBuf[0] && stat(tmpBuf, &sbuf) < 0) { // not exist
 				if ( mkdir(tmpBuf, S_IREAD|S_IWRITE) < 0) {
					printf("Create directory %s failed!\n", tmpBuf);
					return -1;
				}
			}
		}
// for debug
//printf("write file %s, size=%ld\n", tmpFile, pEntry->size);

		fh = open(tmpFile, O_RDWR|O_CREAT|O_TRUNC);
		if (fh == -1) {
			printf("Create output file error %s!\n", tmpFile );
			return -1;
		}
// for debug
//if ( (i+sizeof(FILE_ENTRY_T)+pEntry->size) > size ) {
//printf("error in size, %ld !\n", pEntry->size);
//}

		if ( write(fh, &buf[i+sizeof(FILE_ENTRY_T)], pEntry->size) != pEntry->size ) {
			printf("Write file error %s, len=%ld!\n", tmpFile, pEntry->size);
			return -1;
		}
		close(fh);
		// always set execuatble for script file
//		chmod(tmpFile,  S_IXUSR);

		i += (pEntry->size + sizeof(FILE_ENTRY_T));
	}

	return 0;
}
#endif
////////////////////////////////////////////////////////////////////////////////
static void __inline__ WRITE_WPA_FILE(int fh, unsigned char *buf)
{
	if ( write(fh, buf, strlen((char *)buf)) != strlen((char *)buf) ) {
		printf("Write WPA config file error!\n");
		close(fh);
		exit(1);
	}
}
#ifdef CONFIG_RTL_ETH_802DOT1X_SUPPORT
static void generateEthDot1xConf(char *outputFile)
{
	int fh,intVal,enable1x, opmode;
	unsigned char  buf1[1024],buf2[1024];
	if ( !apmib_init()) {
		printf("Initialize AP MIB failed!\n");
		return;
	}


	fh = open(outputFile, O_RDWR|O_CREAT|O_TRUNC);
	if (fh == -1) {
		printf("Create WPA config file error!\n");
		return;
	}
	
	apmib_get(MIB_OP_MODE,(void *)&opmode);
	
	apmib_get( MIB_ELAN_ENABLE_1X, (void *)&enable1x);
	sprintf((char *)buf2, "enable1x = %d\n", enable1x);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_ELAN_MAC_AUTH_ENABLED, (void *)&intVal);
	sprintf((char *)buf2, "enableMacAuth = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);
	apmib_get( MIB_ELAN_RS_REAUTH_TO, (void *)&intVal);
	sprintf((char *)buf2, "rsReAuthTO = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);
	apmib_get( MIB_ELAN_RS_PORT, (void *)&intVal);
	sprintf((char *)buf2, "rsPort = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_ELAN_RS_IP, (void *)buf1);
	sprintf((char *)buf2, "rsIP = %s\n", inet_ntoa(*((struct in_addr *)buf1)));
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_ELAN_RS_PASSWORD, (void *)buf1);
	sprintf((char *)buf2, "rsPassword = \"%s\"\n", buf1);
	WRITE_WPA_FILE(fh, buf2);
	
#ifdef CONFIG_RTL_ETH_802DOT1X_CLIENT_MODE_SUPPORT
	apmib_get( MIB_ELAN_EAP_TYPE, (void *)&intVal);
	sprintf(buf2, "eapType = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_ELAN_EAP_INSIDE_TYPE, (void *)&intVal);
	sprintf(buf2, "eapInsideType = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);
	
	apmib_get( MIB_ELAN_EAP_PHASE2_TYPE, (void *)&intVal);
	sprintf(buf2, "eapPhase2Type = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);
	
	apmib_get( MIB_ELAN_PHASE2_EAP_METHOD, (void *)&intVal);
	sprintf(buf2, "phase2EapMethod = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);
	apmib_get( MIB_ELAN_EAP_USER_ID, (void *)buf1);
	sprintf(buf2, "eapUserId = \"%s\"\n", buf1);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_ELAN_RS_USER_NAME, (void *)buf1);
	sprintf(buf2, "rsUserName = \"%s\"\n", buf1);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_ELAN_RS_USER_PASSWD, (void *)buf1);
	sprintf(buf2, "rsUserPasswd = \"%s\"\n", buf1);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_ELAN_RS_USER_CERT_PASSWD, (void *)buf1);
	sprintf(buf2, "rsUserCertPasswd = \"%s\"\n", buf1);
	WRITE_WPA_FILE(fh, buf2);
#endif
	apmib_get( MIB_ELAN_RS_MAXRETRY, (void *)&intVal);
	sprintf((char *)buf2, "rsMaxReq = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_ELAN_RS_INTERVAL_TIME, (void *)&intVal);
	sprintf((char *)buf2, "rsAWhile = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);
	apmib_get( MIB_ELAN_ACCOUNT_RS_ENABLED, (void *)&intVal);
	sprintf((char *)buf2, "accountRsEnabled = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_ELAN_ACCOUNT_RS_PORT, (void *)&intVal);
	sprintf((char *)buf2, "accountRsPort = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_ELAN_ACCOUNT_RS_IP, (void *)buf1);
	sprintf((char *)buf2, "accountRsIP = %s\n", inet_ntoa(*((struct in_addr *)buf1)));
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_ELAN_ACCOUNT_RS_PASSWORD, (void *)buf1);
	sprintf((char *)buf2, "accountRsPassword = \"%s\"\n", buf1);
	WRITE_WPA_FILE(fh, buf2);
	apmib_get( MIB_ELAN_ACCOUNT_RS_UPDATE_ENABLED, (void *)&intVal);
	sprintf((char *)buf2, "accountRsUpdateEnabled = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_ELAN_ACCOUNT_RS_UPDATE_DELAY, (void *)&intVal);
	sprintf((char *)buf2, "accountRsUpdateTime = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_ELAN_ACCOUNT_RS_MAXRETRY, (void *)&intVal);
	sprintf((char *)buf2, "accountRsMaxReq = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_ELAN_ACCOUNT_RS_INTERVAL_TIME, (void *)&intVal);
	sprintf((char *)buf2, "accountRsAWhile = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_ELAN_DOT1X_MODE, (void *)&intVal);
	if ((enable1x & ETH_DOT1X_CLIENT_MODE_ENABLE_BIT)&&(intVal & ETH_DOT1X_CLIENT_MODE_BIT))
	{
		if ((opmode == BRIDGE_MODE) || (opmode == WISP_MODE))
		{
			intVal &= ~(ETH_DOT1X_CLIENT_MODE_BIT);
		}
	}	
	if (!(enable1x & ETH_DOT1X_PROXY_MODE_ENABLE_BIT))
	{		
		intVal &= ~(ETH_DOT1X_PROXY_MODE_BIT);
	}
	sprintf((char *)buf2, "ethDot1xMode = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_ELAN_DOT1X_PROXY_TYPE, (void *)&intVal);
	sprintf((char *)buf2, "ethDot1xProxyType = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_ELAN_DOT1X_CLIENT_MODE_PORT_MASK, (void *)&intVal);
	sprintf((char *)buf2, "ethDot1xClientModePortMask = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);
	
	apmib_get( MIB_ELAN_DOT1X_PROXY_MODE_PORT_MASK, (void *)&intVal);
	sprintf((char *)buf2, "ethDot1xProxyModePortMask = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);
	apmib_get( MIB_ELAN_EAPOL_UNICAST_ENABLED, (void *)&intVal);
	sprintf((char *)buf2, "ethDot1xEapolUnicastEnabled = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	close(fh);
}
#endif

#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT

typedef struct _OCTET_STRING {
    unsigned char *Octet;
    unsigned short Length;
} OCTET_STRING;

typedef enum _BssType {
    infrastructure = 1,
    independent = 2,
} BssType;

typedef	struct _IbssParms {
    unsigned short	atimWin;
} IbssParms;

typedef struct _BssDscr {
    unsigned char bdBssId[6];
    unsigned char bdSsIdBuf[32];
    OCTET_STRING  bdSsId;

#if defined(CONFIG_RTK_MESH) || defined(CONFIG_RTL_819X) 
	//by GANTOE for site survey 2008/12/26
	unsigned char bdMeshIdBuf[32]; 
	OCTET_STRING bdMeshId; 
#endif 
    BssType bdType;
    unsigned short bdBcnPer;			// beacon period in Time Units
    unsigned char bdDtimPer;			// DTIM period in beacon periods
    unsigned long bdTstamp[2];			// 8 Octets from ProbeRsp/Beacon
    IbssParms bdIbssParms;			// empty if infrastructure BSS
    unsigned short bdCap;				// capability information
    unsigned char ChannelNumber;			// channel number
    unsigned long bdBrates;
    unsigned long bdSupportRates;		
    unsigned char bdsa[6];			// SA address
    unsigned char rssi, sq;			// RSSI and signal strength
    unsigned char network;			// 1: 11B, 2: 11G, 4:11G
	// P2P_SUPPORT
	unsigned char	p2pdevname[33];		
	unsigned char	p2prole;	
	unsigned short	p2pwscconfig;		
	unsigned char	p2paddress[6];	
	unsigned char	stage;	    
} BssDscr, *pBssDscr;

typedef struct _sitesurvey_status {
    unsigned char number;
    unsigned char pad[3];
    BssDscr bssdb[64];
} SS_STATUS_T, *SS_STATUS_Tp;


static inline int iw_get_ext(int                  skfd,           /* Socket to the kernel */
           char *               ifname,         /* Device name */
           int                  request,        /* WE ID */
           struct iwreq *       pwrq)           /* Fixed part of the request */
{
  /* Set device name */
  strncpy(pwrq->ifr_name, ifname, IFNAMSIZ);
  /* Do the request */
  return(ioctl(skfd, request, pwrq));
}

int getWlSiteSurveyRequest(char *interface, int *pStatus)
{
#ifndef NO_ACTION
    int skfd=0;
    struct iwreq wrq;
    unsigned char result;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;

    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0){
      /* If no wireless name : no wireless extensions */
      close( skfd );
        return -1;
	}
    wrq.u.data.pointer = (caddr_t)&result;
    wrq.u.data.length = sizeof(result);

    if (iw_get_ext(skfd, interface, SIOCGIWRTLSCANREQ, &wrq) < 0){
    	//close( skfd );
		//return -1;
	}
    close( skfd );

    if ( result == 0xff )
    	*pStatus = -1;
    else
	*pStatus = (int) result;
#else
	*pStatus = -1;
#endif
#ifdef CONFIG_RTK_MESH 
	// ==== modified by GANTOE for site survey 2008/12/26 ==== 
	return (int)*(char*)wrq.u.data.pointer; 
#else
	return 0;
#endif
}

int getWlSiteSurveyResult(char *interface, SS_STATUS_Tp pStatus )
{
#ifndef NO_ACTION
    int skfd=0;
    struct iwreq wrq;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;
    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0){
      /* If no wireless name : no wireless extensions */
      close( skfd );
        return -1;
	}
    wrq.u.data.pointer = (caddr_t)pStatus;

    if ( pStatus->number == 0 )
    	wrq.u.data.length = sizeof(SS_STATUS_T);
    else
        wrq.u.data.length = sizeof(pStatus->number);

    if (iw_get_ext(skfd, interface, SIOCGIWRTLGETBSSDB, &wrq) < 0){
    	close( skfd );
	return -1;
	}
    close( skfd );
#else
	return -1 ;
#endif

    return 0;
}


int  getEncryptInfoAccordingAP(int *encryption, int *unicastCipher, int *wpa2UnicastCipher)
{
	int wait_time, sts, i;
	wait_time = 0;
	unsigned char res;
	int wpa_exist = 0, idx = 0;

	char ssidBuf[64], tmp_ssidBuf[64], tmp2Buf[20];
	
	BssDscr *pBss=NULL;
	
	SS_STATUS_Tp pStatus=NULL;

	pStatus = calloc(1, sizeof(SS_STATUS_T));
	if ( pStatus == NULL ) 
	{
		printf("Allocate buffer failed!\n");
		return 0;
	}
	
	pStatus->number = 0;

	char root_wlan_ifname[16]={0};

	strncpy(root_wlan_ifname, wlan_1x_ifname, 5);
	
	//if ( getWlSiteSurveyResult(wlan_1x_ifname, pStatus) < 0  || pStatus->number==0) 
	if ( getWlSiteSurveyResult(root_wlan_ifname, pStatus) < 0  || pStatus->number==0)
	{
		//printf("%s:%d first call getWlSiteSurveyResult() fail!\n",__FUNCTION__,__LINE__);
		while (1)
		{		
			//switch(getWlSiteSurveyRequest(wlan_1x_ifname, &sts)) 
			switch(getWlSiteSurveyRequest(root_wlan_ifname, &sts))
			{ 
				case -2: 
					printf("Auto scan running!!\n"); 				 			
					break; 
				case -1: 
					printf("Site-survey request failed!\n"); 				
					break; 
				default: 
					break; 
			} 	
			if (sts != 0) 
			{	// not ready
				if (wait_time++ > 15) 
				{
					//enlarge wait time for root can't scan when vxd is do site survey. wait until vxd finish scan
					printf("scan request timeout!\n");		
					goto ss_err;
				}
#ifdef	CONFIG_RTK_MESH
				// ==== modified by GANTOE for site survey 2008/12/26 ==== 
				usleep(1000000 + (rand() % 2000000));
#else
				sleep(1);
#endif
			}
			else
				break;
		}	

		wait_time = 0;
		
		while (1) 
		{
			res = 1;	// only request request status
			//if ( getWlSiteSurveyResult(wlan_1x_ifname, (SS_STATUS_Tp)&res) < 0 ) 
			if ( getWlSiteSurveyResult(root_wlan_ifname, (SS_STATUS_Tp)&res) < 0 ) 
			{
				printf("Read site-survey status failed!\n");
				goto ss_err;
			}
			if (res == 0xff) 
			{   // in progress
				/*prolong wait time due to scan both 2.4G and 5G */
				if (wait_time++ > 20) 
				{
					printf("scan timeout!\n");					
					goto ss_err;
				}
				sleep(1);
			}
			else
				break;
		}

		pStatus->number=0;
		
		//if(getWlSiteSurveyResult(wlan_1x_ifname, pStatus)<0)
		if(getWlSiteSurveyResult(root_wlan_ifname, pStatus)<0)
		{			
			//printf("%s:%d second call getWlSiteSurveyResult() fail!\n",__FUNCTION__,__LINE__);
			goto ss_err; 
		}
	}

	apmib_get( MIB_WLAN_SSID,  (void *)ssidBuf);
	//printf("%s:%d ssidBuf=%s\n",__FUNCTION__,__LINE__,ssidBuf);

	//printf("%s:%d pStatus->number=%d\n",__FUNCTION__,__LINE__,pStatus->number);

	for (i=0; i<pStatus->number && pStatus->number!=0xff; i++) 
	{
		pBss = &pStatus->bssdb[i];
		memcpy(tmp_ssidBuf, pBss->bdSsIdBuf, pBss->bdSsId.Length);
		tmp_ssidBuf[pBss->bdSsId.Length] = '\0';
		if(strcmp(ssidBuf, tmp_ssidBuf)==0)
		{			
			//printf("%s:%d tmp_ssidBuf=%s\n",__FUNCTION__,__LINE__,tmp_ssidBuf);		
			
			if (pBss->bdTstamp[0] & 0x0000ffff) 
			{			
				idx = sprintf(tmp2Buf, "WPA");
				if (((pBss->bdTstamp[0] & 0x0000f000) >> 12) == 0x4)
					idx += sprintf(tmp2Buf+idx, "-PSK");
				else if(((pBss->bdTstamp[0] & 0x0000f000) >> 12) == 0x2)
					idx += sprintf(tmp2Buf+idx, "-1X");
				
				wpa_exist = 1;

				if (((pBss->bdTstamp[0] & 0x00000f00) >> 8) == 0x5)
				{
					//sprintf(wpa_tkip_aes,"%s","aes/tkip");
					*unicastCipher=2;
					*wpa2UnicastCipher=2;
				}
				else if (((pBss->bdTstamp[0] & 0x00000f00) >> 8) == 0x4)
				{
					//sprintf(wpa_tkip_aes,"%s","aes");
					*unicastCipher=2;
					*wpa2UnicastCipher=2;
				}
				else if (((pBss->bdTstamp[0] & 0x00000f00) >> 8) == 0x1)
				{
					//sprintf(wpa_tkip_aes,"%s","tkip");
					*unicastCipher=1;
					*wpa2UnicastCipher=1;
				}
			}
			if (pBss->bdTstamp[0] & 0xffff0000) 
			{
				if (wpa_exist)
					idx += sprintf(tmp2Buf+idx, "/");
				idx += sprintf(tmp2Buf+idx, "WPA2");
				if (((pBss->bdTstamp[0] & 0xf0000000) >> 28) == 0x4)
					idx += sprintf(tmp2Buf+idx, "-PSK");
				else if (((pBss->bdTstamp[0] & 0xf0000000) >> 28) == 0x2)
					idx += sprintf(tmp2Buf+idx, "-1X");

				if (((pBss->bdTstamp[0] & 0x0f000000) >> 24) == 0x5)
				{
					//sprintf(wpa2_tkip_aes,"%s","aes/tkip");
					*unicastCipher=1;
					*wpa2UnicastCipher=2;
				}
				else if (((pBss->bdTstamp[0] & 0x0f000000) >> 24) == 0x4)
				{
					//sprintf(wpa2_tkip_aes,"%s","aes");
					*unicastCipher=1;
					*wpa2UnicastCipher=2;
				}
				else if (((pBss->bdTstamp[0] & 0x0f000000) >> 24) == 0x1)
				{
					//sprintf(wpa2_tkip_aes,"%s","tkip");
					*unicastCipher=1;
					*wpa2UnicastCipher=1;
				}
			}

			if(strcmp(tmp2Buf, "WPA-1X")==0)
				*encryption=2;
			if(strcmp(tmp2Buf, "WPA2-1X")==0)
				*encryption=4;
			if(strcmp(tmp2Buf, "WPA-1X/WPA2-1X")==0)
				*encryption=4;
			
			break;
		}		
	}

	if(pStatus!=NULL)
		free(pStatus);
	
	//printf("%s:%d success!\n",__FUNCTION__,__LINE__);
	return 0;
	
ss_err:
	
	if(pStatus!=NULL)
		free(pStatus);
	
	//printf("%s:%d ss_err!\n",__FUNCTION__,__LINE__);
	
	return -1;
}
#endif
////////////////////////////////////////////////////////////////////////////////
static void generateWpaConf(char *outputFile, int isWds)
{
	int fh, intVal, encrypt, enable1x, wep;
	unsigned char buf1[1024], buf2[1024];

#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
	int encryption, unicastCipher, wpa2UnicastCipher;
	int wlan_mode;
#endif

#if 0
//#ifdef UNIVERSAL_REPEATER	
	int isVxd = 0;
	
	if (strstr(outputFile, "-vxd")) 
		isVxd = 1;	
#endif		
	
	if ( !apmib_init()) {
		printf("Initialize AP MIB failed!\n");
		return;
	}

	fh = open(outputFile, O_RDWR|O_CREAT|O_TRUNC);
	if (fh == -1) {
		printf("Create WPA config file error!\n");
		return;
	}

#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
	apmib_get( MIB_WLAN_MODE, (void *)&wlan_mode);

	if(wlan_mode == CLIENT_MODE)
	{
		getEncryptInfoAccordingAP(&encryption, &unicastCipher, &wpa2UnicastCipher);
		printf("%s:%d encryption=%d unicastCipher=%d wpa2UnicastCipher=%d\n",__FUNCTION__,__LINE__, 
			encryption, unicastCipher, wpa2UnicastCipher);
	}
#endif
	
	if (!isWds) {
	apmib_get( MIB_WLAN_ENCRYPT, (void *)&encrypt);

#if 0
//#ifdef UNIVERSAL_REPEATER
	if (isVxd && (encrypt == ENCRYPT_WPA2_MIXED)) {
		apmib_get( MIB_WLAN_MODE, (void *)&intVal);
		if (intVal == AP_MODE || intVal == AP_WDS_MODE) 
			encrypt = ENCRYPT_WPA;		
	}
#endif			

#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
	if (wlan_mode == CLIENT_MODE)
		encrypt=encryption;
#endif

	sprintf((char *)buf2, "encryption = %d\n", encrypt);
	WRITE_WPA_FILE(fh, buf2);

#if 0
//#ifdef UNIVERSAL_REPEATER
	if (isVxd) {
		if (strstr(outputFile, "wlan0-vxd"))
			apmib_get( MIB_REPEATER_SSID1, (void *)buf1);		
		else			
			apmib_get( MIB_REPEATER_SSID2, (void *)buf1);	
	}
	else
#endif
	apmib_get( MIB_WLAN_SSID,  (void *)buf1);
	sprintf((char *)buf2, "ssid = \"%s\"\n", buf1);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_ENABLE_1X, (void *)&enable1x);
	sprintf((char *)buf2, "enable1x = %d\n", enable1x);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_MAC_AUTH_ENABLED, (void *)&intVal);
	sprintf((char *)buf2, "enableMacAuth = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

#ifdef CONFIG_IEEE80211W	
	apmib_get( MIB_WLAN_IEEE80211W, (void *)&intVal);
	sprintf((char *)buf2, "ieee80211w = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);
	
	apmib_get( MIB_WLAN_SHA256_ENABLE, (void *)&intVal);
	sprintf((char *)buf2, "sha256 = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);
#endif
	apmib_get( MIB_WLAN_ENABLE_SUPP_NONWPA, (void *)&intVal);
	if (intVal)
		apmib_get( MIB_WLAN_SUPP_NONWPA, (void *)&intVal);

	sprintf((char *)buf2, "supportNonWpaClient = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_WEP, (void *)&wep);
	sprintf((char *)buf2, "wepKey = %d\n", wep);
	WRITE_WPA_FILE(fh, buf2);

	if ( encrypt==1 && enable1x ) {
		if (wep == 1) {
			apmib_get( MIB_WLAN_WEP64_KEY1, (void *)buf1);
			sprintf((char *)buf2, "wepGroupKey = \"%02x%02x%02x%02x%02x\"\n", buf1[0],buf1[1],buf1[2],buf1[3],buf1[4]);
		}
		else {
			apmib_get( MIB_WLAN_WEP128_KEY1, (void *)buf1);
			sprintf((char *)buf2, "wepGroupKey = \"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\"\n",
				buf1[0],buf1[1],buf1[2],buf1[3],buf1[4],
				buf1[5],buf1[6],buf1[7],buf1[8],buf1[9],
				buf1[10],buf1[11],buf1[12]);
		}
	}
	else
		strcpy((char *)buf2, "wepGroupKey = \"\"\n");
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_WPA_AUTH, (void *)&intVal);
	sprintf((char *)buf2, "authentication = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_WPA_CIPHER_SUITE, (void *)&intVal);

	
#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
	if (wlan_mode == CLIENT_MODE)
		intVal=unicastCipher;
#endif

	sprintf((char *)buf2, "unicastCipher = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&intVal);

#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
	if (wlan_mode == CLIENT_MODE)
		intVal=wpa2UnicastCipher;
#endif

	sprintf((char *)buf2, "wpa2UnicastCipher = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_WPA2_PRE_AUTH, (void *)&intVal);
	sprintf((char *)buf2, "enablePreAuth = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_PSK_FORMAT, (void *)&intVal);
	if (intVal==0)
		sprintf((char *)buf2, "usePassphrase = 1\n");
	else
		sprintf((char *)buf2, "usePassphrase = 0\n");
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_WPA_PSK, (void *)buf1);
	sprintf((char *)buf2, "psk = \"%s\"\n", buf1);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_WPA_GROUP_REKEY_TIME, (void *)&intVal);
	sprintf((char *)buf2, "groupRekeyTime = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_RS_REAUTH_TO, (void *)&intVal);
	sprintf((char *)buf2, "rsReAuthTO = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

#ifdef CONFIG_PMKCACHE
	apmib_get( MIB_WLAN_MAX_PMKSA, (void *)&intVal);
	sprintf(buf2, "MaxPmksa = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);
#endif

#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
	if (wlan_mode == CLIENT_MODE) { // wlan client mode		
		apmib_get( MIB_WLAN_EAP_TYPE, (void *)&intVal);
		sprintf(buf2, "eapType = %d\n", intVal);
		WRITE_WPA_FILE(fh, buf2);

		apmib_get( MIB_WLAN_EAP_INSIDE_TYPE, (void *)&intVal);
		sprintf(buf2, "eapInsideType = %d\n", intVal);
		WRITE_WPA_FILE(fh, buf2);

		apmib_get( MIB_WLAN_EAP_USER_ID, (void *)buf1);
		sprintf(buf2, "eapUserId = \"%s\"\n", buf1);
		WRITE_WPA_FILE(fh, buf2);

		apmib_get( MIB_WLAN_RS_USER_NAME, (void *)buf1);
		sprintf(buf2, "rsUserName = \"%s\"\n", buf1);
		WRITE_WPA_FILE(fh, buf2);

		apmib_get( MIB_WLAN_RS_USER_PASSWD, (void *)buf1);
		sprintf(buf2, "rsUserPasswd = \"%s\"\n", buf1);
		WRITE_WPA_FILE(fh, buf2);

		apmib_get( MIB_WLAN_RS_USER_CERT_PASSWD, (void *)buf1);
		sprintf(buf2, "rsUserCertPasswd = \"%s\"\n", buf1);
		WRITE_WPA_FILE(fh, buf2);

		apmib_get( MIB_WLAN_RS_BAND_SEL, (void *)&intVal);
		sprintf(buf2, "rsBandSel = %d\n", intVal);
		WRITE_WPA_FILE(fh, buf2);

		//Patch for auth daemon at wlan client mode
		// 127.0.0.1 : 12345
		if(intVal == PHYBAND_5G)
			sprintf(buf2, "rsPort = %d\n", 12344);
		else
			sprintf(buf2, "rsPort = %d\n", 12345);
		WRITE_WPA_FILE(fh, buf2);

		sprintf(buf2, "rsIP = %s\n", "127.0.0.1");
		WRITE_WPA_FILE(fh, buf2);
		//End patch.
	}
	else
#endif
	{
	apmib_get( MIB_WLAN_RS_PORT, (void *)&intVal);
	sprintf((char *)buf2, "rsPort = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_RS_IP, (void *)buf1);
	sprintf((char *)buf2, "rsIP = %s\n", inet_ntoa(*((struct in_addr *)buf1)));
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_RS_PASSWORD, (void *)buf1);
	sprintf((char *)buf2, "rsPassword = \"%s\"\n", buf1);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_RS_MAXRETRY, (void *)&intVal);
	sprintf((char *)buf2, "rsMaxReq = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_RS_INTERVAL_TIME, (void *)&intVal);
	sprintf((char *)buf2, "rsAWhile = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);
#ifdef CONFIG_APP_AUTH_2NDSRV
	apmib_get( MIB_WLAN_RS2_PORT, (void *)&intVal);
	sprintf((char *)buf2, "rs2Port = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_RS2_IP, (void *)buf1);
	sprintf((char *)buf2, "rs2IP = %s\n", inet_ntoa(*((struct in_addr *)buf1)));
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_RS2_PASSWORD, (void *)buf1);
	sprintf((char *)buf2, "rs2Password = \"%s\"\n", buf1);
	WRITE_WPA_FILE(fh, buf2);
#endif

	apmib_get( MIB_WLAN_ACCOUNT_RS_ENABLED, (void *)&intVal);
	sprintf((char *)buf2, "accountRsEnabled = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_ACCOUNT_RS_PORT, (void *)&intVal);
	sprintf((char *)buf2, "accountRsPort = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_ACCOUNT_RS_IP, (void *)buf1);
	sprintf((char *)buf2, "accountRsIP = %s\n", inet_ntoa(*((struct in_addr *)buf1)));
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_ACCOUNT_RS_PASSWORD, (void *)buf1);
	sprintf((char *)buf2, "accountRsPassword = \"%s\"\n", buf1);
	WRITE_WPA_FILE(fh, buf2);

#ifdef CONFIG_APP_AUTH_2NDSRV
	apmib_get( MIB_WLAN_ACCOUNT_RS2_PORT, (void *)&intVal);
	sprintf((char *)buf2, "accountRs2Port = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_ACCOUNT_RS2_IP, (void *)buf1);
	sprintf((char *)buf2, "accountRs2IP = %s\n", inet_ntoa(*((struct in_addr *)buf1)));
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_ACCOUNT_RS2_PASSWORD, (void *)buf1);
	sprintf((char *)buf2, "accountRs2Password = \"%s\"\n", buf1);
	WRITE_WPA_FILE(fh, buf2);
#endif

	apmib_get( MIB_WLAN_ACCOUNT_RS_UPDATE_ENABLED, (void *)&intVal);
	sprintf((char *)buf2, "accountRsUpdateEnabled = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_ACCOUNT_RS_UPDATE_DELAY, (void *)&intVal);
	sprintf((char *)buf2, "accountRsUpdateTime = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_ACCOUNT_RS_MAXRETRY, (void *)&intVal);
	sprintf((char *)buf2, "accountRsMaxReq = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);

	apmib_get( MIB_WLAN_ACCOUNT_RS_INTERVAL_TIME, (void *)&intVal);
	sprintf((char *)buf2, "accountRsAWhile = %d\n", intVal);
	WRITE_WPA_FILE(fh, buf2);
	}
	}

	else {
		apmib_get( MIB_WLAN_WDS_ENCRYPT, (void *)&encrypt);
		if (encrypt == WDS_ENCRYPT_TKIP)		
			encrypt = ENCRYPT_WPA;
		else if (encrypt == WDS_ENCRYPT_AES)		
			encrypt = ENCRYPT_WPA2;		
		else
			encrypt = 0;
	
		sprintf((char *)buf2, "encryption = %d\n", encrypt);
		WRITE_WPA_FILE(fh, buf2);
		WRITE_WPA_FILE(fh, (unsigned char *)"ssid = \"REALTEK\"\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"enable1x = 1\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"enableMacAuth = 0\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"supportNonWpaClient = 0\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"wepKey = 0\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"wepGroupKey = \"\"\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"authentication = 2\n");

		if (encrypt == ENCRYPT_WPA)
			intVal = WPA_CIPHER_TKIP;
		else
			intVal = WPA_CIPHER_AES;
			
		sprintf((char *)buf2, "unicastCipher = %d\n", intVal);
		WRITE_WPA_FILE(fh, buf2);

		sprintf((char *)buf2, "wpa2UnicastCipher = %d\n", intVal);
		WRITE_WPA_FILE(fh, buf2);

		WRITE_WPA_FILE(fh, (unsigned char *)"enablePreAuth = 0\n");

		apmib_get( MIB_WLAN_WDS_PSK_FORMAT, (void *)&intVal);
		if (intVal==0)
			sprintf((char *)buf2, "usePassphrase = 1\n");
		else
			sprintf((char *)buf2, "usePassphrase = 0\n");
		WRITE_WPA_FILE(fh, buf2);

		apmib_get( MIB_WLAN_WDS_PSK, (void *)buf1);
		sprintf((char *)buf2, "psk = \"%s\"\n", buf1);
		WRITE_WPA_FILE(fh, buf2);

		WRITE_WPA_FILE(fh, (unsigned char *)"groupRekeyTime = 0\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"rsPort = 1812\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"rsIP = 192.168.1.1\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"rsPassword = \"\"\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"rsMaxReq = 3\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"rsAWhile = 10\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"accountRsEnabled = 0\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"accountRsPort = 1813\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"accountRsIP = 192.168.1.1\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"accountRsPassword = \"\"\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"accountRsUpdateEnabled = 0\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"accountRsUpdateTime = 1000\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"accountRsMaxReq = 3\n");
		WRITE_WPA_FILE(fh, (unsigned char *)"accountRsAWhile = 1\n");
	}

	close(fh);
}

static int generateHostapdConf(void)
{

	FILE *fh = NULL;
	int intValue;

	char line[1024], strValue[1024];


	if ( !apmib_init()) {
		printf("Initialize AP MIB failed!\n");
		return -1;
	}

	fh = fopen("/etc/hostapd.conf", "w");
	if (!fh) {
		printf("Create hostapd config file error!\n");
		return -1;
	}
	else {
		/* mib independent fields */
		sprintf(line,"interface=wlan0\n");
		fputs(line,fh);
		/*
		sprintf(line,"bridge=br0\n");
		fputs(line,fh); 
		sprintf(line,"driver=nl80211\n");
		fputs(line,fh);*/
		/*
		bit 0 (1) = IEEE 802.11
		bit 1 (2) = IEEE 802.1X
		bit 2 (4) = RADIUS
		bit 3 (8) = WPA
		bit 4 (16) = driver interface
		bit 5 (32) = IAPP
		bit 6 (64) = MLME
		*/
		sprintf(line,"logger_syslog=-1\n");
		fputs(line,fh);
		sprintf(line,"logger_stdout=-1\n");
		fputs(line,fh);
		/*
		Levels (minimum value for logged events):
		 0 = verbose debugging
		 1 = debugging
		 2 = informational messages
		 3 = notification
		 4 = warning
		*/
		sprintf(line,"logger_syslog_level=2\n");
		fputs(line,fh);
		sprintf(line,"logger_stdout_level=2\n");
		fputs(line,fh);
		sprintf(line,"dump_file=/tmp/hostapd.dump\n");
		fputs(line,fh);
		sprintf(line,"ctrl_interface=/var/run/hostapd\n");
		fputs(line,fh);
		sprintf(line,"ctrl_interface_group=0\n");
		fputs(line,fh);
		
	
		/* mib dependent fields */
		apmib_get(MIB_WLAN_SSID,strValue);
		sprintf(line,"ssid=%s\n",strValue);
		fputs(line,fh);
		/* country_code
		apmib_get(MIB_WLAN_SSID,value);
		sprintf(line,"ssid=%s\n",value);
		fputs(line,fh);*/
		/* ieee80211d
		apmib_get(MIB_WLAN_SSID,value);
		intValue=atoi(value);
		sprintf(line,"ssid=%d\n",intValue);
		fputs(line,fh); */
		/* hw_mode */
		apmib_get(MIB_WLAN_BAND,&intValue);
		if(intValue == 1)
			sprintf(line,"hw_mode=b\n");
		else
			sprintf(line,"hw_mode=g\n");
		fputs(line,fh);

		if( intValue > 6 ){
			sprintf(line,"ieee80211n=1\n");
			sprintf(line,"%sht_capab=",line);
			/*ht_capab [40+][40-] [LDPC] [SMPS-STATIC][SMPS-DYNAMIC]  [SHORT-GI-20][SHORT-GI-40] 
			   [TX-STBC][RX-STBC] [DELAYED-BA] [MAX-AMSDU-7935] [DSSS_CCK-40] [PSMP] [LSIG-TXOP-PROT] */
			/* channel */
			apmib_get(MIB_WLAN_CHANNEL_BONDING, &intValue);
			if( intValue ){
				apmib_get(MIB_WLAN_CONTROL_SIDEBAND, &intValue);
				if( intValue )
					sprintf(line,"%s%s",line,"[40+]");
				else
					sprintf(line,"%s%s",line,"[40-]");
				apmib_get(MIB_WLAN_SHORT_GI,&intValue);
				if( intValue )
					sprintf(line,"%s %s",line,"[SHORT-GI-40]");
			}
			else{
				apmib_get(MIB_WLAN_SHORT_GI,&intValue);
				if( intValue )
					sprintf(line,"%s%s",line,"[SHORT-GI-20]");
			}
			/*
			apmib_get(MIB_WLAN_11N_STBC,&intValue);
			if( intValue )
				sprintf(line,"%s %s",line,"[TX-STBC]");
			*/
			apmib_get(MIB_WLAN_AGGREGATION,&intValue);
			if( intValue > 0 )
				sprintf(line,"%s %s",line,"[MAX-AMSDU-7935]");
		}
		else{
			sprintf(line,"ieee80211n=0");
		}
		sprintf(line,"%s\n",line);
		fputs(line,fh);
		
		apmib_get(MIB_WLAN_CHANNEL,&intValue);
		// channel=0 means auto then bypass
		if(intValue){
			sprintf(line,"channel=%d\n",intValue);
			fputs(line,fh);
		}
		/* beacon_int */
		apmib_get(MIB_WLAN_BEACON_INTERVAL,&intValue);
		sprintf(line,"beacon_int=%d\n",intValue);
		fputs(line,fh);
		/* dtim_period */
		apmib_get(MIB_WLAN_DTIM_PERIOD, &intValue);
		if( atoi(strValue) )
			sprintf(line,"dtim_period=%d\n",intValue);
		else
			sprintf(line,"dtim_period=2\n");
		fputs(line,fh);
		/* max_num_sta */
		sprintf(line,"max_num_sta=255\n");
		fputs(line,fh);
		/* rts_threshold */
		apmib_get(MIB_WLAN_RTS_THRESHOLD, &intValue);
		if( intValue )
			sprintf(line,"rts_threshold=%d\n",intValue);
		else
			sprintf(line,"rts_threshold=2347\n");
		fputs(line,fh);
		/* fragm_threshold */
		apmib_get(MIB_WLAN_FRAG_THRESHOLD, &intValue);
		if( intValue )
			sprintf(line,"fragm_threshold=%d\n",intValue);
		else
			sprintf(line,"fragm_threshold=2346\n");
		fputs(line,fh);
		/* supported_rates : in most case, setting for the selected hw */
		/* basic_rates */
		/* preamble 0:not use short;1:use */
		apmib_get(MIB_WLAN_PREAMBLE_TYPE, &intValue);
		sprintf(line,"preamble=%d\n",intValue);
		fputs(line,fh);
		/* macaddr_acl:0:accept unless in deny list ; 1:deny unless in accept list ; external RADIUS server */
		{
			int count;
			FILE *fp = NULL;
			apmib_get(MIB_WLAN_MACAC_ENABLED, &intValue);
			apmib_get(MIB_WLAN_MACAC_NUM, &count);
			if( intValue == 1 ){		/* deny unless in (accept) list */
				sprintf(line,"macaddr_acl=1\n");
				fputs(line,fh);
				if( (fp = fopen("/etc/hostapd.accept","w")) ){
					while( count ){
						apmib_get(MIB_WLAN_MACAC_ADDR, strValue);
						sprintf(line,"%c%c:%c%c:%c%c:%c%c:%c%c:%c%c\n",strValue[0],strValue[1],strValue[2],strValue[3],strValue[4],strValue[5],\
																strValue[6],strValue[7],strValue[8],strValue[9],strValue[10],strValue[11]);
						fputs(line,fp);
						count--;
					}
					fclose(fp);
				}
			}
			else if( intValue == 2 ){	/* accept unless in (deny) list */
				sprintf(line,"macaddr_acl=0\n");
				fputs(line,fh);
				if( (fp = fopen("/etc/hostapd.deny","w") ) ){
					while( count ){
						apmib_get(MIB_WLAN_MACAC_ADDR, strValue);
						sprintf(line,"%c%c:%c%c:%c%c:%c%c:%c%c:%c%c\n",strValue[0],strValue[1],strValue[2],strValue[3],strValue[4],strValue[5],\
																strValue[6],strValue[7],strValue[8],strValue[9],strValue[10],strValue[11]);
						fputs(line,fp);
						count--;
					}
					fclose(fp);
				}
			}
			else{
				sprintf(line,"#macaddr_acl=0\n");
				fputs(line,fh);
			}
		}
		/* 802.11i */
		/* WEP */
		apmib_get(MIB_WLAN_ENCRYPT,&intValue);
		if( intValue == 1 ){
			/* auth_algs 1:open ; 2:WEP ; 3: open & WEP */
			/*
			apmib_get(MIB_WLAN_AUTH_TYPE,&intValue);
			if( !intValue )
				sprintf(line,"auth_algs=1\n");
			else
				sprintf(line,"auth_algs=2\n");
			*/
			sprintf(line,"auth_algs=2\n");
			apmib_get(MIB_WLAN_WEP,&intValue);
			if( intValue == 1 ){
				apmib_get(MIB_WLAN_WEP64_KEY1,strValue);
				if( strlen(strValue) > 0 )
					sprintf(line,"%swep_key0=%s\n",line,strValue);
				apmib_get(MIB_WLAN_WEP64_KEY2,strValue);
				if( strlen(strValue) > 0 )
					sprintf(line,"%swep_key1=%s\n",line,strValue);
				apmib_get(MIB_WLAN_WEP64_KEY3,strValue);
				if( strlen(strValue) > 0 )
					sprintf(line,"%swep_key2=%s\n",line,strValue);
				apmib_get(MIB_WLAN_WEP64_KEY4,strValue);
				if( strlen(strValue) > 0 )
					sprintf(line,"%swep_key3=%s\n",line,strValue);
			}
			else{
				apmib_get(MIB_WLAN_WEP128_KEY1,strValue);
				if( strlen(strValue) > 0 )
					sprintf(line,"%swep_key0=%s\n",line,strValue);
				apmib_get(MIB_WLAN_WEP128_KEY2,strValue);
				if( strlen(strValue) > 0 )
					sprintf(line,"%swep_key1=%s\n",line,strValue);
				apmib_get(MIB_WLAN_WEP128_KEY3,strValue);
				if( strlen(strValue) > 0 )
					sprintf(line,"%swep_key2=%s\n",line,strValue);
				apmib_get(MIB_WLAN_WEP128_KEY4,strValue);
				if( strlen(strValue) > 0 )
					sprintf(line,"%swep_key3=%s\n",line,strValue);
			}
			fputs(line,fh);
		}
		else if( intValue > 1 ){
			int security=0;
			/* WPA */
			sprintf(line,"auth_algs=1\n");
			sprintf(line,"%swpa=%d\n",line,intValue>>1);
			security = intValue;
			apmib_get(MIB_WLAN_WPA_PSK,strValue);
			if( strlen(strValue) > 0 )
				sprintf(line,"%swpa_passphrase=%s\n",line,strValue);
			//wpa_key_mgmt WPA-PSK;WPA-EAP
			apmib_get(MIB_WLAN_WPA_AUTH,&intValue);
			if( intValue == 1 ){
				sprintf(line,"%swpa_key_mgmt=WPA-EAP\n",line);
				apmib_get(MIB_WLAN_RS_IP,&intValue);
				sprintf(line,"%sauth_server_addr=%u.%u.%u.%u\n",line,*((char *)&intValue)&0xff,*(((char *)&intValue)+1)&0xff,*(((char *)&intValue)+2)&0xff,*(((char *)&intValue)+3)&0xff);
				apmib_get(MIB_WLAN_RS_PORT,&intValue);
				sprintf(line,"%sauth_server_port=%d\n",line,intValue);
				apmib_get(MIB_WLAN_RS_PASSWORD,strValue);
				if(strlen(strValue))
					sprintf(line,"%sauth_server_shared_secret=%s\n",line,strValue);
				else
					sprintf(line,"%sauth_server_shared_secret=\n",line);
			}
			else{
				sprintf(line,"%swpa_key_mgmt=WPA-PSK\n",line);
			}
			//wpa_passphrase
			//wpa_psk_file			
			//wpa_pairwise TKIP;CCMP
			apmib_get(MIB_WLAN_WPA_CIPHER_SUITE,&intValue);
			if( (intValue > 0) && (security&0x02) ){
				sprintf(line,"%swpa_pairwise=",line);
				if( intValue == 1 )
					sprintf(line,"%sTKIP\n",line);
				else if( intValue == 3 )
					sprintf(line,"%sTKIP CCMP\n",line);
				else
					sprintf(line,"%sCCMP\n",line);
			}
			//rsn_pairwise CCMP (WP2)
			apmib_get(MIB_WLAN_WPA2_CIPHER_SUITE,&intValue);
			if( (intValue > 0) && (security&0x04) ){
				sprintf(line,"%srsn_pairwise=",line);
				if( intValue == 1 )
					sprintf(line,"%sTKIP\n",line);
				else if( intValue == 3 )
					sprintf(line,"%sTKIP CCMP\n",line);
				else
					sprintf(line,"%sCCMP\n",line);
			}
			apmib_get(MIB_WLAN_WPA_GROUP_REKEY_TIME,&intValue);
			sprintf(line,"%swpa_group_rekey=%d\n",line,intValue);
			//sprintf(line,"%swpa_strict_rekey=%d\n",line,intValue);
			//sprintf(line,"%swpa_gmk_rekey=%d\n",line,intValue);
			sprintf(line,"%swpa_ptk_rekey=%d\n",line,intValue);
			apmib_get(MIB_WLAN_WPA2_PRE_AUTH,&intValue);
			sprintf(line,"%srsn_preauth=%d\n",line,intValue);
			//rsn_preauth_interfaces
			//peerkey
			fputs(line,fh);
		}
		else{
			sprintf(line,"wpa=0\n");
			fputs(line,fh);
		}
		
		/* ignore_broadcast_ssid 0:disable ; 1:empty bssid in beacon ; 2:set SSID in beacon as 0 1,2 all ignore probe with broadcast SSID */
		apmib_get(MIB_WLAN_HIDDEN_SSID,&intValue);
		sprintf(line,"ignore_broadcast_ssid=%d\n",intValue);
		fputs(line,fh);
		/*TX queue parameters*/
		//tx_queue_data3_aifs
		//tx_queue_data3_cwmin
		//tx_queue_data3_cwmax
		//tx_queue_data3_burst
		//tx_queue_data2_aifs
		//tx_queue_data2_cwmin
		//tx_queue_data2_cwmax
		//tx_queue_data2_burst
		//tx_queue_data1_aifs
		//tx_queue_data1_cwmin
		//tx_queue_data1_cwmax
		//tx_queue_data0_aifs
		//tx_queue_data0_cwmin
		//tx_queue_data0_cwmax
		//tx_queue_data0_burst
		//tx_queue_after_beacon_aifs
		//tx_queue_after_beacon_cwmin
		//tx_queue_after_beacon_cwmax
		//tx_queue_after_beacon_burst
		//tx_queue_beacon_aifs
		//tx_queue_beacon_cwmin
		//tx_queue_beacon_cwmax
		//tx_queue_beacon_burst

		/* wmm configuration */
		apmib_get(MIB_WLAN_WMM_ENABLED,&intValue);
		sprintf(line,"wme_enabled=%d\n",intValue);
		fputs(line, fh);
		//wme_ac_bk_cwmin
		//wme_ac_bk_cwmax
		//wme_ac_bk_aifs
		//wme_ac_bk_txop_limit
		//wme_ac_bk_acm
		//wme_ac_be_cwmin
		//wme_ac_be_cwmax
		//wme_ac_be_aifs
		//wme_ac_be_txop_limit
		//wme_ac_be_acm
		//wme_ac_vi_cwmin
		//wme_ac_vi_cwmax
		//wme_ac_vi_aifs
		//wme_ac_vi_txop_limit
		//wme_ac_vi_acm
		//wme_ac_vo_cwmin
		//wme_ac_vo_cwmax
		//wme_ac_vo_aifs
		//wme_ac_vo_txop_limit
		//wme_ac_vo_acm

		/* ap_max_inactivity */
		apmib_get(MIB_WLAN_INACTIVITY_TIME,&intValue);
		sprintf(line,"ap_max_inactivity=%d\n",intValue);
		fputs(line,fh);
		//bridge_packets : internal bridge
		//max_listen_interval

		/* Authentication */
		/* ieee8021x */
		apmib_get(MIB_WLAN_ENABLE_1X,&intValue);
		if( intValue ){
			/* RADIUS client configuration */
			sprintf(line,"ieee8021x=%d\n",intValue);
			//own_ip_addr
			//nas_identifier
			apmib_get(MIB_WLAN_RS_IP,&intValue);
			sprintf(line,"%sauth_server_addr=%u.%u.%u.%u\n",line,*((char *)&intValue)&0xff,*(((char *)&intValue)+1)&0xff,*(((char *)&intValue)+2)&0xff,*(((char *)&intValue)+3)&0xff);
			apmib_get(MIB_WLAN_RS_PORT,&intValue);
			sprintf(line,"%sauth_server_port=%d\n",line,intValue);
			apmib_get(MIB_WLAN_RS_PASSWORD,strValue);
			if(strlen(strValue))
				sprintf(line,"%sauth_server_shared_secret=%s\n",line,strValue);
			else
				sprintf(line,"%sauth_server_shared_secret=\n",line);
			apmib_get(MIB_WLAN_ACCOUNT_RS_ENABLED,&intValue);
			if( intValue ){
				apmib_get(MIB_WLAN_ACCOUNT_RS_IP,&intValue);
				sprintf(line,"%sacct_server_addr=%u.%u.%u.%u\n",line,*((char *)&intValue)&0xff,*(((char *)&intValue)+1)&0xff,*(((char *)&intValue)+2)&0xff,*(((char *)&intValue)+3)&0xff);
				apmib_get(MIB_WLAN_ACCOUNT_RS_PORT,&intValue);
				sprintf(line,"%sacct_server_port=%d\n",line,intValue);
			}
			//acct_server_shared_secret
			apmib_get(MIB_WLAN_RS_MAXRETRY,&intValue);
			sprintf(line,"%sradius_retry_primary_interval=%d\n",line,intValue);
			apmib_get(MIB_WLAN_RS_INTERVAL_TIME,&intValue);
			sprintf(line,"%sradius_acct_interim_interval=%d\n",line,intValue);
			fputs(line,fh);
			//dynamic_vlan
			//vlan_file
			//vlan_tagged_interface
		}
		//eapol_version
		//eap_message
		//wep_key_len_broadcast
		//wep_key_len_unicast
		//wep_rekey_period
		//eapol_key_index_workaround
		//eap_reauth_period
		//use_pae_group_addr

		/* eap server */
		//eap_server = 0

		/* 802.11f */
		apmib_get(MIB_WLAN_IAPP_DISABLED,&intValue);
		if( !intValue ){
			//sprintf(line,"iapp_interface=eth0\n");
			sprintf(line,"iapp_interface=br0\n");
			fputs(line,fh);
		}

		/* RADIUS server configuration */
		//radius_server_clients
		//radius_server_auth_port
		//radius_server_ipv6

		//ieee80211w 0:disabled ; 1:optional ; 2:required
		//assoc_sa_query_max_timeout
		//assoc_sa_query_retry_timeout
		//okc

		/* 8021.11r */
		//mobility_domain (2-octet identifier as a hex string)
		//r0_key_lifetime
		//r1_key_holder
		//reassociation_deadline
		//r0kh format: <MAC address> <R0KH-ID> <128-bit key as hex string>
		//r1kh format: <MAC address> <R0KH-ID> <128-bit key as hex string>
		//pmk_r1_push
		//passive_scan_interval
		//passive_scan_listen
		//passive_scan_mode
		//ap_table_max_size
		//ap_table_expiration_time

		/* WPS */
#if 0
		//wps_state 0:disabled;1:enabled, not configured;2:enabled, configured
		apmib_get(MIB_WSC_DISABLE,&intValue);
		if( intValue ){
			sprintf(line,"wps_state=0\n");
		}
		else{
			apmib_get(MIB_WLAN_WSC_CONFIGURED,&intValue);
			if( intValue )
				sprintf(line,"wps_state=2\n");
			else
				sprintf(line,"wps_state=1\n");
		}
		sprintf(line,"%sdevice_name=Realtek Wireless AP\n",line);
		sprintf(line,"%smanufacturer=Realtek Semiconductor Corp.\n",line);
		sprintf(line,"%smodel_name=RTL8xxx\n",line);
		sprintf(line,"%smodel_number=EV-2009-02-06\n",line);
		sprintf(line,"%sserial_number=123456789012347\n",line);
		/*
		   device_attrib_id=1
		   device_oui=0050f204
		   device_category_id=6
		   device_sub_category_id=1
		*/
		sprintf(line,"%sdevice_type=6-0050f204-1\n",line);
		//ap_setup_locked
		sprintf(line,"%suuid=63041253101920061228aabbccddeeff\n",line);
		sprintf(line,"%smanufacturer_url=http://www.realtek.com/\n",line);
		sprintf(line,"%smodel_description=WLAN Access Point\n",line);
		sprintf(line,"%smodel_url=http://www.realtek.com\n",line);
		apmib_get(MIB_WSC_METHOD,&intValue);
		sprintf(line,"%sconfig_methods=%d\n",line,intValue);
		fputs(line,fh);
		//wps_pin_requests
		//os_version
		//ap_pin
		//skip_cred_build
		//extra_cred
		//wps_cred_processing
		//ap_settings
		//upnp_iface
		//friendly_name
		//upc
#endif
		/* Multiple BSSID */
		//bss
		//ssid
		//bssid
		fclose(fh);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#ifdef WLAN_FAST_INIT

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/wireless.h>
#include <ieee802_mib.h>

void calc_incr(unsigned char *mac, int idx)
{
	if( (*mac+idx) == 0x0 )
		calc_incr(mac-1,1);
	else
		*mac += idx;
}

int get_root_mac(unsigned char *ifname,unsigned char *mac)
{
	int fd;
	struct ifreq ifr;
	unsigned char zero_mac[6]={0}, broadcat_mac[6]={0xff};
	unsigned char if_root[6]={'\0'};
#if defined(CONFIG_RTL_92D_SUPPORT)
	unsigned char buf2[1024] = {'\0'};
	int intVal;
#endif

	fd = socket(AF_INET, SOCK_DGRAM, 0);

#if defined(CONFIG_RTL_92D_SUPPORT)
	apmib_get(MIB_WLAN_PHY_BAND_SELECT,&intVal);
	if(!strcmp(ifname,"wlan0") && intVal == 1) {
		memcpy(if_root,"wlan1",5);
	}
	else
#endif
	{
		memcpy(if_root,ifname,5);
	}

#if defined(CONFIG_RTL_92D_SUPPORT) && !defined(CONFIG_RTL_TRI_BAND)
	if( strlen(ifname) < 6 ) {
		sprintf(buf2,"iwpriv %s bandadd %d",ifname,intVal);
	    system(buf2);	
	}
#endif
	ifr.ifr_addr.sa_family = AF_INET;
	strcpy(ifr.ifr_name, (char *)if_root);
	if( ioctl(fd, SIOCGIFHWADDR, &ifr) < 0 ){
		close(fd);
		return -1;
	}
	close(fd);

	if( !memcmp(ifr.ifr_hwaddr.sa_data,zero_mac,6) || !memcmp(ifr.ifr_hwaddr.sa_data,broadcat_mac,6) ){
		return -1;
	}

	memcpy(mac,ifr.ifr_hwaddr.sa_data,6);
	return 0;
}

#ifdef WLAN_PROFILE
 void set_profile(int id, struct wifi_mib *pmib)
{
	int i, i1, i2;
	WLAN_PROFILE_T profile;

	if (id == 0) {
		apmib_get(MIB_PROFILE_ENABLED1, (void *)&i1);
		apmib_get(MIB_PROFILE_NUM1, (void *)&i2);
	}
#if !defined(CONFIG_RTL_TRI_BAND) 
	else {
		apmib_get(MIB_PROFILE_ENABLED2, (void *)&i1);
		apmib_get(MIB_PROFILE_NUM2, (void *)&i2);		
	}
#else
	else if (id==1){
		apmib_get(MIB_PROFILE_ENABLED2, (void *)&i1);
		apmib_get(MIB_PROFILE_NUM2, (void *)&i2);		
	}else if(id==2){
		apmib_get(MIB_PROFILE_ENABLED3, (void *)&i1);
		apmib_get(MIB_PROFILE_NUM3, (void *)&i2);
	}else
		printf("[%s %d]support %d interface !!\n", __FUNCTION__,__LINE__);
#endif

	pmib->ap_profile.enable_profile = ((i1 && i2) ? 1 : 0);
	if (i1 && i2) {
		printf("Init wireless[%d] profile...\r\n", id);
		for (i=0; i<i2; i++) {
			*((char *)&profile) = (char)(i+1);
			if (id == 0)
				apmib_get(MIB_PROFILE_TBL1, (void *)&profile);
#if !defined(CONFIG_RTL_TRI_BAND) 
			else
				apmib_get(MIB_PROFILE_TBL2, (void *)&profile);							
#else			
			else if(id==1)
				apmib_get(MIB_PROFILE_TBL2, (void *)&profile);	
			else if(id==2)
				apmib_get(MIB_PROFILE_TBL3, (void *)&profile);
			else
				printf("[%s %d]support %d interface !!\n", __FUNCTION__,__LINE__);
#endif
			strcpy(pmib->ap_profile.profile[i].ssid, profile.ssid);
			pmib->ap_profile.profile[i].encryption = profile.encryption;
			pmib->ap_profile.profile[i].auth_type = profile.auth;
			pmib->ap_profile.profile[i].wep_default_key = profile.wep_default_key;
			memcpy(pmib->ap_profile.profile[i].wep_key1, profile.wepKey1, 13);
			memcpy(pmib->ap_profile.profile[i].wep_key2, profile.wepKey2, 13);
			memcpy(pmib->ap_profile.profile[i].wep_key3, profile.wepKey3, 13);						
			memcpy(pmib->ap_profile.profile[i].wep_key4, profile.wepKey4, 13);
			pmib->ap_profile.profile[i].wpa_cipher = profile.wpa_cipher;
			strcpy(pmib->ap_profile.profile[i].wpa_psk, profile.wpaPSK);			

//printf("\r\n pmib->ap_profile.profile[%d].ssid=[%s],__[%s-%u]\r\n",i, pmib->ap_profile.profile[i].ssid,__FILE__,__LINE__);			
		}		
	}
	pmib->ap_profile.profile_num = i2;
}
#endif


//====================================================================
#ifdef CONFIG_RTL_AIRTIME
#include <errno.h>

#define SIOCMIBSYNC_ATM 0x8B50 /* Sorry! we don't have a header to include this!! 
								copy from drivers/net/wireless/rtl8192cd/8192cd_ioctl.c */

//#define DUMP_ATM_STATION_TABLE

#ifdef DUMP_ATM_STATION_TABLE
void dumpAirtime(struct ATMConfigEntry *pEntry, int num_id)
{
	int i, val;
	apmib_get(num_id, (void *)&val);

	printf("=== %s =============================================\n",__FILE__);
	printf("enable: %d\n", pEntry->atm_en);
	printf("mode:   %d\n", pEntry->atm_mode);
	printf("num:    %d\n", val);

	printf("\nInterface:\n", pEntry->atm_mode);
	for (i=0;i<6;i++) {
		printf("  intf[%d]:%d\n", i, pEntry->atm_iftime[i]);
	}

	printf("\nStation:\n", pEntry->atm_mode);
	for (i=0;i<val;i++) {
		printf("atm_sta[%d]:\n",i);
		printf("  ipAddr:%d.%d.%d.%d\n",
			pEntry->atm_sta[i].ipaddr[0],
			pEntry->atm_sta[i].ipaddr[1],
			pEntry->atm_sta[i].ipaddr[2],
			pEntry->atm_sta[i].ipaddr[3]);
		printf("  macAddr:%02X%02X%02X%02X%02X%02X\n",
			pEntry->atm_sta[i].hwaddr[0],
			pEntry->atm_sta[i].hwaddr[1],
			pEntry->atm_sta[i].hwaddr[2],
			pEntry->atm_sta[i].hwaddr[3],
			pEntry->atm_sta[i].hwaddr[4],
			pEntry->atm_sta[i].hwaddr[5]);
		printf("  atm_time:%d\n\n", pEntry->atm_sta[i].atm_time);
	}
	printf("=== %s =============================================\n",__FILE__);
}
#endif /* DUMP_ATM_STATION_TABLE */

int initAirtime(char *ifname)
{
	struct ATMConfigEntry *pEntry;
	AIRTIME_T atmEntry;
	struct iwreq wrq;
	unsigned char atime[6];
	unsigned int val;
	int i;
	int skfd;
		
	int enable_id, mode_id, iftime_id, num_id, tbl_id;
	
	if (wlan_idx==0) {
		enable_id = MIB_AIRTIME_ENABLED;
		mode_id   = MIB_AIRTIME_MODE;
		iftime_id = MIB_AIRTIME_IFTIME;
		num_id    = MIB_AIRTIME_TBL_NUM;
		tbl_id    = MIB_AIRTIME_TBL;
	}
	else {
		enable_id = MIB_AIRTIME2_ENABLED;
		mode_id   = MIB_AIRTIME2_MODE;
		iftime_id = MIB_AIRTIME2_IFTIME;
		num_id    = MIB_AIRTIME2_TBL_NUM;
		tbl_id    = MIB_AIRTIME2_TBL;
	}	
	
	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (skfd < 0) {
		printf("socket() fail\n");
		return -1;
	}

	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(skfd, SIOCGIWNAME, &wrq) < 0) {
		printf("Interface %s open failed!\n", ifname);
		close(skfd);
		return -1;
	}

	if ((pEntry = (struct ATMConfigEntry *)malloc(sizeof(struct ATMConfigEntry))) == NULL) {
		printf("MIB buffer allocation failed!\n");
		close(skfd);
		return -1;
	}

	memset(pEntry, 0, sizeof(struct ATMConfigEntry));
	wrq.u.data.pointer = (caddr_t)pEntry;
	wrq.u.data.length = sizeof(struct ATMConfigEntry);

	apmib_get(enable_id, (void *)&val);
	pEntry->atm_en = (unsigned char)val;

	apmib_get(mode_id, (void *)&val);
	pEntry->atm_mode = (unsigned char)val;

	apmib_get(iftime_id, (void *)atime);
	for (i=0; i<= NUM_VWLAN_INTERFACE; i++) {
		pEntry->atm_iftime[i] = (unsigned char)atime[i];
	}

	apmib_get(num_id, (void *)&val);
	for (i=1; i<=val; i++) {
		memset(&atmEntry, 0x00, sizeof(atmEntry));
		*((char *)&atmEntry) = (char)i;
		if (apmib_get(tbl_id, (void *)&atmEntry)) {
			memcpy(pEntry->atm_sta[i-1].hwaddr, atmEntry.macAddr, 6);
			memcpy(pEntry->atm_sta[i-1].ipaddr, atmEntry.ipAddr, 4);
			pEntry->atm_sta[i-1].atm_time = atmEntry.atm_time;
		}
	}

#ifdef DUMP_ATM_STATION_TABLE
	dumpAirtime(pEntry, num_id);
#endif

	if (ioctl(skfd, SIOCMIBSYNC_ATM, &wrq) < 0) {
		printf("set airtime failed!\n");
		printf("ioctl failed and returned errno %s \n",strerror(errno));
		free(pEntry);
		close(skfd);
		return -1;
	}
	close(skfd);
	return 0;
}
#endif /* CONFIG_RTL_AIRTIME */
//====================================================================


static int initWlan(char *ifname)
{
	struct wifi_mib *pmib;
	int i, intVal, intVal2, encrypt, enable1x, wep, mode/*, enable1xVxd*/;
	unsigned char buf1[1024], buf2[1024], mac[6];
	int skfd;
	struct iwreq wrq, wrq_root;
	int wlan_band=0, channel_bound=0, aggregation=0;
	MACFILTER_T *pAcl=NULL;
	struct wdsEntry *wds_Entry=NULL;
	WDS_Tp pwds_EntryUI;
#ifdef MBSSID
	int v_previous=0;
#ifdef CONFIG_RTL_819X
	int vap_enable=0, intVal4=0;
#endif
#endif



	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (skfd < 0) {
		printf("socket() fail\n");
		return -1;
	}
	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(skfd, SIOCGIWNAME, &wrq) < 0) {
		printf("Interface %s open failed!\n", ifname);
		close(skfd);
		return -1;
	}

	if ((pmib = (struct wifi_mib *)malloc(sizeof(struct wifi_mib))) == NULL) {
		printf("MIB buffer allocation failed!\n");
		close(skfd);
		return -1;
	}

	if (!apmib_init()) {
		printf("Initialize AP MIB failed!\n");
		free(pmib);
		close(skfd);
		return -1;
	}

	// Disable WLAN MAC driver and shutdown interface first
	sprintf((char *)buf1, "ifconfig %s down", ifname);
	system((char *)buf1);

	if (vwlan_idx == 0) {
		// shutdown all WDS interface
		for (i=0; i<8; i++) {
			sprintf((char *)buf1, "ifconfig %s-wds%d down", ifname, i);
			system((char *)buf1);
		}
	
#ifndef RTK_REINIT_SUPPORT
		// kill wlan application daemon
		sprintf((char *)buf1, "wlanapp.sh kill %s", ifname);
		system((char *)buf1);
#endif
	}
	else { // virtual interface
		sprintf((char *)buf1, "wlan%d", wlan_idx);
		strncpy(wrq_root.ifr_name, (char *)buf1, IFNAMSIZ);
		if (ioctl(skfd, SIOCGIWNAME, &wrq_root) < 0) {
			printf("Root Interface %s open failed!\n", buf1);
			free(pmib);
			close(skfd);
			return -1;
		}
	}

	if (vwlan_idx == 0) {
		apmib_get(MIB_HW_RF_TYPE, (void *)&intVal);
		if (intVal == 0) {
			printf("RF type is NULL!\n");
			free(pmib);
			close(skfd);
			return 0;
		}
	}
	apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&intVal);

	if (intVal == 1) {
		free(pmib);
		close(skfd);
		return 0;
	}

#ifdef CONFIG_RTL_AIRTIME
	if (vwlan_idx == 0 && initAirtime(ifname) != 0) {
		return -1;
	}
#endif /* CONFIG_RTL_AIRTIME */

	// get mib from driver
	wrq.u.data.pointer = (caddr_t)pmib;
	wrq.u.data.length = sizeof(struct wifi_mib);

	if (vwlan_idx == 0) {
		if (ioctl(skfd, 0x8B42, &wrq) < 0) {
			printf("Get WLAN MIB failed!\n");
			free(pmib);
			close(skfd);
			return -1;
		}
	}
	else {
        #ifdef CONFIG_RTL_COMAPI_CFGFILE
            system("iwpriv wlan0 cfgfile"); // is it right to be here ?
        #endif
		wrq_root.u.data.pointer = (caddr_t)pmib; 
		wrq_root.u.data.length = sizeof(struct wifi_mib);				
		if (ioctl(skfd, 0x8B42, &wrq_root) < 0) {
			printf("Get WLAN MIB failed!\n");
			free(pmib);
			close(skfd);
			return -1;
		}		
	}

	// check mib version
	if (pmib->mib_version != MIB_VERSION) {
		printf("WLAN MIB version mismatch!\n");
		free(pmib);
		close(skfd);
		return -1;
	}

	if (vwlan_idx > 0) {	//if not root interface, clone root mib to virtual interface
		wrq.u.data.pointer = (caddr_t)pmib;
		wrq.u.data.length = sizeof(struct wifi_mib);
		if (ioctl(skfd, 0x8B43, &wrq) < 0) {
			printf("Set WLAN MIB failed!\n");
			free(pmib);
			close(skfd);
			return -1;
		}
		pmib->miscEntry.func_off = 0;	
	}

	// Set parameters to driver

#if 0	// set func_off=0 will cause the Windows WHQL WPS test item failed
	int func_off;
	apmib_get(MIB_WLAN_FUNC_OFF, (void *)&func_off);
	pmib->miscEntry.func_off = func_off;
#endif

	if (vwlan_idx == 0) {	
		apmib_get(MIB_HW_REG_DOMAIN, (void *)&intVal);
		pmib->dot11StationConfigEntry.dot11RegDomain = intVal;
	}

	apmib_get(MIB_WLAN_WLAN_MAC_ADDR, (void *)mac);
	if (!memcmp(mac, "\x00\x00\x00\x00\x00\x00", 6)) {
//#ifdef WLAN_MAC_FROM_EFUSE
#if 0 //def WLAN_MAC_FROM_EFUSE
        if( !get_root_mac(ifname,mac) ){
#ifdef MBSSID
		if( vwlan_idx > 0 ) {
        				if (*(char *)(ifname+4) == '0' ) //wlan0
		        		  *(char *)mac |= LOCAL_ADMIN_BIT;
		        		else {
		        			(*(char *)mac) += 4; 
		        			(*(char *)mac) |= LOCAL_ADMIN_BIT;
		        		}
		}
                calc_incr((char *)mac+MACADDRLEN-1,vwlan_idx);
#endif
        } else 
#endif
	{
#ifdef MBSSID
		if (vwlan_idx > 0 && vwlan_idx != NUM_VWLAN_INTERFACE) {
			switch (vwlan_idx)
			{
				case 1:
					apmib_get(MIB_HW_WLAN_ADDR1, (void *)mac);
					break;
				case 2:
					apmib_get(MIB_HW_WLAN_ADDR2, (void *)mac);
					break;
				case 3:
					apmib_get(MIB_HW_WLAN_ADDR3, (void *)mac);
					break;
				case 4:
					apmib_get(MIB_HW_WLAN_ADDR4, (void *)mac);
					break;
				default:
					printf("Fail to get MAC address of VAP%d!\n", vwlan_idx-1);
					free(pmib);
					close(skfd);
					return 0;
			}
		}
		else
#endif
		apmib_get(MIB_HW_WLAN_ADDR, (void *)mac);
	}
	}
	// ifconfig all wlan interface when not in WISP
	// ifconfig wlan1 later interface when in WISP mode, the wlan0  will be setup in WAN interface
	apmib_get(MIB_OP_MODE, (void *)&intVal);
	apmib_get(MIB_WISP_WAN_ID, (void *)&intVal2);
	sprintf((char *)buf1, "wlan%d", intVal2);
	/*notice: config repeater interface's mac address will overwrite root interface's mac*/
	if (	
#ifdef MBSSID		
		(vwlan_idx != NUM_VWLAN_INTERFACE) && 
#endif		
		((intVal != 2) || strcmp((char *)buf1,ifname) ||
#ifdef MBSSID
		vwlan_idx > 0 
#endif
		)) {
		sprintf((char *)buf2, "ifconfig %s hw ether %02x%02x%02x%02x%02x%02x", ifname, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		system((char *)buf2);
		memcpy(&(pmib->dot11OperationEntry.hwaddr[0]), mac, 6);
	}

#ifdef BR_SHORTCUT	
	if (intVal == 2
#ifdef MBSSID
		&& vwlan_idx == 0
#endif
	) 
		pmib->dot11OperationEntry.disable_brsc = 1;
	else
		pmib->dot11OperationEntry.disable_brsc = 0;
#endif
	
	apmib_get(MIB_HW_LED_TYPE, (void *)&intVal);
	pmib->dot11OperationEntry.ledtype = intVal;

	// set AP/client/WDS mode
	apmib_get(MIB_WLAN_SSID, (void *)buf1);
	intVal2 = strlen((char *)buf1);
	pmib->dot11StationConfigEntry.dot11DesiredSSIDLen = intVal2;
	memset(pmib->dot11StationConfigEntry.dot11DesiredSSID, 0, 32);
	memcpy(pmib->dot11StationConfigEntry.dot11DesiredSSID, buf1, intVal2);
	if ((pmib->dot11StationConfigEntry.dot11DesiredSSIDLen == 3) &&
		((pmib->dot11StationConfigEntry.dot11DesiredSSID[0] == 'A') || (pmib->dot11StationConfigEntry.dot11DesiredSSID[0] == 'a')) &&
		((pmib->dot11StationConfigEntry.dot11DesiredSSID[1] == 'N') || (pmib->dot11StationConfigEntry.dot11DesiredSSID[1] == 'n')) &&
		((pmib->dot11StationConfigEntry.dot11DesiredSSID[2] == 'Y') || (pmib->dot11StationConfigEntry.dot11DesiredSSID[2] == 'y'))) {
		pmib->dot11StationConfigEntry.dot11SSIDtoScanLen = 0;
		memset(pmib->dot11StationConfigEntry.dot11SSIDtoScan, 0, 32);
	}
	else {
		pmib->dot11StationConfigEntry.dot11SSIDtoScanLen = intVal2;
		memset(pmib->dot11StationConfigEntry.dot11SSIDtoScan, 0, 32);
		memcpy(pmib->dot11StationConfigEntry.dot11SSIDtoScan, buf1, intVal2);
	}

	apmib_get(MIB_WLAN_MODE, (void *)&mode);

#ifdef RTL_MULTI_CLONE_SUPPORT
        // when RTL_MULTI_CLONE_SUPPORT enabled let wlan0-va1,wlan0-va2 can setting macclone
        apmib_get(MIB_WLAN_MACCLONE_ENABLED, (void *)&intVal);
        if ((intVal == 1) && (mode == 1)) {
            pmib->ethBrExtInfo.macclone_enable = 1;
        }
        else {
            pmib->ethBrExtInfo.macclone_enable = 0;
        }       
#endif
  
	if (mode == 1) {
		// client mode
		apmib_get(MIB_WLAN_NETWORK_TYPE, (void *)&intVal2);
		if (intVal2 == 0){
			pmib->dot11OperationEntry.opmode = 8;
#ifdef WLAN_PROFILE


			set_profile(*((char *)(ifname+4))-'0', pmib);
#endif
		}
		else {
			pmib->dot11OperationEntry.opmode = 32;
			apmib_get(MIB_WLAN_DEFAULT_SSID, (void *)buf1);
			intVal2 = strlen((char *)buf1);
			pmib->dot11StationConfigEntry.dot11DefaultSSIDLen = intVal2;
			memset(pmib->dot11StationConfigEntry.dot11DefaultSSID, 0, 32);
			memcpy(pmib->dot11StationConfigEntry.dot11DefaultSSID, buf1, intVal2);

#ifdef WLAN_PROFILE
			set_profile(*((char *)(ifname+4))-'0', pmib);			
#endif
		}

#ifdef CONFIG_IEEE80211W
		apmib_get(MIB_WLAN_IEEE80211W,(void *)&intVal);
		pmib->dot1180211AuthEntry.dot11IEEE80211W = intVal;
		apmib_get(MIB_WLAN_SHA256_ENABLE,(void *)&intVal);
		pmib->dot1180211AuthEntry.dot11EnableSHA256 = intVal;
#endif
	
        pmib->dot11hTPCEntry.tpc_enable = 0; /*disable tpc in client mode*/
	}
	else
		pmib->dot11OperationEntry.opmode = 16;

	if (mode == 2)	// WDS only
		pmib->dot11WdsInfo.wdsPure = 1;
	else
		pmib->dot11WdsInfo.wdsPure = 0;

#ifdef CONFIG_RTL_P2P_SUPPORT
#define WIFI_AP_STATE	 0x00000010
#define WIFI_STATION_STATE 0x08
	apmib_get(MIB_WLAN_P2P_TYPE, (void *)&intVal);
    pmib->p2p_mib.p2p_enabled = 0;
	if(mode==P2P_SUPPORT_MODE)
	{

		switch(intVal)
		{
			P2P_DEBUG("initWlan:");
			case P2P_DEVICE:
				P2P_DEBUG("P2P_DEVICE mode \n");
				break;
			case P2P_PRE_CLIENT:
				P2P_DEBUG("P2P_PRE_CLIENT mode \n");
				break;
			case P2P_CLIENT:
				P2P_DEBUG("P2P_CLIENT mode \n");
				break;
			case P2P_PRE_GO:
				P2P_DEBUG("P2P_PRE_GO mode \n");
				break;
			case P2P_TMP_GO:
				P2P_DEBUG("P2P_TMP_GO mode \n");
				break;
			default:
                
				P2P_DEBUG("Unknow P2P type;use dev as default!\n");
              	intVal=P2P_DEVICE;
              	apmib_set(MIB_WLAN_P2P_TYPE,(void *)&intVal); // save to flash mib                
				SDEBUG("\n");              	                

		}



		
		if(intVal>=P2P_DEVICE && intVal<=P2P_CLIENT){			
			/*use STA mode as base*/ 			
			if(intVal==P2P_DEVICE){
				/*if is P2P device mode then clear dot11DesiredSSIDLen and dot11DesiredSSID*/
				pmib->dot11StationConfigEntry.dot11DesiredSSIDLen = 0;
				memset(pmib->dot11StationConfigEntry.dot11DesiredSSID, 0, 32);
			}
			pmib->dot11OperationEntry.opmode = WIFI_STATION_STATE;					
		}else if(intVal>=P2P_PRE_GO && intVal<=P2P_TMP_GO){
			/*use AP as base*/ 
			pmib->dot11OperationEntry.opmode = WIFI_AP_STATE;			

		}else{
			SDEBUG("unknow P2P type chk!\n");			
		}
		
		/*fill p2p type*/
		pmib->p2p_mib.p2p_type= intVal;
		pmib->p2p_mib.p2p_enabled = 2;

		// wlan driver will know it's will start under p2p client mode, but flash reset to P2P_device mode
		//if(intVal == P2P_CLIENT){
		//	intVal = P2P_DEVICE;
		//	apmib_set(MIB_WLAN_P2P_TYPE, (void *)&intVal);
		//	}

		// set DHCP=0 ; will dymanic deter start dhcp server or dhcp client
		//P2P_DEBUG("under P2P mode disable DHCP\n\n");
		//intVal = DHCP_DISABLED;
		//apmib_set(MIB_DHCP, (void *)&intVal);			

		
		/*fill intent value*/
		apmib_get(MIB_WLAN_P2P_INTENT, (void *)&intVal2);		
		pmib->p2p_mib.p2p_intent = (unsigned char)intVal2;
		//printf("\n\n(p2p)intent=%d\n\n\n",pmib->p2p_mib.p2p_intent);
		
		/*fill listen channel value*/
		apmib_get(MIB_WLAN_P2P_LISTEN_CHANNEL, (void *)&intVal2);		
		pmib->p2p_mib.p2p_listen_channel= (unsigned char)intVal2;
		
		/*fill op_channel value*/		
		apmib_get(MIB_WLAN_P2P_OPERATION_CHANNEL, (void *)&intVal2);				
		pmib->p2p_mib.p2p_op_channel= (unsigned char)intVal2;		


		/*get device  name*/
		apmib_get(MIB_DEVICE_NAME, (void *)buf1);	
		memcpy(pmib->p2p_mib.p2p_device_name, buf1, MAX_SSID_LEN);	

		/*get pin code */
	    apmib_get(MIB_HW_WSC_PIN, (void *)buf1);	
		memcpy(pmib->p2p_mib.p2p_wsc_pin_code, buf1, 9);	


		/*get wsc method*/
		apmib_get(MIB_WLAN_WSC_METHOD, (void *)&intVal);

		// 2011/ 03 / 07
		if(mode==P2P_SUPPORT_MODE){
			intVal =  (CONFIG_METHOD_PBC | CONFIG_METHOD_DISPLAY |CONFIG_METHOD_KEYPAD);

		}else{

			//Ethernet(0x2)+Label(0x4)+PushButton(0x80) Bitwise OR
			
			if (intVal == 1) //Pin+Ethernet
				intVal = (CONFIG_METHOD_ETH | CONFIG_METHOD_PIN);
			else if (intVal == 2) //PBC+Ethernet
				intVal = (CONFIG_METHOD_ETH | CONFIG_METHOD_PBC);
			if (intVal == 3) //Pin+PBC+Ethernet
				intVal = (CONFIG_METHOD_ETH | CONFIG_METHOD_PIN | CONFIG_METHOD_PBC);

		}


		

		pmib->p2p_mib.p2p_wsc_config_method=(unsigned short)intVal;		
		
	}
#endif

	pmib->dot11StationConfigEntry.dot11AclNum = 0;
	apmib_get(MIB_WLAN_MACAC_ENABLED, (void *)&intVal);
	pmib->dot11StationConfigEntry.dot11AclMode = intVal;
	if (intVal != 0) {
		apmib_get(MIB_WLAN_MACAC_NUM, (void *)&intVal);
		if (intVal != 0) {
			for (i=0; i<intVal; i++) {
				buf1[0] = i+1;
				apmib_get(MIB_WLAN_MACAC_ADDR, (void *)buf1);
				pAcl = (MACFILTER_T *)buf1;
				memcpy(&(pmib->dot11StationConfigEntry.dot11AclAddr[i][0]), &(pAcl->macAddr[0]), 6);
				pmib->dot11StationConfigEntry.dot11AclNum++;
			}
		}
	}

	if (vwlan_idx == 0) { // root interface	
		// set RF parameters
		apmib_get(MIB_HW_RF_TYPE, (void *)&intVal);
		pmib->dot11RFEntry.dot11RFType = intVal;
		
#if defined(CONFIG_RTL_8196B)

		apmib_get(MIB_HW_BOARD_VER, (void *)&intVal);
		if (intVal == 1)
			pmib->dot11RFEntry.MIMO_TR_mode = 3;	// 2T2R
		else if(intVal == 2)
			pmib->dot11RFEntry.MIMO_TR_mode = 4; // 1T1R
		else
			pmib->dot11RFEntry.MIMO_TR_mode = 1;	// 1T2R

		apmib_get(MIB_HW_TX_POWER_CCK, (void *)buf1);
		memcpy(pmib->dot11RFEntry.pwrlevelCCK, buf1, 14);
		
		apmib_get(MIB_HW_TX_POWER_OFDM_1S, (void *)buf1);
		memcpy(pmib->dot11RFEntry.pwrlevelOFDM_1SS, buf1, 162);
		
		apmib_get(MIB_HW_TX_POWER_OFDM_2S, (void *)buf1);
		memcpy(pmib->dot11RFEntry.pwrlevelOFDM_2SS, buf1, 162);

		// not used for RTL8192SE
		//apmib_get(MIB_HW_11N_XCAP, (void *)&intVal);
		//pmib->dot11RFEntry.crystalCap = intVal;
		
		apmib_get(MIB_HW_11N_LOFDMPWDA, (void *)&intVal);
		pmib->dot11RFEntry.LOFDM_pwd_A = intVal;

		apmib_get(MIB_HW_11N_LOFDMPWDB, (void *)&intVal);
		pmib->dot11RFEntry.LOFDM_pwd_B = intVal;

		apmib_get(MIB_HW_11N_TSSI1, (void *)&intVal);
		pmib->dot11RFEntry.tssi1 = intVal;

		apmib_get(MIB_HW_11N_TSSI2, (void *)&intVal);
		pmib->dot11RFEntry.tssi2 = intVal;

		apmib_get(MIB_HW_11N_THER, (void *)&intVal);
		pmib->dot11RFEntry.ther = intVal;

		if (pmib->dot11RFEntry.dot11RFType == 10) { // Zebra
			apmib_get(MIB_WLAN_RFPOWER_SCALE, (void *)&intVal);
			if(intVal == 1)
				intVal = 3;
			else if(intVal == 2)
					intVal = 6;
				else if(intVal == 3)
						intVal = 9;
					else if(intVal == 4)
							intVal = 17;
			if (intVal) {
				for (i=0; i<14; i++) {
					if(pmib->dot11RFEntry.pwrlevelCCK[i] != 0){ 
						if ((pmib->dot11RFEntry.pwrlevelCCK[i] - intVal) >= 1)
							pmib->dot11RFEntry.pwrlevelCCK[i] -= intVal;
						else
							pmib->dot11RFEntry.pwrlevelCCK[i] = 1;
					}
				}
				for (i=0; i<162; i++) {
					if (pmib->dot11RFEntry.pwrlevelOFDM_1SS[i] != 0){
						if((pmib->dot11RFEntry.pwrlevelOFDM_1SS[i] - intVal) >= 1)
							pmib->dot11RFEntry.pwrlevelOFDM_1SS[i] -= intVal;
						else
							pmib->dot11RFEntry.pwrlevelOFDM_1SS[i] = 1;
					}
					if (pmib->dot11RFEntry.pwrlevelOFDM_2SS[i] != 0){
						if((pmib->dot11RFEntry.pwrlevelOFDM_2SS[i] - intVal) >= 1)
							pmib->dot11RFEntry.pwrlevelOFDM_2SS[i] -= intVal;
						else
							pmib->dot11RFEntry.pwrlevelOFDM_2SS[i] = 1;
					}
				}		
			}
		}
#elif defined(CONFIG_RTL_8198C)||defined(CONFIG_RTL_8196C) || defined(CONFIG_RTL_8198) || defined(CONFIG_RTL_819XD) || defined(CONFIG_RTL_8196E) || defined(CONFIG_RTL_8198B) || defined(CONFIG_RTL_8197F)
#ifdef CONFIG_WLAN_HAL_8814AE
	apmib_get(MIB_HW_BOARD_VER, (void *)&intVal);
        if (intVal == 1)
                pmib->dot11RFEntry.MIMO_TR_mode = 5; // 3T3R
        else if(intVal == 2)
                pmib->dot11RFEntry.MIMO_TR_mode = 3; // 2T2R
        else if(intVal == 3)
                pmib->dot11RFEntry.MIMO_TR_mode = 2; // 2T4R
        else
                pmib->dot11RFEntry.MIMO_TR_mode = 5; // 3T3R

#ifdef CONFIG_RTL_8814_8194_2T2R_SUPPORT
	printf("Force 2T2R for 8814/8194 !!!\n");
	pmib->dot11RFEntry.MIMO_TR_mode = 3; // 2T2R
#endif
#else
	apmib_get(MIB_HW_BOARD_VER, (void *)&intVal);
	if (intVal == 1)
		pmib->dot11RFEntry.MIMO_TR_mode = 3;	// 2T2R
	else if(intVal == 2)
		pmib->dot11RFEntry.MIMO_TR_mode = 4; // 1T1R
	else
		pmib->dot11RFEntry.MIMO_TR_mode = 1;	// 1T2R

#ifdef CONFIG_RTL_8812_1T1R_SUPPORT
	if(wlan_idx == 0)
	{
		printf("\n\n Force 1T1R for WLAN0 !!!\n\n");
		pmib->dot11RFEntry.MIMO_TR_mode = 4; // 1T1R
	}	
#endif
#endif

	apmib_get(MIB_HW_TX_POWER_CCK_A, (void *)buf1);
	memcpy(pmib->dot11RFEntry.pwrlevelCCK_A, buf1, MAX_2G_CHANNEL_NUM_MIB);	
	
	apmib_get(MIB_HW_TX_POWER_CCK_B, (void *)buf1);
	memcpy(pmib->dot11RFEntry.pwrlevelCCK_B, buf1, MAX_2G_CHANNEL_NUM_MIB);
	
	apmib_get(MIB_HW_TX_POWER_HT40_1S_A, (void *)buf1);
	memcpy(pmib->dot11RFEntry.pwrlevelHT40_1S_A, buf1, MAX_2G_CHANNEL_NUM_MIB);
	
	apmib_get(MIB_HW_TX_POWER_HT40_1S_B, (void *)buf1);
	memcpy(pmib->dot11RFEntry.pwrlevelHT40_1S_B, buf1, MAX_2G_CHANNEL_NUM_MIB);
	
	apmib_get(MIB_HW_TX_POWER_DIFF_HT40_2S, (void *)buf1);
	memcpy(pmib->dot11RFEntry.pwrdiffHT40_2S, buf1, MAX_2G_CHANNEL_NUM_MIB);
	
	apmib_get(MIB_HW_TX_POWER_DIFF_HT20, (void *)buf1);
	memcpy(pmib->dot11RFEntry.pwrdiffHT20, buf1, MAX_2G_CHANNEL_NUM_MIB);
	
	apmib_get(MIB_HW_TX_POWER_DIFF_OFDM, (void *)buf1);
	memcpy(pmib->dot11RFEntry.pwrdiffOFDM, buf1, MAX_2G_CHANNEL_NUM_MIB);
	
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_8812_SUPPORT)
	apmib_get(MIB_HW_TX_POWER_5G_HT40_1S_A, (void *)buf1);
	memcpy(pmib->dot11RFEntry.pwrlevel5GHT40_1S_A, buf1, MAX_5G_CHANNEL_NUM_MIB);
	
	apmib_get(MIB_HW_TX_POWER_5G_HT40_1S_B, (void *)buf1);
	memcpy(pmib->dot11RFEntry.pwrlevel5GHT40_1S_B, buf1, MAX_5G_CHANNEL_NUM_MIB);
	
	apmib_get(MIB_HW_TX_POWER_DIFF_5G_HT40_2S, (void *)buf1);
	memcpy(pmib->dot11RFEntry.pwrdiff5GHT40_2S, buf1, MAX_5G_CHANNEL_NUM_MIB);
	
	apmib_get(MIB_HW_TX_POWER_DIFF_5G_HT20, (void *)buf1);
	memcpy(pmib->dot11RFEntry.pwrdiff5GHT20, buf1, MAX_5G_CHANNEL_NUM_MIB);
	
	apmib_get(MIB_HW_TX_POWER_DIFF_5G_OFDM, (void *)buf1);
	memcpy(pmib->dot11RFEntry.pwrdiff5GOFDM, buf1, MAX_5G_CHANNEL_NUM_MIB);
#endif
	

#if defined(CONFIG_RTL_8812_SUPPORT)
	// 5G
	apmib_get(MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_A, (void *)buf1);
	assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_20BW1S_OFDM1T_A, (unsigned char*) buf1);	
	apmib_get(MIB_HW_TX_POWER_DIFF_5G_40BW2S_20BW2S_A, (void *)buf1);
	assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_40BW2S_20BW2S_A, (unsigned char*)buf1);
	apmib_get(MIB_HW_TX_POWER_DIFF_5G_80BW1S_160BW1S_A, (void *)buf1);
	assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_80BW1S_160BW1S_A, (unsigned char*)buf1);
	apmib_get(MIB_HW_TX_POWER_DIFF_5G_80BW2S_160BW2S_A, (void *)buf1);
	assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_80BW2S_160BW2S_A, (unsigned char*)buf1);

	apmib_get(MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_B, (void *)buf1);
	assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_20BW1S_OFDM1T_B, (unsigned char*)buf1);	
	apmib_get(MIB_HW_TX_POWER_DIFF_5G_40BW2S_20BW2S_B, (void *)buf1);
	assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_40BW2S_20BW2S_B, (unsigned char*)buf1);
	apmib_get(MIB_HW_TX_POWER_DIFF_5G_80BW1S_160BW1S_B, (void *)buf1);
	assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_80BW1S_160BW1S_B, (unsigned char*)buf1);
	apmib_get(MIB_HW_TX_POWER_DIFF_5G_80BW2S_160BW2S_B, (void *)buf1);
	assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_80BW2S_160BW2S_B, (unsigned char*)buf1);

	// 2G
	apmib_get(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_A, (void *)buf1);
	memcpy(pmib->dot11RFEntry.pwrdiff_20BW1S_OFDM1T_A, buf1, MAX_2G_CHANNEL_NUM_MIB);	
	apmib_get(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_A, (void *)buf1);
	memcpy(pmib->dot11RFEntry.pwrdiff_40BW2S_20BW2S_A, buf1, MAX_2G_CHANNEL_NUM_MIB);

	apmib_get(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_B, (void *)buf1);
	memcpy(pmib->dot11RFEntry.pwrdiff_20BW1S_OFDM1T_B, buf1, MAX_2G_CHANNEL_NUM_MIB);
	apmib_get(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_B, (void *)buf1);
	memcpy(pmib->dot11RFEntry.pwrdiff_40BW2S_20BW2S_B, buf1, MAX_2G_CHANNEL_NUM_MIB);
#endif

#if defined(CONFIG_WLAN_HAL_8814AE)
        //3 5G

		apmib_get(MIB_HW_TX_POWER_DIFF_5G_40BW3S_20BW3S_A, (void *)buf1);
		assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_40BW3S_20BW3S_A, (unsigned char*)buf1);

		apmib_get(MIB_HW_TX_POWER_DIFF_5G_80BW3S_160BW3S_A, (void *)buf1);
		assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_80BW3S_160BW3S_A, (unsigned char*)buf1);

		apmib_get(MIB_HW_TX_POWER_DIFF_5G_40BW3S_20BW3S_B, (void *)buf1);
		assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_40BW3S_20BW3S_B, (unsigned char*)buf1);

		apmib_get(MIB_HW_TX_POWER_DIFF_5G_80BW3S_160BW3S_B, (void *)buf1);
		assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_80BW3S_160BW3S_B, (unsigned char*)buf1);


        apmib_get(MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_C, (void *)buf1);
        assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_20BW1S_OFDM1T_C, (unsigned char*) buf1);   
        apmib_get(MIB_HW_TX_POWER_DIFF_5G_40BW2S_20BW2S_C, (void *)buf1);
        assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_40BW2S_20BW2S_C, (unsigned char*)buf1);
        apmib_get(MIB_HW_TX_POWER_DIFF_5G_80BW1S_160BW1S_C, (void *)buf1);
        assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_80BW1S_160BW1S_C, (unsigned char*)buf1);
        apmib_get(MIB_HW_TX_POWER_DIFF_5G_80BW2S_160BW2S_C, (void *)buf1);
        assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_80BW2S_160BW2S_C, (unsigned char*)buf1);
		apmib_get(MIB_HW_TX_POWER_DIFF_5G_40BW3S_20BW3S_C, (void *)buf1);
		assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_40BW3S_20BW3S_C, (unsigned char*)buf1);
		apmib_get(MIB_HW_TX_POWER_DIFF_5G_80BW3S_160BW3S_C, (void *)buf1);
		assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_80BW3S_160BW3S_C, (unsigned char*)buf1);
	
        apmib_get(MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_D, (void *)buf1);
        assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_20BW1S_OFDM1T_D, (unsigned char*)buf1);    
        apmib_get(MIB_HW_TX_POWER_DIFF_5G_40BW2S_20BW2S_D, (void *)buf1);
        assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_40BW2S_20BW2S_D, (unsigned char*)buf1);
		apmib_get(MIB_HW_TX_POWER_DIFF_5G_40BW3S_20BW3S_D, (void *)buf1);
		assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_40BW3S_20BW3S_D, (unsigned char*)buf1);
        apmib_get(MIB_HW_TX_POWER_DIFF_5G_80BW1S_160BW1S_D, (void *)buf1);
        assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_80BW1S_160BW1S_D, (unsigned char*)buf1);
        apmib_get(MIB_HW_TX_POWER_DIFF_5G_80BW2S_160BW2S_D, (void *)buf1);
        assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_80BW2S_160BW2S_D, (unsigned char*)buf1);
		apmib_get(MIB_HW_TX_POWER_DIFF_5G_80BW3S_160BW3S_D, (void *)buf1);
		assign_diff_AC(pmib->dot11RFEntry.pwrdiff_5G_80BW3S_160BW3S_D, (unsigned char*)buf1);

	
        //3 2G

		apmib_get(MIB_HW_TX_POWER_HT40_1S_C, (void *)buf1);
		memcpy(pmib->dot11RFEntry.pwrlevelHT40_1S_C, buf1, MAX_2G_CHANNEL_NUM_MIB);
		apmib_get(MIB_HW_TX_POWER_HT40_1S_D, (void *)buf1);
		memcpy(pmib->dot11RFEntry.pwrlevelHT40_1S_D, buf1, MAX_2G_CHANNEL_NUM_MIB);

		apmib_get(MIB_HW_TX_POWER_CCK_C, (void *)buf1);
		memcpy(pmib->dot11RFEntry.pwrlevelCCK_C, buf1, MAX_2G_CHANNEL_NUM_MIB);	
		apmib_get(MIB_HW_TX_POWER_CCK_D, (void *)buf1);
		memcpy(pmib->dot11RFEntry.pwrlevelCCK_D, buf1, MAX_2G_CHANNEL_NUM_MIB);

		apmib_get(MIB_HW_TX_POWER_DIFF_40BW3S_20BW3S_A, (void *)buf1);
		memcpy(pmib->dot11RFEntry.pwrdiff_40BW3S_20BW3S_A, buf1, MAX_2G_CHANNEL_NUM_MIB);
		apmib_get(MIB_HW_TX_POWER_DIFF_40BW3S_20BW3S_B, (void *)buf1);
		memcpy(pmib->dot11RFEntry.pwrdiff_40BW3S_20BW3S_B, buf1, MAX_2G_CHANNEL_NUM_MIB);

        apmib_get(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_C, (void *)buf1);
        memcpy(pmib->dot11RFEntry.pwrdiff_20BW1S_OFDM1T_C, buf1, MAX_2G_CHANNEL_NUM_MIB);   
        apmib_get(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_C, (void *)buf1);
        memcpy(pmib->dot11RFEntry.pwrdiff_40BW2S_20BW2S_C, buf1, MAX_2G_CHANNEL_NUM_MIB);
    	apmib_get(MIB_HW_TX_POWER_DIFF_40BW3S_20BW3S_C, (void *)buf1);
		memcpy(pmib->dot11RFEntry.pwrdiff_40BW3S_20BW3S_C, buf1, MAX_2G_CHANNEL_NUM_MIB);

        apmib_get(MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_D, (void *)buf1);
        memcpy(pmib->dot11RFEntry.pwrdiff_20BW1S_OFDM1T_D, buf1, MAX_2G_CHANNEL_NUM_MIB);
        apmib_get(MIB_HW_TX_POWER_DIFF_40BW2S_20BW2S_D, (void *)buf1);
        memcpy(pmib->dot11RFEntry.pwrdiff_40BW2S_20BW2S_D, buf1, MAX_2G_CHANNEL_NUM_MIB);
		apmib_get(MIB_HW_TX_POWER_DIFF_40BW3S_20BW3S_D, (void *)buf1);
		memcpy(pmib->dot11RFEntry.pwrdiff_40BW3S_20BW3S_D, buf1, MAX_2G_CHANNEL_NUM_MIB);

		apmib_get(MIB_HW_TX_POWER_5G_HT40_1S_C, (void *)buf1);
		memcpy(pmib->dot11RFEntry.pwrlevel5GHT40_1S_C, buf1, MAX_5G_CHANNEL_NUM_MIB);
		//printf("BUF pwrlevel5GHT40_1S_C=%s\n",buf1);
		
		apmib_get(MIB_HW_TX_POWER_5G_HT40_1S_D, (void *)buf1);
		memcpy(pmib->dot11RFEntry.pwrlevel5GHT40_1S_D, buf1, MAX_5G_CHANNEL_NUM_MIB);
		//printf("BUF pwrlevel5GHT40_1S_D=%s\n",buf1);
		
		//printf("pmib->dot11RFEntry.pwrlevel5GHT40_1S_A=%s\n",pmib->dot11RFEntry.pwrlevel5GHT40_1S_A);
		//printf("pmib->dot11RFEntry.pwrlevel5GHT40_1S_B=%s\n",pmib->dot11RFEntry.pwrlevel5GHT40_1S_B);
		//printf("pmib->dot11RFEntry.pwrlevel5GHT40_1S_C=%s\n",pmib->dot11RFEntry.pwrlevel5GHT40_1S_C);
		//printf("pmib->dot11RFEntry.pwrlevel5GHT40_1S_D=%s\n",pmib->dot11RFEntry.pwrlevel5GHT40_1S_D);
#endif

	apmib_get(MIB_HW_11N_TSSI1, (void *)&intVal);
	pmib->dot11RFEntry.tssi1 = intVal;

	apmib_get(MIB_HW_11N_TSSI2, (void *)&intVal);
	pmib->dot11RFEntry.tssi2 = intVal;

#if defined(CONFIG_RTL_8881A_SELECTIVE) && !defined(CONFIG_WLAN_HAL_8814AE)
	apmib_get(MIB_WLAN_PHY_BAND_SELECT, (void *)&intVal);	

	apmib_get(MIB_HW_11N_THER, (void *)&intVal);
	pmib->dot11RFEntry.ther = intVal;		

	apmib_get(MIB_HW_11N_XCAP, (void *)&intVal);
	pmib->dot11RFEntry.xcap = intVal;
	
	apmib_get(MIB_HW_11N_THER_2, (void *)&intVal);
	pmib->dot11RFEntry.ther2 = intVal;		

	apmib_get(MIB_HW_11N_XCAP_2, (void *)&intVal);
	pmib->dot11RFEntry.xcap2 = intVal;

#else
	apmib_get(MIB_HW_11N_THER, (void *)&intVal);
	pmib->dot11RFEntry.ther = intVal;

	apmib_get(MIB_HW_11N_XCAP, (void *)&intVal);
	pmib->dot11RFEntry.xcap = intVal;
#endif
#if 0
	apmib_get(MIB_HW_11N_KFREE_ENABLE, (void *)&intVal);
	pmib->dot11RFEntry.kfree_enable= intVal;
#endif	
	apmib_get(MIB_HW_11N_TRSWITCH, (void *)&intVal);
	pmib->dot11RFEntry.trswitch = intVal;	
	
	apmib_get(MIB_HW_11N_TRSWPAPE_C9, (void *)&intVal);
	pmib->dot11RFEntry.trsw_pape_C9 = intVal;

	apmib_get(MIB_HW_11N_TRSWPAPE_CC, (void *)&intVal);
	pmib->dot11RFEntry.trsw_pape_CC = intVal;

	apmib_get(MIB_HW_11N_TARGET_PWR, (void *)&intVal);
	pmib->dot11RFEntry.target_pwr = intVal;
	
	//apmib_get(MIB_HW_11N_PA_TYPE, (void *)&intVal);
	//pmib->dot11RFEntry.pa_type = intVal;

	if (pmib->dot11RFEntry.dot11RFType == 10) { // Zebra
		apmib_get(MIB_WLAN_RFPOWER_SCALE, (void *)&intVal);
		if(intVal == 1)
			intVal = 3;
		else if(intVal == 2)
				intVal = 6;
			else if(intVal == 3)
					intVal = 9;
				else if(intVal == 4)
						intVal = 17;
		if (intVal) {
			for (i=0; i<MAX_2G_CHANNEL_NUM_MIB; i++) {
				if(pmib->dot11RFEntry.pwrlevelCCK_A[i] != 0){ 
					if ((pmib->dot11RFEntry.pwrlevelCCK_A[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevelCCK_A[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevelCCK_A[i] = 1;
				}
				if(pmib->dot11RFEntry.pwrlevelCCK_B[i] != 0){ 
					if ((pmib->dot11RFEntry.pwrlevelCCK_B[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevelCCK_B[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevelCCK_B[i] = 1;
				}
#ifdef CONFIG_WLAN_HAL_8814AE
				if(pmib->dot11RFEntry.pwrlevelCCK_C[i] != 0){ 
					if ((pmib->dot11RFEntry.pwrlevelCCK_C[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevelCCK_C[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevelCCK_C[i] = 1;
				}
				if(pmib->dot11RFEntry.pwrlevelCCK_D[i] != 0){ 
					if ((pmib->dot11RFEntry.pwrlevelCCK_D[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevelCCK_D[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevelCCK_D[i] = 1;
				}
#endif
				if(pmib->dot11RFEntry.pwrlevelHT40_1S_A[i] != 0){ 
					if ((pmib->dot11RFEntry.pwrlevelHT40_1S_A[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevelHT40_1S_A[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevelHT40_1S_A[i] = 1;
				}
				if(pmib->dot11RFEntry.pwrlevelHT40_1S_B[i] != 0){ 
					if ((pmib->dot11RFEntry.pwrlevelHT40_1S_B[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevelHT40_1S_B[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevelHT40_1S_B[i] = 1;
				}
#ifdef CONFIG_WLAN_HAL_8814AE
				if(pmib->dot11RFEntry.pwrlevelHT40_1S_C[i] != 0){ 
					if ((pmib->dot11RFEntry.pwrlevelHT40_1S_C[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevelHT40_1S_C[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevelHT40_1S_C[i] = 1;
				}
				if(pmib->dot11RFEntry.pwrlevelHT40_1S_D[i] != 0){ 
					if ((pmib->dot11RFEntry.pwrlevelHT40_1S_D[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevelHT40_1S_D[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevelHT40_1S_D[i] = 1;
				}
#endif
			}	
			
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_8812_SUPPORT)			
			for (i=0; i<MAX_5G_CHANNEL_NUM_MIB; i++) {
				if(pmib->dot11RFEntry.pwrlevel5GHT40_1S_A[i] != 0){ 
					if ((pmib->dot11RFEntry.pwrlevel5GHT40_1S_A[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevel5GHT40_1S_A[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevel5GHT40_1S_A[i] = 1;					
				}
				if(pmib->dot11RFEntry.pwrlevel5GHT40_1S_B[i] != 0){ 
					if ((pmib->dot11RFEntry.pwrlevel5GHT40_1S_B[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevel5GHT40_1S_B[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevel5GHT40_1S_B[i] = 1;
				}
#ifdef CONFIG_WLAN_HAL_8814AE
				if(pmib->dot11RFEntry.pwrlevel5GHT40_1S_C[i] != 0){ 
					if ((pmib->dot11RFEntry.pwrlevel5GHT40_1S_C[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevel5GHT40_1S_C[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevel5GHT40_1S_C[i] = 1;					
				}
				if(pmib->dot11RFEntry.pwrlevel5GHT40_1S_D[i] != 0){ 
					if ((pmib->dot11RFEntry.pwrlevel5GHT40_1S_D[i] - intVal) >= 1)
						pmib->dot11RFEntry.pwrlevel5GHT40_1S_D[i] -= intVal;
					else
						pmib->dot11RFEntry.pwrlevel5GHT40_1S_D[i] = 1;
				}
#endif
			}
#endif //#if defined(CONFIG_RTL_92D_SUPPORT)						
		}	
	}	
#else
//!CONFIG_RTL_8196B => rtl8651c+rtl8190
		apmib_get(MIB_HW_ANT_DIVERSITY, (void *)&intVal);
		pmib->dot11RFEntry.dot11DiversitySupport = intVal;

		apmib_get(MIB_HW_TX_ANT, (void *)&intVal);
		pmib->dot11RFEntry.defaultAntennaB = intVal;

#if 0
		apmib_get(MIB_HW_INIT_GAIN, (void *)&intVal);
		pmib->dot11RFEntry.initialGain = intVal;
#endif

		apmib_get(MIB_HW_TX_POWER_CCK, (void *)buf1);
		memcpy(pmib->dot11RFEntry.pwrlevelCCK, buf1, 14);
		
		apmib_get(MIB_HW_TX_POWER_OFDM, (void *)buf1);
		memcpy(pmib->dot11RFEntry.pwrlevelOFDM, buf1, 162);

		apmib_get(MIB_HW_11N_LOFDMPWD, (void *)&intVal);
		pmib->dot11RFEntry.legacyOFDM_pwrdiff = intVal;
		
		apmib_get(MIB_HW_11N_ANTPWD_C, (void *)&intVal);
		pmib->dot11RFEntry.antC_pwrdiff = intVal;
		
		apmib_get(MIB_HW_11N_THER_RFIC, (void *)&intVal);
		pmib->dot11RFEntry.ther_rfic = intVal;
		
		apmib_get(MIB_HW_11N_XCAP, (void *)&intVal);
		pmib->dot11RFEntry.crystalCap = intVal;

		// set output power scale
		//if (pmib->dot11RFEntry.dot11RFType == 7) { // Zebra
			if (pmib->dot11RFEntry.dot11RFType == 10) { // Zebra
			apmib_get(MIB_WLAN_RFPOWER_SCALE, (void *)&intVal);
			if(intVal == 1)
				intVal = 3;
			else if(intVal == 2)
					intVal = 6;
				else if(intVal == 3)
						intVal = 9;
					else if(intVal == 4)
							intVal = 17;
			if (intVal) {
				for (i=0; i<14; i++) {
					if(pmib->dot11RFEntry.pwrlevelCCK[i] != 0){ 
						if ((pmib->dot11RFEntry.pwrlevelCCK[i] - intVal) >= 1)
							pmib->dot11RFEntry.pwrlevelCCK[i] -= intVal;
						else
							pmib->dot11RFEntry.pwrlevelCCK[i] = 1;
					}
				}
				for (i=0; i<162; i++) {
					if (pmib->dot11RFEntry.pwrlevelOFDM[i] != 0){
						if((pmib->dot11RFEntry.pwrlevelOFDM[i] - intVal) >= 1)
							pmib->dot11RFEntry.pwrlevelOFDM[i] -= intVal;
						else
							pmib->dot11RFEntry.pwrlevelOFDM[i] = 1;
					}
				}		
			}
		}
#endif  //For Check CONFIG_RTL_8196B
		
		apmib_get(MIB_WLAN_BEACON_INTERVAL, (void *)&intVal);
		pmib->dot11StationConfigEntry.dot11BeaconPeriod = intVal;

		apmib_get(MIB_WLAN_CHANNEL, (void *)&intVal);
		pmib->dot11RFEntry.dot11channel = intVal;

		apmib_get(MIB_WLAN_RTS_THRESHOLD, (void *)&intVal);
		pmib->dot11OperationEntry.dot11RTSThreshold = intVal;
		
		apmib_get(MIB_WLAN_RETRY_LIMIT, (void *)&intVal);
		pmib->dot11OperationEntry.dot11ShortRetryLimit = intVal;

		apmib_get(MIB_WLAN_FRAG_THRESHOLD, (void *)&intVal);
		pmib->dot11OperationEntry.dot11FragmentationThreshold = intVal;

		apmib_get(MIB_WLAN_INACTIVITY_TIME, (void *)&intVal);
		pmib->dot11OperationEntry.expiretime = intVal;

		apmib_get(MIB_WLAN_PREAMBLE_TYPE, (void *)&intVal);
		pmib->dot11RFEntry.shortpreamble = intVal;

		apmib_get(MIB_WLAN_DTIM_PERIOD, (void *)&intVal);
		pmib->dot11StationConfigEntry.dot11DTIMPeriod = intVal;

		/*STBC and Coexist*/
		apmib_get(MIB_WLAN_STBC_ENABLED,(void *)&intVal);
		pmib->dot11nConfigEntry.dot11nSTBC = intVal;

		apmib_get(MIB_WLAN_LDPC_ENABLED,(void *)&intVal);
		pmib->dot11nConfigEntry.dot11nLDPC = intVal;
		

		apmib_get(MIB_WLAN_COEXIST_ENABLED,(void *)&intVal);
		pmib->dot11nConfigEntry.dot11nCoexist = intVal;
#ifdef CONFIG_IEEE80211V
		apmib_get(MIB_WLAN_DOT11V_ENABLE, (void *)&intVal);		
		pmib->wnmEntry.dot11vBssTransEnable = intVal;
#endif
#ifdef TDLS_SUPPORT
		apmib_get(MIB_WLAN_TDLS_PROHIBITED, (void *)&intVal);		
		pmib->dot11OperationEntry.tdls_prohibited = intVal;
		
		apmib_get(MIB_WLAN_TDLS_CS_PROHIBITED, (void *)&intVal);
		pmib->dot11OperationEntry.tdls_cs_prohibited = intVal;
#endif		

#ifdef FAST_BSS_TRANSITION 
		if(mode == AP_MODE  || mode == AP_MESH_MODE || mode == AP_WDS_MODE) {
			apmib_get(MIB_WLAN_ENABLE_1X, (void *)&enable1x);
			apmib_get(MIB_WLAN_ENCRYPT, (void *)&encrypt);
			
			if((enable1x != 1) && (encrypt == ENCRYPT_WPA2 || encrypt == ENCRYPT_WPA2_MIXED)) {
				apmib_get(MIB_WLAN_FT_ENABLE, (void *)&intVal);
				pmib->dot11FTEntry.dot11FastBSSTransitionEnabled = intVal;
				apmib_get(MIB_WLAN_FT_OVER_DS, (void *)&intVal);
				pmib->dot11FTEntry.dot11FTOverDSEnabled = intVal;
			}else{
				pmib->dot11FTEntry.dot11FastBSSTransitionEnabled = 0;
				pmib->dot11FTEntry.dot11FTOverDSEnabled = 0;
			}
		}
#endif
		apmib_get(MIB_WLAN_ACK_TIMEOUT,(void *)&intVal);
		pmib->miscEntry.ack_timeout = intVal;

		//### add by sen_liu 2011.3.29 TX Beamforming update to mib in 92D
		apmib_get(MIB_WLAN_TX_BEAMFORMING,(void *)&intVal);
		pmib->dot11RFEntry.txbf = intVal;

		apmib_get(MIB_WLAN_TXBF_MU,(void *)&intVal);
		pmib->dot11RFEntry.txbf_mu = intVal;
		//### end priv->pmib->dot11RFEntry.txbf
		
		// enable/disable the notification for IAPP
		apmib_get(MIB_WLAN_IAPP_DISABLED, (void *)&intVal);
		if (intVal == 0)
			pmib->dot11OperationEntry.iapp_enable = 1;
		else
			pmib->dot11OperationEntry.iapp_enable = 0;

		// set 11g protection mode
		apmib_get(MIB_WLAN_PROTECTION_DISABLED, (void *)&intVal);
		pmib->dot11StationConfigEntry.protectionDisabled = intVal;

		// set block relay
		apmib_get(MIB_WLAN_BLOCK_RELAY, (void *)&intVal);
		pmib->dot11OperationEntry.block_relay = intVal;

		// set WiFi specific mode
		apmib_get(MIB_WIFI_SPECIFIC, (void *)&intVal);
		pmib->dot11OperationEntry.wifi_specific = intVal;

		// Set WDS
		apmib_get(MIB_WLAN_WDS_ENABLED, (void *)&intVal);
		apmib_get(MIB_WLAN_WDS_NUM, (void *)&intVal2);
		pmib->dot11WdsInfo.wdsNum = 0;
#ifdef MBSSID 
		if (v_previous > 0) 
			intVal = 0;
#endif
		if (((mode == 2) || (mode == 3)) &&
			(intVal != 0) &&
			(intVal2 != 0)) {
			for (i=0; i<intVal2; i++) {
				buf1[0] = i+1;
				apmib_get(MIB_WLAN_WDS, (void *)buf1);
				pwds_EntryUI = (WDS_Tp)buf1;
				wds_Entry = &(pmib->dot11WdsInfo.entry[i]);
				memcpy(wds_Entry->macAddr, &(pwds_EntryUI->macAddr[0]), 6);
				wds_Entry->txRate = pwds_EntryUI->fixedTxRate;
				pmib->dot11WdsInfo.wdsNum++;
				sprintf((char *)buf2, "ifconfig %s-wds%d hw ether %02x%02x%02x%02x%02x%02x", ifname, i, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				system((char *)buf2);
			}
			pmib->dot11WdsInfo.wdsEnabled = intVal;
		}
		else
			pmib->dot11WdsInfo.wdsEnabled = 0;

		if (((mode == 2) || (mode == 3)) &&
			(intVal != 0)) {
			apmib_get(MIB_WLAN_WDS_ENCRYPT, (void *)&intVal);
			if (intVal == 0)
				pmib->dot11WdsInfo.wdsPrivacy = 0;
			else if (intVal == 1) {
				apmib_get(MIB_WLAN_WDS_WEP_KEY, (void *)buf1);
				pmib->dot11WdsInfo.wdsPrivacy = 1;
				string_to_hex((char *)buf1, &(pmib->dot11WdsInfo.wdsWepKey[0]), 10);
			}
			else if (intVal == 2) {
				apmib_get(MIB_WLAN_WDS_WEP_KEY, (void *)buf1);
				pmib->dot11WdsInfo.wdsPrivacy = 5;
				string_to_hex((char *)buf1, &(pmib->dot11WdsInfo.wdsWepKey[0]), 26);
			}
			else if (intVal == 3){
				pmib->dot11WdsInfo.wdsPrivacy = 2;
				apmib_get(MIB_WLAN_WDS_PSK, (void *)buf1);
				strcpy((char *)pmib->dot11WdsInfo.wdsPskPassPhrase, (char *)buf1);
			}
			else{
				pmib->dot11WdsInfo.wdsPrivacy = 4;
				apmib_get(MIB_WLAN_WDS_PSK, (void *)buf1);
				strcpy((char *)pmib->dot11WdsInfo.wdsPskPassPhrase, (char *)buf1);
			}
		}

		// enable/disable the notification for IAPP
		apmib_get(MIB_WLAN_IAPP_DISABLED, (void *)&intVal);
		if (intVal == 0)
			pmib->dot11OperationEntry.iapp_enable = 1;
		else
			pmib->dot11OperationEntry.iapp_enable = 0;

#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_) // below code copy above ACL code
		// Copy Webpage setting to userspace MIB struct table
		pmib->dot1180211sInfo.mesh_acl_num = 0;
		apmib_get(MIB_WLAN_MESH_ACL_ENABLED, (void *)&intVal);
		pmib->dot1180211sInfo.mesh_acl_mode = intVal;

		if (intVal != 0) {
			apmib_get(MIB_WLAN_MESH_ACL_NUM, (void *)&intVal);
			if (intVal != 0) {
				for (i=0; i<intVal; i++) {
					buf1[0] = i+1;
					apmib_get(MIB_WLAN_MESH_ACL_ADDR, (void *)buf1);
					pAcl = (MACFILTER_T *)buf1;
					memcpy(&(pmib->dot1180211sInfo.mesh_acl_addr[i][0]), &(pAcl->macAddr[0]), 6);
					pmib->dot1180211sInfo.mesh_acl_num++;
				}
			}
		}
#endif

		// set nat2.5 disable when client and mac clone is set
		apmib_get(MIB_WLAN_MACCLONE_ENABLED, (void *)&intVal);
		if ((intVal == 1) && (mode == 1)) {
			// let Nat25 and CloneMacAddr can use concurrent
			//pmib->ethBrExtInfo.nat25_disable = 1;
			pmib->ethBrExtInfo.macclone_enable = 1;
		}
		else {
			// let Nat25 and CloneMacAddr can use concurrent		
			//pmib->ethBrExtInfo.nat25_disable = 0;
			pmib->ethBrExtInfo.macclone_enable = 0;
		}		

		// set nat2.5 disable and macclone disable when wireless isp mode
		apmib_get(MIB_OP_MODE, (void *)&intVal);
		if (intVal == 2) {
			pmib->ethBrExtInfo.nat25_disable = 0;	// enable nat25 for ipv6-passthru ping6 fail issue at wisp mode && wlan client mode .
			pmib->ethBrExtInfo.macclone_enable = 0;
		}


#ifdef WLAN_HS2_CONFIG
		// enable/disable the notification for IAPP
			apmib_get(MIB_WLAN_HS2_ENABLE, (void *)&intVal);
			if (intVal == 0)
				pmib->hs2Entry.hs_enable = 0;
			else
				pmib->hs2Entry.hs_enable = 1;
#endif


	// for 11n
		apmib_get(MIB_WLAN_CHANNEL_BONDING, &channel_bound);
		pmib->dot11nConfigEntry.dot11nUse40M = channel_bound;
		apmib_get(MIB_WLAN_CONTROL_SIDEBAND, &intVal);
		if(channel_bound ==0){
			pmib->dot11nConfigEntry.dot11n2ndChOffset = 0;
		}else {
			if(intVal == 0 )
				pmib->dot11nConfigEntry.dot11n2ndChOffset = 1;
			if(intVal == 1 )
				pmib->dot11nConfigEntry.dot11n2ndChOffset = 2;	
#ifdef CONFIG_RTL_8812_SUPPORT
			apmib_get(MIB_WLAN_CHANNEL, (void *)&intVal);
			if(intVal > 14)
			{
				printf("!!! adjust 5G 2ndoffset for 8812 !!!\n");//eric_pf3
			if(intVal==36 || intVal==44 || intVal==52 || intVal==60
				|| intVal==100 || intVal==108 || intVal==116 || intVal==124
				|| intVal==132 || intVal==140 || intVal==149 || intVal==157
				|| intVal==165 || intVal==173)
				pmib->dot11nConfigEntry.dot11n2ndChOffset = 2;
			else
				pmib->dot11nConfigEntry.dot11n2ndChOffset = 1;
			}
#if 0
			else
			{
				apmib_get(MIB_WLAN_BAND, (void *)&intVal);
				wlan_band = intVal;
				if(wlan_band == 75)
				{
					printf("!!! adjust 2.4G AC mode 2ndoffset for 8812 !!!\n");//ac2g
					if(intVal==1 || intVal==9)
						pmib->dot11nConfigEntry.dot11n2ndChOffset = 2;
					else
						pmib->dot11nConfigEntry.dot11n2ndChOffset = 1;
				}
			}
#endif
#endif
		}
		apmib_get(MIB_WLAN_SHORT_GI, &intVal);
		pmib->dot11nConfigEntry.dot11nShortGIfor20M = intVal;
		pmib->dot11nConfigEntry.dot11nShortGIfor40M = intVal;
		pmib->dot11nConfigEntry.dot11nShortGIfor80M = intVal;
		/*
		apmib_get(MIB_WLAN_11N_STBC, &intVal);
		pmib->dot11nConfigEntry.dot11nSTBC = intVal;
		apmib_get(MIB_WLAN_11N_COEXIST, &intVal);
		pmib->dot11nConfigEntry.dot11nCoexist = intVal;
		*/
		apmib_get(MIB_WLAN_AGGREGATION, &aggregation);
		if(aggregation ==0){
			pmib->dot11nConfigEntry.dot11nAMPDU = 0;
			pmib->dot11nConfigEntry.dot11nAMSDU = 0;
		}else if(aggregation ==1){
			pmib->dot11nConfigEntry.dot11nAMPDU = 1;
			pmib->dot11nConfigEntry.dot11nAMSDU = 0;
		}else if(aggregation ==2){
			pmib->dot11nConfigEntry.dot11nAMPDU = 0;
			pmib->dot11nConfigEntry.dot11nAMSDU = 1;
		}
		else if(aggregation ==3){
			pmib->dot11nConfigEntry.dot11nAMPDU = 1;
			pmib->dot11nConfigEntry.dot11nAMSDU = 2;
		}

#if defined(CONFIG_RTL_819X) && defined(MBSSID)
		if(pmib->dot11OperationEntry.opmode & 0x00000010){// AP mode
			for (vwlan_idx = 1; vwlan_idx < 5; vwlan_idx++) {
				apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&intVal4);
				if (intVal4 == 0)
					vap_enable++;
				intVal4=0;
			}
			vwlan_idx = 0;
		}
		if (vap_enable && (mode ==  AP_MODE || mode ==  AP_WDS_MODE || mode ==  AP_MESH_MODE))	
			pmib->miscEntry.vap_enable=1;
		else
			pmib->miscEntry.vap_enable=0;
#endif
		apmib_get(MIB_WLAN_LOWEST_MLCST_RATE, &intVal);
		pmib->dot11StationConfigEntry.lowestMlcstRate = intVal;
	}

#ifdef WIFI_SIMPLE_CONFIG
	pmib->wscEntry.wsc_enable = 0;
#endif

	if (vwlan_idx != NUM_VWLAN_INTERFACE) { // not repeater interface
		apmib_get(MIB_WLAN_BASIC_RATES, (void *)&intVal);
		pmib->dot11StationConfigEntry.dot11BasicRates = intVal;

		apmib_get(MIB_WLAN_SUPPORTED_RATES, (void *)&intVal);
		pmib->dot11StationConfigEntry.dot11SupportedRates = intVal;

		apmib_get(MIB_WLAN_RATE_ADAPTIVE_ENABLED, (void *)&intVal);
		if (intVal == 0) {
			unsigned int uintVal=0;
			pmib->dot11StationConfigEntry.autoRate = 0;
			apmib_get(MIB_WLAN_FIX_RATE, (void *)&uintVal);
			pmib->dot11StationConfigEntry.fixedTxRate = uintVal;
		}
		else
			pmib->dot11StationConfigEntry.autoRate = 1;

		apmib_get(MIB_WLAN_HIDDEN_SSID, (void *)&intVal);
		pmib->dot11OperationEntry.hiddenAP = intVal;

#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_8812_SUPPORT)
		apmib_get(MIB_WLAN_PHY_BAND_SELECT, (void *)&intVal);
		pmib->dot11RFEntry.phyBandSelect = intVal;
		apmib_get(MIB_WLAN_MAC_PHY_MODE, (void *)&intVal);
		pmib->dot11RFEntry.macPhyMode = intVal;
#endif

	// set band
		apmib_get(MIB_WLAN_BAND, (void *)&intVal);
		wlan_band = intVal;
		if ((mode != 1) && (pmib->dot11OperationEntry.wifi_specific == 1) && (wlan_band == 2))
			wlan_band = 3;

		if (wlan_band == 8) { // pure-11n
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_8812_SUPPORT)
			if(pmib->dot11RFEntry.phyBandSelect == PHYBAND_5G){
				wlan_band += 4; // a+n
				pmib->dot11StationConfigEntry.legacySTADeny = 4;
			}
			else if (pmib->dot11RFEntry.phyBandSelect == PHYBAND_2G)
#endif
			{
				wlan_band += 3; // b+g+n
			pmib->dot11StationConfigEntry.legacySTADeny = 3;
			}
		}
		else if (wlan_band == 2) { // pure-11g
			wlan_band += 1; // b+g
			pmib->dot11StationConfigEntry.legacySTADeny = 1;
		}
		else if (wlan_band == 10) { // g+n
			wlan_band += 1; // b+g+n
			pmib->dot11StationConfigEntry.legacySTADeny = 1;
		}
		else if (wlan_band == 64) { // pure-11ac
			wlan_band += 12; // a+n
			pmib->dot11StationConfigEntry.legacySTADeny = 12;
		}
		else if (wlan_band == 72) { //ac+n
			wlan_band += 4; //a
			pmib->dot11StationConfigEntry.legacySTADeny = 4;
		}
		else
			pmib->dot11StationConfigEntry.legacySTADeny = 0;	

		pmib->dot11BssType.net_work_type = wlan_band;
		
		// set guest access
		apmib_get(MIB_WLAN_ACCESS, (void *)&intVal);
		pmib->dot11OperationEntry.guest_access = intVal;

		// set WMM
		apmib_get(MIB_WLAN_WMM_ENABLED, (void *)&intVal);
		pmib->dot11QosEntry.dot11QosEnable = intVal;		
		
		apmib_get(MIB_WLAN_UAPSD_ENABLED, (void *)&intVal);
		pmib->dot11QosEntry.dot11QosAPSD = intVal;		
	}

	apmib_get(MIB_WLAN_AUTH_TYPE, (void *)&intVal);
	apmib_get(MIB_WLAN_ENCRYPT, (void *)&encrypt);
#ifdef CONFIG_RTL_WAPI_SUPPORT
	/*wapi is independed. disable WAPI first if not WAPI*/
	if(7 !=encrypt)
	{
		pmib->wapiInfo.wapiType=0;	
	}
#endif
	if ((intVal == 1) && (encrypt != 1)) {
		// shared-key and not WEP enabled, force to open-system
		intVal = 0;
	}
	pmib->dot1180211AuthEntry.dot11AuthAlgrthm = intVal;

	if (encrypt == 0)
		pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 0;
	else if (encrypt == 1) {
		// WEP mode
		apmib_get(MIB_WLAN_WEP, (void *)&wep);
		if (wep == 1) {
			pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 1;
			apmib_get(MIB_WLAN_WEP64_KEY1, (void *)buf1);
			memcpy(&(pmib->dot11DefaultKeysTable.keytype[0]), buf1, 5);
			apmib_get(MIB_WLAN_WEP64_KEY2, (void *)buf1);
			memcpy(&(pmib->dot11DefaultKeysTable.keytype[1]), buf1, 5);
			apmib_get(MIB_WLAN_WEP64_KEY3, (void *)buf1);
			memcpy(&(pmib->dot11DefaultKeysTable.keytype[2]), buf1, 5);
			apmib_get(MIB_WLAN_WEP64_KEY4, (void *)buf1);
			memcpy(&(pmib->dot11DefaultKeysTable.keytype[3]), buf1, 5);
			apmib_get(MIB_WLAN_WEP_DEFAULT_KEY, (void *)&intVal);
			pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex = intVal;
		}
		else {
			pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 5;
			apmib_get(MIB_WLAN_WEP128_KEY1, (void *)buf1);
			memcpy(&(pmib->dot11DefaultKeysTable.keytype[0]), buf1, 13);
			apmib_get(MIB_WLAN_WEP128_KEY2, (void *)buf1);
			memcpy(&(pmib->dot11DefaultKeysTable.keytype[1]), buf1, 13);
			apmib_get(MIB_WLAN_WEP128_KEY3, (void *)buf1);
			memcpy(&(pmib->dot11DefaultKeysTable.keytype[2]), buf1, 13);
			apmib_get(MIB_WLAN_WEP128_KEY4, (void *)buf1);
			memcpy(&(pmib->dot11DefaultKeysTable.keytype[3]), buf1, 13);
			apmib_get(MIB_WLAN_WEP_DEFAULT_KEY, (void *)&intVal);
			pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex = intVal;
		}
	}
#ifdef CONFIG_RTL_WAPI_SUPPORT	
	else if(7 == encrypt)
	{
		pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 7;
		pmib->dot1180211AuthEntry.dot11AuthAlgrthm = 0;
	}
#endif	
	else {
		// WPA mode
		pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 2;
	}

#ifndef CONFIG_RTL8196B_TLD
#ifdef MBSSID
	if (vwlan_idx > 0 && pmib->dot11OperationEntry.guest_access)
		pmib->dot11OperationEntry.block_relay = 1;	
#endif
#endif

#ifdef CONFIG_IEEE80211W
			apmib_get(MIB_WLAN_IEEE80211W,(void *)&intVal);
			pmib->dot1180211AuthEntry.dot11IEEE80211W = intVal;
			apmib_get(MIB_WLAN_SHA256_ENABLE,(void *)&intVal);
			pmib->dot1180211AuthEntry.dot11EnableSHA256 = intVal;
#endif


	// Set 802.1x flag
	enable1x = 0;
	apmib_get(MIB_WLAN_ENABLE_1X, (void *)&intVal);
	if (encrypt < 2) {
		apmib_get(MIB_WLAN_MAC_AUTH_ENABLED, (void *)&intVal2);
		if ((intVal != 0) || (intVal2 != 0))
			enable1x = 1;
	}
#ifdef CONFIG_RTL_WAPI_SUPPORT
	else if(encrypt == 7)
	{
		/*wapi*/
		enable1x = 0;
	}
#endif	
	else
		enable1x = intVal;

	pmib->dot118021xAuthEntry.dot118021xAlgrthm = enable1x;
	apmib_get(MIB_WLAN_ACCOUNT_RS_ENABLED, (void *)&intVal);
	pmib->dot118021xAuthEntry.acct_enabled = intVal;

#ifdef CONFIG_RTL_WAPI_SUPPORT
	if(7 == encrypt)
	{
		//apmib_get(MIB_WLAN_WAPI_ASIPADDR,);
		apmib_get(MIB_WLAN_WAPI_AUTH,(void *)&intVal);
		pmib->wapiInfo.wapiType=intVal;

		apmib_get(MIB_WLAN_WAPI_MCAST_PACKETS,(void *)&intVal);
		pmib->wapiInfo.wapiUpdateMCastKeyPktNum=intVal;
		
		apmib_get(MIB_WLAN_WAPI_MCASTREKEY,(void *)&intVal);
		pmib->wapiInfo.wapiUpdateMCastKeyType=intVal;

		apmib_get(MIB_WLAN_WAPI_MCAST_TIME,(void *)&intVal);
		pmib->wapiInfo.wapiUpdateMCastKeyTimeout=intVal;

		apmib_get(MIB_WLAN_WAPI_UCAST_PACKETS,(void *)&intVal);
		pmib->wapiInfo.wapiUpdateUCastKeyPktNum=intVal;
		
		apmib_get(MIB_WLAN_WAPI_UCASTREKEY,(void *)&intVal);
		pmib->wapiInfo.wapiUpdateUCastKeyType=intVal;

		apmib_get(MIB_WLAN_WAPI_UCAST_TIME,(void *)&intVal);
		pmib->wapiInfo.wapiUpdateUCastKeyTimeout=intVal;

		/*1: hex  -else passthru*/
		apmib_get(MIB_WLAN_WAPI_PSK_FORMAT,(void *)&intVal2);
		apmib_get(MIB_WLAN_WAPI_PSKLEN,(void *)&intVal);
		apmib_get(MIB_WLAN_WAPI_PSK,(void *)buf1);
		pmib->wapiInfo.wapiPsk.len=intVal;
		if(1 == intVal2 )
		{
			/*hex*/	
			string_to_hex(buf1, buf2, pmib->wapiInfo.wapiPsk.len*2);
		}else
		{
			/*passthru*/
			strcpy(buf2,buf1);
		}
		memcpy(pmib->wapiInfo.wapiPsk.octet,buf2,pmib->wapiInfo.wapiPsk.len);
	}
#endif

	// for QoS control
	{	
		#define GBWC_MODE_DISABLE			0
		#define GBWC_MODE_LIMIT_MAC_INNER	1 // limit bw by mac address
		#define GBWC_MODE_LIMIT_MAC_OUTTER	2 // limit bw by excluding the mac
		#define GBWC_MODE_LIMIT_IF_TX		3 // limit bw by interface tx
		#define GBWC_MODE_LIMIT_IF_RX		4 // limit bw by interface rx
		#define GBWC_MODE_LIMIT_IF_TRX		5 // limit bw by interface tx/rx

		int tx_bandwidth, rx_bandwidth, qbwc_mode = GBWC_MODE_DISABLE;
		apmib_get(MIB_WLAN_TX_RESTRICT, (void *)&tx_bandwidth);
		apmib_get(MIB_WLAN_RX_RESTRICT, (void *)&rx_bandwidth);
		if (tx_bandwidth && rx_bandwidth == 0)
			qbwc_mode = GBWC_MODE_LIMIT_IF_TX;
		else if (tx_bandwidth == 0 && rx_bandwidth)
			qbwc_mode = GBWC_MODE_LIMIT_IF_RX;			
		else if (tx_bandwidth && rx_bandwidth)
			qbwc_mode = GBWC_MODE_LIMIT_IF_TRX;

		pmib->gbwcEntry.GBWCMode = qbwc_mode;
		pmib->gbwcEntry.GBWCThrd_tx = tx_bandwidth*1024;
		pmib->gbwcEntry.GBWCThrd_rx = rx_bandwidth*1024;		
	}

#ifdef STA_CONTROL
    pmib->staControl.stactrl_enable = 0;
    if ((mode == AP_MODE || mode == AP_MESH_MODE || mode == AP_WDS_MODE)) {                 
        apmib_get(MIB_WLAN_STACTRL_ENABLE,(void *)&intVal);
        if(intVal) {
            apmib_get(MIB_WLAN_SSID, (void *)buf1);
            apmib_save_wlanIdx();
            /*get the other band's mib*/
            wlan_idx = wlan_idx?0:1;     
            apmib_get(MIB_WLAN_SSID, (void *)buf2);
            apmib_recov_wlanIdx();

			/* shift SSID check to wlan driver */
            //if(strcmp(buf1, buf2) == 0)
			{     
                pmib->staControl.stactrl_enable = 1;
                pmib->staControl.stactrl_groupID = vwlan_idx;
                apmib_get(MIB_WLAN_STACTRL_PREFER,(void *)&intVal);
                pmib->staControl.stactrl_prefer_band = intVal;
            }
        }
    }
#endif

#ifdef RTK_SMART_ROAMING
	pmib->sr_profile.enable = 0;

	apmib_get(MIB_CAPWAP_MODE,(void *)&intVal);
	if(intVal) {
		pmib->sr_profile.enable = 1;
	}

#endif

#ifdef RTK_CROSSBAND_REPEATER
	pmib->crossBand.crossband_enable = 0;

	apmib_save_wlanIdx();	
	wlan_idx = 0;
	apmib_get(MIB_WLAN_CROSSBAND_ACTIVATE, (void *)&intVal);
	apmib_get(MIB_WLAN_CROSSBAND_ENABLE, (void *)&intVal2);
	apmib_recov_wlanIdx();
	
	if ((mode == AP_MODE || mode == AP_MESH_MODE || mode == AP_WDS_MODE)){
		if(intVal && intVal2)
		{
			pmib->crossBand.crossband_enable = 1; //setmib to enable crossband in driver			
		}
	}
#endif

#if defined(DOT11K)
    pmib->dot11StationConfigEntry.dot11RadioMeasurementActivated = 0;
    /*if Fast BSS Transition is enabled,  activate 11k and set 11K neighbor report*/
    apmib_get(MIB_WLAN_DOT11K_ENABLE,(void*)&intVal);
    if(intVal && (mode == AP_MODE || mode == AP_MESH_MODE || mode == AP_WDS_MODE)) {    
        /*delete all neightbor report first*/
        sprintf(buf2, "echo delall > "NEIGHBOR_REPORT_FILE, ifname); 
        system(buf2);
        /*activate 11k*/
        pmib->dot11StationConfigEntry.dot11RadioMeasurementActivated = 1; 
    }
#endif

#ifdef CONFIG_RTK_MESH

#ifdef CONFIG_NEW_MESH_UI
	//new feature:Mesh enable/disable
	//brian add new key:MIB_MESH_ENABLE
	pmib->dot1180211sInfo.meshSilence = 0;

	apmib_get(MIB_WLAN_MESH_ENABLE,(void *)&intVal);
	if (mode == AP_MESH_MODE || mode == MESH_MODE)
	{
		if( intVal )
			pmib->dot1180211sInfo.mesh_enable = 1;
		else
			pmib->dot1180211sInfo.mesh_enable = 0;
	}
	else
		pmib->dot1180211sInfo.mesh_enable = 0;

	// set mesh argument
	// brian change to shutdown portal/root as default
	if (mode == AP_MESH_MODE)
	{
		pmib->dot1180211sInfo.mesh_ap_enable = 1;
		pmib->dot1180211sInfo.mesh_portal_enable = 0;
	}
	else if (mode == MESH_MODE)
	{
		if( !intVal )
			//pmib->dot11OperationEntry.opmode += 64; // WIFI_MESH_STATE = 0x00000040
			pmib->dot1180211sInfo.meshSilence = 1;

		pmib->dot1180211sInfo.mesh_ap_enable = 0;
		pmib->dot1180211sInfo.mesh_portal_enable = 0;		
	}
	else
	{
		pmib->dot1180211sInfo.mesh_ap_enable = 0;
		pmib->dot1180211sInfo.mesh_portal_enable = 0;	
	}
	#if 0	//by brian, dont enable root by default
	apmib_get(MIB_WLAN_MESH_ROOT_ENABLE, (void *)&intVal);
	pmib->dot1180211sInfo.mesh_root_enable = intVal;
	#else
	pmib->dot1180211sInfo.mesh_root_enable = 0;
	#endif
#else
	if (mode == AP_MPP_MODE)
	{
		pmib->dot1180211sInfo.mesh_enable = 1;
		pmib->dot1180211sInfo.mesh_ap_enable = 1;
		pmib->dot1180211sInfo.mesh_portal_enable = 1;	
	}
	else if (mode == MPP_MODE)
	{
		pmib->dot1180211sInfo.mesh_enable = 1;
		pmib->dot1180211sInfo.mesh_ap_enable = 0;
		pmib->dot1180211sInfo.mesh_portal_enable = 1;
	}
	else if (mode == MAP_MODE)
	{
		pmib->dot1180211sInfo.mesh_enable = 1;
		pmib->dot1180211sInfo.mesh_ap_enable = 1;
		pmib->dot1180211sInfo.mesh_portal_enable = 0;
	}		
	else if (mode == MP_MODE)
	{
		pmib->dot1180211sInfo.mesh_enable = 1;
		pmib->dot1180211sInfo.mesh_ap_enable = 0;
		pmib->dot1180211sInfo.mesh_portal_enable = 0;		
	}
	else
	{
		pmib->dot1180211sInfo.mesh_enable = 0;
		pmib->dot1180211sInfo.mesh_ap_enable = 0;
		pmib->dot1180211sInfo.mesh_portal_enable = 0;	
	}

	apmib_get(MIB_WLAN_MESH_ROOT_ENABLE, (void *)&intVal);
	pmib->dot1180211sInfo.mesh_root_enable = intVal;
#endif
	pmib->dot1180211sInfo.mesh_max_neightbor = 16;
	apmib_get(MIB_SCRLOG_ENABLED, (void *)&intVal);
	pmib->dot1180211sInfo.log_enabled = intVal;

	apmib_get(MIB_WLAN_MESH_ID, (void *)buf1);
	intVal2 = strlen(buf1);
	memset(pmib->dot1180211sInfo.mesh_id, 0, 32);
	memcpy(pmib->dot1180211sInfo.mesh_id, buf1, intVal2);

	apmib_get(MIB_WLAN_MESH_ENCRYPT, (void *)&intVal);
	apmib_get(MIB_WLAN_MESH_WPA_AUTH, (void *)&intVal2);
	if( intVal2 == 2 && intVal) {
		pmib->dot11sKeysTable.dot11Privacy  = 4;
   		apmib_get(MIB_WLAN_MESH_WPA_PSK, (void *)buf1);
		strcpy((char *)pmib->dot1180211sInfo.dot11PassPhrase, (char *)buf1);
    }
	else
		pmib->dot11sKeysTable.dot11Privacy  = 0;

#ifdef RTK_MESH_METRIC_REFINE
	apmib_get(MIB_WLAN_MESH_CROSSBAND_ENABLED, (void *)&intVal);
	if(pmib->dot1180211sInfo.mesh_enable && intVal) //keith-mesh crossband
		pmib->meshPathsel.mesh_crossbandEnable = 1;
	else
		pmib->meshPathsel.mesh_crossbandEnable = 0;
#endif

#endif // CONFIG_RTK_MESH

	// When using driver base WPA, set wpa setting to driver
#if 1
	int intVal3;
	apmib_get(MIB_WLAN_WPA_AUTH, (void *)&intVal3);
//#ifdef CONFIG_RTL_8196B
// button 2009.05.21
#if 1
	if ((intVal3 & WPA_AUTH_PSK) && encrypt >= 2 
#ifdef CONFIG_RTL_WAPI_SUPPORT
&& encrypt < 7
#endif
)
#else
	if (mode != 1 && (intVal3 & WPA_AUTH_PSK) && encrypt >= 2 
#ifdef CONFIG_RTL_WAPI_SUPPORT
&& encrypt < 7
#endif
)
#endif
	{
		if (encrypt == 2)
			intVal = 1;
		else if (encrypt == 4)
			intVal = 2;
		else if (encrypt == 6)
			intVal = 3;
		else {
			printf("invalid ENCRYPT value!\n");
			return -1;
		}
		pmib->dot1180211AuthEntry.dot11EnablePSK = intVal;

		apmib_get(MIB_WLAN_WPA_PSK, (void *)buf1);
		strcpy((char *)pmib->dot1180211AuthEntry.dot11PassPhrase, (char *)buf1);

		apmib_get(MIB_WLAN_WPA_GROUP_REKEY_TIME, (void *)&intVal);
		pmib->dot1180211AuthEntry.dot11GKRekeyTime = intVal;			
	}
	else		
		pmib->dot1180211AuthEntry.dot11EnablePSK = 0;


#ifdef CONFIG_RTL_P2P_SUPPORT
    if(mode == P2P_SUPPORT_MODE) {
        apmib_get(MIB_WLAN_WPA_GROUP_REKEY_TIME, (void *)&intVal);
        pmib->dot1180211AuthEntry.dot11GKRekeyTime = intVal;	
    }
#endif 

#if 1
if (intVal3 != 0 && encrypt >= 2 
#ifdef CONFIG_RTL_WAPI_SUPPORT
&& encrypt < 7
#endif
)
#else
	if (mode != 1 && intVal3 != 0 && encrypt >= 2 
#ifdef CONFIG_RTL_WAPI_SUPPORT
&& encrypt < 7
#endif
)
#endif
	{
		if (encrypt == 2 || encrypt == 6) {
			apmib_get(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&intVal2);
			if (intVal2 == 1)
				intVal = 2;
			else if (intVal2 == 2)
				intVal = 8;
			else if (intVal2 == 3)
				intVal = 10;
			else {
				printf("invalid WPA_CIPHER_SUITE value!\n");
				return -1;
			}
			pmib->dot1180211AuthEntry.dot11WPACipher = intVal;			
		}
		
		if (encrypt == 4 || encrypt == 6) {
			apmib_get(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&intVal2);
			if (intVal2 == 1)
				intVal = 2;
			else if (intVal2 == 2)
				intVal = 8;
			else if (intVal2 == 3)
				intVal = 10;
			else {
				printf("invalid WPA2_CIPHER_SUITE value!\n");
				return -1;
			}
			pmib->dot1180211AuthEntry.dot11WPA2Cipher = intVal;			
		}
		if (encrypt == 6){
			pmib->dot1180211AuthEntry.dot11WPACipher = 10;			
			pmib->dot1180211AuthEntry.dot11WPA2Cipher = 10;
		}
	}
#endif

#ifdef CONFIG_APP_SIMPLE_CONFIG
	apmib_get(MIB_WLAN_MODE, (void *)&intVal);
	if(intVal != CLIENT_MODE)
		pmib->dot11StationConfigEntry.sc_enabled = 0;
	apmib_get(MIB_SC_DEVICE_TYPE, (void *)&intVal);
	pmib->dot11StationConfigEntry.sc_device_type = intVal;
	apmib_get(MIB_SC_DEVICE_NAME, (void *)buf1);
	strcpy((char *)pmib->dot11StationConfigEntry.sc_device_name, (char *)buf1);
	apmib_get(MIB_HW_WSC_PIN, (void *)buf1);
	strcpy((char *)pmib->dot11StationConfigEntry.sc_pin, (char *)buf1);
	//apmib_get(MIB_HW_SC_DEFAULT_PIN, (void *)buf1);
	strcpy((char *)pmib->dot11StationConfigEntry.sc_default_pin, sc_default_pin);
	apmib_get(MIB_WLAN_SC_PASSWD, (void *)buf1);
	strcpy((char *)pmib->dot11StationConfigEntry.sc_passwd, (char *)buf1);
	apmib_get(MIB_WLAN_SC_SYNC_PROFILE, (void *)&intVal);
#ifdef SYNC_VXD_PROFILE
	intVal = 1;
#endif
	//if(intVal !=0 )
	//	intVal++;
	pmib->dot11StationConfigEntry.sc_sync_vxd_to_root = intVal;
	
	apmib_get(MIB_WLAN_SC_PIN_ENABLED, (void *)&intVal);
	pmib->dot11StationConfigEntry.sc_pin_enabled = intVal;
#endif

	apmib_get(MIB_WLAN_COUNTRY_STRING, (void *)&buf1);
	if(buf1[0]!='\0'){
		memcpy(pmib->dot11dCountry.dot11CountryString, buf1, 3);
	}

#ifdef CONFIG_RTL_COMAPI_CFGFILE 
     dumpCfgFile(ifname, pmib, vwlan_idx);
#else
	wrq.u.data.pointer = (caddr_t)pmib;
	wrq.u.data.length = sizeof(struct wifi_mib);
	if (ioctl(skfd, 0x8B43, &wrq) < 0) {
		printf("Set WLAN MIB failed!\n");
		free(pmib);
		close(skfd);
		return -1;
	}
#endif
	close(skfd);

#if 0  //if anyone want to open this part, please create a new function.
//#ifdef UNIVERSAL_REPEATER
	// set repeater interface
	if (!strcmp(ifname, "wlan0")) {
		apmib_get(MIB_REPEATER_ENABLED1, (void *)&intVal);
		apmib_get(MIB_WLAN_NETWORK_TYPE, (void *)&intVal2);		
		system("ifconfig wlan0-vxd down");
		if (intVal != 0 && mode != WDS_MODE && 
				!(mode==CLIENT_MODE && intVal2==ADHOC)) {
			skfd = socket(AF_INET, SOCK_DGRAM, 0);
			strncpy(wrq.ifr_name, "wlan0-vxd", IFNAMSIZ);
			if (ioctl(skfd, SIOCGIWNAME, &wrq) < 0) {
				printf("Interface open failed!\n");
				free(pmib);
				close(skfd);
				return -1;
			}

			wrq.u.data.pointer = (caddr_t)pmib;
			wrq.u.data.length = sizeof(struct wifi_mib);
			if (ioctl(skfd, 0x8B42, &wrq) < 0) {
				printf("Get WLAN MIB failed!\n");
				free(pmib);
				close(skfd);
				return -1;
			}

			apmib_get(MIB_REPEATER_SSID1, (void *)buf1);
			intVal2 = strlen(buf1);
			pmib->dot11StationConfigEntry.dot11DesiredSSIDLen = intVal2;
			memset(pmib->dot11StationConfigEntry.dot11DesiredSSID, 0, 32);
			memcpy(pmib->dot11StationConfigEntry.dot11DesiredSSID, buf1, intVal2);

			sprintf(buf1, "ifconfig %s-vxd hw ether %02x%02x%02x%02x%02x%02x", ifname, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			system(buf1);

			enable1xVxd = 0;
			if (encrypt == 0)
				pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 0;
			else if (encrypt == 1) {
				if (enable1x == 0) {
					if (wep == 1)
						pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 1;
					else
						pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 5;
				}
				else
					pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 0;
			}
			else {
				apmib_get(MIB_WLAN_WPA_AUTH, (void *)&intVal2);
				if (intVal2 == 2) {
					pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 2;
					enable1xVxd = 1;
				}
				else
					pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 0;
			}
			pmib->dot118021xAuthEntry.dot118021xAlgrthm = enable1xVxd;
			
			wrq.u.data.pointer = (caddr_t)pmib;
			wrq.u.data.length = sizeof(struct wifi_mib);
			if (ioctl(skfd, 0x8B43, &wrq) < 0) {
				printf("Set WLAN MIB failed!\n");
				free(pmib);
				close(skfd);
				return -1;
			}
			close(skfd);
		}
	}

	if (!strcmp(ifname, "wlan1")) {
		apmib_get(MIB_REPEATER_ENABLED1, (void *)&intVal);
		system("ifconfig wlan1-vxd down");
		if (intVal != 0) {
			skfd = socket(AF_INET, SOCK_DGRAM, 0);
			strncpy(wrq.ifr_name, "wlan1-vxd", IFNAMSIZ);
			if (ioctl(skfd, SIOCGIWNAME, &wrq) < 0) {
				printf("Interface open failed!\n");
				free(pmib);
				close(skfd);
				return -1;
			}

			wrq.u.data.pointer = (caddr_t)pmib;
			wrq.u.data.length = sizeof(struct wifi_mib);
			if (ioctl(skfd, 0x8B42, &wrq) < 0) {
				printf("Get WLAN MIB failed!\n");
				free(pmib);
				close(skfd);
				return -1;
			}

			apmib_get(MIB_REPEATER_SSID1, (void *)buf1);
			intVal2 = strlen(buf1);
			pmib->dot11StationConfigEntry.dot11DesiredSSIDLen = intVal2;
			memset(pmib->dot11StationConfigEntry.dot11DesiredSSID, 0, 32);
			memcpy(pmib->dot11StationConfigEntry.dot11DesiredSSID, buf1, intVal2);

			sprintf(buf1, "ifconfig %s-vxd hw ether %02x%02x%02x%02x%02x%02x", ifname, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			system(buf1);

			enable1xVxd = 0;
			if (encrypt == 0)
				pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 0;
			else if (encrypt == 1) {
				if (enable1x == 0) {
					if (wep == 1)
						pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 1;
					else
						pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 5;
				}
				else
					pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 0;
			}
			else {
				apmib_get(MIB_WLAN_WPA_AUTH, (void *)&intVal2);
				if (intVal2 == 2) {
					pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 2;
					enable1xVxd = 1;
				}
				else
					pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 0;
			}
			pmib->dot118021xAuthEntry.dot118021xAlgrthm = enable1xVxd;

			wrq.u.data.pointer = (caddr_t)pmib;
			wrq.u.data.length = sizeof(struct wifi_mib);
			if (ioctl(skfd, 0x8B43, &wrq) < 0) {
				printf("Set WLAN MIB failed!\n");
				free(pmib);
				close(skfd);
				return -1;
			}
			close(skfd);
		}
	}
#endif

	free(pmib);
	return 0;
}
#endif // WLAN_FAST_INIT

#ifdef HOME_GATEWAY
#if defined(MULTI_WAN_SUPPORT)
static int generatePPPConf(int is_pppoe, char *option_file, char *pap_file, char *chap_file, char* order_file)
#else
static int generatePPPConf(int is_pppoe, char *option_file, char *pap_file, char *chap_file)
#endif
{
	FILE *fd;
	char tmpbuf[200], buf1[100], buf2[100];
	int srcIdx, dstIdx;

	if ( !apmib_init()) {
		printf("Initialize AP MIB failed!\n");
		return -1;
	}

	if (is_pppoe) {

#if defined(MULTI_WAN_SUPPORT)
		{
			int index;
			WANIFACE_T WanIfaceEntry;
			sscanf(order_file,"%d",&index);
			memset(&WanIfaceEntry, '\0', sizeof(WanIfaceEntry));
			*((char *)&WanIfaceEntry) = (char)index;
			apmib_get(MIB_WANIFACE_TBL,(void *)&WanIfaceEntry);		
			strcpy(buf1,WanIfaceEntry.pppUsername);
			strcpy(buf2,WanIfaceEntry.pppPassword);
		}
#elif defined(MULTI_PPPOE)
		if(!strcmp(order_file,"FIRST")){
			apmib_get(MIB_PPP_USER_NAME, (void *)buf1);
			apmib_get(MIB_PPP_PASSWORD, (void *)buf2);
		}else if(!strcmp(order_file,"SECOND")){		
			apmib_get(MIB_PPP_USER_NAME2, (void *)buf1);
			apmib_get(MIB_PPP_PASSWORD2, (void *)buf2);
			
		}else if(!strcmp(order_file,"THIRD")){		
			apmib_get(MIB_PPP_USER_NAME3, (void *)buf1);
			apmib_get(MIB_PPP_PASSWORD3, (void *)buf2);
			
		}else if(!strcmp(order_file,"FORTH")){		
			apmib_get(MIB_PPP_USER_NAME4, (void *)buf1);
			apmib_get(MIB_PPP_PASSWORD4, (void *)buf2);
		}

#else	
		apmib_get(MIB_PPP_USER_NAME, (void *)buf1);
		apmib_get(MIB_PPP_PASSWORD, (void *)buf2);
#endif
	}
	else {
#if defined(MULTI_WAN_SUPPORT)
#if defined(SINGLE_WAN_SUPPORT)

		{
			int index;
			WANIFACE_T WanIfaceEntry;
			sscanf(order_file,"%d",&index);
			memset(&WanIfaceEntry, '\0', sizeof(WanIfaceEntry));
			*((char *)&WanIfaceEntry) = (char)index;
			apmib_get(MIB_WANIFACE_TBL,(void *)&WanIfaceEntry);		
			strcpy(buf1,WanIfaceEntry.pptpUserName);
			strcpy(buf2,WanIfaceEntry.pptpPassword);
		}
#endif
#else
		apmib_get(MIB_PPTP_USER_NAME, (void *)buf1);
		apmib_get(MIB_PPTP_PASSWORD, (void *)buf2);		
#endif
	}
	// delete '"' in the value
	for (srcIdx=0, dstIdx=0; buf1[srcIdx]; srcIdx++, dstIdx++) {
		if (buf1[srcIdx] == '"')
			tmpbuf[dstIdx++] = '\\';
		tmpbuf[dstIdx] = buf1[srcIdx];
	}
	if (dstIdx != srcIdx) {
		memcpy(buf1, tmpbuf, dstIdx);
		buf1[dstIdx] ='\0';
	}
	
	for (srcIdx=0, dstIdx=0; buf2[srcIdx]; srcIdx++, dstIdx++) {
		if (buf2[srcIdx] == '"')
			tmpbuf[dstIdx++] = '\\';
		tmpbuf[dstIdx] = buf2[srcIdx];
	}
	if (dstIdx != srcIdx) {
		memcpy(buf2, tmpbuf, dstIdx);
		buf2[dstIdx] ='\0';
	}
	
	fd = fopen(option_file, "w");
	if (fd == NULL) {
		printf("open file %s error!\n", option_file);
		return -1;
	}
	sprintf(tmpbuf, "name \"%s\"\n", buf1);
	fputs(tmpbuf, fd);
	if(strlen(buf2)>31)
	{
	  sprintf(tmpbuf, "-mschap\r\n");
		fputs(tmpbuf, fd);
		sprintf(tmpbuf, "-mschap-v2\r\n");
		fputs(tmpbuf, fd);		
	}
	fclose(fd);
	
	fd = fopen(pap_file, "w");
	if (fd == NULL) {
		printf("open file %s error!\n", pap_file);
		return -1;
	}
	fputs("#################################################\n", fd);
	sprintf(tmpbuf, "\"%s\"	*	\"%s\"\n", buf1, buf2);	
	fputs(tmpbuf, fd);

	fd = fopen(chap_file, "w");
	if (fd == NULL) {
		printf("open file %s error!\n", chap_file);
		return -1;
	}
	fputs("#################################################\n", fd);
	sprintf(tmpbuf, "\"%s\"	*	\"%s\"\n", buf1, buf2);	
	fputs(tmpbuf, fd);
	return 0;
}
#endif // HOME_GATEWAY

#ifdef WIFI_SIMPLE_CONFIG
enum { 
	MODE_AP_UNCONFIG=1, 			// AP unconfigured (enrollee)
	MODE_CLIENT_UNCONFIG=2, 		// client unconfigured (enrollee) 
	MODE_CLIENT_CONFIG=3,			// client configured (registrar) 
	MODE_AP_PROXY=4, 			// AP configured (proxy)
	MODE_AP_PROXY_REGISTRAR=5,		// AP configured (proxy and registrar)
	MODE_CLIENT_UNCONFIG_REGISTRAR=6,	// client unconfigured (registrar) 
	MODE_P2P_DEVICE=7		//  P2P_SUPPORT  for p2p_device mode
};

#define WRITE_WSC_PARAM(dst, tmp, str, val) {	\
	sprintf(tmp, str, val); \
	memcpy(dst, tmp, strlen(tmp)); \
	dst += strlen(tmp); \
}

#ifdef CONFIG_RTL_TRI_BAND_SUPPORT
#define WRITE_WSC_PARAM_EXT(dst, tmp, str, val1, val) {	\
	sprintf(tmp, str, val1, val); \
	memcpy(dst, tmp, strlen(tmp)); \
	dst += strlen(tmp); \
}
#endif

static void convert_bin_to_str(unsigned char *bin, int len, char *out)
{
	int i;
	char tmpbuf[10];

	out[0] = '\0';

	for (i=0; i<len; i++) {
		sprintf(tmpbuf, "%02x", bin[i]);
		strcat(out, tmpbuf);
	}
}

static void convert_hex_to_ascii(unsigned long code, char *out)
{
	*out++ = '0' + ((code / 10000000) % 10);  
	*out++ = '0' + ((code / 1000000) % 10);
	*out++ = '0' + ((code / 100000) % 10);
	*out++ = '0' + ((code / 10000) % 10);
	*out++ = '0' + ((code / 1000) % 10);
	*out++ = '0' + ((code / 100) % 10);
	*out++ = '0' + ((code / 10) % 10);
	*out++ = '0' + ((code / 1) % 10);
	*out = '\0';
}

static int compute_pin_checksum(unsigned long int PIN)
{
	unsigned long int accum = 0;
	int digit;
	
	PIN *= 10;
	accum += 3 * ((PIN / 10000000) % 10); 	
	accum += 1 * ((PIN / 1000000) % 10);
	accum += 3 * ((PIN / 100000) % 10);
	accum += 1 * ((PIN / 10000) % 10); 
	accum += 3 * ((PIN / 1000) % 10); 
	accum += 1 * ((PIN / 100) % 10); 
	accum += 3 * ((PIN / 10) % 10);

	digit = (accum % 10);
	return (10 - digit) % 10;
}

#ifdef CONFIG_RTL_COMAPI_CFGFILE
static int defaultWscConf(char *in, char *out)
{
	int fh;
	struct stat status;
	char *buf, *ptr;
	int intVal, intVal2, is_client, is_registrar, len, is_wep=0, wep_key_type=0, wep_transmit_key=0;
	char tmpbuf[100], tmp1[100];
		
	if ( !apmib_init()) {
		printf("Initialize AP MIB failed!\n");
		return -1;
	}

        apmib_get(MIB_WLAN_MODE, (void *)&is_client);
	if (is_client == CLIENT_MODE)
                return ;

	if (stat(in, &status) < 0) {
		printf("stat() error [%s]!\n", in);
		return -1;
	}

	buf = malloc(status.st_size+2048);
	if (buf == NULL) {
		printf("malloc() error [%d]!\n", (int)status.st_size+2048);
		return -1;		
	}

	ptr = buf;

	WRITE_WSC_PARAM(ptr, tmpbuf, "mode = %d\n", MODE_AP_UNCONFIG);
	WRITE_WSC_PARAM(ptr, tmpbuf, "upnp = 1\n", NULL);
	WRITE_WSC_PARAM(ptr, tmpbuf, "config_method = %d\n", CONFIG_METHOD_ETH | CONFIG_METHOD_PIN | CONFIG_METHOD_PBC);
	WRITE_WSC_PARAM(ptr, tmpbuf, "auth_type = 1\n", NULL);
	WRITE_WSC_PARAM(ptr, tmpbuf, "encrypt_type = 1\n", NULL);
	WRITE_WSC_PARAM(ptr, tmpbuf, "connection_type = 1\n", NULL);
	WRITE_WSC_PARAM(ptr, tmpbuf, "manual_config = 0\n", NULL);
	WRITE_WSC_PARAM(ptr, tmpbuf, "network_key = \n", NULL);
	WRITE_WSC_PARAM(ptr, tmpbuf, "ssid = \n", NULL);
    apmib_get(MIB_HW_WSC_PIN, (void *)&tmp1);	
	WRITE_WSC_PARAM(ptr, tmpbuf, "pin_code = %s\n", tmp1);
	WRITE_WSC_PARAM(ptr, tmpbuf, "rf_band = 1\n", NULL);
	WRITE_WSC_PARAM(ptr, tmpbuf, "device_name = \n", NULL);

	len = (int)(((long)ptr)-((long)buf));
	
	fh = open(in, O_RDONLY);
	if (fh == -1) {
		printf("open() error [%s]!\n", in);
		return -1;
	}

	lseek(fh, 0L, SEEK_SET);
	if (read(fh, ptr, status.st_size) != status.st_size) {		
		printf("read() error [%s]!\n", in);
		return -1;	
	}
	close(fh);

	// search UUID field, replace last 12 char with hw mac address
	ptr = strstr(ptr, "uuid =");
	if (ptr) {
		char tmp2[100];
		apmib_get(MIB_HW_NIC0_ADDR, (void *)&tmp1);
		convert_bin_to_str(tmp1, 6, tmp2);
		memcpy(ptr+27, tmp2, 12);
	}

	fh = open(out, O_RDWR|O_CREAT|O_TRUNC);
	if (fh == -1) {
		printf("open() error [%s]!\n", out);
		return -1;
	}

	if (write(fh, buf, len+status.st_size) != len+status.st_size ) {
		printf("Write() file error [%s]!\n", out);
		return -1;
	}
	close(fh);
	free(buf);

	return 0;
}
#endif


static int updateWscConf(char *in, char *out, int genpin, char *wlanif_name)
{
    int fh;
    struct stat status;
    char *buf, *ptr;
    int intVal;
    //int intVal2;
    int wlan0_mode=0;
    int wlan1_mode=0;    
    int is_config, is_registrar, len, is_wep=0, wep_key_type=0, wep_transmit_key=0;
    char tmpbuf[100], tmp1[100];

    //int is_repeater_enabled=0;
    int isUpnpEnabled=0, wsc_method = 0, wsc_auth=0, wsc_enc=0;
    int wlan_network_type=0, wsc_manual_enabled=0, wlan_wep=0;
    char wlan_wep64_key1[100], wlan_wep64_key2[100], wlan_wep64_key3[100], wlan_wep64_key4[100];
    char wlan_wep128_key1[100], wlan_wep128_key2[100], wlan_wep128_key3[100], wlan_wep128_key4[100];
    char wlan_wpa_psk[100];
    char wlan_ssid[100], device_name[100], wsc_pin[100];
    int wlan_chan_num=0, wsc_config_by_ext_reg=0;
    int three_wps_icons=0;
    P2P_DEBUG("\n\n       wlanif_name=[%s]  \n\n\n",wlanif_name);
    // 1104	
    int wlan0_wlan_disabled=0;	
    int wlan1_wlan_disabled=0;		
    int wlan0_wsc_disabled=0;
#ifdef FOR_DUAL_BAND	
    int wlan1_wsc_disabled=0;
#endif
#ifdef CONFIG_RTL_TRI_BAND_SUPPORT
	int wlan0_va0_wlan_disabled = 0;	
	int wlan0_va0_wsc_disabled = 0;
	int wlan0_va0_encrypt=0;		
    int wlan0_va0_wpa_cipher=0;
    int wlan0_va0_wpa2_cipher=0;
#endif
	/*for detial mixed mode info*/ 
#define WSC_WPA_TKIP		1
#define WSC_WPA_AES			2
#define WSC_WPA2_TKIP		4
#define WSC_WPA2_AES		8

    int wlan0_encrypt=0;		
    int wlan0_wpa_cipher=0;
    int wlan0_wpa2_cipher=0;

    int wlan1_encrypt=0;		
    int wlan1_wpa_cipher=0;
    int wlan1_wpa2_cipher=0;	
    /*for detial mixed mode info*/ 
    int band_select_5g2g;  // 0:2.4g  ; 1:5G   ; 2:both
    char *token=NULL, *token1=NULL, *savestr1=NULL;  
#ifdef CONFIG_RTL_TRI_BAND_SUPPORT
	char token2[20] = {0};
	int wlan2Disable = 0, vapDisable = 0;	
#endif
	
    if ( !apmib_init()) {
        printf("Initialize AP MIB failed!\n");
        return -1;
    }

    if(wlanif_name != NULL) {
        token = strtok_r(wlanif_name," ", &savestr1);
        if(token)
            token1 = strtok_r(NULL," ", &savestr1);
    }
    else {
        token = "wlan0";
    }	
#ifdef CONFIG_RTL_TRI_BAND_SUPPORT
	apmib_get(MIB_THREE_WPS_ICONS,&three_wps_icons);
#endif
    SetWlan_idx(token);
    apmib_get(MIB_HW_WSC_PIN, (void *)wsc_pin);  
    apmib_get(MIB_WLAN_WSC_MANUAL_ENABLED, (void *)&wsc_manual_enabled);
    apmib_get(MIB_DEVICE_NAME, (void *)&device_name);
#ifdef CONFIG_RTL_TRI_BAND_SUPPORT
	if(three_wps_icons)
	{
		if(!strcmp(wlanif_name, WSCD_VAP_INTERFACE))
		{
			strcat(device_name,"_5G_VAP");
		}
		else if(!strcmp(wlanif_name,"wlan0"))
		{
			strcat(device_name,"_5G_1");
		}
		else if(!strcmp(wlanif_name,"wlan1"))
		{
			strcat(device_name,"_5G_2");
		}else if(!strcmp(wlanif_name, WSCD_WLAN2_INTERFACE))
		{
			strcat(device_name,"_2.4G");
		}
		WSC_THREE_DEBUG("device_name(%s)\n", device_name);
	}
#endif
    SDEBUG("device_name=[%s]\n",device_name);
    apmib_get(MIB_WLAN_BAND2G5G_SELECT, (void *)&band_select_5g2g);	


    apmib_get(MIB_WLAN_NETWORK_TYPE, (void *)&wlan_network_type);    

    apmib_get(MIB_WLAN_WSC_REGISTRAR_ENABLED, (void *)&is_registrar);
    apmib_get(MIB_WLAN_WSC_METHOD, (void *)&wsc_method);
    apmib_get(MIB_WLAN_WSC_UPNP_ENABLED, (void *)&isUpnpEnabled);	
    apmib_get(MIB_WLAN_WSC_CONFIGURED, (void *)&is_config);
    apmib_get(MIB_WLAN_WSC_CONFIGBYEXTREG, (void *)&wsc_config_by_ext_reg);
    apmib_get(MIB_WLAN_MODE, (void *)&wlan0_mode);  


    
    if (genpin || !memcmp(wsc_pin, "\x0\x0\x0\x0\x0\x0\x0\x0", PIN_LEN)) {
        #include <sys/time.h>			
        struct timeval tod;
        unsigned long num;

        gettimeofday(&tod , NULL);

        apmib_get(MIB_HW_NIC0_ADDR, (void *)&tmp1);			
        tod.tv_sec += tmp1[4]+tmp1[5];		
        srand(tod.tv_sec);
        num = rand() % 10000000;
        num = num*10 + compute_pin_checksum(num);
        convert_hex_to_ascii((unsigned long)num, wsc_pin);

        apmib_set(MIB_HW_WSC_PIN, (void *)wsc_pin);
#ifdef FOR_DUAL_BAND           // 2010-10-20
        wlan_idx=1;
        apmib_set(MIB_HW_WSC_PIN, (void *)wsc_pin);
        wlan_idx=0;
#endif		
        //apmib_update(CURRENT_SETTING);		
        apmib_update(HW_SETTING);		

        printf("Generated PIN = %s\n", wsc_pin);

        if (genpin)
        return 0;
    }

    if (stat(in, &status) < 0) {
        printf("stat() error [%s]!\n", in);
        return -1;
    }

    buf = malloc(status.st_size+2048);
    if (buf == NULL) {
        printf("malloc() error [%d]!\n", (int)status.st_size+2048);
        return -1;		
    }

    ptr = buf;
		

    if (wlan0_mode == CLIENT_MODE) {
        if (is_registrar) {
            if (!is_config)
                intVal = MODE_CLIENT_UNCONFIG_REGISTRAR;
            else
                intVal = MODE_CLIENT_CONFIG;			
        }
        else
            intVal = MODE_CLIENT_UNCONFIG;
    }
    else {
        if (!is_config)
            intVal = MODE_AP_UNCONFIG;
        else
            intVal = MODE_AP_PROXY_REGISTRAR;
    }


#ifdef CONFIG_RTL_P2P_SUPPORT
	/*for *CONFIG_RTL_P2P_SUPPORT*/
    if(wlan0_mode == P2P_SUPPORT_MODE){
        //printf("(%s %d)mode=p2p_support_mode\n\n", __FUNCTION__ , __LINE__);
        int intVal2;
        apmib_get(MIB_WLAN_P2P_TYPE, (void *)&intVal2);		
        if(intVal2==P2P_DEVICE){
            WRITE_WSC_PARAM(ptr, tmpbuf, "p2pmode = %d\n", intVal2);
            intVal = MODE_CLIENT_UNCONFIG;
        }
    }
#endif


    WRITE_WSC_PARAM(ptr, tmpbuf, "mode = %d\n", intVal);


    if (wlan0_mode == CLIENT_MODE) {
#ifdef CONFIG_RTL8186_KLD_REPEATER
        if (wps_vxdAP_enabled)
            apmib_get(MIB_WSC_UPNP_ENABLED, (void *)&intVal);
        else
#endif
            intVal = 0;
    }
#ifdef CONFIG_RTL_P2P_SUPPORT
    else if(wlan0_mode == P2P_SUPPORT_MODE){
    /*when the board has two interfaces, system will issue two wscd then should consisder upnp conflict 20130927*/
        intVal = 0;
    }
#endif    
    else{
        intVal = isUpnpEnabled;
    }

    WRITE_WSC_PARAM(ptr, tmpbuf, "upnp = %d\n", intVal);    
	WRITE_WSC_PARAM(ptr, tmpbuf, "three_wps_icons = %d\n", three_wps_icons);

	intVal = wsc_method;
#ifdef CONFIG_RTL_P2P_SUPPORT
    if(wlan0_mode == P2P_SUPPORT_MODE){

        intVal = ( CONFIG_METHOD_PIN | CONFIG_METHOD_PBC | CONFIG_METHOD_DISPLAY |CONFIG_METHOD_KEYPAD);


    }else
#endif	
    {
        //Ethernet(0x2)+Label(0x4)+PushButton(0x80) Bitwise OR
        if (intVal == 1) //Pin+Ethernet
            intVal = (CONFIG_METHOD_KEYPAD | CONFIG_METHOD_VIRTUAL_PIN);
        else if (intVal == 2) //PBC+Ethernet
            intVal = (CONFIG_METHOD_KEYPAD | CONFIG_METHOD_PHYSICAL_PBC | CONFIG_METHOD_VIRTUAL_PBC);
        if (intVal == 3) //Pin+PBC+Ethernet
            intVal = (CONFIG_METHOD_KEYPAD | CONFIG_METHOD_VIRTUAL_PIN | CONFIG_METHOD_PHYSICAL_PBC | CONFIG_METHOD_VIRTUAL_PBC);
    }
	WRITE_WSC_PARAM(ptr, tmpbuf, "config_method = %d\n", intVal);

	if (wlan0_mode == CLIENT_MODE) 
	{
		if (wlan_network_type == 0)
			intVal = 1;
		else
			intVal = 2;
	}
	else
		intVal = 1;


    WRITE_WSC_PARAM(ptr, tmpbuf, "connection_type = %d\n", intVal);

    WRITE_WSC_PARAM(ptr, tmpbuf, "manual_config = %d\n", wsc_manual_enabled);

    WRITE_WSC_PARAM(ptr, tmpbuf, "pin_code = %s\n", wsc_pin);

// wlan0 wlan1 wlan0-va0 wlan2
#ifdef CONFIG_RTL_TRI_BAND_SUPPORT
	if(token){
        SetWlan_idx(token);
		WSC_THREE_DEBUG("SetWlan_idx(%s)\n", token);
    }else	
#endif 
    if(token && strstr(token, "wlan0")) {
        SetWlan_idx(token);
		WSC_THREE_DEBUG("SetWlan_idx(%s)\n", token);
    }
    else if(token1 && strstr(token1, "wlan1")) {
        SetWlan_idx(token1);
		WSC_THREE_DEBUG("SetWlan_idx(%s)\n", token1);
    }
    else{
        SetWlan_idx("wlan0");
		WSC_THREE_DEBUG("SetWlan_idx(wlan0)\n");
    }

    apmib_get(MIB_WLAN_CHANNEL, (void *)&wlan_chan_num);
    if (wlan_chan_num > 14)
        intVal = 2;
    else
        intVal = 1;

#ifdef CONFIG_RTL_TRI_BAND_SUPPORT
	if(token && strstr(token, WSCD_VAP_INTERFACE))
		intVal = 2; // 5g
#endif
		
	WSC_THREE_DEBUG("wlan_chan_num(%d), rf_band(%d)\n", wlan_chan_num, intVal);
    WRITE_WSC_PARAM(ptr, tmpbuf, "rf_band = %d\n", intVal);



    apmib_get(MIB_WLAN_WSC_AUTH, (void *)&wsc_auth);
    apmib_get(MIB_WLAN_WSC_ENC, (void *)&wsc_enc);
    apmib_get(MIB_WLAN_SSID, (void *)&wlan_ssid);	
    apmib_get(MIB_WLAN_MODE, (void *)&wlan0_mode);	
    apmib_get(MIB_WLAN_WEP, (void *)&wlan_wep);
    apmib_get(MIB_WLAN_WEP_KEY_TYPE, (void *)&wep_key_type);
    apmib_get(MIB_WLAN_WEP_DEFAULT_KEY, (void *)&wep_transmit_key);
    apmib_get(MIB_WLAN_WEP64_KEY1, (void *)&wlan_wep64_key1);
    apmib_get(MIB_WLAN_WEP64_KEY2, (void *)&wlan_wep64_key2);
    apmib_get(MIB_WLAN_WEP64_KEY3, (void *)&wlan_wep64_key3);
    apmib_get(MIB_WLAN_WEP64_KEY4, (void *)&wlan_wep64_key4);
    apmib_get(MIB_WLAN_WEP128_KEY1, (void *)&wlan_wep128_key1);
    apmib_get(MIB_WLAN_WEP128_KEY2, (void *)&wlan_wep128_key2);
    apmib_get(MIB_WLAN_WEP128_KEY3, (void *)&wlan_wep128_key3);
    apmib_get(MIB_WLAN_WEP128_KEY4, (void *)&wlan_wep128_key4);
    apmib_get(MIB_WLAN_WPA_PSK, (void *)&wlan_wpa_psk);
    apmib_get(MIB_WLAN_WSC_DISABLE, (void *)&wlan0_wsc_disabled);	// 1104
    apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&wlan0_wlan_disabled);	// 0908	

    /*for detial mixed mode info*/ 
    apmib_get(MIB_WLAN_ENCRYPT, (void *)&wlan0_encrypt);
    apmib_get(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&wlan0_wpa_cipher);
    apmib_get(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&wlan0_wpa2_cipher);
	/*for detial mixed mode info*/ 

    /*if  wlanif_name doesn't include "wlan0", disable wlan0 wsc*/
    if((token == NULL || strstr(token, "wlan0") == 0) && (token1 == NULL || strstr(token1, "wlan0") == 0)) {
        wlan0_wsc_disabled = 1;
    }

#ifdef CONFIG_RTL_TRI_BAND_SUPPORT 
	if( strcmp(token, WSCD_VAP_INTERFACE)==0 )
		 wlan0_wsc_disabled = 1;
#endif

    /* for dual band*/ 	
    if(wlan0_wlan_disabled ){
        wlan0_wsc_disabled=1 ; // if wlan0 interface is disabled ; 
    }
    /* for dual band*/ 	

    WRITE_WSC_PARAM(ptr, tmpbuf, "#=====wlan0 start==========%d\n", wlan0_wsc_disabled);    
    // 1104
    WRITE_WSC_PARAM(ptr, tmpbuf, "wlan0_wsc_disabled = %d\n", wlan0_wsc_disabled);
	WSC_THREE_DEBUG("wlan0_wsc_disabled(%d)\n", wlan0_wsc_disabled);
    WRITE_WSC_PARAM(ptr, tmpbuf, "auth_type = %d\n", wsc_auth);

    WRITE_WSC_PARAM(ptr, tmpbuf, "encrypt_type = %d\n", wsc_enc);

	/*for detial mixed mode info*/ 
	intVal=0;	
	if(wlan0_encrypt==6){	// mixed mode
		if(wlan0_wpa_cipher==1){
			intVal |= WSC_WPA_TKIP;
		}else if(wlan0_wpa_cipher==2){
			intVal |= WSC_WPA_AES;		
		}else if(wlan0_wpa_cipher==3){
			intVal |= (WSC_WPA_TKIP | WSC_WPA_AES);		
		}
		if(wlan0_wpa2_cipher==1){
			intVal |= WSC_WPA2_TKIP;
		}else if(wlan0_wpa2_cipher==2){
			intVal |= WSC_WPA2_AES;		
		}else if(wlan0_wpa2_cipher==3){
			intVal |= (WSC_WPA2_TKIP | WSC_WPA2_AES);		
		}		
	}
	WRITE_WSC_PARAM(ptr, tmpbuf, "mixedmode = %d\n", intVal);	
	//printf("[%s %d],mixedmode = 0x%02x\n", __FUNCTION__,__LINE__,intVal);	
	/*for detial mixed mode info*/ 

	if (wsc_enc == WSC_ENCRYPT_WEP)
		is_wep = 1;

	if (is_wep) { // only allow WEP in none-MANUAL mode (configured by external registrar)		
		if (wlan0_encrypt != ENCRYPT_WEP) {
			printf("WEP mismatched between WPS and host system\n");
			free(buf);
			return -1;
		}		
		if (wlan_wep <= WEP_DISABLED || wlan_wep > WEP128) {
			printf("WEP encrypt length error\n");
			free(buf);
			return -1;
		}
		
		wep_transmit_key++;
		WRITE_WSC_PARAM(ptr, tmpbuf, "wep_transmit_key = %d\n", wep_transmit_key);

		/*whatever key type is ASSIC or HEX always use String-By-Hex fromat
		;2011-0419,fixed,need patch with wscd daemon , search 2011-0419*/ 
		if (wlan_wep == WEP64) {
			sprintf(tmpbuf,"%s",wlan_wep64_key1);
			convert_bin_to_str((unsigned char *)tmpbuf, 5, tmp1);
			tmp1[10] = '\0';
			WRITE_WSC_PARAM(ptr, tmpbuf, "network_key = %s\n", tmp1);

			sprintf(tmpbuf,"%s",wlan_wep64_key2);

			convert_bin_to_str((unsigned char *)tmpbuf, 5, tmp1);
			tmp1[10] = '\0';

			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key2 = %s\n", tmp1);

			sprintf(tmpbuf,"%s",wlan_wep64_key3);

			convert_bin_to_str((unsigned char *)tmpbuf, 5, tmp1);
			tmp1[10] = '\0';

			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key3 = %s\n", tmp1);


			sprintf(tmpbuf,"%s",wlan_wep64_key4);

			convert_bin_to_str((unsigned char *)tmpbuf, 5, tmp1);
			tmp1[10] = '\0';
			
			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key4 = %s\n", tmp1);
		}
		else {
			sprintf(tmpbuf,"%s",wlan_wep128_key1);

			convert_bin_to_str((unsigned char *)tmpbuf, 13, tmp1);
			tmp1[26] = '\0';


			WRITE_WSC_PARAM(ptr, tmpbuf, "network_key = %s\n", tmp1);

			sprintf(tmpbuf,"%s",wlan_wep128_key2);

			convert_bin_to_str((unsigned char *)tmpbuf, 13, tmp1);
			tmp1[26] = '\0';

			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key2 = %s\n", tmp1);

			sprintf(tmpbuf,"%s",wlan_wep128_key3);

			convert_bin_to_str((unsigned char *)tmpbuf, 13, tmp1);
			tmp1[26] = '\0';

			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key3 = %s\n", tmp1);

			sprintf(tmpbuf,"%s",wlan_wep128_key3);

			convert_bin_to_str((unsigned char *)tmpbuf, 13, tmp1);
			tmp1[26] = '\0';

			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key4 = %s\n", tmp1);
		}
	}
	else {
		WRITE_WSC_PARAM(ptr, tmpbuf, "network_key = \"%s\"\n", wlan_wpa_psk);		
	}

	WRITE_WSC_PARAM(ptr, tmpbuf, "ssid = \"%s\"\n", wlan_ssid);
	WSC_THREE_DEBUG("ssid(%s)\n", wlan_ssid);

    WRITE_WSC_PARAM(ptr, tmpbuf, "#=====wlan0 end==========:%d\n", wlan0_wsc_disabled);


#ifdef FOR_DUAL_BAND

    /* switch to wlan1 */
    if(token && strstr(token, "wlan1")) {
        SetWlan_idx(token);
		WSC_THREE_DEBUG("SetWlan_idx: %s\n", token);
    }
    else if(token1 && strstr(token1, "wlan1")) {
        SetWlan_idx(token1);
		WSC_THREE_DEBUG("SetWlan_idx: %s\n", token1);
    }
    else {
        SetWlan_idx("wlan1");
		WSC_THREE_DEBUG("SetWlan_idx: wlan1\n");
    }

    apmib_get(MIB_WLAN_WSC_AUTH, (void *)&wsc_auth);
    apmib_get(MIB_WLAN_WSC_ENC, (void *)&wsc_enc);
    apmib_get(MIB_WLAN_SSID, (void *)&wlan_ssid);	
    apmib_get(MIB_WLAN_MODE, (void *)&wlan1_mode);	
    apmib_get(MIB_WLAN_WEP, (void *)&wlan_wep);
    apmib_get(MIB_WLAN_WEP_KEY_TYPE, (void *)&wep_key_type);
    apmib_get(MIB_WLAN_WEP_DEFAULT_KEY, (void *)&wep_transmit_key);
    apmib_get(MIB_WLAN_WEP64_KEY1, (void *)&wlan_wep64_key1);
    apmib_get(MIB_WLAN_WEP64_KEY2, (void *)&wlan_wep64_key2);
    apmib_get(MIB_WLAN_WEP64_KEY3, (void *)&wlan_wep64_key3);
    apmib_get(MIB_WLAN_WEP64_KEY4, (void *)&wlan_wep64_key4);
    apmib_get(MIB_WLAN_WEP128_KEY1, (void *)&wlan_wep128_key1);
    apmib_get(MIB_WLAN_WEP128_KEY2, (void *)&wlan_wep128_key2);
    apmib_get(MIB_WLAN_WEP128_KEY3, (void *)&wlan_wep128_key3);
    apmib_get(MIB_WLAN_WEP128_KEY4, (void *)&wlan_wep128_key4);
    apmib_get(MIB_WLAN_WPA_PSK, (void *)&wlan_wpa_psk);
    apmib_get(MIB_WLAN_WSC_DISABLE, (void *)&wlan1_wsc_disabled);	// 1104
    apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&wlan1_wlan_disabled);	// 0908	

    /*for detial mixed mode info*/ 
    apmib_get(MIB_WLAN_ENCRYPT, (void *)&wlan1_encrypt);
    apmib_get(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&wlan1_wpa_cipher);
    apmib_get(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&wlan1_wpa2_cipher);
	/*for detial mixed mode info*/ 


    /*if  wlanif_name doesn't include "wlan1", disable wlan1 wsc*/
    if((token == NULL || strstr(token, "wlan1") == 0) && (token1 == NULL || strstr(token1, "wlan1") == 0)) {
        wlan1_wsc_disabled = 1;
    }


    /* for dual band*/ 	
    if(wlan1_wlan_disabled){
        wlan1_wsc_disabled = 1 ; // if wlan1 interface is disabled			
    }
    /* for dual band*/ 		

    WRITE_WSC_PARAM(ptr, tmpbuf, "#=====wlan1 start==========%d\n",wlan1_wsc_disabled);
    WRITE_WSC_PARAM(ptr, tmpbuf, "ssid2 = \"%s\"\n",wlan_ssid );	
	WSC_THREE_DEBUG("ssid2(%s)\n", wlan_ssid);
    WRITE_WSC_PARAM(ptr, tmpbuf, "auth_type2 = %d\n", wsc_auth);
	WSC_THREE_DEBUG("auth_type2(%d)\n", wsc_auth);
    WRITE_WSC_PARAM(ptr, tmpbuf, "encrypt_type2 = %d\n", wsc_enc);
	WSC_THREE_DEBUG("encrypt_type2(%d)\n", wsc_enc);

	/*for detial mixed mode info*/ 
	intVal=0;	
	if(wlan1_encrypt==6){	// mixed mode
		if(wlan1_wpa_cipher==1){
			intVal |= WSC_WPA_TKIP;
		}else if(wlan1_wpa_cipher==2){
			intVal |= WSC_WPA_AES;		
		}else if(wlan1_wpa_cipher==3){
			intVal |= (WSC_WPA_TKIP | WSC_WPA_AES);		
		}
		if(wlan1_wpa2_cipher==1){
			intVal |= WSC_WPA2_TKIP;
		}else if(wlan1_wpa2_cipher==2){
			intVal |= WSC_WPA2_AES;		
		}else if(wlan1_wpa2_cipher==3){
			intVal |= (WSC_WPA2_TKIP | WSC_WPA2_AES);		
		}		
	}
	WRITE_WSC_PARAM(ptr, tmpbuf, "mixedmode2 = %d\n", intVal);	
	//printf("[%s %d],mixedmode2 = 0x%02x\n", __FUNCTION__,__LINE__,intVal);	
	/*for detial mixed mode info*/ 
	

	if(band_select_5g2g!=2)		//  != dual band
	{
		intVal=1;
		WRITE_WSC_PARAM(ptr, tmpbuf, "wlan1_wsc_disabled = %d\n",intVal);
		WSC_THREE_DEBUG("wlan1_wsc_disabled(%d)\n", wlan1_wsc_disabled);
	}
	else	// else see if wlan1 is enabled
	{
		WRITE_WSC_PARAM(ptr, tmpbuf, "wlan1_wsc_disabled = %d\n", wlan1_wsc_disabled);		
		WSC_THREE_DEBUG("wlan1_wsc_disabled(%d)\n", wlan1_wsc_disabled);
	}

	is_wep = 0;
	
	if (wsc_enc == WSC_ENCRYPT_WEP)
		is_wep = 1;

	if (is_wep) { // only allow WEP in none-MANUAL mode (configured by external registrar)		
		if (wlan1_encrypt != ENCRYPT_WEP) {
			printf("WEP mismatched between WPS and host system\n");
			free(buf);
			return -1;
		}		
		if (wlan_wep <= WEP_DISABLED || wlan_wep > WEP128) {
			printf("WEP encrypt length error\n");
			free(buf);
			return -1;
		}
		
		wep_transmit_key++;
		WRITE_WSC_PARAM(ptr, tmpbuf, "wep_transmit_key2 = %d\n", wep_transmit_key);


		/*whatever key type is ASSIC or HEX always use String-By-Hex fromat
		;2011-0419,fixed,need patch with wscd daemon , search 2011-0419*/ 
		if (wlan_wep == WEP64) {
			
			sprintf(tmpbuf,"%s",wlan_wep64_key1);
				
			convert_bin_to_str(tmpbuf, 5, tmp1);
			tmp1[10] = '\0';
				
			
			WRITE_WSC_PARAM(ptr, tmpbuf, "network_key2 = %s\n", tmp1);

			sprintf(tmpbuf,"%s",wlan_wep64_key2);
				
			convert_bin_to_str(tmpbuf, 5, tmp1);
			tmp1[10] = '\0';
				
			
			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key22 = %s\n", tmp1);

			sprintf(tmpbuf,"%s",wlan_wep64_key3);
				
			convert_bin_to_str(tmpbuf, 5, tmp1);
			tmp1[10] = '\0';
				
			
			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key32 = %s\n", tmp1);


			sprintf(tmpbuf,"%s",wlan_wep64_key4);
				
			convert_bin_to_str(tmpbuf, 5, tmp1);
			tmp1[10] = '\0';
				
			
			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key42 = %s\n", tmp1);
		}
		else {
			sprintf(tmpbuf,"%s",wlan_wep128_key1);

			convert_bin_to_str(tmpbuf, 13, tmp1);
			tmp1[26] = '\0';


			WRITE_WSC_PARAM(ptr, tmpbuf, "network_key2 = %s\n", tmp1);

			sprintf(tmpbuf,"%s",wlan_wep128_key2);

			convert_bin_to_str(tmpbuf, 13, tmp1);
			tmp1[26] = '\0';


			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key22 = %s\n", tmp1);

			sprintf(tmpbuf,"%s",wlan_wep128_key3);

			convert_bin_to_str(tmpbuf, 13, tmp1);
			tmp1[26] = '\0';


			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key32 = %s\n", tmp1);

			sprintf(tmpbuf,"%s",wlan_wep128_key3);

			convert_bin_to_str(tmpbuf, 13, tmp1);
			tmp1[26] = '\0';


			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key42 = %s\n", tmp1);
		}
	}
	else {
		WRITE_WSC_PARAM(ptr, tmpbuf, "network_key2 = \"%s\"\n", wlan_wpa_psk);		
	}


	WRITE_WSC_PARAM(ptr, tmpbuf, "#=====wlan1 end==========%d\n", wlan1_wsc_disabled);	
#ifdef CONFIG_RTL_TRI_BAND_SUPPORT	
	// for wlan0-va0 or wlan2
	if(token && (strstr(token, WSCD_VAP_INTERFACE)))
	{
        SetWlan_idx(token);
		sprintf(token2, "%s", WSCD_VAP_INTERFACE);
		WSC_THREE_DEBUG("SetWlan_idx: %s\n", token);		
    }else if(token && strstr(token, WSCD_WLAN2_INTERFACE))
	{
		SetWlan_idx(token);
		sprintf(token2, "%s", WSCD_WLAN2_INTERFACE);
		WSC_THREE_DEBUG("SetWlan_idx: %s\n", token);
	}
	else {     	
    	SetWlan_idx(WSCD_WLAN2_INTERFACE);	 // defalt wlan2    	
		sprintf(token2, "%s", WSCD_WLAN2_INTERFACE);
		apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&wlan2Disable);
		if(wlan2Disable){			
        	SetWlan_idx(WSCD_VAP_INTERFACE);
			apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&vapDisable);			
			sprintf(token2, "%s", WSCD_VAP_INTERFACE);			
		}
		WSC_THREE_DEBUG("SetWlan_idx: %s\n", token2);
    }   
	apmib_get(MIB_WLAN_WSC_AUTH, (void *)&wsc_auth);
    apmib_get(MIB_WLAN_WSC_ENC, (void *)&wsc_enc);
    apmib_get(MIB_WLAN_SSID, (void *)&wlan_ssid);	
    apmib_get(MIB_WLAN_MODE, (void *)&wlan1_mode);	
    apmib_get(MIB_WLAN_WEP, (void *)&wlan_wep);
    apmib_get(MIB_WLAN_WEP_KEY_TYPE, (void *)&wep_key_type);
    apmib_get(MIB_WLAN_WEP_DEFAULT_KEY, (void *)&wep_transmit_key);
    apmib_get(MIB_WLAN_WEP64_KEY1, (void *)&wlan_wep64_key1);
    apmib_get(MIB_WLAN_WEP64_KEY2, (void *)&wlan_wep64_key2);
    apmib_get(MIB_WLAN_WEP64_KEY3, (void *)&wlan_wep64_key3);
    apmib_get(MIB_WLAN_WEP64_KEY4, (void *)&wlan_wep64_key4);
    apmib_get(MIB_WLAN_WEP128_KEY1, (void *)&wlan_wep128_key1);
    apmib_get(MIB_WLAN_WEP128_KEY2, (void *)&wlan_wep128_key2);
    apmib_get(MIB_WLAN_WEP128_KEY3, (void *)&wlan_wep128_key3);
    apmib_get(MIB_WLAN_WEP128_KEY4, (void *)&wlan_wep128_key4);
    apmib_get(MIB_WLAN_WPA_PSK, (void *)&wlan_wpa_psk);
    apmib_get(MIB_WLAN_WSC_DISABLE, (void *)&wlan0_va0_wsc_disabled);	
    apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&wlan0_va0_wlan_disabled);    
    apmib_get(MIB_WLAN_ENCRYPT, (void *)&wlan0_va0_encrypt);
    apmib_get(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&wlan0_va0_wpa_cipher);
    apmib_get(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&wlan0_va0_wpa2_cipher);
	    
    if(token == NULL || (strstr(token, WSCD_VAP_INTERFACE) == 0 && strstr(token, WSCD_WLAN2_INTERFACE) == 0)) {
        wlan0_va0_wsc_disabled = 1;
    }	
    if(wlan0_va0_wlan_disabled) {
        wlan0_va0_wsc_disabled = 1 ; 		
    }
	if(wlan2Disable && vapDisable){
		wlan0_va0_wsc_disabled = 1 ; 
	}
#if 0
	if(strstr(token, WSCD_VAP_INTERFACE)){
    	WRITE_WSC_PARAM(ptr, tmpbuf, "#=====%s start==========%d\n",token, wlan0_va0_wsc_disabled);
	}
	else if(!strcmp(token, WSCD_WLAN2_INTERFACE)){
		WRITE_WSC_PARAM(ptr, tmpbuf, "#=====%s start==========%d\n",token, wlan0_va0_wsc_disabled);
	}
#endif
	WRITE_WSC_PARAM_EXT(ptr, tmpbuf, "#=====%s start==========%d\n",token2, wlan0_va0_wsc_disabled);
    WRITE_WSC_PARAM(ptr, tmpbuf, "ssid3 = \"%s\"\n",wlan_ssid );		
    WRITE_WSC_PARAM(ptr, tmpbuf, "auth_type3 = %d\n", wsc_auth);
    WRITE_WSC_PARAM(ptr, tmpbuf, "encrypt_type3 = %d\n", wsc_enc);

	intVal=0;	
	if(wlan0_va0_encrypt==6){	// mixed mode
		if(wlan0_va0_wpa_cipher==1){
			intVal |= WSC_WPA_TKIP;
		}else if(wlan0_va0_wpa_cipher==2){
			intVal |= WSC_WPA_AES;		
		}else if(wlan0_va0_wpa_cipher==3){
			intVal |= (WSC_WPA_TKIP | WSC_WPA_AES);		
		}
		if(wlan0_va0_wpa2_cipher==1){
			intVal |= WSC_WPA2_TKIP;
		}else if(wlan0_va0_wpa2_cipher==2){
			intVal |= WSC_WPA2_AES;		
		}else if(wlan0_va0_wpa2_cipher==3){
			intVal |= (WSC_WPA2_TKIP | WSC_WPA2_AES);		
		}		
	}
	WRITE_WSC_PARAM(ptr, tmpbuf, "mixedmode3 = %d\n", intVal);	
	if(band_select_5g2g!=2)		// 5g va0
	{
		intVal=1;
	#if 0
		if(!strcmp(token, WSCD_VAP_INTERFACE)){
			WRITE_WSC_PARAM(ptr, tmpbuf, "wlan0_va0_wsc_disabled = %d\n", intVal); // depend on wlan interface
		}
		else if(!strcmp(token, WSCD_WLAN2_INTERFACE)){
			WRITE_WSC_PARAM(ptr, tmpbuf, "wlan2_wsc_disabled = %d\n", intVal);	// depend on wlan interface
		}
	#endif
		WRITE_WSC_PARAM_EXT(ptr, tmpbuf, "%s_wsc_disabled = %d\n", token2, intVal);
		WSC_THREE_DEBUG("%s_wsc_disabled(%d)\n", token2, intVal);
	}
	else	
	{
	#if 0
		if(!strcmp(token, WSCD_VAP_INTERFACE)){
			WRITE_WSC_PARAM(ptr, tmpbuf, "wlan0_va0_wsc_disabled = %d\n", wlan0_va0_wsc_disabled);
		}
		else if(!strcmp(token, WSCD_WLAN2_INTERFACE)){
			WRITE_WSC_PARAM(ptr, tmpbuf, "wlan2_wsc_disabled = %d\n", wlan0_va0_wsc_disabled);	
		}
	#endif
		WRITE_WSC_PARAM_EXT(ptr, tmpbuf, "%s_wsc_disabled = %d\n", token2, wlan0_va0_wsc_disabled);
		WSC_THREE_DEBUG("%s_wsc_disabled(%d)\n", token2, wlan0_va0_wsc_disabled);
	}

	is_wep = 0;	
	if (wsc_enc == WSC_ENCRYPT_WEP)
		is_wep = 1;
	if (is_wep) { // only allow WEP in none-MANUAL mode (configured by external registrar)		
		if (wlan0_va0_encrypt != ENCRYPT_WEP) {
			printf("WEP mismatched between WPS and host system\n");
			free(buf);
			return -1;
		}		
		if (wlan_wep <= WEP_DISABLED || wlan_wep > WEP128) {
			printf("WEP encrypt length error\n");
			free(buf);
			return -1;
		}
		
		wep_transmit_key++;
		WRITE_WSC_PARAM(ptr, tmpbuf, "wep_transmit_key3 = %d\n", wep_transmit_key);


		/*whatever key type is ASSIC or HEX always use String-By-Hex fromat
		;2011-0419,fixed,need patch with wscd daemon , search 2011-0419*/ 
		if (wlan_wep == WEP64) {
			
			sprintf(tmpbuf,"%s",wlan_wep64_key1);				
			convert_bin_to_str(tmpbuf, 5, tmp1);
			tmp1[10] = '\0';			
			WRITE_WSC_PARAM(ptr, tmpbuf, "network_key3 = %s\n", tmp1);
			sprintf(tmpbuf,"%s",wlan_wep64_key2);				
			convert_bin_to_str(tmpbuf, 5, tmp1);
			tmp1[10] = '\0';			
			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key23 = %s\n", tmp1);

			sprintf(tmpbuf,"%s",wlan_wep64_key3);
				
			convert_bin_to_str(tmpbuf, 5, tmp1);
			tmp1[10] = '\0';
				
			
			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key33 = %s\n", tmp1);


			sprintf(tmpbuf,"%s",wlan_wep64_key4);
				
			convert_bin_to_str(tmpbuf, 5, tmp1);
			tmp1[10] = '\0';
				
			
			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key43 = %s\n", tmp1);
		}
		else {
			sprintf(tmpbuf,"%s",wlan_wep128_key1);

			convert_bin_to_str(tmpbuf, 13, tmp1);
			tmp1[26] = '\0';


			WRITE_WSC_PARAM(ptr, tmpbuf, "network_key3 = %s\n", tmp1);

			sprintf(tmpbuf,"%s",wlan_wep128_key2);

			convert_bin_to_str(tmpbuf, 13, tmp1);
			tmp1[26] = '\0';


			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key23 = %s\n", tmp1);

			sprintf(tmpbuf,"%s",wlan_wep128_key3);

			convert_bin_to_str(tmpbuf, 13, tmp1);
			tmp1[26] = '\0';


			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key33 = %s\n", tmp1);

			sprintf(tmpbuf,"%s",wlan_wep128_key3);

			convert_bin_to_str(tmpbuf, 13, tmp1);
			tmp1[26] = '\0';


			WRITE_WSC_PARAM(ptr, tmpbuf, "wep_key43 = %s\n", tmp1);
		}
	}
	else {
		WRITE_WSC_PARAM(ptr, tmpbuf, "network_key3 = \"%s\"\n", wlan_wpa_psk);		
	}
#if 0
	if(!strcmp(token, WSCD_VAP_INTERFACE)){
		WRITE_WSC_PARAM(ptr, tmpbuf, "#=====wlan0-va0 end==========%d\n", wlan0_va0_wsc_disabled);
	}
	else if(!strcmp(token, WSCD_WLAN2_INTERFACE)){
		WRITE_WSC_PARAM(ptr, tmpbuf, "#=====wlan2 end==========%d\n", wlan0_va0_wsc_disabled);
	}
#endif
	WRITE_WSC_PARAM_EXT(ptr, tmpbuf, "#=====%s end==========%d\n", token2, wlan0_va0_wsc_disabled);        
#endif // end CONFIG_RTL_TRI_BAND_SUPPORT

	/*sync the PIN code of wlan0 and wlan1*/
	apmib_set(MIB_HW_WSC_PIN, (void *)wsc_pin);


	/* switch back to wlan0 */
	wlan_idx = 0; 
	vwlan_idx = 0;

	
#endif	// END of FOR_DUAL_BAND
	WRITE_WSC_PARAM(ptr, tmpbuf, "device_name = \"%s\"\n", device_name);
	
	WRITE_WSC_PARAM(ptr, tmpbuf, "config_by_ext_reg = %d\n", wsc_config_by_ext_reg);

	len = (int)(((long)ptr)-((long)buf));
	
	fh = open(in, O_RDONLY);
	if (fh == -1) {
		printf("open() error [%s]!\n", in);
		return -1;
	}

	lseek(fh, 0L, SEEK_SET);
	if (read(fh, ptr, status.st_size) != status.st_size) {		
		printf("read() error [%s]!\n", in);
		return -1;	
	}
	close(fh);

	// search UUID field, replace last 12 char with hw mac address
	ptr = strstr(ptr, "uuid =");
	if (ptr) {
		char tmp2[100];
		apmib_get(MIB_WLAN_WLAN_MAC_ADDR, (void *)tmp1);
		if (!memcmp(tmp1, "\x00\x00\x00\x00\x00\x00", 6)) {
			apmib_get(MIB_HW_NIC0_ADDR, (void *)&tmp1);
		}else if(three_wps_icons)
		{
			if(!strcmp(wlanif_name,"wlan1"))
			{
				(*(char *)(tmp1+5)) += 0x10;
			}else if(!strcmp(wlanif_name, WSCD_VAP_INTERFACE) || !strcmp(wlanif_name, WSCD_WLAN2_INTERFACE) )
			{
				(*(char *)(tmp1+5)) += 0x20;
			}				
		}
		convert_bin_to_str((unsigned char *)tmp1, 6, tmp2);
		memcpy(ptr+27, tmp2, 12);		
	}

	fh = open(out, O_RDWR|O_CREAT|O_TRUNC);
	if (fh == -1) {
		printf("open() error [%s]!\n", out);
		return -1;
	}

	if (write(fh, buf, len+status.st_size) != len+status.st_size ) {
		printf("Write() file error [%s]!\n", out);
		return -1;
	}
	close(fh);
	free(buf);


	
	return 0;
}

#endif
	

#ifdef COMPRESS_MIB_SETTING

int flash_mib_checksum_ok(int offset)
{	
	unsigned char* pContent = NULL;

	COMPRESS_MIB_HEADER_T compHeader;
	unsigned char *expPtr, *compPtr;
	unsigned int expLen = 0;
	unsigned int compLen = 0;
	unsigned int real_size = 0;
	int zipRate=0;
#ifdef HEADER_LEN_INT
	HW_PARAM_HEADER_T hwHeader;
#endif
	PARAM_HEADER_T header;
	CONFIG_DATA_T type;
	char *pcomp_sig;
	int status;
	
	switch(offset)
	{
		case HW_SETTING_OFFSET:
			pcomp_sig = COMP_HS_SIGNATURE;
			type = HW_SETTING;
			break;
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		case BLUETOOTH_HW_SETTING_OFFSET:
			pcomp_sig = COMP_BT_SIGNATURE;
			type = BLUETOOTH_HW_SETTING;
			break;
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		case CUSTOMER_HW_SETTING_OFFSET:
			pcomp_sig = COMP_US_SIGNATURE;
			type = CUSTOMER_HW_SETTING;
			break;
#endif
		case DEFAULT_SETTING_OFFSET:
			type = DEFAULT_SETTING;
			pcomp_sig = COMP_DS_SIGNATURE;
			break;
		case CURRENT_SETTING_OFFSET:
			type = CURRENT_SETTING;
			pcomp_sig = COMP_CS_SIGNATURE;
			break;
		default:
			printf("offset error-[%s-%u]\n",__FILE__,__LINE__);			
			return -1;
	}
	
	real_size = mib_get_real_len(type);

	flash_read((char *)&compHeader, offset, sizeof(COMPRESS_MIB_HEADER_T));	
	if((strncmp((char *)compHeader.signature, pcomp_sig, COMP_SIGNATURE_LEN) == 0) &&
		(DWORD_SWAP(compHeader.compLen) > 0) && 
		(DWORD_SWAP(compHeader.compLen) <= real_size)
	) //check whether compress mib data
	{
		zipRate = WORD_SWAP(compHeader.compRate);
		compLen = DWORD_SWAP(compHeader.compLen);

		compPtr=calloc(1,compLen+sizeof(COMPRESS_MIB_HEADER_T));
		if(compPtr==NULL)
		{
			COMP_TRACE(stderr,"\r\n ERR:compPtr malloc fail! __[%s-%u]",__FILE__,__LINE__);
			return -1;
		}
		expPtr=calloc(1,zipRate*compLen);
		if(expPtr==NULL)
		{
			COMP_TRACE(stderr,"\r\n ERR:expPtr malloc fail! __[%s-%u]",__FILE__,__LINE__);			
			free(compPtr);
			return -1;
		}

		flash_read((char *)compPtr, offset, compLen+sizeof(COMPRESS_MIB_HEADER_T));

		expLen = Decode(compPtr+sizeof(COMPRESS_MIB_HEADER_T), compLen, expPtr);
		//printf("expandLen read len: %d\n", expandLen);

		// copy the mib header from expFile
#ifdef HEADER_LEN_INT
		if(type == HW_SETTING)
		{
			memcpy((char *)&hwHeader, expPtr, sizeof(hwHeader));
			hwHeader.len=WORD_SWAP(hwHeader.len);
			pContent = calloc(1,hwHeader.len);
			if(pContent==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:pContent malloc fail! __[%s-%u]",__FILE__,__LINE__);			
				free(compPtr);
				free(expPtr);
				return -1;
			}
			memcpy(pContent, expPtr+sizeof(HW_PARAM_HEADER_T), hwHeader.len);
			status = CHECKSUM_OK(pContent, hwHeader.len);
		}
		else
#endif
	{
		memcpy((char *)&header, expPtr, sizeof(header));
		header.len=HEADER_SWAP(header.len);
		pContent = calloc(1,header.len);
		if(pContent==NULL)
		{
			COMP_TRACE(stderr,"\r\n ERR:pContent malloc fail! __[%s-%u]",__FILE__,__LINE__);			
			free(compPtr);
			free(expPtr);
			return -1;
		}
		memcpy(pContent, expPtr+sizeof(PARAM_HEADER_T), header.len);
		status = CHECKSUM_OK(pContent, header.len);
	}
		if ( !status) {
			COMP_TRACE(stderr,"comp Checksum type[%u] error!\n", type);
			return -1;
		}
		if(compPtr != NULL)
			free(compPtr);
		if(expPtr != NULL)
			free(expPtr);
		if(pContent != NULL)
			free(pContent);

		return 1;
	} 
	else
	{
		COMP_TRACE(stderr,"\r\n Invalid comp sig or compress len __[%s-%u]",__FILE__,__LINE__);			
	}
	return -1;
}

int flash_mib_compress_write(CONFIG_DATA_T type, char *data, PARAM_HEADER_T *pheader, unsigned char *pchecksum)
{
	unsigned char* pContent = NULL;

	COMPRESS_MIB_HEADER_T compHeader;
	unsigned char *expPtr, *compPtr;
	unsigned int expLen = 0;
	unsigned int compLen;
	unsigned int real_size = 0;
	//int zipRate=0;
	char *pcomp_sig;
	unsigned char checksum  = *pchecksum;
	int dst = mib_get_flash_offset(type);
#ifdef HEADER_LEN_INT
	HW_PARAM_HEADER_T *phwHeader;
#endif
	if(dst < 0)
	{
		COMP_TRACE("\r\n ERR!! no flash offset! __[%s-%u]\n",__FILE__,__LINE__);
		return 0;
	}
	
	switch(type)
	{
		case HW_SETTING:
			pcomp_sig = COMP_HS_SIGNATURE;
#ifdef HEADER_LEN_INT
			phwHeader=(HW_PARAM_HEADER_Tp)pheader;
#endif
			break;
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		case BLUETOOTH_HW_SETTING:
			pcomp_sig = COMP_BT_SIGNATURE;
			break;
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		case CUSTOMER_HW_SETTING:
			pcomp_sig = COMP_US_SIGNATURE;
			break;
#endif
		case DEFAULT_SETTING:
			pcomp_sig = COMP_DS_SIGNATURE;
			break;
		case CURRENT_SETTING:
			pcomp_sig = COMP_CS_SIGNATURE;
			break;
		default:
			COMP_TRACE("\r\n ERR!! no type match __[%s-%u]\n",__FILE__,__LINE__);
			return 0;

	}
#ifdef HEADER_LEN_INT
	if(type==HW_SETTING)
		expLen = phwHeader->len+sizeof(HW_PARAM_HEADER_T);
	else
#endif
	expLen = pheader->len+sizeof(PARAM_HEADER_T);
	if(expLen == 0)
	{
		COMP_TRACE("\r\n ERR!! no expLen! __[%s-%u]\n",__FILE__,__LINE__);
		return 0;
	}
	real_size = mib_get_real_len(type);
	if(real_size == 0)
	{
		COMP_TRACE("\r\n ERR!! no expLen! __[%s-%u]\n",__FILE__,__LINE__);
		return 0;
	}
	
	if( (compPtr = malloc(real_size)) == NULL)
	{
		COMP_TRACE("\r\n ERR!! malloc  size %u failed! __[%s-%u]\n",real_size,__FILE__,__LINE__);
	}

	if( (expPtr = malloc(expLen)) == NULL)
	{
		COMP_TRACE("\r\n ERR!! malloc  size %u failed! __[%s-%u]\n",expLen,__FILE__,__LINE__);
		if(compPtr != NULL)
			free(compPtr);
	}
	
	if(compPtr != NULL && expPtr!= NULL)
	{
		//int status;
#ifdef HEADER_LEN_INT
		if(type==HW_SETTING)
		{
			pContent = &expPtr[sizeof(HW_PARAM_HEADER_T)];	// point to start of MIB data 

			memcpy(pContent, data, phwHeader->len-sizeof(checksum));
	
			memcpy(pContent+phwHeader->len-sizeof(checksum), &checksum, sizeof(checksum));
			phwHeader->len=WORD_SWAP(phwHeader->len);
			memcpy(expPtr, phwHeader, sizeof(HW_PARAM_HEADER_T));
		}
		else
#endif
	{
		pContent = &expPtr[sizeof(PARAM_HEADER_T)];	// point to start of MIB data 

		memcpy(pContent, data, pheader->len-sizeof(checksum));
	
		memcpy(pContent+pheader->len-sizeof(checksum), &checksum, sizeof(checksum));
		
		pheader->len=HEADER_SWAP(pheader->len);
		
		memcpy(expPtr, pheader, sizeof(PARAM_HEADER_T));
	}
		compLen = Encode(expPtr, expLen, compPtr+sizeof(COMPRESS_MIB_HEADER_T));
		sprintf((char *)compHeader.signature,"%s",pcomp_sig);
		compHeader.compRate = WORD_SWAP((expLen/compLen)+1);
		compHeader.compLen = DWORD_SWAP(compLen);
		memcpy(compPtr, &compHeader, sizeof(COMPRESS_MIB_HEADER_T));

		#ifdef __i386__
		if ( flash_write_file(expPtr, dst, expLen, "setting.raw")==0 )
		{
			printf("%s(%d)\t error\n\n",__FUNCTION__,__LINE__);
		}
		#endif

		if ( flash_write((char *)compPtr, dst, compLen+sizeof(COMPRESS_MIB_HEADER_T))==0 )
//		if ( write(fh, (const void *)compPtr, compLen+sizeof(COMPRESS_MIB_HEADER_T))!=compLen+sizeof(COMPRESS_MIB_HEADER_T) ) 
		{
			printf("Write flash compress setting[%u] failed![%s-%u]\n",type,__FILE__,__LINE__);			
			return 0;
		}
#if 0 
fprintf(stderr,"\r\n Write compress data to 0x%x. Expenlen=%u, CompRate=%u, CompLen=%u, __[%s-%u]",dst, expLen, compHeader.compRate, compHeader.compLen,__FILE__,__LINE__);

#endif
		
		if(compPtr != NULL)
			free(compPtr);
		if(expPtr != NULL)
			free(expPtr);

		return 1;
			
	}
	else
	{
		return 0;
	}

	return 0;
}
#endif //#ifdef COMPRESS_MIB_SETTING

static int send_ppp_terminate(int sock, struct sockaddr_ll *ME,
					 struct sockaddr_ll *HE, int sessid)
{	
	unsigned char buf[256];
	struct pppoe_hdr *ah = (struct pppoe_hdr *) buf;
//	unsigned char *p = (unsigned char *) (ah + 1);
	unsigned char *p = buf;
	int c;
	
	ah->ver = 1;
	ah->type = 1;
	ah->code = 0;
	
	//ah->sid = (short*)sessid;
	ah->sid = (short)sessid;
	
	ah->length = 18;
	
	p+=sizeof(struct pppoe_hdr);

	memcpy(p, "\xc0\x21", 2);
	p += 2;

	memcpy(p, "\x05", 1);
	p += 1;
	
	memcpy(p, "\x02", 1);
	p += 1;
	
	memcpy(p, "\x00\x10", 2);
	p += 2;
	
	memcpy(p, "\x55\x73\x65\x72\x20\x72\x65\x71\x75\x65\x73\x74", 12);
	p += 12;
		
	c = sendto(sock, buf, p - buf, 0, (struct sockaddr *) HE, sizeof(*HE));
	
	if (c < 0 || c != p - buf) 
	{
		printf("\r\n \r\n c = %d",c);
		return -1;
	}
	
	return 0;
}

static int send_PPPoE_PADT(int sock, struct sockaddr_ll *ME,
					 struct sockaddr_ll *HE, int sessid)
{	
	unsigned char buf[256];
	struct pppoe_hdr *ah = (struct pppoe_hdr *) buf;
//	unsigned char *p = (unsigned char *) (ah + 1);
	unsigned char *p = buf;
	int c;
	
	ah->ver = 1;
	ah->type = 1;
	ah->code = 0xa7;
	
	//ah->sid = (short*)sessid;
	ah->sid = (short)sessid;
	
	ah->length = 0;	

	HE->sll_protocol = htons(ETH_P_PPP_DISC);
		
	c = sendto(sock, buf, sizeof(struct pppoe_hdr), 0, (struct sockaddr *) HE, sizeof(*HE));
	
	if (c < 0 || c != sizeof(struct pppoe_hdr)) 
	{
		printf("\r\n \r\n c = %d",c);
		return -1;
	}
	
	return 0;
}
static int send_ppp_msg(int sessid, unsigned char *peerMacStr)
{
	unsigned char peerMacAddr[MAC_ADDR_LEN];
	char *device_if[]={"eth1"};
	char *device = NULL;
	int i;

	for(i=0 ; i<1 ; i++)
	{
		struct sockaddr_ll me;
		struct sockaddr_ll he;
		int sock;
		struct ifreq ifr;
		
		int ifindex = 0;
		//unsigned char lanIp[4];
		
		
		//int	entryNum, index;
		//DHCPRSVDIP_T entry;
		
		device = device_if[i];			
		
		sock = socket(PF_PACKET, SOCK_DGRAM, 0);
	
		if (sock < 0) {
			printf("\r\n  Get sock fail!");
			return -1;
		}
		
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, device, IFNAMSIZ - 1);
		if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
			printf("\r\n Interface %s not found", device);
			close(sock);
			continue;
		}
		ifindex = ifr.ifr_ifindex;
	
		if (ioctl(sock, SIOCGIFFLAGS, (char *) &ifr)) {
			printf("\r\n SIOCGIFFLAGS");
			close(sock);
			return -1;
		}
		if (!(ifr.ifr_flags & IFF_UP)) {
			printf("\r\n Interface %s is down", device);
			close(sock);
			continue;
		}
		if (ifr.ifr_flags & (IFF_NOARP | IFF_LOOPBACK)) {			
			printf("\r\n Interface %s is not ARPable", device);
			close(sock);
			return -1;
		}
	
		//apmib_get(MIB_IP_ADDR,  (void *)lanIp);
		//memcpy(&src, lanIp, 4);
		
		//if (src.s_addr) 
		{
			struct sockaddr_in saddr;
			int probe_fd = socket(AF_INET, SOCK_DGRAM, 0);
		
			if (probe_fd < 0) {			
				printf("\r\n socket");
				close(sock);
				return -1;
			}
			
			if (setsockopt(probe_fd, SOL_SOCKET, SO_BINDTODEVICE, device,strlen(device) + 1) == -1)
					printf("\r\n WARNING: interface %s is ignored", device);
					
			memset(&saddr, 0, sizeof(saddr));
			saddr.sin_family = AF_INET;
			
			//saddr.sin_addr = src;
			if (bind(probe_fd, (struct sockaddr *) &saddr, sizeof(saddr)) == -1) 
			{
				fprintf(stderr,"bind");
				close(sock);
				close(probe_fd);
				return -1;
			}
			close(probe_fd);
		}
		
		me.sll_family = AF_PACKET;
		me.sll_ifindex = ifindex;
		me.sll_protocol = htons(ETH_P_PPP_SES);
		if (bind(sock, (struct sockaddr *) &me, sizeof(me)) == -1) {
			printf("\r\n bind");
			close(sock);
			return -1;
		}
	
		{
			//int alen = sizeof(me);
			socklen_t alen = sizeof(me);
	
			if (getsockname(sock, (struct sockaddr *) &me, &alen) == -1) {
				printf("\r\n getsockname");
				close(sock);
				return -1;
			}
		}
		if (me.sll_halen == 0) {
			printf("\r\n Interface \"%s\" is not ARPable (no ll address)", device);
			close(sock);
			return -1;
		}
		he = me;
		memset(he.sll_addr, -1, he.sll_halen);
		
		//apmib_get(MIB_DHCPRSVDIP_NUM, (void *)&entryNum);
		
		
		//for(index = 1; index<=entryNum ; index++)
		{
#if 0			
			*((char *)&entry) = (char)index;
			if (!apmib_get(MIB_DHCPRSVDIP, (void *)&entry))
				continue;
			
			if(strlen(entry.hostName) == 0)
				continue;
				
			memcpy(&dst, entry.ipAddr , 4);
#endif			
			//memcpy(he.sll_addr, "\xff\xff\xff\xff\xff\xff" , he.sll_halen);
			memset(peerMacAddr,0, sizeof(peerMacAddr));
			string_to_hex((char *)peerMacStr,peerMacAddr,MAC_ADDR_LEN<<1);

			if((he.sll_halen != MAC_ADDR_LEN) || (!memcmp(peerMacAddr, "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LEN))){
				printf("%s(%d): wrong he.sll_halen(%d) or wrong peerMacAddr[%02x:%02x:%02x:%02x:%02x:%02x]!\n",
					__FUNCTION__,__LINE__,he.sll_halen,
					peerMacAddr[0],peerMacAddr[1],peerMacAddr[2],peerMacAddr[3],peerMacAddr[4],peerMacAddr[5]);//Added for test
				close(sock);
				return -1;
			}
			else{
				memcpy(he.sll_addr, peerMacAddr , he.sll_halen);
			}
			
			
			if(send_ppp_terminate(sock, &me, &he, sessid) == -1)
			{
				printf("\r\n \r\n send ppp lcp terminate Fail!");
				close(sock);
				return -1; 
				
			}

			if(send_PPPoE_PADT(sock, &me, &he, sessid) == -1)
			{
				printf("\r\n \r\n send PADT Fail!");
				close(sock);
				return -1; 
				
			}
		}
		
		close(sock);
	
	} //end of for(i=0 ; i<3 ; i++)
	
	return 0;
	
}

static int send_l2tp_msg(unsigned short l2tp_ns, int buf_length, unsigned char * l2tp_buff, unsigned char *lanIp, unsigned char *serverIp)
{
	struct sockaddr_in client_addr;
	unsigned char l2tp_buff_hex[256];
	unsigned char lanIpAddr[4], serverIpAddr[4];
	unsigned int lanIpAd, serverIpAd;
	int on = 1; 
	int c;
	
	memset(lanIpAddr,0, sizeof(lanIpAddr));
	string_to_hex((char *)lanIp,lanIpAddr,4<<1);

	memset(serverIpAddr,0, sizeof(serverIpAddr));
	string_to_hex((char *)serverIp,serverIpAddr,4<<1);

	memcpy((void *)&lanIpAd, (void* )lanIpAddr, 4);
	memcpy((void *)&serverIpAd, (void* )serverIpAddr, 4);

	string_to_hex((char *)l2tp_buff, l2tp_buff_hex, buf_length<<1);
	
#if 0
	printf("lanIp is %d.%d.%d.%d\n",  lanIpAddr[0], lanIpAddr[1], lanIpAddr[2], lanIpAddr[3]);
	printf("serverIp is %d.%d.%d.%d\n",  serverIpAddr[0], serverIpAddr[1], serverIpAddr[2], serverIpAddr[3]);
	printf("mib length is %d\n", buf_length);

	printf("send_l2tp_msg l2tp_payload is %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
	l2tp_buff_hex[0], l2tp_buff_hex[1], l2tp_buff_hex[2], l2tp_buff_hex[3], l2tp_buff_hex[4], l2tp_buff_hex[5], l2tp_buff_hex[6], l2tp_buff_hex[7], 
	l2tp_buff_hex[8], l2tp_buff_hex[9], l2tp_buff_hex[10], l2tp_buff_hex[11], l2tp_buff_hex[12], l2tp_buff_hex[13], l2tp_buff_hex[14], l2tp_buff_hex[15], 
	l2tp_buff_hex[16], l2tp_buff_hex[17], l2tp_buff_hex[18], l2tp_buff_hex[19], l2tp_buff_hex[20], l2tp_buff_hex[21], l2tp_buff_hex[22], l2tp_buff_hex[23], 
	l2tp_buff_hex[24], l2tp_buff_hex[25], l2tp_buff_hex[26], l2tp_buff_hex[27], l2tp_buff_hex[28], l2tp_buff_hex[29], l2tp_buff_hex[30], l2tp_buff_hex[31], 
	l2tp_buff_hex[32], l2tp_buff_hex[33], l2tp_buff_hex[34], l2tp_buff_hex[35], l2tp_buff_hex[36], l2tp_buff_hex[37], l2tp_buff_hex[38], l2tp_buff_hex[39], 
	l2tp_buff_hex[40], l2tp_buff_hex[41], l2tp_buff_hex[42], l2tp_buff_hex[43], l2tp_buff_hex[44], l2tp_buff_hex[45], l2tp_buff_hex[46], l2tp_buff_hex[47], 
	l2tp_buff_hex[48], l2tp_buff_hex[49], l2tp_buff_hex[50], l2tp_buff_hex[51], l2tp_buff_hex[52], l2tp_buff_hex[53]);
#endif

	bzero(&client_addr,sizeof(client_addr)); 
	client_addr.sin_family = AF_INET;	
//	client_addr.sin_addr.s_addr = htonl(lanIpAd);
	client_addr.sin_addr.s_addr = 0;
	client_addr.sin_port = htons(1701);	

	int client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	setsockopt( client_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ); 
	if(client_socket < 0)
	{
		printf("Create Socket Failed!\n");
		exit(1);
	}

	if (bind(client_socket, (struct sockaddr *) &client_addr, sizeof(client_addr)) == -1) 
	{
		perror("bind");
		close(client_socket);
		return -1;
	}
	
	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = serverIpAd;
	server_addr.sin_port = htons(1701);	
	socklen_t n = sizeof(server_addr);
	
	memcpy((void*)&l2tp_buff_hex[8], (void*)&l2tp_ns, 2);
	c = sendto(client_socket, l2tp_buff_hex, buf_length, 0, (struct sockaddr*)&server_addr, n);
	if (c < 0 || c != buf_length) 
	{
		printf("\r\n \r\n l2tp sendto is failed: c = %d", c);
		return -1;
	}
	
	close(client_socket);
	return 0;
}
static int updateConfigIntoFlash(unsigned char *data, int total_len, int *pType, int *pStatus)
{
	int len=0, status=1, type=0, ver, force;
#ifdef HEADER_LEN_INT
		HW_PARAM_HEADER_Tp phwHeader;
		int isHdware=0;
#endif
	PARAM_HEADER_Tp pHeader;
#ifdef COMPRESS_MIB_SETTING
	COMPRESS_MIB_HEADER_Tp pCompHeader;
	unsigned char *expFile=NULL;
	unsigned int expandLen=0;
	int complen=0;
#endif
	char *ptr;

	do {
#ifdef COMPRESS_MIB_SETTING
		pCompHeader =(COMPRESS_MIB_HEADER_Tp)&data[complen];

#ifdef _LITTLE_ENDIAN_
		pCompHeader->compRate = WORD_SWAP(pCompHeader->compRate);
		pCompHeader->compLen = DWORD_SWAP(pCompHeader->compLen);
#endif
		/*decompress and get the tag*/
		expFile=malloc(pCompHeader->compLen*pCompHeader->compRate);
		if(NULL==expFile)
		{
			printf("malloc for expFile error!!\n");
			return 0;
		}
		expandLen = Decode(data+complen+sizeof(COMPRESS_MIB_HEADER_T), pCompHeader->compLen, expFile);
		pHeader = (PARAM_HEADER_Tp)expFile;
#ifdef HEADER_LEN_INT

		if (memcmp(pHeader->signature, HW_SETTING_HEADER_TAG, TAG_LEN)==0 ||
			memcmp(pHeader->signature, HW_SETTING_HEADER_FORCE_TAG, TAG_LEN)==0 ||
			memcmp(pHeader->signature, HW_SETTING_HEADER_UPGRADE_TAG, TAG_LEN)==0 )
			{
				isHdware=1;
				phwHeader=(HW_PARAM_HEADER_Tp)expFile;
			}			
#endif
#else
#ifdef HEADER_LEN_INT
		if(isHdware)
			phwHeader=(HW_PARAM_HEADER_Tp)&data[len];
		else
#endif
		pHeader = (PARAM_HEADER_Tp)&data[len];
#endif
		
#ifdef _LITTLE_ENDIAN_
#ifdef HEADER_LEN_INT
		if(isHdware)
			phwHeader->len = WORD_SWAP(phwHeader->len);
		else
#endif
		pHeader->len = HEADER_SWAP(pHeader->len);
#endif
#ifdef HEADER_LEN_INT
		if(isHdware)
			len += sizeof(HW_PARAM_HEADER_T);
		else
#endif
		len += sizeof(PARAM_HEADER_T);

		if ( sscanf(&pHeader->signature[TAG_LEN], "%02d", &ver) != 1)
			ver = -1;
			
		force = -1;
		if ( !memcmp(pHeader->signature, CURRENT_SETTING_HEADER_TAG, TAG_LEN) )
			force = 1; // update
		else if ( !memcmp(pHeader->signature, CURRENT_SETTING_HEADER_FORCE_TAG, TAG_LEN))
			force = 2; // force
		else if ( !memcmp(pHeader->signature, CURRENT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN))
			force = 0; // upgrade

		if ( force >= 0 ) {
#if 0
			if ( !force && (ver < CURRENT_SETTING_VER || // version is less than current
				(pHeader->len < (sizeof(APMIB_T)+1)) ) { // length is less than current
				status = 0;
				break;
			}
#endif

#ifdef COMPRESS_MIB_SETTING
#ifdef HEADER_LEN_INT
			if(isHdware)
				ptr = expFile+sizeof(HW_PARAM_HEADER_T);
			else
#endif
			ptr = expFile+sizeof(PARAM_HEADER_T);
#else
			ptr = &data[len];
#endif

#ifdef COMPRESS_MIB_SETTING
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				DECODE_DATA(ptr, phwHeader->len);
			else
#endif
			DECODE_DATA(ptr, pHeader->len);
#endif
			
#ifdef HEADER_LEN_INT
			if(isHdware)
			{
				if ( !CHECKSUM_OK(ptr, phwHeader->len)) {
				status = 0;
				break;
				}
			}
			else
#endif
			if ( !CHECKSUM_OK(ptr, pHeader->len)) {
				status = 0;
				break;
			}
//#ifdef _LITTLE_ENDIAN_
//			swap_mib_word_value((APMIB_Tp)ptr);
//#endif

// added by rock /////////////////////////////////////////
#ifdef VOIP_SUPPORT
		#ifndef VOIP_SUPPORT_TLV_CFG
			flash_voip_import_fix(&((APMIB_Tp)ptr)->voipCfgParam, &pMib->voipCfgParam);
		#endif
#endif

#ifdef COMPRESS_MIB_SETTING
			apmib_updateFlash(CURRENT_SETTING, &data[complen], pCompHeader->compLen+sizeof(COMPRESS_MIB_HEADER_T), force, ver);
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				apmib_updateFlash(CURRENT_SETTING, ptr, phwHeader->len-1, force, ver);
			else
#endif
			apmib_updateFlash(CURRENT_SETTING, ptr, pHeader->len-1, force, ver);
#endif

#ifdef COMPRESS_MIB_SETTING
			complen += pCompHeader->compLen+sizeof(COMPRESS_MIB_HEADER_T);
			if(expFile)
			{
				free(expFile);
				expFile=NULL;
			}
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				len += phwHeader->len;
			else
#endif
			len += pHeader->len;
#endif
			type |= CURRENT_SETTING;
			continue;
		}


		if ( !memcmp(pHeader->signature, DEFAULT_SETTING_HEADER_TAG, TAG_LEN) )
			force = 1;	// update
		else if ( !memcmp(pHeader->signature, DEFAULT_SETTING_HEADER_FORCE_TAG, TAG_LEN) )
			force = 2;	// force
		else if ( !memcmp(pHeader->signature, DEFAULT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) )
			force = 0;	// upgrade

		if ( force >= 0 ) {
#if 0
			if ( (ver < DEFAULT_SETTING_VER) || // version is less than current
				(pHeader->len < (sizeof(APMIB_T)+1)) ) { // length is less than current
				status = 0;
				break;
			}
#endif

#ifdef COMPRESS_MIB_SETTING
#ifdef HEADER_LEN_INT
			if(isHdware)
				ptr = expFile+sizeof(HW_PARAM_HEADER_T);
			else
#endif
			ptr = expFile+sizeof(PARAM_HEADER_T);
#else
			ptr = &data[len];
#endif

#ifdef COMPRESS_MIB_SETTING
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				DECODE_DATA(ptr, phwHeader->len);
			else
#endif
			DECODE_DATA(ptr, pHeader->len);
#endif
#ifdef HEADER_LEN_INT
			if(isHdware)
			{
				if ( !CHECKSUM_OK(ptr, phwHeader->len)) {
				status = 0;
				break;
				}
			}
			else
#endif
			if ( !CHECKSUM_OK(ptr, pHeader->len)) {
				status = 0;
				break;
			}

//#ifdef _LITTLE_ENDIAN_
//			swap_mib_word_value((APMIB_Tp)ptr);
//#endif

// added by rock /////////////////////////////////////////
#ifdef VOIP_SUPPORT
		#ifndef VOIP_SUPPORT_TLV_CFG
			flash_voip_import_fix(&((APMIB_Tp)ptr)->voipCfgParam, &pMibDef->voipCfgParam);
		#endif
#endif

#ifdef COMPRESS_MIB_SETTING
			apmib_updateFlash(DEFAULT_SETTING, &data[complen], pCompHeader->compLen+sizeof(COMPRESS_MIB_HEADER_T), force, ver);
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				apmib_updateFlash(DEFAULT_SETTING, ptr, phwHeader->len-1, force, ver);
			else
#endif
			apmib_updateFlash(DEFAULT_SETTING, ptr, pHeader->len-1, force, ver);
#endif

#ifdef COMPRESS_MIB_SETTING
			complen += pCompHeader->compLen+sizeof(COMPRESS_MIB_HEADER_T);
			if(expFile)
			{
				free(expFile);
				expFile=NULL;
			}	
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				len += phwHeader->len;
			else
#endif
			len += pHeader->len;
#endif
			type |= DEFAULT_SETTING;
			continue;
		}

		if ( !memcmp(pHeader->signature, HW_SETTING_HEADER_TAG, TAG_LEN) )
			force = 1;	// update
		else if ( !memcmp(pHeader->signature, HW_SETTING_HEADER_FORCE_TAG, TAG_LEN) )
			force = 2;	// force
		else if ( !memcmp(pHeader->signature, HW_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) )
			force = 0;	// upgrade

		if ( force >= 0 ) {
#if 0
			if ( (ver < HW_SETTING_VER) || // version is less than current
				(pHeader->len < (sizeof(HW_SETTING_T)+1)) ) { // length is less than current
				status = 0;
				break;
			}
#endif
#ifdef COMPRESS_MIB_SETTING
#ifdef HEADER_LEN_INT
			if(isHdware)
				ptr = expFile+sizeof(HW_PARAM_HEADER_T);
			else
#endif
			ptr = expFile+sizeof(PARAM_HEADER_T);
#else
			ptr = &data[len];
#endif
			

#ifdef COMPRESS_MIB_SETTING
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				DECODE_DATA(ptr, phwHeader->len);
			else
#endif
			DECODE_DATA(ptr, pHeader->len);
#endif
#ifdef HEADER_LEN_INT
			if(isHdware)
			{
				if ( !CHECKSUM_OK(ptr, phwHeader->len)) {
				status = 0;
				break;
				}
			}
			else
#endif
			if ( !CHECKSUM_OK(ptr, pHeader->len)) {
				status = 0;
				break;
			}
#ifdef COMPRESS_MIB_SETTING
			apmib_updateFlash(HW_SETTING, &data[complen], pCompHeader->compLen+sizeof(COMPRESS_MIB_HEADER_T), force, ver);
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				apmib_updateFlash(HW_SETTING, ptr, phwHeader->len-1, force, ver);
			else
#endif
			apmib_updateFlash(HW_SETTING, ptr, pHeader->len-1, force, ver);
#endif

#ifdef COMPRESS_MIB_SETTING
			complen += pCompHeader->compLen+sizeof(COMPRESS_MIB_HEADER_T);
			if(expFile)
			{
				free(expFile);
				expFile=NULL;
			}
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				len += phwHeader->len;
			else
#endif
			len += pHeader->len;
#endif

			type |= HW_SETTING;
			continue;
		}
	}
#ifdef COMPRESS_MIB_SETTING	
	while (complen < total_len);
#else
	while (len < total_len);
#endif
	if(expFile)
	{
		free(expFile);
		expFile=NULL;
	}

	*pType = type;
	*pStatus = status;
#ifdef COMPRESS_MIB_SETTING	
	return complen;
#else
	return len;
#endif
}
