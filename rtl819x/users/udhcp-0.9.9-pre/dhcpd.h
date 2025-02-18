/* dhcpd.h */
#ifndef _DHCPD_H
#define _DHCPD_H

#include <netinet/ip.h>
#include <netinet/udp.h>

#include "libbb_udhcp.h"
#include "leases.h"
#include "version.h"

#ifdef TR069_ANNEX_F
#define TR069_ANNEX_F_FILE			"/var/udhcpc/tr069_annex_f.dat"
#define TR069_ANNEX_F_DEVICE_FILE	"/var/udhcpd/tr069_annex_f_device.dat"
#define MANUFACTURER_OUI			"00E04C" 
#define PRODUCT_CLASS_GATEWAY		"InternetGatewayDevice"
#define PRODUCT_CLASS_DEVICE		"DEVICE"
#endif

/************************************/
/* Defaults _you_ may want to tweak */
/************************************/

/* the period of time the client is allowed to use that address */
#define LEASE_TIME              (60*60*24*10) /* 10 days of seconds */

/* where to find the DHCP server configuration file */
#define DHCPD_CONF_FILE         "/etc/udhcpd.conf"

/*****************************************************************/
/* Do not modify below here unless you know what you are doing!! */
/*****************************************************************/
#if defined(CONFIG_RTL8186_TR) || defined(SUPPORT_OPTION_33_121_249)  ||  defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
#define RFC3442
#define UDHCPC_STATIC_ROUTE
#define UDHCPC_PASS_DOMAINNAME2DHCPSERVER
//#ifdef UDHCPC_STATIC_ROUTE
//#define UDHCPC_STATIC_FLASH
//#endif
#endif
#ifdef CONFIG_RTL865X_AC
#define RFC3442
#define UDHCPC_STATIC_ROUTE
#endif
#ifdef RFC3442
//#define UDHCPC_STATIC_FLASH
#define UDHCPC_MS_CLASSLESS_STATIC_ROUTE
#define UDHCPC_RFC_CLASSLESS_STATIC_ROUTE
#endif


/* DHCP protocol -- see RFC 2131 */
#define SERVER_PORT		67
#define CLIENT_PORT		68

#define DHCP_MAGIC		0x63825363

/* DHCP option codes (partial list) */
#define DHCP_PADDING		0x00
#define DHCP_SUBNET		0x01
#define DHCP_TIME_OFFSET	0x02
#define DHCP_ROUTER		0x03
#define DHCP_TIME_SERVER	0x04
#define DHCP_NAME_SERVER	0x05
#define DHCP_DNS_SERVER		0x06
#define DHCP_LOG_SERVER		0x07
#define DHCP_COOKIE_SERVER	0x08
#define DHCP_LPR_SERVER		0x09
#define DHCP_HOST_NAME		0x0c
#define DHCP_BOOT_SIZE		0x0d
#define DHCP_DOMAIN_NAME	0x0f
#define DHCP_SWAP_SERVER	0x10
#define DHCP_ROOT_PATH		0x11
#define DHCP_IP_TTL		0x17
#define DHCP_MTU		0x1a
#define DHCP_BROADCAST		0x1c
#ifdef UDHCPC_STATIC_ROUTE
#define DHCP_STATIC_ROUTE	0x21 
#endif
#define DHCP_NTP_SERVER		0x2a
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
#define DHCP_VENDOR_SPEC	0x2b
#endif
#define DHCP_WINS_SERVER	0x2c
#define DHCP_REQUESTED_IP	0x32
#define DHCP_LEASE_TIME		0x33
#define DHCP_OPTION_OVER	0x34
#define DHCP_MESSAGE_TYPE	0x35
#define DHCP_SERVER_ID		0x36
#define DHCP_PARAM_REQ		0x37
#define DHCP_MESSAGE		0x38
#define DHCP_MAX_SIZE		0x39
#define DHCP_T1			0x3a
#define DHCP_T2			0x3b
#define DHCP_VENDOR		0x3c
#define DHCP_CLIENT_ID		0x3d
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
#define DHCP_USER_ID           0x4d
#endif
#define DHCP_NETBIOS_NODETYPE 0x2e
#define DHCP_NETBIOS_SCOPE 0x2F
#ifdef UDHCPC_PASS_DOMAINNAME2DHCPSERVER
#define DOMAIN_NAME_ISP 0xF
//#define UDHCPC_UPDATE_FLASH_ISP
#endif
/*  added 20080508 for option 121 */
#ifdef UDHCPC_RFC_CLASSLESS_STATIC_ROUTE
#define RFC_CLASSLESS_STATIC_ROUTE 0x79
#endif
#ifdef TR069_ANNEX_F
#define	DHCP_VI_VENSPEC		0x7d
#endif
/* ----------------------------------- */
/* jimmy added 20080423 for option 249 */
#ifdef UDHCPC_MS_CLASSLESS_STATIC_ROUTE
#define MS_CLASSLESS_STATIC_ROUTE 0xF9
#endif

#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
#define DHCP_SRC_INTF		0xfe
#endif

#define DHCP_END		0xFF


#define BOOTREQUEST		1
#define BOOTREPLY		2

#define ETH_10MB		1
#define ETH_10MB_LEN		6

#define DHCPDISCOVER		1
#define DHCPOFFER		2
#define DHCPREQUEST		3
#define DHCPDECLINE		4
#define DHCPACK			5
#define DHCPNAK			6
#define DHCPRELEASE		7
#define DHCPINFORM		8

#define BROADCAST_FLAG		0x8000

#define OPTION_FIELD		0
#define FILE_FIELD		1
#define SNAME_FIELD		2

/* miscellaneous defines */
#define MAC_BCAST_ADDR		(unsigned char *) "\xff\xff\xff\xff\xff\xff"
#define OPT_CODE 0
#define OPT_LEN 1
#define OPT_DATA 2

struct option_set {
	unsigned char *data;
	struct option_set *next;
};

#ifdef STATIC_LEASE
struct static_lease {
	unsigned char *mac;
#ifdef SUPPORT_DHCP_PORT_IP_BIND
	int port_id;
#endif
	u_int32_t *ip;
	unsigned char *host;
	struct static_lease *next;
};
#endif


struct guest_mac_entry {
	int valid;
	unsigned char addr[6];	
};

#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
enum DeviceType
{
	CTC_Computer=0,
	CTC_Camera,
	CTC_HGW,
	CTC_STB,
	CTC_PHONE,
	CTC_UNKNOWN=100
};


#define MAX_DHCP_OPT_60_LEN 100 
#define MAX_DHCP_OPT_RSV_LEN 100
struct client_category_t {	
	u_int32_t ipstart;
	u_int32_t ipend;
	unsigned char option60[MAX_DHCP_OPT_60_LEN+1];
	unsigned int device_type;
	unsigned int optionCode;
	unsigned char optionStr[MAX_DHCP_OPT_RSV_LEN+1];
	unsigned int index;
	struct client_category_t *next;
};
#define CLIENT_TYPE_NUM		8


#define CTC_DHCP_OPTION43
#define CTC_DHCP_OPTION60

enum Option60_FieldType
{
	Vendor=1,
	Category,
	Model,
	Version,
	ProtocolType,
	Reserved0,Reserved1,
	ShangHaiSTB0=31,ShangHaiSTB1
};

enum Option60_PortForwarding_ProtocolType
{
	PF_UDP,
	PF_TCP,
	PF_TCP_UDP,
};

struct dhcp_ctc_port_forwaring
{
	unsigned short usProtocol; /* enum Option60_PortForwarding_ProtocolType */
	unsigned short usPort;     /* 1-65535 */
	int iSet;                  /* Set to iptables */
};

#define DHCP_CTC_MIN_FIELD_LEN 1
#define DHCP_CTC_MAX_FIELD_LEN 32
#define DHCP_CTC_FIELD_LEN 36

struct dhcp_ctc_client_info
{
	char szVendor[DHCP_CTC_FIELD_LEN];
	char szModel[DHCP_CTC_FIELD_LEN];
	char szVersion[DHCP_CTC_FIELD_LEN];
	int category;
	struct client_category_t *iCategory;
	struct dhcp_ctc_port_forwaring stPortForwarding;
};

#define DEBUG_CHN_TEL(format, ...) printf
#endif


struct server_config_t {
	u_int32_t server;		/* Our IP, in network order */
	u_int32_t start;		/* Start address of leases, network order */
	u_int32_t end;			/* End of leases, network order */
	u_int32_t netmask;		/* add by sen_liu 2012.05.09 lan netmask to determine 
								NetworkIP and BroadcastIP which should not lease*/
	struct option_set *options;	/* List of DHCP options loaded from the config file */
	char *interface;		/* The name of the interface to use */
	int ifindex;			/* Index number of the interface to use */
	unsigned char arp[6];		/* Our arp address */
	unsigned long lease;		/* lease time in seconds (host order) */
	unsigned long max_leases; 	/* maximum number of leases (including reserved address) */
	char remaining; 		/* should the lease file be interpreted as lease time remaining, or
			 		 * as the time the lease expires */
	unsigned long auto_time; 	/* how long should udhcpd wait before writing a config file.
					 * if this is zero, it will only write one on SIGUSR1 */
	unsigned long decline_time; 	/* how long an address is reserved if a client returns a
				    	 * decline message */
	unsigned long conflict_time; 	/* how long an arp conflict offender is leased for */
	unsigned long offer_time; 	/* how long an offered address is reserved */
	unsigned long min_lease; 	/* minimum lease a client can request*/
	char *lease_file;
	char *pidfile;
	char *notify_file;		/* What to run whenever leases are written */
	u_int32_t siaddr;		/* next server bootp option */
	char *sname;			/* bootp server name */
	char *boot_file;		/* bootp boot file option */
	
#ifdef GUEST_ZONE
	unsigned long guestmac_check;	// check if dhcp client is a guest
	struct guest_mac_entry *guestmac_tbl;	// guest mac list
#endif

#ifdef STATIC_LEASE
	struct static_lease *static_leases; /* List of ip/mac pairs to assign static leases */
#endif

#if defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8186_TR)	
unsigned int upateConfig_isp; /* update config from isp*/
unsigned int response_broadcast; /* */
unsigned int upateConfig_isp_dns; /* */
#endif

#ifdef SUPPORT_T1_T2_OPTION
	unsigned char t1_time[6];
	unsigned char t2_time[6];
#endif

#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	char *poolname;
	u_int32_t cwmpinstnum;
	u_int32_t poolorder;
	u_int32_t sourceinterface;
	char *vendorclass;
	u_int32_t vendorclassflag;
	char *vendorclassmode;
	char *clientid;
	u_int32_t clientidflag;
	char *userclass;
	u_int32_t userclassflag;
	char *chaddr;
	char *chaddrmask;
	u_int32_t chaddrflag;	
	struct client_category_t* clientRange;
	struct server_config_t *next;
#endif
};	

#ifdef TR069_ANNEX_F
// option 125 VI Vendor-Specific
struct device_id_t {
	u_int32_t yiaddr;
	// 1 enterprise only
	u_int32_t ent_num;
	unsigned char oui[7];
	unsigned char serialNo[65];
	unsigned char productClass[65];
	struct device_id_t *next;
};
#endif

extern struct server_config_t server_config;
extern struct dhcpOfferedAddr *leases;
#ifdef SUPPORT_DHCP_PORT_IP_BIND	
#define	RTL8651_IOCTL_GETPORTIDBYCLIENTMAC	2013		
#define LAN_IFNAME "eth0"
#endif
#endif
