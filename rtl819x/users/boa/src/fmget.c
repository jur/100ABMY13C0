/*
 *      Web server handler routines for get info and index (getinfo(), getindex())
 *
 *      Authors: David Hsu	<davidhsu@realtek.com.tw>
 *
 *      $Id: fmget.c,v 1.51 2009/09/04 07:06:05 keith_huang Exp $
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "boa.h"
#include "asp_page.h"
#include "apmib.h"
#include "apform.h"
#include "utility.h"

#define FW_VERSION	fwVersion

//#define SDEBUG(fmt, args...) printf("[%s %d]"fmt,__FUNCTION__,__LINE__,## args)
#define SDEBUG(fmt, args...) {}

#ifdef SUPER_NAME_SUPPORT
#include "auth.h"
#endif


#ifdef CONFIG_RTL_WAPI_SUPPORT
#define CA_CERT "/var/myca/CA.cert"
//#define AS_CER "/web/as.cer"
#define CA_CER "/web/ca.cer"
#define WAPI_CERT_CHANGED		"/var/tmp/certSatusChanged"
#endif

extern char *fwVersion;	// defined in version.c
#ifdef HOME_GATEWAY
#ifdef VPN_SUPPORT
extern int getIpsecInfo(IPSECTUNNEL_T *entry);
#endif
#endif

#ifdef MULTI_PPPOE

int PPPoE_Number;
char  ppp_iface[32];
#endif
static COUNTRY_IE_ELEMENT countryIEArray[] =
{
	/*
	 format: countryNumber | CountryCode(A2) | support (5G) A band? | support (2.4G)G band? |
	*/
	{8,"AL ",   3, 3, "ALBANIA"},
	{12,"DZ ",  3, 3, "ALGERIA"},
	{32,"AR ",  3, 3, "ARGENTINA"},
	{51,"AM ",  3, 3, "ARMENIA"},
	{36,"AU ",  3, 3, "AUSTRALIA"},
	{40,"AT ",  3, 3, "AUSTRIA"},
	{31,"AZ ",  3, 3, "AZERBAIJAN"},
	{48,"BH ",  3, 3, "BAHRAIN"},
	{112,"BY",  3, 3, "BELARUS"},
	{56,"BE ",  3, 3, "BELGIUM"},
	{84,"BZ ",  3, 3, "BELIZE"},
	{68,"BO ",  3, 3, "BOLIVIA"},
	{76,"BR ",  3, 3, "BRAZIL"},
	{96,"BN ",  3, 3, "BRUNEI"},
	{100,"BG ", 3, 3, "BULGARIA"},
	{124,"CA ", 1, 1, "CANADA"},
	{152,"CL ", 3, 3, "CHILE"},
	{156,"CN ",13,13, "CHINA"},
	{170,"CO ", 1, 1, "COLOMBIA"},
	{188,"CR ", 3, 3, "COSTA RICA"},
	{191,"HR ", 3, 3, "CROATIA"},
	{196,"CY ", 3, 3, "CYPRUS"},
	{203,"CZ ", 3, 3, "CZECH REPUBLIC"},
	{208,"DK ", 3, 3, "DENMARK"},
	{214,"DO ", 1, 1, "DOMINICAN REPUBLIC"},
	{218,"EC ", 3, 3, "ECUADOR"},
	{818,"EG ", 3, 3, "EGYPT"},
	{222,"SV ", 3, 3, "EL SALVADOR"},
	{233,"EE ", 3, 3, "ESTONIA"},
	{246,"FI ", 3, 3, "FINLAND"},
	{250,"FR ", 3, 3, "FRANCE"},
	{268,"GE ", 3, 3, "GEORGIA"},
	{276,"DE ", 3, 3, "GERMANY"},
	{300,"GR ", 3, 3, "GREECE"},
	{320,"GT ", 1, 1, "GUATEMALA"},
	{340,"HN ", 3, 3, "HONDURAS"},
	{344,"HK ", 3, 3, "HONG KONG"},
	{348,"HU ", 3, 3, "HUNGARY"},
	{352,"IS ", 3, 3, "ICELAND"},
	{356,"IN ", 3, 3, "INDIA"},
	{360,"ID ", 3, 3, "INDONESIA"},
	{364,"IR ", 3, 3, "IRAN"},
	{372,"IE ", 3, 3, "IRELAND"},
	{376,"IL ", 7, 7, "ISRAEL"},
	{380,"IT ", 3, 3, "ITALY"},
	{392,"JP ", 6, 6, "JAPAN"},
	{400,"JO ", 3, 3, "JORDAN"},
	{398,"KZ ", 3, 3, "KAZAKHSTAN"},
	{410,"KR ", 3, 3, "NORTH KOREA"},
	{408,"KP ", 3, 3, "KOREA REPUBLIC"},
	{414,"KW ", 3, 3, "KUWAIT"},
	{428,"LV ", 3, 3, "LATVIA"},
	{422,"LB ", 3, 3, "LEBANON"},
	{438,"LI ", 3, 3, "LIECHTENSTEIN"},
	{440,"LT ", 3, 3, "LITHUANIA"},
	{442,"LU ", 3, 3, "LUXEMBOURG"},
	{446,"MO ", 3, 3, "CHINA MACAU"},
	{807,"MK ", 3, 3, "MACEDONIA"},
	{458,"MY ", 3, 3, "MALAYSIA"},
	{484,"MX ", 1, 1, "MEXICO"},
	{492,"MC ", 3, 3, "MONACO"},
	{504,"MA ", 3, 3, "MOROCCO"},
	{528,"NL ", 3, 3, "NETHERLANDS"},
	{554,"NZ ", 3, 3, "NEW ZEALAND"},
	{578,"NO ", 3, 3, "NORWAY"},
	{512,"OM ", 3, 3, "OMAN"},
	{586,"PK ", 3, 3, "PAKISTAN"},
	{591,"PA ", 1, 1, "PANAMA"},
	{604,"PE ", 3, 3, "PERU"},
	{608,"PH ", 3, 3, "PHILIPPINES"},
	{616,"PL ", 3, 3, "POLAND"},
	{620,"PT ", 3, 3, "PORTUGAL"},
	{630,"PR ", 1, 1, "PUERTO RICO"},
	{634,"QA ", 3, 3, "QATAR"},
	{642,"RA ", 3, 3, "ROMANIA"},
	{643,"RU ",12,12, "RUSSIAN"},
	{682,"SA ", 3, 3, "SAUDI ARABIA"},
	{702,"SG ", 3, 3, "SINGAPORE"},
	{703,"SK ", 3, 3, "SLOVAKIA"},
	{705,"SI ", 3, 3, "SLOVENIA"},
	{710,"ZA ", 3, 3, "SOUTH AFRICA"},
	{724,"ES ", 3, 3, "SPAIN"},
	{752,"SE ", 3, 3, "SWEDEN"},
	{756,"CH ", 3, 3, "SWITZERLAND"},
	{760,"SY ", 3, 3, "SYRIAN ARAB REPUBLIC"},
	{158,"TW ",11,11, "TAIWAN"},
	{764,"TH ", 3, 3, "THAILAND"},
	{780,"TT ", 3, 3, "TRINIDAD AND TOBAGO"},
	{788,"TN ", 3, 3, "TUNISIA"},
	{792,"TR ", 3, 3, "TURKEY"},
	{804,"UA ", 3, 3, "UKRAINE"},
	{784,"AE ", 3, 3, "UNITED ARAB EMIRATES"},
	{826,"GB ", 3, 3, "UNITED KINGDOM"},
	{840,"US ", 1, 1, "UNITED STATES"},
	{858,"UY ", 3, 3, "URUGUAY"},
	{860,"UZ ", 1, 1, "UZBEKISTAN"},
	{862,"VE ", 3, 3, "VENEZUELA"},
	{704,"VN ", 3, 3, "VIET NAM"},
	{887,"YE ", 3, 3, "YEMEN"},
	{716,"ZW ", 3, 3, "ZIMBABWE"},
};

static REG_DOMAIN_TABLE_ELEMENT_T Bandtable_2dot4G[]={
		{0, 0,  ""},
		{1, 11, "FCC"},			//FCC
		{2, 11, "IC"},			//IC
		{3, 13, "ETSI"},			//ETSI world
		{4, 13,  "SPAIN"},		//SPAIN
		{5, 4, "FRANCE"},		//FRANCE
		{6, 14, "MKK"},			//MKK , Japan	
		{7, 11,  "ISRAEL"},			//ISRAEL
		{8, 14,  "MKK1"},
		{9, 14,  "MKK2"},
		{10,14,  "MKK3"},
		{11,11,  "NCC"},  //NCC (Taiwan)
		{12,13,  "RUSSIAN"},
		{13,13,  "CN "},
		{14,14,  "Global"},
		{15,13,  "World_wide"},
		{16,14,"Test"}
};

static REG_DOMAIN_TABLE_ELEMENT_T Bandtable_5G[]={	

		{0, 1 ,""},
		{1, 20 ,"FCC"},	
		{2, 12 ,"IC"},
		{3, 19 ,"ETSI"},	
		{4, 3 ,"SPAIN"},	
		{5, 3 ,"FRANCE"},	
		{6, 19 ,"MKK"},	
		{7, 19 ,"ISRAEL"},	
		{8, 1 ,"MKK1"},	
		{9, 1 ,"MKK2"},	
		{10, 2 ,"MKK3"},	
		{11, 15 ,"NCC"},	
		{12, 16 ,"RUSSIAN"},	
		{13, 13 ,"CN "},	
		{14, 20 ,"GLOBAL"},	
		{15, 10,"World_wide"},
		{16, 4,"Test "},
		{17, 1 ,"5M10M"}
};


#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT)
unsigned char WaitCountTime=1;
#endif

#ifdef REBOOT_CHECK
char okMsg[300]={0};
char lastUrl[100]={0};
int countDownTime = 40;
int needReboot = 0;
int run_init_script_flag = 0;
#endif


#if defined(MULTI_WAN_SUPPORT)
int getWanIfaceEntry(int index,WANIFACE_T* pEntry)
{	
	memset(pEntry, '\0', sizeof(*pEntry));	
	*((char *)pEntry) = (char)index;
	if(!apmib_get(MIB_WANIFACE_TBL,(void *)pEntry)){
		printf("get wanIface mib error\n");
		return FALSE;
	}
}
int multiwan_getWanInfo(char * phy_iface,char *wan_iface,char *pWanIP, char *pWanMask, char *pWanDefIP, char *pWanHWAddr)
{
	struct in_addr	intaddr;
	struct sockaddr hwaddr;
	unsigned char *pMacAddr;
	int isWanPhyLink = 0;
	
	isWanPhyLink = getWanLink("eth1");
		
	if ( wan_iface && getInAddr(wan_iface, IP_ADDR, (void *)&intaddr ) && ((isWanPhyLink >= 0)) )
		sprintf(pWanIP,"%s",inet_ntoa(intaddr));
	else
		sprintf(pWanIP,"%s","0.0.0.0");

	if ( wan_iface && getInAddr(wan_iface, SUBNET_MASK, (void *)&intaddr ) && ((isWanPhyLink >= 0) ))
		sprintf(pWanMask,"%s",inet_ntoa(intaddr));
	else
		sprintf(pWanMask,"%s","0.0.0.0");
	
	if ( wan_iface && getDefaultRoute(wan_iface, &intaddr) && ((isWanPhyLink >= 0) ))
		sprintf(pWanDefIP,"%s",inet_ntoa(intaddr));
	else
		sprintf(pWanDefIP,"%s","0.0.0.0");	
	
	if ( getInAddr(phy_iface, HW_ADDR, (void *)&hwaddr ) ) 
	{
		pMacAddr = (unsigned char *)hwaddr.sa_data;
		sprintf(pWanHWAddr,"%02x:%02x:%02x:%02x:%02x:%02x",pMacAddr[0], pMacAddr[1],pMacAddr[2], pMacAddr[3], pMacAddr[4], pMacAddr[5]);
	}
	else
		sprintf(pWanHWAddr,"%s","00:00:00:00:00:00");

	return 0;
}
int get_gw_from_file(int wan_index, char *pWanHWAddr)
{
	struct stat status;
	char wan_info_file[64];
	FILE *fp =NULL;
	sprintf(wan_info_file,"/var/wan_info_%d",wan_index);
	if((fp=fopen(wan_info_file,"r"))!=NULL)
	{
		fscanf(fp,"%s",pWanHWAddr);
	//	printf("%s.%d.pWanHWAddr(%s)\n",__FUNCTION__,__LINE__,pWanHWAddr);
		fclose(fp);
	}
	return 0;
}

void get_multiWan_info(char *argv,char *phy_interface,char *real_interface,int *wan_index)
{
	WANIFACE_T WanIfaceEntry;
	int	wan_type;
	if(NULL == argv) 	return;
	if(!strcmp(argv,"first"))
		*wan_index = 1;
	if(!strcmp(argv,"second"))
		*wan_index = 2;
	if(!strcmp(argv,"third"))
		*wan_index = 3;
	if(!strcmp(argv,"forth"))
		*wan_index = 4;

	if(!getWanIfaceEntry(*wan_index,&WanIfaceEntry))
	{
		printf("%s.%d.get waniface entry fail\n",__FUNCTION__,__LINE__);
		return 0;
	}	
	
	wan_type = WanIfaceEntry.AddressType;
	sprintf(phy_interface,"eth1.%d",*wan_index);
	if(wan_type ==PPPOE)
		sprintf(real_interface,"ppp%d",*wan_index);
	else
		strcpy(real_interface,phy_interface);

}

#endif

// added by rock /////////////////////////////////////////
#include <regex.h>
#ifdef VOIP_SUPPORT
#include "web_voip.h"
#endif
void translate_control_code_sprintf_shrink(char *buffer)
{
	char tmpBuf[200], *p1 = buffer, *p2 = tmpBuf;


	while (*p1) {
		if(*p1 == '\\'){
			memcpy(p2, "\\\\", 2);
			p2 += 2;
		}
		else if(*p1 =='\"'){
			memcpy(p2, "\\\"", 2);
			p2 += 2;
		}
		else if(*p1 =='\`'){
			memcpy(p2, "\\\`", 2);
			p2 += 2;
		}
		else
			*p2++ = *p1;
		p1++;
	}
	*p2 = '\0';

	strcpy(buffer, tmpBuf);
}


void translate_control_code_sprintf(char *buffer)
{
	char tmpBuf[200], *p1 = buffer, *p2 = tmpBuf;


	while (*p1) {
		if(*p1 == '\\'){
			memcpy(p2, "\\\\", 2);
			p2 += 2;
		}
		else if (*p1 == '%') {
			memcpy(p2, "%%", 2);
			p2 += 2;
		}
		else if(*p1 =='\"'){
			memcpy(p2, "\\\"", 2);
			p2 += 2;
		}
		else if(*p1 =='\''){
			memcpy(p2, "\\\'", 2);
			p2 += 2;
		}
		else if(*p1 =='\`'){
			memcpy(p2, "\\\`", 2);
			p2 += 2;
		}
		else
			*p2++ = *p1;
		p1++;
	}
	*p2 = '\0';

	strcpy(buffer, tmpBuf);
}

/////////////////////////////////////////////////////////////////////////////
void translate_control_code(char *buffer)
{
	char tmpBuf[500], *p1 = buffer, *p2 = tmpBuf;


	while (*p1) {
		if (*p1 == '"') {
			memcpy(p2, "&quot;", 6);
			p2 += 6;
		}
		else if (*p1 == '\x27') {
			memcpy(p2, "&#39;", 5);
			p2 += 5;
		}
		else if (*p1 == '\x5c') {
			memcpy(p2, "&#92;", 5);
			p2 += 5;
		}
		else if (*p1 == '\x3c') {
			memcpy(p2, "&#60;", 5);
			p2 += 5;
		}		
		else if (*p1 == '\x3e') {
			memcpy(p2, "&#62;", 5);
			p2 += 5;
		}
		else if (*p1 == '\x25') {
			memcpy(p2, "&#37;", 5);
			p2 += 5;
		}
		else if (*p1 == '\x26') {
			memcpy(p2, "&#38;", 5);
			p2 += 5;
		}
		else
			*p2++ = *p1;
		p1++;
	}
	*p2 = '\0';

	strcpy(buffer, tmpBuf);
}

#ifdef WIFI_SIMPLE_CONFIG
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
#endif

/////////////////////////////////////////////////////////////////////////////
#ifdef MULTI_PPPOE

void checkwan(char *waninfo)
{
	DHCP_T dhcp;
	apmib_get( MIB_WAN_DHCP, (void *)&dhcp);
	if(dhcp == PPPOE)
	{		
		FILE *pF;
		int num;
		char Name[32];
		if(!strcmp(waninfo,"first"))
			PPPoE_Number = 1;
		else if(!strcmp(waninfo,"second"))
			PPPoE_Number = 2;		
		else if(!strcmp(waninfo,"third"))
			PPPoE_Number = 3;	
		else if(!strcmp(waninfo,"forth"))
			PPPoE_Number = 4;			
		if((pF=fopen("/etc/ppp/ppp_order_info","r+"))==NULL){
			printf("[%s],[%d]Cannot open this file\n",__FUNCTION__,__LINE__);
			return 0;
		}
		while(fscanf(pF,"%d--%s",&num,Name) > 0 ){															
			if(PPPoE_Number == num)
				strcpy(ppp_iface,Name);
		}

	}
}

#endif


int getInfo(request *wp, int argc, char **argv)
{
	char	*name;
	struct in_addr	intaddr;
	struct sockaddr hwaddr;
	unsigned char *pMacAddr;
	unsigned long sec, mn, hr, day;
	char buffer[500];
	int i,intVal;
 	struct user_net_device_stats stats;
	DHCP_T dhcp;
	bss_info bss;
	struct tm * tm_time;
	time_t current_secs;
	char *iface=NULL;
	OPMODE_T opmode=-1;
	
#ifdef RTK_USB3G
	DHCP_T   wantype = -1;
#endif
	int wispWanId=0;
#ifdef HOME_GATEWAY
#ifdef VPN_SUPPORT
	IPSECTUNNEL_T entry ;
#endif
#endif

#ifdef CONFIG_IPV6
	addr6CfgParam_t ipaddr6;
#ifdef CONFIG_DSLITE_SUPPORT
	if(!strcmp(argv[0],("ipv6_comment_start")))
	{
		req_format_write(wp, "");
		return 0;
	}
	else if(!strcmp(argv[0],("ipv6_comment_end")))
	{
		req_format_write(wp, "");
		return 0;
	}
	else if(!strcmp(argv[0],("ipv6_jscomment_start")))
	{
		req_format_write(wp, "");
		return 0;
	}
	else if(!strcmp(argv[0],("ipv6_jscomment_end")))
	{
		req_format_write(wp, "");
		return 0;
	}
#else
	if(!strcmp(argv[0],("ipv6_comment_start")))
	{
		req_format_write(wp, "<!--");
		return 0;
	}
	else if(!strcmp(argv[0],("ipv6_comment_end")))
	{
		req_format_write(wp, "-->");
		return 0;
	}
	else if(!strcmp(argv[0],("ipv6_jscomment_start")))
	{
		req_format_write(wp, "/*");
		return 0;
	}
	else if(!strcmp(argv[0],("ipv6_jscomment_end")))
	{
		req_format_write(wp, "*/");
		return 0;
	}

#endif
	if(!strcmp(argv[0],("ipv6_passthrough_start")))
	{
		req_format_write(wp, "<!--");
		return 0;
	}
	else if(!strcmp(argv[0],("ipv6_passthrough_end")))
	{
		req_format_write(wp, "-->");
		return 0;
	}
	else if(!strcmp(argv[0],("ipv6_passthrough_js_start")))
	{
		req_format_write(wp, "/*");
		return 0;
	}
	else if(!strcmp(argv[0],("ipv6_passthrough_js_end")))
	{
		req_format_write(wp, "*/");
		return 0;
	}
#else
	if(!strcmp(argv[0],("ipv6_comment_start")))
	{
		req_format_write(wp, "<!--");
		return 0;
	}
	else if(!strcmp(argv[0],("ipv6_comment_end")))
	{
		req_format_write(wp, "-->");
		return 0;
	}
	else if(!strcmp(argv[0],("ipv6_jscomment_start")))
	{
		req_format_write(wp, "/*");
		return 0;
	}
	else if(!strcmp(argv[0],("ipv6_jscomment_end")))
	{
		req_format_write(wp, "*/");
		return 0;
	}
	else if(!strcmp(argv[0],("ipv6_passthrough_start")))
	{
		req_format_write(wp, "");
		return 0;
	}
	else if(!strcmp(argv[0],("ipv6_passthrough_end")))
	{
		req_format_write(wp, "");
		return 0;
	}
		else if(!strcmp(argv[0],("ipv6_passthrough_js_start")))
	{
		req_format_write(wp, "");
		return 0;
	}
	else if(!strcmp(argv[0],("ipv6_passthrough_js_end")))
	{
		req_format_write(wp, "");
		return 0;
	}
#endif

	int wlan_idx_keep = wlan_idx;
	char tmpStr[20];
	
	memset(tmpStr ,'\0',20);
	//printf("get parameter=%s\n", argv[0]);
	name = argv[0];
	if (name == NULL) {
   		fprintf(stderr, "Insufficient args\n");
   		return -1;
   	}
   	
	// P2P_SUPPORT ; need modify
   	if ( !strcmp(name, "device_name") ) {
		buffer[0]='\0';
		if ( !apmib_get(MIB_DEVICE_NAME,  (void *)buffer) )
			return -1;
		return req_format_write(wp, "%s", buffer);
	}
#ifdef CONFIG_APP_OPENVPN
	else if ( !strcmp(name, "openvpnEnabled")) 
	{
		if ( !apmib_get( MIB_OPENVPN_ENABLED, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal) ;
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "openvpnMode"))
	{
		if ( !apmib_get(MIB_OPENVPN_MODE,  (void *)&intVal) )
			return -1;
		
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "authenticationType"))
	{
		if ( !apmib_get(MIB_OPENVPN_AUTH_TYPE,  (void *)&intVal) )
			return -1;
		
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "openvpnPortNumber"))
	{
		if ( !apmib_get(MIB_OPENVPN_PORT,  (void *)&intVal) )
			return -1;
		
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
#endif
#if defined(MULTI_WAN_SUPPORT)
	else if ( !strcmp(name, "multiWan-current") ) {		
		int wan_index,wan_type,isWanPhy_Link = 0,isOwnIp=0;
		WANIFACE_T WanIfaceEntry;
		char phy_interface[32],real_interface[32];
		if(!argv[1])
			return 0;
		get_multiWan_info(argv[1],phy_interface,real_interface,&wan_index);
		if(!getWanIfaceEntry(wan_index,&WanIfaceEntry))
		{
			printf("%s.%d.get waniface entry fail\n",__FUNCTION__,__LINE__);
			return 0;
		}
		if(!WanIfaceEntry.enable)			
			return req_format_write(wp," ");
		
		wan_type = WanIfaceEntry.AddressType;
#if 0
		printf("%s.%d.wan_index(%d).phy_interface(%s).real_interface(%s)\n",
			__FUNCTION__,__LINE__,wan_index,phy_interface,real_interface);
#endif
		isWanPhy_Link = getWanLink("eth1");
		isOwnIp = getInAddr(real_interface, IP_ADDR, (void *)&intaddr );

		switch(wan_type)
		{
			case	PPPOE:
					if(isWanPhy_Link < 0 || (0==isOwnIp))					
						strcpy(buffer,"PPPoE Disconnected");
					else
						strcpy(buffer,"PPPoE Connected");
					break;
			case	DHCP_DISABLED:
					if(isWanPhy_Link < 0 || (0==isOwnIp))					
						strcpy(buffer,"Fixed IP Disconnected");
					else
						strcpy(buffer,"Fixed IP Connected");
					break;
			case	DHCP_CLIENT:
					if(isWanPhy_Link < 0 || (0==isOwnIp))					
						strcpy(buffer,"Getting IP from DHCP server...");
					else
						strcpy(buffer,"DHCP");				
					break;
			default:
					break;
		}
		return req_format_write(wp,"%s", buffer);
	}
#endif  	
	else if(!strcmp(name, "wlProfileSupport"))
	{
#if defined(WLAN_PROFILE)
		sprintf(buffer, "%s", "1" );
#else
		sprintf(buffer, "%s", "0");
#endif
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "wlProfile_checkbox"))
	{
		int profile_enabled_id, wlProfileEnabled;
#if defined(WLAN_PROFILE)		
		if(wlan_idx == 0)
		{
			profile_enabled_id = MIB_PROFILE_ENABLED1;
		}
#if !defined(CONFIG_RTL_TRI_BAND) 
		else
		{	
			profile_enabled_id = MIB_PROFILE_ENABLED2;
		}	
#else
		else if(wlan_idx ==1)
		{	
			profile_enabled_id = MIB_PROFILE_ENABLED2;
		}
		else if(wlan_idx ==2)
		{
			profile_enabled_id = MIB_PROFILE_ENABLED3;
		}
		else
			printf("[%s %d]support %d interface !!\n", __FUNCTION__,__LINE__,wlan_num);
#endif
		apmib_get( profile_enabled_id, (void *)&wlProfileEnabled);

		if(wlProfileEnabled == 1)
			return req_format_write(wp, "%s", "checked");
		else
#endif //#if defined(WLAN_PROFILE)			
			return req_format_write(wp, "%s", "");
	}
	else if(!strcmp(name, "wlProfile_value"))
	{
		int profile_enabled_id, wlProfileEnabled;
#if defined(WLAN_PROFILE)		
		if(wlan_idx == 0)
		{
			profile_enabled_id = MIB_PROFILE_ENABLED1;
		}
#if !defined(CONFIG_RTL_TRI_BAND) 
		else
		{	
			profile_enabled_id = MIB_PROFILE_ENABLED2;
		}
#else
		else if(wlan_idx ==1)
		{	
			profile_enabled_id = MIB_PROFILE_ENABLED2;
		}
		else if(wlan_idx ==2)
		{
			profile_enabled_id = MIB_PROFILE_ENABLED3;
		}
		else
			printf("[%s %d]support %d interface !!\n", __FUNCTION__,__LINE__, wlan_num);
#endif
		apmib_get( profile_enabled_id, (void *)&wlProfileEnabled);

		if(wlProfileEnabled == 1)
			return req_format_write(wp, "%s", "1");
		else
#endif //#if defined(WLAN_PROFILE)			
			return req_format_write(wp, "%s", "0");
	}
	else if(!strcmp(name, "wlan_profile_num"))
	{
#if defined(WLAN_PROFILE)
		int profile_num_id, entryNum;
		if(wlan_idx == 0)
		{
			profile_num_id = MIB_PROFILE_NUM1;
		}
#if !defined(CONFIG_RTL_TRI_BAND) 
		else
		{
			profile_num_id = MIB_PROFILE_NUM2;
		}
#else
		else if(wlan_idx==1)
		{
			profile_num_id = MIB_PROFILE_NUM2;
		}
		else if(wlan_idx==2)
		{
			profile_num_id = MIB_PROFILE_NUM3;
		}
		else
			printf("[%s %d]support %d interface !!\n", __FUNCTION__,__LINE__, wlan_num);
#endif	
		apmib_get(profile_num_id, (void *)&entryNum);
		return req_format_write(wp, "%d", entryNum);
#else
		return req_format_write(wp, "%s", "0");
#endif //#if defined(WLAN_PROFILE)
	}
	else if(!strcmp(name, "wlEnableProfile"))
	{
#if defined(WLAN_PROFILE)
		int profile_enabled_id, profileEnabledVal;
		if(wlan_idx == 0)
		{
			profile_enabled_id = MIB_PROFILE_ENABLED1;
		}
#if !defined(CONFIG_RTL_TRI_BAND) 
		else
		{
			profile_enabled_id = MIB_PROFILE_ENABLED2;
		}
#else
		else if(wlan_idx == 1)
		{
			profile_enabled_id = MIB_PROFILE_ENABLED2;
		}
		else if(wlan_idx == 2)
		{
			profile_enabled_id = MIB_PROFILE_ENABLED3;
		}
		else
			printf("[%s %d]support %d interface !!\n", __FUNCTION__,__LINE__, wlan_num);
#endif
		apmib_get(profile_enabled_id, (void *)&profileEnabledVal);
		return req_format_write(wp, "%d", profileEnabledVal);
#else
		return req_format_write(wp, "%s", "0");
#endif //#if defined(WLAN_PROFILE)
	}
	else if ( !strcmp(name, "uptime")) {
		struct sysinfo info ;

		sysinfo(&info);
		sec = (unsigned long) info.uptime ;
		day = sec / 86400;
		//day -= 10957; // day counted from 1970-2000

		sec %= 86400;
		hr = sec / 3600;
		sec %= 3600;
		mn = sec / 60;
		sec %= 60;

		return req_format_write(wp, "%dday:%dh:%dm:%ds",
							day, hr, mn, sec);
	}
	else if( !strcmp(name, "smtpclient_enable")) {
		#ifdef  CONFIG_APP_SMTP_CLIENT
			return req_format_write(wp, "%s", "1");
		#endif
			return req_format_write(wp, "%s", "0");
	}
	else if ( !strcmp(name, "year")) {

		time(&current_secs);
		tm_time = localtime(&current_secs);
		#if 0
		sprintf(buffer , "%2d/%2d/%d %2d:%2d:%2d %s",
				(tm_time->tm_mon),
				(tm_time->tm_mday), (tm_time->tm_year+ 1900),
				(tm_time->tm_hour),
				(tm_time->tm_min),(tm_time->tm_sec)
				, _tzname[tm_time->tm_isdst]);
		#endif
		sprintf(buffer,"%d", (tm_time->tm_year+ 1900));

		return req_format_write(wp, "%s", buffer);

	}
	else if ( !strcmp(name, "month")) {
		time(&current_secs);
		tm_time = localtime(&current_secs);
		sprintf(buffer,"%d", (tm_time->tm_mon+1));
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "day")) {
		time(&current_secs);
		tm_time = localtime(&current_secs);
		sprintf(buffer,"%d", (tm_time->tm_mday));
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "hour")) {
		time(&current_secs);
		tm_time = localtime(&current_secs);
		sprintf(buffer,"%d", (tm_time->tm_hour));
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "minute")) {
		time(&current_secs);
		tm_time = localtime(&current_secs);
		sprintf(buffer,"%d", (tm_time->tm_min));
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "second")) {
		time(&current_secs);
		tm_time = localtime(&current_secs);
		sprintf(buffer,"%d", (tm_time->tm_sec));
		return req_format_write(wp, "%s", buffer);
	}
   	else if ( !strcmp(name, "clientnum")) {
		apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&intVal);

		if (intVal == 1 || wlan_num<=0)	// disable
			intVal = 0;
		else if(!check_wlan_downup(wlan_idx))//if wlanx down
			intVal = 0;
		else {
			if ( getWlStaNum(WLAN_IF, &intVal) < 0)
			intVal = 0;
		}
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
   	else if ( !strcmp(name, "ssid")) {
		if ( !apmib_get( MIB_WLAN_SSID,  (void *)buffer) )
			return -1;

		translate_control_code(buffer);

		return req_format_write(wp, "%s", buffer);
	}
   	else if ( !strcmp(name, "channel")) {
		if ( !apmib_get( MIB_WLAN_CHANNEL,  (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
        else if ( !strcmp(name, "wep")) {
                ENCRYPT_T encrypt;

		strcpy( buffer, "Disabled");

#if defined(WLAN_PROFILE)
		int wlan_mode, rptEnabled;
		char ifname[10]={0};
		char openFileStr[60]={0};
		int inUseProfile=-1;
		char inUseProfileStr[80]={0};
		FILE *fp;
		char *ptr  = NULL;
		int profile_enabled_id, profileEnabledVal;

//printf("\r\n wlan_idx=[%d],vwlan_idx=[%d],__[%s-%u]\r\n",wlan_idx,vwlan_idx,__FILE__,__LINE__);	

		if(wlan_idx == 0)
		{
			profile_enabled_id = MIB_PROFILE_ENABLED1;
			apmib_get(MIB_REPEATER_ENABLED1, (void *)&rptEnabled);
		}
#if !defined(CONFIG_RTL_TRI_BAND) 
		else
		{
			profile_enabled_id = MIB_PROFILE_ENABLED2;
			apmib_get(MIB_REPEATER_ENABLED2, (void *)&rptEnabled);
		}
#else
		else if(wlan_idx == 1)
		{
			profile_enabled_id = MIB_PROFILE_ENABLED2;
			apmib_get(MIB_REPEATER_ENABLED2, (void *)&rptEnabled);
		}
		else if(wlan_idx == 2)
		{
			profile_enabled_id = MIB_PROFILE_ENABLED3;
			apmib_get(MIB_REPEATER_ENABLED3, (void *)&rptEnabled);
		}
#endif

		apmib_get(MIB_WLAN_MODE, (void *)&wlan_mode);


		apmib_get(profile_enabled_id, (void *)&profileEnabledVal);

		if( (vwlan_idx == 0 || vwlan_idx == NUM_VWLAN_INTERFACE) 
			&& profileEnabledVal == 1 
			&& (wlan_mode == CLIENT_MODE) )
		{
			

			
			if( vwlan_idx == NUM_VWLAN_INTERFACE)
			{
				sprintf(ifname,"wlan%d-vxd",wlan_idx);
			}
			else
			{
				sprintf(ifname,"wlan%d",wlan_idx);
			}

			sprintf(openFileStr,"cat /proc/%s/mib_ap_profile | grep in_use_profile",ifname);


			fp = popen(openFileStr, "r");
			if(fp && (NULL != fgets(inUseProfileStr, sizeof(inUseProfileStr),fp)))
			{
				char *searchPtr;

				searchPtr = strstr(inUseProfileStr,"in_use_profile"); //move to first, 
//printf("\r\n inUseProfileStr[%s],__[%s-%u]\r\n",inUseProfileStr,__FILE__,__LINE__);

				sscanf(searchPtr, "in_use_profile: %d", &inUseProfile);
				pclose(fp);
			}

//printf("\r\n inUseProfile[%d],__[%s-%u]\r\n",inUseProfile,__FILE__,__LINE__);
			if(inUseProfile >= 0)
			{
				WLAN_PROFILE_T entry;
				memset(&entry,0x00, sizeof(WLAN_PROFILE_T));
				*((char *)&entry) = (char)(inUseProfile+1);

				if(wlan_idx == 0)
					apmib_get(MIB_PROFILE_TBL1, (void *)&entry);
#if !defined(CONFIG_RTL_TRI_BAND) 
				else
					apmib_get(MIB_PROFILE_TBL2, (void *)&entry);
#else
				else if(wlan_idx == 1)
					apmib_get(MIB_PROFILE_TBL2, (void *)&entry);
				else if(wlan_idx ==2)
					apmib_get(MIB_PROFILE_TBL3, (void *)&entry);
				else
					printf("<%s-%d>supported wlan interface %d\n", __FUNCTION__,__LINE__,wlan_num);
#endif				
				if (entry.encryption == WEP64)
					strcpy( buffer, "WEP 64bits");
				else if (entry.encryption == WEP128)
					strcpy( buffer, "WEP 128bits");
				else if (entry.encryption == 3)
					strcpy( buffer, "WPA");
				else if (entry.encryption == 4)
		            strcpy( buffer, "WPA2");
				else if (entry.encryption == 6)
		            strcpy( buffer, "WPA-Mixed");
				else 
		            strcpy( buffer, "Disabled");

//printf("\r\n buffer[%s],__[%s-%u]\r\n",buffer,__FILE__,__LINE__);			

			}
		}
		else
#endif //#if defined(WLAN_PROFILE
		{
		
                if ( !apmib_get( MIB_WLAN_ENCRYPT,  (void *)&encrypt) )
                        return -1;
                if (encrypt == ENCRYPT_DISABLED)
                        strcpy( buffer, "Disabled");
                else if (encrypt == ENCRYPT_WPA)
                        strcpy( buffer, "WPA");
		else if (encrypt == ENCRYPT_WPA2)
                        strcpy( buffer, "WPA2");
		else if (encrypt == (ENCRYPT_WPA | ENCRYPT_WPA2))
                        strcpy( buffer, "WPA2 Mixed");
		else if (encrypt == ENCRYPT_WAPI)
				strcpy(buffer,"WAPI");
                else {
                        WEP_T wep;
                        if ( !apmib_get( MIB_WLAN_WEP,  (void *)&wep) )
                                return -1;
                        if ( wep == WEP_DISABLED )
                                strcpy( buffer, "Disabled");
                        else if ( wep == WEP64 )
                                strcpy( buffer, "WEP 64bits");
                        else if ( wep == WEP128)
                                strcpy( buffer, "WEP 128bits");
                }
		}
                return req_format_write(wp, buffer);
        }
   	else if ( !strcmp(name, "wdsEncrypt")) {
   		WDS_ENCRYPT_T encrypt;
		if ( !apmib_get( MIB_WLAN_WDS_ENCRYPT,  (void *)&encrypt) )
			return -1;
		if ( encrypt == WDS_ENCRYPT_DISABLED)
			strcpy( buffer, "Disabled");
		else if ( encrypt == WDS_ENCRYPT_WEP64)
			strcpy( buffer, "WEP 64bits");
		else if ( encrypt == WDS_ENCRYPT_WEP128)
			strcpy( buffer, "WEP 128bits");
		else if ( encrypt == WDS_ENCRYPT_TKIP)
			strcpy( buffer, "TKIP");
		else if ( encrypt == WDS_ENCRYPT_AES)
			strcpy( buffer, "AES");
		else
			buffer[0] = '\0';
   		return req_format_write(wp, buffer);
   	}
#ifdef CONFIG_RTK_MESH
   	else if ( !strcmp(name, "meshEncrypt")) {
   		ENCRYPT_T encrypt;
		if ( !apmib_get( MIB_WLAN_MESH_ENCRYPT,  (void *)&encrypt) )
			return -1;
		if ( encrypt == ENCRYPT_DISABLED)
			strcpy( buffer, "Disabled");
		else if ( encrypt == ENCRYPT_WPA2)
			strcpy( buffer, "WPA2");
		else
			buffer[0] = '\0';
   		return req_format_write(wp, buffer);
   	}
	else if(!strcmp(argv[0],("mesh_comment_start")))
	{
		req_format_write(wp, "");
		return 0;
	}
	else if(!strcmp(argv[0],("mesh_comment_end")))
	{
		req_format_write(wp, "");
		return 0;
	}
	else if(!strcmp(argv[0],("mesh_jscomment_start")))
	{
		req_format_write(wp, "");
		return 0;
	}
	else if(!strcmp(argv[0],("mesh_jscomment_end")))
	{
		req_format_write(wp, "");
		return 0;
	}
#else
	else if(!strcmp(argv[0],("mesh_comment_start")))
	{
		req_format_write(wp, "<!--");
		return 0;
	}
	else if(!strcmp(argv[0],("mesh_comment_end")))
	{
		req_format_write(wp, "-->");
		return 0;
	}
	else if(!strcmp(argv[0],("mesh_jscomment_start")))
	{
		req_format_write(wp, "/*");
		return 0;
	}
	else if(!strcmp(argv[0],("mesh_jscomment_end")))
	{
		req_format_write(wp, "*/");
		return 0;
	}
#endif /* CONFIG_RTK_MESH */
  	else if ( !strcmp(name, "ip")) {
		if ( getInAddr(BRIDGE_IF, IP_ADDR, (void *)&intaddr ) )
			return req_format_write(wp, "%s", inet_ntoa(intaddr) );
		else
			return req_format_write(wp, "0.0.0.0");
	}
   	else if ( !strcmp(name, "mask")) {
		if ( getInAddr(BRIDGE_IF, SUBNET_MASK, (void *)&intaddr ))
			return req_format_write(wp, "%s", inet_ntoa(intaddr) );
		else
			return req_format_write(wp, "0.0.0.0");
	}
   	else if ( !strcmp(name, "gateway")) {
		DHCP_T dhcp;
  		apmib_get( MIB_DHCP, (void *)&dhcp);
		if ( dhcp == DHCP_SERVER ) {
		// if DHCP server, default gateway is set to LAN IP
			if ( getInAddr(BRIDGE_IF, IP_ADDR, (void *)&intaddr ) )
				return req_format_write(wp, "%s", inet_ntoa(intaddr) );
			else
				return req_format_write(wp, "0.0.0.0");
		}
		else
		if ( getDefaultRoute(BRIDGE_IF, &intaddr) )
			return req_format_write(wp, "%s", inet_ntoa(intaddr) );
		else
			return req_format_write(wp, "0.0.0.0");
	}
	else if ( !strcmp(name, "ip-rom")) {
		if ( !apmib_get( MIB_IP_ADDR,  (void *)buffer) )
			return -1;
   		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
   	else if ( !strcmp(name, "mask-rom")) {
		if ( !apmib_get( MIB_SUBNET_MASK,  (void *)buffer) )
			return -1;
   		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
   	else if ( !strcmp(name, "gateway-rom")) {
		if ( !apmib_get( MIB_DEFAULT_GATEWAY,  (void *)buffer) )
			return -1;
		if (!memcmp(buffer, "\x0\x0\x0\x0", 4))
			return req_format_write(wp, "0.0.0.0");
   		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
	else if ( !strcmp(name, "static_dhcp_onoff")) {
		if ( !apmib_get(MIB_DHCPRSVDIP_ENABLED, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal);
		return req_format_write(wp, "%s", buffer);
	}
	
	else if(strcmp(name, "static_dhcp_entry_num")==0)
	{
		int	entryNum;
		if(!apmib_get(MIB_DHCPRSVDIP_TBL_NUM, (void *)&entryNum))
		{
			return -1;
		}
		sprintf(buffer, "%d", entryNum);
		return req_format_write(wp, "%s", buffer);
	}

 	else if ( !strcmp(name, "dhcp-current") ) {
   		if ( !apmib_get( MIB_DHCP, (void *)&dhcp) )
			return -1;

		if (dhcp==DHCP_CLIENT) {
			if (!isDhcpClientExist(BRIDGE_IF) &&
					!getInAddr(BRIDGE_IF, IP_ADDR, (void *)&intaddr))
				return req_format_write(wp, "Getting IP from DHCP server...");
			if (isDhcpClientExist(BRIDGE_IF))
				return req_format_write(wp, "DHCP");
		}
		return req_format_write(wp, "Fixed IP");
	}
   	else if ( !strcmp(name, "dhcpRangeStart")) {
		if ( !apmib_get( MIB_DHCP_CLIENT_START,  (void *)buffer) )
			return -1;
   		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
  	else if ( !strcmp(name, "dhcpRangeEnd")) {
		if ( !apmib_get( MIB_DHCP_CLIENT_END,  (void *)buffer) )
			return -1;
   		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
	else if ( !strcmp(name, "dhcpLeaseTime")) {
        	apmib_get( MIB_DHCP_LEASE_TIME, (void *)&intVal);
        	if( (intVal==0) || (intVal<0) || (intVal>10080))
        	{
            		intVal = 480;
            		if(!apmib_set(MIB_DHCP_LEASE_TIME, (void *)&intVal))
        		{
        			printf("set MIB_DHCP_LEASE_TIME error\n");
        		}

			apmib_update(CURRENT_SETTING);
        	}
        	sprintf(buffer, "%d", intVal);
        	return req_format_write(wp, buffer);
        }
 	else if ( !strcmp(name, "wan-dns1")) {
		if ( !apmib_get( MIB_DNS1,  (void *)buffer) )
			return -1;
		if (!memcmp(buffer, "\x0\x0\x0\x0", 4))
			return req_format_write(wp, "");
   		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
  	else if ( !strcmp(name, "wan-dns2")) {
		if ( !apmib_get( MIB_DNS2,  (void *)buffer) )
			return -1;
		if (!memcmp(buffer, "\x0\x0\x0\x0", 4))
			return req_format_write(wp, "");
   		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
  	else if ( !strcmp(name, "wan-dns3")) {
		if ( !apmib_get( MIB_DNS3,  (void *)buffer) )
			return -1;
		if (!memcmp(buffer, "\x0\x0\x0\x0", 4))
			return req_format_write(wp, "");
   		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
	else if ( !strcmp(name, "ntpTimeZone")) { // sc_yang
                if ( !apmib_get( MIB_NTP_TIMEZONE,  (void *)buffer) )
                        return -1;
                return req_format_write(wp, "%s", buffer);
        }
	else if ( !strcmp(name, "ntpServerIp1")) { // sc_yang
                if ( !apmib_get( MIB_NTP_SERVER_IP1,  (void *)buffer) )
                        return -1;
                if (!memcmp(buffer, "\x0\x0\x0\x0", 4))
                        return req_format_write(wp, "");

                return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
        }
	else if ( !strcmp(name, "ntpServerIp2")) { // sc_yang
				if ( !apmib_get( MIB_NTP_SERVER_ID, (void *)&intVal) )
						return -1;
				if ( intVal == 1) {
	                if ( !apmib_get( MIB_NTP_SERVER_IP2,  (void *)buffer) )
	                        return -1;
				}
				else if ( intVal == 2) {
	                if ( !apmib_get( MIB_NTP_SERVER_IP3,  (void *)buffer) )
	                        return -1;
				}
				else if ( intVal == 3) {
	                if ( !apmib_get( MIB_NTP_SERVER_IP4,  (void *)buffer) )
	                        return -1;
				}
				else if ( intVal == 4) {
	                if ( !apmib_get( MIB_NTP_SERVER_IP5,  (void *)buffer) )
	                        return -1;
				}
				else if ( intVal == 5) {
	                if ( !apmib_get( MIB_NTP_SERVER_IP6,  (void *)buffer) )
	                        return -1;
				}
				else {
					memset(buffer, 0, sizeof(buffer));
				}
				
                if (!memcmp(buffer, "\x0\x0\x0\x0", 4))
                        return req_format_write(wp, "");

                return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
        }
	else if(!strcmp(argv[0],"wan_access_type_s"))
        {
#ifdef CONFIG_RTL_8198_AP_ROOT || defined(CONFIG_RTL_8197D_AP)
                req_format_write(wp, "%s","<!--");
#else
                req_format_write(wp, "%s","");
#endif
                return 0;
        }
        else if(!strcmp(argv[0],"wan_access_type_e"))
        {
#ifdef CONFIG_RTL_8198_AP_ROOT || defined(CONFIG_RTL_8197D_AP)
                req_format_write(wp, "%s","-->");
#else
                req_format_write(wp, "%s","");
#endif
                return 0;
	}
	
#ifdef  HOME_GATEWAY
	/*
	else if ( !strcmp(name, "ntpServerIp1")) { // sc_yang
		if ( !apmib_get( MIB_NTP_SERVER_IP1,  (void *)buffer) )
			return -1;
		if (!memcmp(buffer, "\x0\x0\x0\x0", 4))
			return req_format_write(wp, "");

		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
	else if ( !strcmp(name, "ntpServerIp2")) { // sc_yang
		if ( !apmib_get( MIB_NTP_SERVER_IP2,  (void *)buffer) )
			return -1;
		if (!memcmp(buffer, "\x0\x0\x0\x0", 4))
			return req_format_write(wp, "");
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
	else if ( !strcmp(name, "ntpTimeZone")) { // sc_yang
		if ( !apmib_get( MIB_NTP_TIMEZONE,  (void *)buffer) )
			return -1;
   		return req_format_write(wp, "%s", buffer);
	}
	*/



    	else if ( !strcmp(name, "wan-ppp-idle")) {
		if ( !apmib_get( MIB_PPP_IDLE_TIME,  (void *)&intVal) )
			return -1;

		sprintf(buffer, "%d", intVal/60 );
   		return req_format_write(wp, buffer);
	}
		else if ( !strcmp(name, "wan-ppp-idle2")) {
		if ( !apmib_get( MIB_PPP_IDLE_TIME2,  (void *)&intVal) )
			return -1;

		sprintf(buffer, "%d", intVal/60 );
		return req_format_write(wp, buffer);
		}
    	else if ( !strcmp(name, "wan-ppp-idle3")) {
		if ( !apmib_get( MIB_PPP_IDLE_TIME3,  (void *)&intVal) )
			return -1;

		sprintf(buffer, "%d", intVal/60 );
   		return req_format_write(wp, buffer);
		}		
		else if ( !strcmp(name, "wan-ppp-idle4")) {
		if ( !apmib_get( MIB_PPP_IDLE_TIME4,  (void *)&intVal) )
			return -1;
	
		sprintf(buffer, "%d", intVal/60 );
		return req_format_write(wp, buffer);
		}
		else if ( !strcmp(name, "S1_F1_start")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET1_F1_START,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}
		else if ( !strcmp(name, "S1_F1_end")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET1_F1_END,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}
		else if ( !strcmp(name, "S1_F2_start")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET1_F2_START,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}
		else if ( !strcmp(name, "S1_F2_end")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET1_F2_END,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}
		else if ( !strcmp(name, "S1_F3_start")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET1_F3_START,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}
		else if ( !strcmp(name, "S1_F3_end")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET1_F3_END,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}
		else if ( !strcmp(name, "S2_F1_start")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET2_F1_START,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}
		else if ( !strcmp(name, "S2_F1_end")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET2_F1_END,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}
		else if ( !strcmp(name, "S2_F2_start")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET2_F2_START,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}		
		else if ( !strcmp(name, "S2_F2_end")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET2_F2_END,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}	
		else if ( !strcmp(name, "S2_F3_start")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET2_F3_START,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}			
		else if ( !strcmp(name, "S2_F3_end")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET2_F3_END,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}	
		else if ( !strcmp(name, "S3_F1_start")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET3_F1_START,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}	
		else if ( !strcmp(name, "S3_F1_end")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET3_F1_END,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}	
		else if ( !strcmp(name, "S3_F2_start")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET3_F2_START,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}
		else if ( !strcmp(name, "S3_F2_end")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET3_F2_END,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}
		else if ( !strcmp(name, "S3_F3_start")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET3_F3_START,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}
		else if ( !strcmp(name, "S3_F3_end")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET3_F3_END,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}
		else if ( !strcmp(name, "S4_F1_start")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET4_F1_START,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}
		else if ( !strcmp(name, "S4_F1_end")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET4_F1_END,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}
		else if ( !strcmp(name, "S4_F2_start")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET4_F2_START,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}
		else if ( !strcmp(name, "S4_F2_end")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET4_F2_END,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}
		else if ( !strcmp(name, "S4_F3_start")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET4_F3_START,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}		
		else if ( !strcmp(name, "S4_F3_end")) {
			memset(buffer,0x00,sizeof(buffer));
			apmib_get( MIB_SUBNET4_F3_END,  (void *)buffer);
		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
		}	

	else if ( !strcmp(name, "pppSubNet1")) {
		buffer[0]='\0';		
		apmib_get( MIB_PPP_SUBNET1,  (void *)buffer);
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "pppSubNet2")) {
		buffer[0]='\0';
		apmib_get( MIB_PPP_SUBNET2,  (void *)buffer);
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "pppSubNet3")) {
		buffer[0]='\0';
		apmib_get( MIB_PPP_SUBNET3,  (void *)buffer);
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "pppSubNet4")) {
		buffer[0]='\0';
		apmib_get( MIB_PPP_SUBNET4,  (void *)buffer);
		return req_format_write(wp, "%s", buffer);
	}		

	else if ( !strcmp(name, "pppUserName2")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_PPP_USER_NAME2,	(void *)buffer);
		translate_control_code(buffer);
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "pppPassword2")) {
		buffer[0]='\0';
		apmib_get( MIB_PPP_PASSWORD2,  (void *)buffer);
		translate_control_code(buffer);
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "pppUserName3")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_PPP_USER_NAME3,	(void *)buffer);
		translate_control_code(buffer);
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "pppPassword3")) {
		buffer[0]='\0';
		apmib_get( MIB_PPP_PASSWORD3,  (void *)buffer);
		translate_control_code(buffer);
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "pppUserName4")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_PPP_USER_NAME4,	(void *)buffer);
		translate_control_code(buffer);
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "pppPassword4")) {
		buffer[0]='\0';
		apmib_get( MIB_PPP_PASSWORD4,  (void *)buffer);
		translate_control_code(buffer);
		return req_format_write(wp, "%s", buffer);
	}	
	else if ( !strcmp(name, "pppServiceName2")) {
		buffer[0]='\0';
		apmib_get( MIB_PPP_SERVICE_NAME2,  (void *)buffer);
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "pppServiceName3")) {
		buffer[0]='\0';
		apmib_get( MIB_PPP_SERVICE_NAME3,  (void *)buffer);
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "pppServiceName4")) {
		buffer[0]='\0';
		apmib_get( MIB_PPP_SERVICE_NAME4,  (void *)buffer);
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "pppMtuSize2")) {
		
		apmib_get( MIB_PPP_MTU_SIZE2, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "pppMtuSize3")) {
		apmib_get( MIB_PPP_MTU_SIZE3, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "pppMtuSize4")) {
		apmib_get( MIB_PPP_MTU_SIZE4, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}	
	else if ( !strcmp(name, "wan-pptp-idle")) {
		if ( !apmib_get( MIB_PPTP_IDLE_TIME,  (void *)&intVal) )
			return -1;

		sprintf(buffer, "%d", intVal/60 );
   		return req_format_write(wp, buffer);
	}
		else if ( !strcmp(name, "wan-l2tp-idle")) {
		if ( !apmib_get( MIB_L2TP_IDLE_TIME,  (void *)&intVal) )
			return -1;

		sprintf(buffer, "%d", intVal/60 );
   		return req_format_write(wp, buffer);
	}
#ifdef RTK_USB3G
    else if ( !strcmp(name, "wan-USB3G-idle")) {
        if ( !apmib_get( MIB_USB3G_IDLE_TIME,  (void *)buffer) )
            return -1;
        sprintf(buffer, "%d", atoi(buffer)/60 );
        return req_format_write(wp, buffer);
    }
#else
    else if ( !strcmp(name, "wan-USB3G-idle")) {
        return req_format_write(wp, "");
    }
#endif /* #ifdef RTK_USB3G */
 	else if ( !strcmp(name, "dmzHost")) {
		if ( !apmib_get( MIB_DMZ_HOST,  (void *)buffer) )
			return -1;
		if (!memcmp(buffer, "\x0\x0\x0\x0", 4))
			return req_format_write(wp, "");
   		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
	else if ( !strcmp(name, "wanMac")) {
		if ( !apmib_get(MIB_WAN_MAC_ADDR,  (void *)buffer) )
			return -1;
		return req_format_write(wp, "%02x%02x%02x%02x%02x%02x", (unsigned char)buffer[0], (unsigned char)buffer[1],
						(unsigned char)buffer[2], (unsigned char)buffer[3], (unsigned char)buffer[4], (unsigned char)buffer[5]);
	}
	else if ( !strcmp(name, "wan_mac_clone_address")) {
		
		sprintf(buffer, "%s", wp->remote_ip_addr);
		//printf("%s:%d####buffer=%s\n", __FUNCTION__,__LINE__,buffer);
		intVal=get_clone_mac_by_ip(buffer, tmpStr);
		//printf("%s:%d####clone_mac=%s\n", __FUNCTION__,__LINE__,tmpStr);
		if(intVal==0)
		{
			strcpy(buffer, tmpStr);
			return req_format_write(wp, buffer);
		}
		else
		{
			apmib_get(MIB_HW_NIC1_ADDR,  (void *)buffer);
			return req_format_write(wp, "%02x%02x%02x%02x%02x%02x", (unsigned char)buffer[0], (unsigned char)buffer[1],
						(unsigned char)buffer[2], (unsigned char)buffer[3], (unsigned char)buffer[4], (unsigned char)buffer[5]);
		}
	}
	else if ( !strcmp(name, "wan_default_mac_address")) {		
		apmib_get(MIB_HW_NIC1_ADDR,  (void *)buffer);
		return req_format_write(wp, "%02x%02x%02x%02x%02x%02x", (unsigned char)buffer[0], (unsigned char)buffer[1],
			(unsigned char)buffer[2], (unsigned char)buffer[3], (unsigned char)buffer[4], (unsigned char)buffer[5]);
	}
	else if ( !strcmp(name, "fixedIpMtuSize")) {
		if ( !apmib_get( MIB_FIXED_IP_MTU_SIZE, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "dhcpMtuSize")) {
		if ( !apmib_get( MIB_DHCP_MTU_SIZE, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
#endif
	else if ( !strcmp(name, "hwaddr")) {
		if ( getInAddr(BRIDGE_IF, HW_ADDR, (void *)&hwaddr ) ) {
			pMacAddr = (unsigned char *)hwaddr.sa_data;
			return req_format_write(wp, "%02x:%02x:%02x:%02x:%02x:%02x", pMacAddr[0], pMacAddr[1],
				pMacAddr[2], pMacAddr[3], pMacAddr[4], pMacAddr[5]);
		}
		else
			return req_format_write(wp, "00:00:00:00:00:00");
	}
	else if ( !strcmp(name, "bridgeMac")) {
		if ( !apmib_get(MIB_ELAN_MAC_ADDR,  (void *)buffer) )
			return -1;
		return req_format_write(wp, "%02x%02x%02x%02x%02x%02x", (unsigned char)buffer[0], (unsigned char)buffer[1],
						(unsigned char)buffer[2], (unsigned char)buffer[3], (unsigned char)buffer[4], (unsigned char)buffer[5]);
	}

	/* Advance setting stuffs */
	else if ( !strcmp(name, "fragThreshold")) {
		if ( !apmib_get( MIB_WLAN_FRAG_THRESHOLD, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "rtsThreshold")) {
		if ( !apmib_get( MIB_WLAN_RTS_THRESHOLD, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "beaconInterval")) {
		if ( !apmib_get( MIB_WLAN_BEACON_INTERVAL, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "ackTimeout")) {
		if ( !apmib_get( MIB_WLAN_ACK_TIMEOUT, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "dtimPeriod")) {
		if ( !apmib_get( MIB_WLAN_DTIM_PERIOD, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "fwVersion")) {
		sprintf(buffer, "%s", FW_VERSION );
   		return req_format_write(wp, buffer);
	}
	// added by rock /////////////////////////////////////////
	else if ( !strcmp(name, "buildTime")) {
		FILE *fp;
		regex_t re;
		regmatch_t match[2];
		int status;

		fp = fopen("/proc/version", "r");
		if (!fp) {
			fprintf(stderr, "Read /proc/version failed!\n");
			return req_format_write(wp, "Unknown");
	   	}
		else
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
   				return req_format_write(wp, &buffer[match[1].rm_so]);
			}
		}

		return req_format_write(wp, "Unknown");
	}
	else if ( !strcmp(name, "lanTxPacketNum")) {
		if ( getStats(ELAN_IF, &stats) < 0)
			stats.tx_packets = 0;
		sprintf(buffer, "%d", (int)stats.tx_packets);
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "lanRxPacketNum")) {
		if ( getStats(ELAN_IF, &stats) < 0)
			stats.rx_packets = 0;
		sprintf(buffer, "%d", (int)stats.rx_packets);
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "lan2TxPacketNum")) {
#if defined(VLAN_CONFIG_SUPPORTED)
		if ( getStats(ELAN2_IF, &stats) < 0)
			stats.tx_packets = 0;
		sprintf(buffer, "%d", (int)stats.tx_packets);
#else
		sprintf(buffer, "%d", 0);
#endif
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "lan2RxPacketNum")) {
#if defined(VLAN_CONFIG_SUPPORTED)
		if ( getStats(ELAN2_IF, &stats) < 0)
			stats.rx_packets = 0;
		sprintf(buffer, "%d", (int)stats.rx_packets);
#else
		sprintf(buffer, "%d", 0);
#endif
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "lan3TxPacketNum")) {
#if defined(VLAN_CONFIG_SUPPORTED)
		if ( getStats(ELAN3_IF, &stats) < 0)
			stats.tx_packets = 0;
		sprintf(buffer, "%d", (int)stats.tx_packets);
#else
		sprintf(buffer, "%d", 0);
#endif
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "lan3RxPacketNum")) {
#if defined(VLAN_CONFIG_SUPPORTED)
		if ( getStats(ELAN3_IF, &stats) < 0)
			stats.rx_packets = 0;
		sprintf(buffer, "%d", (int)stats.rx_packets);
#else
		sprintf(buffer, "%d", 0);
#endif
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "lan4TxPacketNum")) {
#if defined(VLAN_CONFIG_SUPPORTED)
		if ( getStats(ELAN4_IF, &stats) < 0)
			stats.tx_packets = 0;
		sprintf(buffer, "%d", (int)stats.tx_packets);
#else
		sprintf(buffer, "%d", 0);
#endif
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "lan4RxPacketNum")) {
#if defined(VLAN_CONFIG_SUPPORTED)
		if ( getStats(ELAN4_IF, &stats) < 0)
			stats.rx_packets = 0;
		sprintf(buffer, "%d", (int)stats.rx_packets);
#else
		sprintf(buffer, "%d", 0);
#endif
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "wlanTxPacketNum")) {
		if ( getStats(WLAN_IF, &stats) < 0)
			stats.tx_packets = 0;
		sprintf(buffer, "%d", (int)stats.tx_packets);
   		return req_format_write(wp, buffer);

	}
	else if ( !strcmp(name, "wlanRxPacketNum")) {
		if ( getStats(WLAN_IF, &stats) < 0)
			stats.rx_packets = 0;
		sprintf(buffer, "%d", (int)stats.rx_packets);
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "bssid")) {
		apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&intVal);
		if ( intVal == 0 &&  getInAddr(WLAN_IF, HW_ADDR, (void *)&hwaddr ) ) {
			pMacAddr = (unsigned char *)hwaddr.sa_data;
			return req_format_write(wp, "%02x:%02x:%02x:%02x:%02x:%02x", pMacAddr[0], pMacAddr[1],
				pMacAddr[2], pMacAddr[3], pMacAddr[4], pMacAddr[5]);
		}
		else
			return req_format_write(wp, "00:00:00:00:00:00");
	}
	else if ( !strcmp(name, "bssid_drv")) {
		if ( getWlBssInfo(WLAN_IF, &bss) < 0)
			return -1;
		return req_format_write(wp, "%02x:%02x:%02x:%02x:%02x:%02x", bss.bssid[0], bss.bssid[1],
				bss.bssid[2], bss.bssid[3], bss.bssid[4], bss.bssid[5]);
	}
	else if ( !strcmp(name, "ssid_drv")) {
		if ( getWlBssInfo(WLAN_IF, &bss) < 0)
			return -1;
		memcpy(buffer, bss.ssid, SSID_LEN+1);
		translate_control_code(buffer);
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "state_drv")) {
		char *pMsg;
		if ( getWlBssInfo(WLAN_IF, &bss) < 0)
			return -1;
		switch (bss.state) {
		case STATE_DISABLED:
			pMsg = "Disabled";
			break;
		case STATE_IDLE:
			pMsg = "Idle";
			break;
		case STATE_STARTED:
			pMsg = "Started";
			break;
		case STATE_CONNECTED:
			pMsg = "Connected";
			break;
		case STATE_WAITFORKEY:
			pMsg = "Waiting for keys";
			break;
		case STATE_SCANNING:
			pMsg = "Scanning";
			break;
		default:
			pMsg=NULL;
		}
		return req_format_write(wp, "%s", pMsg);
	}
	else if ( !strcmp(name, "channel_drv")) {
		if ( getWlBssInfo(WLAN_IF, &bss) < 0)
			return -1;

		if (bss.channel)
			sprintf(buffer, "%d", bss.channel);
		else
			strcpy(buffer,"0");
			//buffer[0] = '\0';

		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "wan-ip-rom")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_WAN_IP_ADDR,  (void *)buffer);
   	return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
	else if ( !strcmp(name, "wan-mask-rom")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_WAN_SUBNET_MASK,  (void *)buffer);
   	return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
  else if ( !strcmp(name, "wan-gateway-rom")) {
  	memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_WAN_DEFAULT_GATEWAY,  (void *)buffer);
		if (!memcmp(buffer, "\x0\x0\x0\x0", 4))
			return req_format_write(wp, "0.0.0.0");
   		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
	else if ( !strcmp(name, "pppUserName")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_PPP_USER_NAME,  (void *)buffer);
		translate_control_code(buffer);
		return req_format_write(wp, "%s", buffer);
	}
  	else if ( !strcmp(name, "pppPassword")) {
		buffer[0]='\0';
		apmib_get( MIB_PPP_PASSWORD,  (void *)buffer);
		translate_control_code(buffer);
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "pptpIp")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_PPTP_IP_ADDR,  (void *)buffer);
   	return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
#if defined(CONFIG_DYNAMIC_WAN_IP)
	else if ( !strcmp(name, ("pptpDefGw"))) {
		if ( !apmib_get( MIB_PPTP_DEFAULT_GW,  (void *)buffer) )
			return -1;
   		return req_format_write(wp, ("%s"), inet_ntoa(*((struct in_addr *)buffer)) );
	}
#endif
	else if ( !strcmp(name, "pptpSubnet")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_PPTP_SUBNET_MASK,  (void *)buffer);
   	return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
	else if ( !strcmp(name, "pptpServerIp")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_PPTP_SERVER_IP_ADDR,  (void *)buffer);
   	return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
#if defined(CONFIG_GET_SERVER_IP_BY_DOMAIN)
	else if ( !strcmp(name, "pptpServerDomain")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_PPTP_SERVER_DOMAIN, (void *)buffer);
   		return req_format_write(wp, buffer);
	}
#endif	
	else if ( !strcmp(name, "pptpMtuSize")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_PPTP_MTU_SIZE, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
  	else if ( !strcmp(name, "pptpUserName")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_PPTP_USER_NAME,  (void *)buffer);
		translate_control_code(buffer);
		return req_format_write(wp, "%s", buffer);
	}
  	else if ( !strcmp(name, "pptpPassword")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_PPTP_PASSWORD,  (void *)buffer);
		translate_control_code(buffer);
		return req_format_write(wp, "%s", buffer);
	}
	/* # keith: add l2tp support. 20080515 */
	else if ( !strcmp(name, "l2tpIp")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_L2TP_IP_ADDR,  (void *)buffer);
   		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
	else if ( !strcmp(name, "l2tpSubnet")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_L2TP_SUBNET_MASK,  (void *)buffer);
   		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
#if defined(CONFIG_DYNAMIC_WAN_IP)
	else if ( !strcmp(name, ("l2tpDefGw"))) {
		if ( !apmib_get( MIB_L2TP_DEFAULT_GW,  (void *)buffer) )
			return -1;
   		return req_format_write(wp, ("%s"), inet_ntoa(*((struct in_addr *)buffer)) );
	}
#endif
#if defined(CONFIG_GET_SERVER_IP_BY_DOMAIN)
	else if ( !strcmp(name, "l2tpServerDomain")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_L2TP_SERVER_DOMAIN, (void *)buffer);
   		return req_format_write(wp, buffer);
	}
#endif	
	else if ( !strcmp(name, "l2tpServerIp")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_L2TP_SERVER_IP_ADDR,  (void *)buffer);
   		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
	else if ( !strcmp(name, "l2tpMtuSize")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_L2TP_MTU_SIZE, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
	

  	else if ( !strcmp(name, "l2tpUserName")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_L2TP_USER_NAME,  (void *)buffer);
		translate_control_code(buffer);
		return req_format_write(wp, "%s", buffer);
	}
  	else if ( !strcmp(name, "l2tpPassword")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_L2TP_PASSWORD,  (void *)buffer);
		translate_control_code(buffer);
		return req_format_write(wp, "%s", buffer);
	}

#ifdef RTK_USB3G
    else if ( !strcmp(name, "USB3G_PIN")) {
        memset(buffer,0x00,sizeof(buffer));
        apmib_get( MIB_USB3G_PIN,  (void *)buffer);
        translate_control_code(buffer);
        return req_format_write(wp, "%s", buffer);
    }
    else if ( !strcmp(name, "USB3G_APN")) {
        memset(buffer,0x00,sizeof(buffer));
        apmib_get( MIB_USB3G_APN,  (void *)buffer);
        translate_control_code(buffer);
        return req_format_write(wp, "%s", buffer);
    }
    else if ( !strcmp(name, "USB3G_DIALNUM")) {
        memset(buffer,0x00,sizeof(buffer));
        apmib_get( MIB_USB3G_DIALNUM,  (void *)buffer);
        translate_control_code(buffer);
        return req_format_write(wp, "%s", buffer);
    }
    else if ( !strcmp(name, "USB3G_USER")) {
        memset(buffer,0x00,sizeof(buffer));
        apmib_get( MIB_USB3G_USER,  (void *)buffer);
        translate_control_code(buffer);
        return req_format_write(wp, "%s", buffer);
    }
    else if ( !strcmp(name, "USB3G_PASS")) {
        memset(buffer,0x00,sizeof(buffer));
        apmib_get( MIB_USB3G_PASS,  (void *)buffer);
        translate_control_code(buffer);
        return req_format_write(wp, "%s", buffer);
    }
    else if ( !strcmp(name, "USB3GMtuSize")) {
        memset(buffer,0x00,sizeof(buffer));
        apmib_get( MIB_USB3G_MTU_SIZE,  (void *)buffer);
        translate_control_code(buffer);
        return req_format_write(wp, "%s", buffer);
    }
#else
	else if(!strncmp(name,"USB3G",strlen("USB3G")))
	{
		 return req_format_write(wp, "%s", "");
	}
#endif /* #ifdef RTK_USB3G */
	else if ( !strcmp(name, "pppServiceName")) {
		buffer[0]='\0';
		apmib_get( MIB_PPP_SERVICE_NAME,  (void *)buffer);
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "pppMtuSize")) {
		apmib_get( MIB_PPP_MTU_SIZE, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
#ifdef _ALPHA_DUAL_WAN_SUPPORT_
	else if ( !strcmp(name, "pppVlanId")) {
		apmib_get( MIB_CWMP_PPPOE_WAN_VLANID, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
#endif
	else if(!strcmp(name, "wizard_menu_onoff")) {
#if defined(CONFIG_POCKET_AP_SUPPORT)
		return req_format_write(wp,"");
#elif defined(CONFIG_RTL_ULINKER)		
		apmib_get( MIB_ULINKER_AUTO, (void *)&intVal);
		if(intVal == 1)
			return req_format_write(wp, "menu.addItem('Setup Wizard', 'wizard.htm', '', 'Setup Wizard');" );	
		else
			return req_format_write(wp,"");
#else
		return req_format_write(wp, "menu.addItem('Setup Wizard', 'wizard.htm', '', 'Setup Wizard');" );
#endif
	}
	else if(!strcmp(name, "opmode_menu_onoff")) {
#if defined(CONFIG_POCKET_AP_SUPPORT) || defined(CONFIG_RTL_8198_AP_ROOT) || defined(CONFIG_RTL_8197D_AP) || defined(CONFIG_RTL_AP_PACKAGE)
		return req_format_write(wp,"");
#elif defined(CONFIG_RTL_ULINKER)
		return req_format_write(wp, "menu.addItem('ULinker Operation Mode', 'ulinker_opmode.htm', '', 'ULinker Operation Mode');" );
#else
		return req_format_write(wp, "menu.addItem('Operation Mode', 'opmode.htm', '', 'Operation Mode');" );
#endif
	}
//	for ap package of new ui, start
	else if(!strcmp(name, "show_opmode_menu")) {	
#if defined(CONFIG_RTL_AP_PACKAGE)	
		return req_format_write(wp,"0");
#elif defined(CONFIG_RTL_ULINKER)
		return req_format_write(wp,"2");
#else	
		return req_format_write(wp,"1");
#endif
	}
	
	else if(!strcmp(name, "show_firewall_all") || !strcmp(name, "show_tcpip_wan") || !strcmp(name, "show_mgt_ddns") 
		||  !strcmp(name, "show_mgt_dos") || !strcmp(name, "show_mgt_ntp") ) {	
#if defined(CONFIG_RTL_AP_PACKAGE)	
			return req_format_write(wp,"0");
#else	
			return req_format_write(wp,"1");
#endif
	}
// for ap package of new ui , end

else if( !strcmp(name, "pmfEnabled")) {
#ifdef CONFIG_IEEE80211W
			return req_format_write(wp,"1");	
#else
			return req_format_write(wp,"0");			
#endif
}
	else if(!strcmp(name, "qos_root_menu")) {

#if defined(GW_QOS_ENGINE) && !defined(VOIP_SUPPORT)
		return req_format_write(wp, "menu.addItem('QoS', 'qos.htm', '', 'Setup QoS');" );
#elif defined(QOS_BY_BANDWIDTH) && !defined(VOIP_SUPPORT)
	#ifdef CONFIG_IPV6
		return req_format_write(wp, "menu.addItem('QoS', 'ip6_qos.htm', '', 'Setup QoS');" );
	#else
		return req_format_write(wp, "menu.addItem('QoS', 'ip_qos.htm', '', 'Setup QoS');" );
	#endif
#else
		return req_format_write(wp,"");
#endif
	}
	else if(!strcmp(name, "route_menu_onoff"))
	{
#if defined(ROUTE_SUPPORT)
		return req_format_write(wp, "menu.addItem(\"Route Setup\", \"route.htm\", \"\", \"Route Setup\");");
#else
		return req_format_write(wp,"");
#endif
	}
	else if(!strcmp(name, "ethdot1x_menu_onoff"))
	{
#if defined(CONFIG_RTL_ETH_802DOT1X_SUPPORT)
		return req_format_write(wp, "menu.addItem(\"Ethernet 802.1x Setup\", \"eth_dot1x.htm\", \"\", \"Ethernet 802.1x Setup\");");
#else
		return req_format_write(wp,"");
#endif
	}
	else if(!strcmp(name,"snmp_menu"))
	{
#if defined(CONFIG_SNMP)
		return req_format_write(wp, "menu.addItem(\"SNMP\", \"snmp.htm\", \"\", \"SNMP Setup\");");
#else
		return req_format_write(wp,"");
#endif
	}
	else if(!strcmp(name,"ddns_menu"))
	{
#if defined(DDNS_SUPPORT)
		return req_format_write(wp, "manage.addItem(\"DDNS\", \"ddns.htm\", \"\", \"Setup Dynamic DNS\");");
#else
		return req_format_write(wp,"");
#endif
	}
#ifdef HOME_GATEWAY
	else if ( !strcmp(name, "wanDhcp-current")) {
	
#if defined(CONFIG_RTL_8198_AP_ROOT) || defined(CONFIG_RTL_8197D_AP)
		return req_format_write(wp, "Brian 5BGG");
#else
 		int isWanPhy_Link=0;
#ifdef MULTI_PPPOE
		if(argc >=2 && argv[1])
			checkwan(argv[1]);
#endif

 		if ( !apmib_get( MIB_OP_MODE, (void *)&opmode) )
			return -1;
		if( !apmib_get(MIB_WISP_WAN_ID, (void *)&wispWanId))
			return -1;
 		if ( !apmib_get( MIB_WAN_DHCP, (void *)&dhcp) )
			return -1;
		
		if(opmode == BRIDGE_MODE){
			return req_format_write(wp, "Disconnected");
		}
		
		if(opmode != WISP_MODE){
 			isWanPhy_Link=getWanLink("eth1");
 		}		
		if ( dhcp == DHCP_CLIENT) {
			if(opmode == WISP_MODE) {
				if(0 == wispWanId)
					iface = "wlan0";
				else if(1 == wispWanId)
					iface = "wlan1";
#ifdef CONFIG_SMART_REPEATER
				if(getWispRptIface(&iface,wispWanId)<0)
					return -1;
#endif

			}
			else
				iface = WAN_IF;

		#if defined(CONFIG_4G_LTE_SUPPORT)
			if (lte_wan()) {
				iface = "usb0";
				isWanPhy_Link = 1;
				//isWanPhy_Link = getWanLink(iface);
			}
			//printf("\n\n\n wanDhcp-current: iface[%s] \n\n\n", iface);
		#endif /* #if defined(CONFIG_4G_LTE_SUPPORT) */

		 	if (!isDhcpClientExist(iface))
				return req_format_write(wp, "Getting IP from DHCP server...");
			else{
				if(isWanPhy_Link < 0)
					return req_format_write(wp, "Getting IP from DHCP server...");
				else
					return req_format_write(wp, "DHCP");
			}

		}
		else if ( dhcp == DHCP_DISABLED ){
			if (opmode == WISP_MODE)
			{
				char wan_intf[MAX_NAME_LEN] = {0};
				char lan_intf[MAX_NAME_LEN] = {0};
				
				getInterfaces(lan_intf,wan_intf);
				//printf("%s %d wan_intf=%s \n", __FUNCTION__, __LINE__, wan_intf);
				memset(&bss, 0x00, sizeof(bss));
				getWlBssInfo(wan_intf, &bss);
				//printf("%s %d wan_intf=%s bss.state=%d\n", __FUNCTION__, __LINE__, wan_intf, bss.state);
				if (bss.state == STATE_CONNECTED){
					return req_format_write(wp, "Fixed IP Connected");
				}
				else
				{
					return req_format_write(wp, "Fixed IP Disconnected");
				}				
			}
			else
			{
				if(isWanPhy_Link < 0)
					return req_format_write(wp, "Fixed IP Disconnected");
				else
					return req_format_write(wp, "Fixed IP Connected");
			}
		}
		else if ( dhcp ==  PPPOE ) {

#ifdef _ALPHA_DUAL_WAN_SUPPORT_
			int pppoeWithDhcpEnabled = 0;

			apmib_get(MIB_PPPOE_DHCP_ENABLED, (void *)&pppoeWithDhcpEnabled);

			if (pppoeWithDhcpEnabled) {
				
				if ( isConnectPPP()){
#ifdef MULTI_PPPOE
					req_format_write(wp, "PPPoE Connected");
#endif
					if(isWanPhy_Link < 0)
						req_format_write(wp, "PPPoE Disconnected");
					else
						req_format_write(wp, "PPPoE Connected");
				}else
					req_format_write(wp, "PPPoE Disconnected");
			}
			else {
				if ( isConnectPPP()){
#ifdef MULTI_PPPOE
					return req_format_write(wp, "PPPoE Connected");
#endif
					if(isWanPhy_Link < 0)
						return req_format_write(wp, "PPPoE Disconnected");
					else
						return req_format_write(wp, "PPPoE Connected");
				}else
					return req_format_write(wp, "PPPoE Disconnected");
			}
#else // _ALPHA_DUAL_WAN_SUPPORT_
			if ( isConnectPPP()){
#ifdef MULTI_PPPOE
				return req_format_write(wp, "PPPoE Connected");
#endif
				if(isWanPhy_Link < 0)
					return req_format_write(wp, "PPPoE Disconnected");
				else
					return req_format_write(wp, "PPPoE Connected");
			}else
				return req_format_write(wp, "PPPoE Disconnected");
#endif // _ALPHA_DUAL_WAN_SUPPORT_
			
#ifdef _ALPHA_DUAL_WAN_SUPPORT_
			{
				if (pppoeWithDhcpEnabled) {
					iface = WAN_IF;
					
				 	if (!isDhcpClientExist(iface))
						return req_format_write(wp, " and Getting IP from DHCP server...");
					else{
						if(isWanPhy_Link < 0)
							return req_format_write(wp, " and Getting IP from DHCP server...");
						else
							return req_format_write(wp, " and DHCP");
					}
				}
			}
#endif // _ALPHA_DUAL_WAN_SUPPORT_
		}
		else if ( dhcp ==  PPTP ) {
			if ( isConnectPPP()){
				if(isWanPhy_Link < 0)
					return req_format_write(wp, "PPTP Disconnected");
				else
					return req_format_write(wp, "PPTP Connected");
			}else
				return req_format_write(wp, "PPTP Disconnected");
		}
		else if ( dhcp ==  L2TP ) { /* # keith: add l2tp support. 20080515 */
			if ( isConnectPPP()){
				if(isWanPhy_Link < 0)
					return req_format_write(wp, "L2TP Disconnected");
				else
					return req_format_write(wp, "L2TP Connected");
			}else
				return req_format_write(wp, "L2TP Disconnected");
		}
#ifdef RTK_USB3G
		else if ( dhcp == USB3G ) {
            int inserted = 0;
            char str[32];

			if (isConnectPPP()){
                return req_format_write(wp, "USB3G Connected");
            }else {
                FILE *fp;
                char str[32];
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
                    return req_format_write(wp, "USB3G Modem Initializing...");
                }
                else if (str != NULL && strstr(str, "dial")) {
                    return req_format_write(wp, "USB3G Dialing...");
                }
                else if (str != NULL && strstr(str, "remove")) {
                    return req_format_write(wp, "USB3G Removed");
                }
                else
                    return req_format_write(wp, "USB3G Disconnected");
            }
        }
#endif /* #ifdef RTK_USB3G */
#endif //#if defined(CONFIG_RTL_8198_AP_ROOT)
	}
	else if ( !strcmp(name, "wan-ip"))
  {
#if defined(MULTI_WAN_SUPPORT)
	int wan_index = 0 ;
	char phy_interface[32],real_interface[32];
	char strWanIP[16],strWanMask[16],strWanDefIP[16],strWanHWAddr[18];
	
	if(!argv[1])
		return 0;
	
	get_multiWan_info(argv[1],phy_interface,real_interface,&wan_index);
	#if 0
	printf("%s.%d.wan_index(%d).phy_interface(%s).real_interface(%s)\n",
		__FUNCTION__,__LINE__,wan_index,phy_interface,real_interface);	
	#endif

	multiwan_getWanInfo(phy_interface,real_interface,strWanIP,strWanMask,strWanDefIP,strWanHWAddr);
	return req_format_write(wp, "%s", strWanIP);	
#else
#if defined(CONFIG_RTL_8198_AP_ROOT) || defined(CONFIG_RTL_8197D_AP)
		return req_format_write(wp, "%s", "0.0.0.0");
#else
#ifdef _ALPHA_DUAL_WAN_SUPPORT_
	char strWanIP[40];
		char strWanMask[40];
		char strWanDefIP[40];
		char strWanHWAddr[40];
#else
  	char strWanIP[16];
		char strWanMask[16];
		char strWanDefIP[16];
		char strWanHWAddr[18];
#endif // _ALPHA_DUAL_WAN_SUPPORT_
#ifdef MULTI_PPPOE
		if(argc >=2 && argv[1])
			checkwan(argv[1]);
#endif

		getWanInfo(strWanIP,strWanMask,strWanDefIP,strWanHWAddr);
		return req_format_write(wp, "%s", strWanIP);
#endif
#endif
	}
   	else if ( !strcmp(name, "wan-mask")) {
#if defined(MULTI_WAN_SUPPORT)
	int wan_index = 0 ;
	char phy_interface[32],real_interface[32];
	char strWanIP[16],strWanMask[16],strWanDefIP[16],strWanHWAddr[18];
	if(!argv[1])
		return 0;
	get_multiWan_info(argv[1],phy_interface,real_interface,&wan_index);
	multiwan_getWanInfo(phy_interface,real_interface,strWanIP,strWanMask,strWanDefIP,strWanHWAddr);
	return req_format_write(wp, "%s", strWanMask);	
#else	
#if defined(CONFIG_RTL_8198_AP_ROOT) || defined(CONFIG_RTL_8197D_AP)
		return req_format_write(wp, "%s", "0.0.0.0");
#else
#ifdef _ALPHA_DUAL_WAN_SUPPORT_
	char strWanIP[40];
		char strWanMask[40];
		char strWanDefIP[40];
		char strWanHWAddr[40];
#else
  	char strWanIP[16];
		char strWanMask[16];
		char strWanDefIP[16];
		char strWanHWAddr[18];
#endif // _ALPHA_DUAL_WAN_SUPPORT_
#ifdef MULTI_PPPOE
		if(argc >=2 && argv[1])
			checkwan(argv[1]);
#endif			
			getWanInfo(strWanIP,strWanMask,strWanDefIP,strWanHWAddr);

			return req_format_write(wp, "%s", strWanMask);
#endif
#endif
	}
   	else if ( !strcmp(name, "wan-gateway")) {
#if defined(MULTI_WAN_SUPPORT)
	int wan_index = 0 ;
	char phy_interface[32],real_interface[32];
	char strWanIP[16],strWanMask[16],strWanDefIP[16],strWanHWAddr[18];	
	if(!argv[1])
		return 0;
	get_multiWan_info(argv[1],phy_interface,real_interface,&wan_index);
	//multiwan_getWanInfo(phy_interface,real_interface,strWanIP,strWanMask,strWanDefIP,strWanHWAddr);
	get_gw_from_file(wan_index,strWanDefIP);
	return req_format_write(wp, "%s", strWanDefIP);	
#else			
#if defined(CONFIG_RTL_8198_AP_ROOT) || defined(CONFIG_RTL_8197D_AP)
		return req_format_write(wp, "%s", "0.0.0.0");
#else
#ifdef _ALPHA_DUAL_WAN_SUPPORT_
	char strWanIP[40];
		char strWanMask[40];
		char strWanDefIP[40];
		char strWanHWAddr[40];
#else
  	char strWanIP[16];
		char strWanMask[16];
		char strWanDefIP[16];
		char strWanHWAddr[18];
#endif // _ALPHA_DUAL_WAN_SUPPORT_
#ifdef MULTI_PPPOE
		if(argc >=2 && argv[1])
			checkwan(argv[1]);
#endif				
			getWanInfo(strWanIP,strWanMask,strWanDefIP,strWanHWAddr);

			return req_format_write(wp, "%s", strWanDefIP);
#endif
#endif
	}
	else if ( !strcmp(name, "wan-hwaddr")) {
#if defined(MULTI_WAN_SUPPORT)
	int wan_index = 0 ;
	char phy_interface[32],real_interface[32];
	char strWanIP[16],strWanMask[16],strWanDefIP[16],strWanHWAddr[18];
	
	if(!argv[1])
		return 0;
	get_multiWan_info(argv[1],phy_interface,real_interface,&wan_index);
	multiwan_getWanInfo(phy_interface,real_interface,strWanIP,strWanMask,strWanDefIP,strWanHWAddr);
	return req_format_write(wp, "%s", strWanHWAddr);	
#else			
#if defined(CONFIG_RTL_8198_AP_ROOT) || defined(CONFIG_RTL_8197D_AP)
		return req_format_write(wp, "%s", "0.0.0.0");
#else
		char strWanIP[16];
		char strWanMask[16];
		char strWanDefIP[16];
		char strWanHWAddr[18];
#ifdef MULTI_PPPOE
		if(argc >=2 && argv[1])
			checkwan(argv[1]);
#endif			
		getWanInfo(strWanIP,strWanMask,strWanDefIP,strWanHWAddr);

    #ifdef RTK_USB3G
    {   /* when wantype is 3G, we dpn't need to show MAC */
        DHCP_T wan_type;
        apmib_get(MIB_WAN_DHCP, (void *)&wan_type);

        if (wan_type == USB3G)
            return req_format_write(wp, "");
    }
    #endif /* #ifdef RTK_USB3G */

		return req_format_write(wp, "%s", strWanHWAddr);
#endif
#endif
	}
	else if ( !strcmp(name, "wanTxPacketNum")) {
#ifdef RTK_USB3G
		apmib_get(MIB_WAN_DHCP, (void *)&wantype);
#endif
  	apmib_get( MIB_OP_MODE, (void *)&opmode);
	if( !apmib_get(MIB_WISP_WAN_ID, (void *)&wispWanId))
			return -1;
		if(opmode == WISP_MODE) {
			if(0 == wispWanId)
				iface = "wlan0";
			else if(1 == wispWanId)
				iface = "wlan1";
		}
#ifdef RTK_USB3G
        else if (wantype == USB3G)
            iface = PPPOE_IF;
#endif
		else
			iface = WAN_IF;

	#if defined(CONFIG_4G_LTE_SUPPORT)
		if (lte_wan()) {
			iface = "usb0";
		}
		//printf("\n\n\n wanTxPacketNum: iface[%s] \n\n\n", iface);
	#endif /* #if defined(CONFIG_4G_LTE_SUPPORT) */

		if ( getStats(iface, &stats) < 0)
			stats.tx_packets = 0;
		sprintf(buffer, "%d", (int)stats.tx_packets);
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "wanRxPacketNum")) {
#ifdef RTK_USB3G
        apmib_get(MIB_WAN_DHCP, (void *)&wantype);
#endif
		apmib_get( MIB_OP_MODE, (void *)&opmode);
		if( !apmib_get(MIB_WISP_WAN_ID, (void *)&wispWanId))
			return -1;

		if(opmode == WISP_MODE) {
			if(0 == wispWanId)
				iface = "wlan0";
			else if(1 == wispWanId)
				iface = "wlan1";
		}
#ifdef RTK_USB3G
        else if (wantype == USB3G)
            iface = PPPOE_IF;
#endif
		else
			iface = WAN_IF;

	#if defined(CONFIG_4G_LTE_SUPPORT)
		if (lte_wan()) {
			iface = "usb0";
		}
		//printf("\n\n\n wanRxPacketNum: iface[%s] \n\n\n", iface);
	#endif /* #if defined(CONFIG_4G_LTE_SUPPORT) */

		if ( getStats(iface, &stats) < 0)
			stats.rx_packets = 0;
		sprintf(buffer, "%d", (int)stats.rx_packets);
   		return req_format_write(wp, buffer);
	}
#endif
	else if(!strcmp(name, "pocketRouter_Mode_countdown")) // 0:non-pocketRouter; 3: Router; 2:Bridge AP; 1:Bridge Client
	{
#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT) || defined(CONFIG_RTL_ULINKER)
		apmib_get( MIB_OP_MODE, (void *)&intVal);
		if(intVal == 1) //opmode is bridge
		{
			apmib_get( MIB_WLAN_MODE, (void *)&intVal);
			if(intVal == 0) //wlan is AP mode
				sprintf(buffer, "%s", "2" );
			else if(intVal == 1) //wlan is client mode
				sprintf(buffer, "%s", "1" );
			else
				sprintf(buffer, "%s", "0" );
		}
		else if(intVal == 0) //opmode is router
		{
			sprintf(buffer, "%s", "3" );
		}

#else
		sprintf(buffer, "%s", "0");
#endif
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "countDownTime_wait")) // 0:non-pocketRouter; 3: Router; 2:Bridge AP; 1:Bridge Client
	{
#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT)
		sprintf(buffer, "%d", WaitCountTime);
#else
		sprintf(buffer, "%s", "1");
#endif
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(argv[0],"vpn_start"))
	{
#if defined(SINGLE_WAN_SUPPORT)
		req_format_write(wp, "");
#else
		req_format_write(wp, "<!--");
#endif
		return 0;
	}	
	else if(!strcmp(argv[0],"vpn_end"))
	{
#if defined(SINGLE_WAN_SUPPORT)
		req_format_write(wp, "");
#else
		req_format_write(wp, "-->");
#endif
		return 0;
	}		
	else if(!strcmp(argv[0],"pptp_comment_start"))
	{
#if defined(PPTP_SUPPORT)
		req_format_write(wp, "");
#else
		req_format_write(wp, "<!--");
#endif
		return 0;
	}
	else if(!strcmp(argv[0],"pptp_comment_end"))
	{
#if defined(PPTP_SUPPORT)
		req_format_write(wp, "");
#else
		req_format_write(wp, "-->");
#endif
		return 0;
	}
	else if(!strcmp(argv[0],"l2tp_comment_start"))
	{
#if defined(L2TP_SUPPORT)
		req_format_write(wp, "");
#else
		req_format_write(wp, "<!--");
#endif
		return 0;
	}
	else if(!strcmp(argv[0],"l2tp_comment_end"))
	{
#if defined(L2TP_SUPPORT)
		req_format_write(wp, "");
#else
		req_format_write(wp, "-->");
#endif
		return 0;
	}
#ifndef APPLY_CHANGE_DIRECT_SUPPORT
	else if(!strcmp(argv[0],"apply_prompt_comment_start")){
		req_format_write(wp, "");
		return 0;		
	}
	else if(!strcmp(argv[0],"apply_prompt_comment_end")){
		req_format_write(wp, "");
		return 0;		
	}
	else if(!strcmp(argv[0],"apply_direct_comment_start")){
		req_format_write(wp, "<!--");
		return 0;		
	}
	else if(!strcmp(argv[0],"apply_direct_comment_end")){
		req_format_write(wp, "-->");
		return 0;		
	}
	else if(!strcmp(argv[0],"apply_direct_jscomment_start")){
		req_format_write(wp, "/*");
		return 0;		
	}
	else if(!strcmp(argv[0],"apply_direct_jscomment_end")){
		req_format_write(wp, "*/");
		return 0;		
	}
#else
	else if(!strcmp(argv[0],"apply_prompt_comment_start")){
		req_format_write(wp, "<!--");
		return 0;		
	}
	else if(!strcmp(argv[0],"apply_prompt_comment_end")){
		req_format_write(wp, "-->");
		return 0;		
	}
	else if(!strcmp(argv[0],"apply_direct_comment_start")){
		req_format_write(wp, "");
		return 0;		
	}
	else if(!strcmp(argv[0],"apply_direct_comment_end")){
		req_format_write(wp, "");
		return 0;		
	}
	else if(!strcmp(argv[0],"apply_direct_jscomment_start")){
		req_format_write(wp, "");
		return 0;		
	}
	else if(!strcmp(argv[0],"apply_direct_jscomment_end")){
		req_format_write(wp, "");
		return 0;		
	}
#endif
#ifdef RTK_USB3G
	else if(!strcmp(argv[0],"usb3g_comment_start"))
	{
		req_format_write(wp, "");
		return 0;
	}
	else if(!strcmp(argv[0],"usb3g_comment_end"))
	{
		req_format_write(wp, "");
		return 0;
	}
	else if(!strcmp(argv[0],("usb3g_jscomment_start")))
	{
		req_format_write(wp, "");
		return 0;
	}
	else if(!strcmp(argv[0],("usb3g_jscomment_end")))
	{
		req_format_write(wp, "");
		return 0;
	}
#else
	else if(!strcmp(argv[0],"usb3g_comment_start"))
	{
		req_format_write(wp, "<!--");
		return 0;
	}
	else if(!strcmp(argv[0],"usb3g_comment_end"))
	{
		req_format_write(wp, "-->");
		return 0;
	}
	else if(!strcmp(argv[0],("usb3g_jscomment_start")))
	{
		req_format_write(wp, "/*");
		return 0;
	}
	else if(!strcmp(argv[0],("usb3g_jscomment_end")))
	{
		req_format_write(wp, "*/");
		return 0;
	}
#endif /* #ifdef RTK_USB3G */
#if defined(CONFIG_4G_LTE_SUPPORT)
	else if(!strcmp(argv[0],"lte4g_comment_start"))
	{
		req_format_write(wp, "");
		return 0;
	}
	else if(!strcmp(argv[0],"lte4g_comment_end"))
	{
		req_format_write(wp, "");
		return 0;
	}
	else if(!strcmp(argv[0],("lte4g_jscomment_start")))
	{
		req_format_write(wp, "");
		return 0;
	}
	else if(!strcmp(argv[0],("lte4g_jscomment_end")))
	{
		req_format_write(wp, "");
		return 0;
	}
#else
	else if(!strcmp(argv[0],"lte4g_comment_start"))
	{
		req_format_write(wp, "<!--");
		return 0;
	}
	else if(!strcmp(argv[0],"lte4g_comment_end"))
	{
		req_format_write(wp, "-->");
		return 0;
	}
	else if(!strcmp(argv[0],("lte4g_jscomment_start")))
	{
		req_format_write(wp, "/*");
		return 0;
	}
	else if(!strcmp(argv[0],("lte4g_jscomment_end")))
	{
		req_format_write(wp, "*/");
		return 0;
	}
#endif /* #if defined(CONFIG_4G_LTE_SUPPORT) */
#ifndef CONFIG_RTL_ETH_802DOT1X_CLIENT_MODE_SUPPORT
	else if(!strcmp(argv[0],("eth1xclient_jscomment_start")))
	{
		req_format_write(wp, "/*");
		return 0;
	}
	else if(!strcmp(argv[0],("eth1xclient_jscomment_end")))
	{
		req_format_write(wp, "*/");
		return 0;
	}
	else if(!strcmp(argv[0],"eth1xclient_comment_start"))
	{
		req_format_write(wp, "<!--");
		return 0;
	}
	else if(!strcmp(argv[0],"eth1xclient_comment_end"))
	{
		req_format_write(wp, "-->");
		return 0;
	}
#else
	else if(!strcmp(argv[0],("eth1xclient_jscomment_start")))
	{
		req_format_write(wp, "");
		return 0;
	}
	else if(!strcmp(argv[0],("eth1xclient_jscomment_end")))
	{
		req_format_write(wp, "");
		return 0;
	}
	else if(!strcmp(argv[0],"eth1xclient_comment_start"))
	{
		req_format_write(wp, "");
		return 0;
	}
	else if(!strcmp(argv[0],"eth1xclient_comment_end"))
	{
		req_format_write(wp, "");
		return 0;
	}
#endif
	else if(!strcmp(argv[0],"rsCertInstall"))
	{
#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
		req_format_write(wp,"wlan0.addItem(\"802.1x Cert Install\", get_form(\"rsCertInstall.htm\",i), \"\", \"Install 802.1x certificates\");");
#endif
		return 0;
	}
	else if(!strcmp(argv[0],"is8021xClient"))
	{
		intVal=0;
#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
		intVal=1;
#endif
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);	
	}
	else if ( !strcmp(name, "eapUserId")) {
		buffer[0]='\0';
#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
		if ( !apmib_get( MIB_WLAN_EAP_USER_ID,  (void *)buffer) )
			return -1;
#endif
  		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "radiusUserName")) {
		buffer[0]='\0';
#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
		if ( !apmib_get( MIB_WLAN_RS_USER_NAME,  (void *)buffer) )
			return -1;
#endif
  		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "radiusUserPass")) {
		buffer[0]='\0';
#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
		if ( !apmib_get( MIB_WLAN_RS_USER_PASSWD,  (void *)buffer) )
			return -1;
#endif
  		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "radiusUserCertPass")) {
		buffer[0]='\0';
#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
		if ( !apmib_get( MIB_WLAN_RS_USER_CERT_PASSWD,  (void *)buffer) )
			return -1;
#endif
  		return req_format_write(wp, "%s", buffer);
	}

	else if ( !strcmp(name, "rsIp")) {
		if ( !apmib_get( MIB_WLAN_RS_IP,  (void *)buffer) )
			return -1;
		if (!memcmp(buffer, "\x0\x0\x0\x0", 4))
			return req_format_write(wp, "");
   		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
	else if ( !strcmp(name, "rsPort")) {
		if ( !apmib_get( MIB_WLAN_RS_PORT, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
 	else if ( !strcmp(name, "rsPassword")) {
		buffer[0]='\0';
		if ( !apmib_get( MIB_WLAN_RS_PASSWORD,  (void *)buffer) )
			return -1;
  		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "accountRsIp")) {
		if ( !apmib_get( MIB_WLAN_ACCOUNT_RS_IP,  (void *)buffer) )
			return -1;
		if (!memcmp(buffer, "\x0\x0\x0\x0", 4))
			return req_format_write(wp, "");
   		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
	else if ( !strcmp(name, "accountRsPort")) {
		if ( !apmib_get( MIB_WLAN_ACCOUNT_RS_PORT, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "accountRsPassword")) {
		buffer[0]='\0';
		if ( !apmib_get( MIB_WLAN_ACCOUNT_RS_PASSWORD,  (void *)buffer) )
			return -1;
		return req_format_write(wp, "%s", buffer);
	}
	else if ( !strcmp(name, "groupRekeyTime")) {
		if ( !apmib_get( MIB_WLAN_WPA_GROUP_REKEY_TIME, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "groupRekeyTimeDay")) {
		if ( !apmib_get( MIB_WLAN_WPA_GROUP_REKEY_TIME, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal/86400 );
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "groupRekeyTimeHr")) {
		if ( !apmib_get( MIB_WLAN_WPA_GROUP_REKEY_TIME, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", (intVal%86400)/3600 );
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "groupRekeyTimeMin")) {
		if ( !apmib_get( MIB_WLAN_WPA_GROUP_REKEY_TIME, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", ((intVal%86400)%3600)/60 );
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "groupRekeyTimeSec")) {
		if ( !apmib_get( MIB_WLAN_WPA_GROUP_REKEY_TIME, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", ((intVal%86400)%3600)%60 );
   		return req_format_write(wp, buffer);
	}
 	else if ( !strcmp(name, "pskValue")) {
//		int i;
		buffer[0]='\0';
		if ( !apmib_get(MIB_WLAN_WPA_PSK,  (void *)buffer) )
			return -1;
		#if 0	//Brad modify 20080703
		for (i=0; i<strlen(buffer); i++)
			buffer[i]='*';
		buffer[i]='\0';
		#endif
		translate_control_code_sprintf(buffer);
   		return req_format_write(wp, buffer);
	}

#ifdef CONFIG_RTK_MESH
 	else if ( !strcmp(name, "meshPskValue")) {
		int i;
		buffer[0]='\0';
		if ( !apmib_get(MIB_WLAN_MESH_WPA_PSK,  (void *)buffer) )
			return -1;
		/*for (i=0; i<strlen(buffer); i++)
			buffer[i]='*';
		buffer[i]='\0';*/	//by brian
		translate_control_code(buffer);
   		return req_format_write(wp, buffer);
	}
#endif

#ifdef WIFI_SIMPLE_CONFIG
 	else if ( !strcmp(name, "pskValueUnmask")) {
		buffer[0]='\0';
		if ( !apmib_get(MIB_WLAN_WPA_PSK,  (void *)buffer) )
			return -1;
		translate_control_code(buffer);
   		return req_format_write(wp, buffer);
	}
 	else if ( !strcmp(name, "wps_key")) {
 		int id;
		apmib_get(MIB_WLAN_WSC_ENC, (void *)&intVal);
		buffer[0]='\0';
		if (intVal == WSC_ENCRYPT_WEP) {
			unsigned char tmp[100];
			apmib_get(MIB_WLAN_WEP, (void *)&intVal);
			apmib_get(MIB_WLAN_WEP_DEFAULT_KEY, (void *)&id);
			if (intVal == 1) {
				if (id == 0)
					id = MIB_WLAN_WEP64_KEY1;
				else if (id == 1)
					id = MIB_WLAN_WEP64_KEY2;
				else if (id == 2)
					id = MIB_WLAN_WEP64_KEY3;
				else
					id = MIB_WLAN_WEP64_KEY4;
				apmib_get(id, (void *)tmp);
				convert_bin_to_str(tmp, 5, buffer);
			}
			else {
				if (id == 0)
					id = MIB_WLAN_WEP128_KEY1;
				else if (id == 1)
					id = MIB_WLAN_WEP128_KEY2;
				else if (id == 2)
					id = MIB_WLAN_WEP128_KEY3;
				else
					id = MIB_WLAN_WEP128_KEY4;
				apmib_get(id, (void *)tmp);
				convert_bin_to_str(tmp, 13, buffer);
			}
		}
		else {
			if (intVal==0 || intVal == WSC_ENCRYPT_NONE)
				strcpy(buffer, "N/A");
			else
				apmib_get(MIB_WLAN_WSC_PSK, (void *)buffer);
		}
		translate_control_code(buffer);
   		return req_format_write(wp,"%s", buffer);
	}
	else if ( !strcmp(name, "wpsRpt_key"))
	{
#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)
 		int id;
 		SetWlan_idx("wlan0-vxd");
		apmib_get(MIB_WLAN_WSC_ENC, (void *)&intVal);
		buffer[0]='\0';
		if (intVal == WSC_ENCRYPT_WEP) {
			unsigned char tmp[100];
			apmib_get(MIB_WLAN_WEP, (void *)&intVal);
			apmib_get(MIB_WLAN_WEP_DEFAULT_KEY, (void *)&id);
			if (intVal == 1) {
				if (id == 0)
					id = MIB_WLAN_WEP64_KEY1;
				else if (id == 1)
					id = MIB_WLAN_WEP64_KEY2;
				else if (id == 2)
					id = MIB_WLAN_WEP64_KEY3;
				else
					id = MIB_WLAN_WEP64_KEY4;
				apmib_get(id, (void *)tmp);
				convert_bin_to_str(tmp, 5, buffer);
			}
			else {
				if (id == 0)
					id = MIB_WLAN_WEP128_KEY1;
				else if (id == 1)
					id = MIB_WLAN_WEP128_KEY2;
				else if (id == 2)
					id = MIB_WLAN_WEP128_KEY3;
				else
					id = MIB_WLAN_WEP128_KEY4;
				apmib_get(id, (void *)tmp);
				convert_bin_to_str(tmp, 13, buffer);
			}
		}
		else {
			if (intVal==0 || intVal == WSC_ENCRYPT_NONE)
				strcpy(buffer, "N/A");
			else
				apmib_get(MIB_WLAN_WSC_PSK, (void *)buffer);
		}

		wlan_idx = wlan_idx_keep;
		sprintf(tmpStr,"wlan%d",wlan_idx);
		SetWlan_idx(tmpStr);
		//SetWlan_idx("wlan0");

#else
	bzero(buffer,sizeof(buffer));	
#endif //#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)
   		return req_format_write(wp, buffer);
	}

#endif 	// WIFI_SIMPLE_CONFIG
 	else if ( !strcmp(name, "wdsPskValue")) {
		int i;
		buffer[0]='\0';
		if ( !apmib_get(MIB_WLAN_WDS_PSK,  (void *)buffer) )
			return -1;
#if 0
		for (i=0; i<strlen(buffer); i++)
			buffer[i]='*';
		buffer[i]='\0';
#endif
		translate_control_code(buffer);
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "accountRsUpdateDelay")) {
		if ( !apmib_get( MIB_WLAN_ACCOUNT_RS_UPDATE_DELAY, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "rsInterval")) {
		if ( !apmib_get( MIB_WLAN_RS_INTERVAL_TIME, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "accountRsInterval")) {
		if ( !apmib_get( MIB_WLAN_ACCOUNT_RS_INTERVAL_TIME, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}

#ifdef HOME_GATEWAY
#ifdef VPN_SUPPORT
	else if( !strcmp(name, "vpnTblIdx")) {
              	sprintf(buffer, "%d", getVpnTblIdx());
                return req_format_write(wp, "%s", buffer);
	}
	else if( !strcmp(name, "ipsecConnName")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%s", ""); // default
		else
			sprintf(buffer, "%s", entry.connName);

                return req_format_write(wp, "%s", buffer);
	}
	else if( !strcmp(name, "ipsecLocalIp")) {
                if ( getIpsecInfo(&entry) < 0){
			if(getInAddr(BRIDGE_IF, IP_ADDR, (void *)&intaddr ))
			 	return req_format_write(wp, "%s", inet_ntoa(intaddr) );
			else{
 				if ( !apmib_get( MIB_IP_ADDR,  (void *)buffer) )
					 return req_format_write(wp, "0.0.0.0");
				return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
			}
		}
		else
                	return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *) entry.lc_ipAddr)));
	}
	else if( !strcmp(name, "ipsecLocalIpMask")) {
                if ( getIpsecInfo(&entry) < 0){
			if ( getInAddr(BRIDGE_IF, SUBNET_MASK, (void *)&intaddr ))
				return req_format_write(wp, "%s", inet_ntoa(intaddr) );
			else{
 				if ( !apmib_get( MIB_SUBNET_MASK,  (void *)buffer) )
					 return req_format_write(wp, "0.0.0.0");
				return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
			}
		}
		else{
			len2Mask(entry.lc_maskLen, buffer);
                	return req_format_write(wp, "%s", buffer);
		}
	}
	else if( !strcmp(name, "ipsecRemoteIp")) {
                if ( getIpsecInfo(&entry) < 0)
			 return req_format_write(wp, "0.0.0.0");
		else
                	return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *) entry.rt_ipAddr)));
	}
	else if( !strcmp(name, "ipsecRemoteIpMask")) {
                if ( getIpsecInfo(&entry) < 0)
			 return req_format_write(wp, "0.0.0.0");
		else{
			len2Mask(entry.rt_maskLen, buffer);
                	return req_format_write(wp, "%s", buffer);
		}
	}
	else if( !strcmp(name, "ipsecRemoteGateway")) {
                if ( getIpsecInfo(&entry) < 0)
			 return req_format_write(wp, "0.0.0.0");
		else
                	return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *) entry.rt_gwAddr)));

	}
	else if( !strcmp(name, "ipsecSpi")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%s", ""); // default
		else
			sprintf(buffer, "%s",entry.spi);

                return req_format_write(wp, "%s", buffer);
	}
	else if( !strcmp(name, "ipsecEncrKey")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%s", ""); // default
		else
			sprintf(buffer, "%s",entry.encrKey);

                return req_format_write(wp, "%s", buffer);
	}
	else if( !strcmp(name, "ipsecAuthKey")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%s", ""); // default
		else
			sprintf(buffer, "%s",entry.authKey);

                return req_format_write(wp, "%s", buffer);
	}
	else if( !strcmp(name, "ikePsKey")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%s", ""); // default
		else
			sprintf(buffer, "%s",entry.psKey);

                return req_format_write(wp, "%s", buffer);
	}
	else if( !strcmp(name, "ikeLifeTime")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", 3600); // default
		else
			sprintf(buffer, "%lu",entry.ikeLifeTime);

                return req_format_write(wp, "%s", buffer);
	}
	else if( !strcmp(name, "ikeEncr")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", TRI_DES_ALGO); // default
		else
			sprintf(buffer, "%d",entry.ikeEncr);

                return req_format_write(wp, "%s", buffer);
	}
	else if( !strcmp(name, "ikeAuth")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", MD5_ALGO); // default
		else
			sprintf(buffer, "%d",entry.ikeAuth);

                return req_format_write(wp, "%s", buffer);
	}
	else if( !strcmp(name, "ikeKeyGroup")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", DH2_GRP); // default 768 bits
		else
			sprintf(buffer, "%d",entry.ikeKeyGroup);

                return req_format_write(wp, "%s", buffer);
	}
	else if( !strcmp(name, "ipsecLifeTime")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", 28800); // default
		else
			sprintf(buffer, "%lu",entry.ipsecLifeTime);

                return req_format_write(wp, "%s", buffer);
	}
	else if( !strcmp(name, "ipsecPfs")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", 1); // default  on
		else
			sprintf(buffer, "%d",entry.ipsecPfs);

                return req_format_write(wp, "%s", buffer);
	}
	else if( !strcmp(name, "ipsecLocalId")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%s", "");
		else
			sprintf(buffer, "%s",entry.lcId);

                return req_format_write(wp, "%s", buffer);
	}
	else if( !strcmp(name, "ipsecRemoteId")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%s", "");
		else
			sprintf(buffer, "%s",entry.rtId);

                return req_format_write(wp, "%s", buffer);
	}
	else if( !strcmp(name, "rtRsaKey")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%s", "");
		else
			sprintf(buffer, "%s",entry.rsaKey);
                return req_format_write(wp, "%s", buffer);
	}
#endif
#endif
	else if ( !strcmp(name, "userName")) {
		buffer[0]='\0';
                if ( !apmib_get(MIB_USER_NAME,  (void *)buffer) )
                        return -1;
                return req_format_write(wp, "%s", buffer);
	}

#ifdef WLAN_EASY_CONFIG
	else if ( !strcmp(name, "autoCfgAlgReq")) {
		apmib_get( MIB_WLAN_MODE, (void *)&intVal);
		if (intVal==CLIENT_MODE) { // client
			if ( !apmib_get( MIB_WLAN_EASYCFG_ALG_REQ, (void *)&intVal) )
				return -1;
		}
		else {
			if ( !apmib_get( MIB_WLAN_EASYCFG_ALG_SUPP, (void *)&intVal) )
				return -1;
		}
		buffer[0]='\0';
		if (intVal & ACF_ALGORITHM_WEP64)
			strcat(buffer, "WEP64");
		if (intVal & ACF_ALGORITHM_WEP128) {
			if (strlen(buffer) > 0)
				strcat(buffer, "+");
			strcat(buffer, "WEP128");
		}
		if (intVal & ACF_ALGORITHM_WPA_TKIP) {
			if (strlen(buffer) > 0)
				strcat(buffer, "+");
			strcat(buffer, "WPA_TKIP");
		}
		if (intVal & ACF_ALGORITHM_WPA_AES) {
			if (strlen(buffer) > 0)
				strcat(buffer, "+");
			strcat(buffer, "WPA_AES");
		}
		if (intVal & ACF_ALGORITHM_WPA2_TKIP) {
			if (strlen(buffer) > 0)
				strcat(buffer, "+");
			strcat(buffer, "WPA2_TKIP");
		}
		if (intVal & ACF_ALGORITHM_WPA2_AES) {
			if (strlen(buffer) > 0)
				strcat(buffer, "+");
			strcat(buffer, "WPA2_AES");
		}
   		return req_format_write(wp, buffer);
	}

	else if ( !strcmp(name, "autoCfgKey")) {
		if ( !apmib_get( MIB_WLAN_EASYCFG_KEY, (void *)buffer) )
			return -1;
		return req_format_write(wp, buffer);
	}
#endif // WLAN_EASY_CONFIG
#ifdef USE_AUTH
   	else if (!strcmp(name, "last_url")) {
		req_format_write(wp, "%s", last_url);
		return 0;
   	}	
#endif
#ifdef WIFI_SIMPLE_CONFIG
	else if ( !strcmp(name, "wscLoocalPin")) {
		buffer[0] = '\0';
		apmib_get(MIB_HW_WSC_PIN,  (void *)buffer);
		return req_format_write(wp, "%s", buffer);
	}
#endif // WIFI_SIMPLE_CONFIG
	else if(!strcmp(name, "powerConsumption_menu"))
	{
#if defined(POWER_CONSUMPTION_SUPPORT)
		//return req_format_write(wp, "manage.addItem('Power Consumption', 'powerConsumption.htm', '', 'Display power consumption');" );
		return req_format_write(wp,""); // keith. hidden page even enable power saving.
#else
		return req_format_write(wp,"");
#endif
	}
	else if(!strcmp(name, "ethernet802dot1xCert_menu"))
	{
#if defined(CONFIG_RTL_ETH_802DOT1X_CLIENT_MODE_SUPPORT)
		return req_format_write(wp,"manage.addItem(\"Ethernet 802.1x Cert Install\", get_form(\"ethdot1xCertInstall.htm\",i), \"\", \"Install Ethernet 802.1x certificates\");");
#else
		return req_format_write(wp,"");
#endif
	}
	else if(!strcmp(name, "ip_filter"))
	{
#ifdef CONFIG_IPV6
        return req_format_write(wp, "firewall.addItem('IP Filtering', 'ip6filter.htm', '', 'Setup IP filering');" );
#else
		return req_format_write(wp, "firewall.addItem('IP Filtering', 'ipfilter.htm', '', 'Setup IP filering');" );
#endif
    }
	else if(!strcmp(name, "port_filter"))
	{
#ifdef CONFIG_IPV6
        return req_format_write(wp, "firewall.addItem('Port Filtering', 'portfilter6.htm', '', 'Setup port filer');" );
#else
		return req_format_write(wp, "firewall.addItem('Port Filtering', 'portfilter.htm', '', 'Setup port filer');" );
#endif
    }
	else if(!strcmp(name, "is_ulinker"))
	{
#if defined(CONFIG_RTL_ULINKER)
		sprintf(buffer, "%s", "1" );
#else
		sprintf(buffer, "%s", "0");
#endif
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "is_wan_link"))
	{
		int ret = getWanLink("eth1");
		if (ret < 0)
			sprintf(buffer, "%s", "0");
		else
			sprintf(buffer, "%s", "1");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "use_boa_new_ui"))
	{
#ifdef CONFIG_APP_BOA_NEW_UI
		sprintf(buffer, "%s", "1");
#else
		sprintf(buffer, "%s", "0");
#endif
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "include_css"))
	{
	//printf("%s:%d\n",__FUNCTION__,__LINE__);
	 	return getIncludeCss(wp);
	}
	else if(!strcmp(name, "isPocketRouter"))
        {
#if defined(CONFIG_POCKET_ROUTER_SUPPORT) || defined(CONFIG_RTL_ULINKER)
			sprintf(buffer, "%s", "1" );
#else
		sprintf(buffer, "%s", "0");
#endif
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "pocketRouter_Mode")) // 0:non-pocketRouter; 3: Router; 2:Bridge AP; 1:Bridge Client
	{
#if defined(CONFIG_POCKET_ROUTER_SUPPORT) || defined(CONFIG_RTL_ULINKER)
		apmib_get( MIB_OP_MODE, (void *)&intVal);
		if(intVal == 1) //opmode is bridge
		{
			apmib_get( MIB_WLAN_MODE, (void *)&intVal);
			if(intVal == 0) //wlan is AP mode
			{
				sprintf(buffer, "%s", "2" );
			}
			else if(intVal == 1) //wlan is client mode
			{
				sprintf(buffer, "%s", "1" );
			}
			else
			{
				sprintf(buffer, "%s", "0" );
			}
		}
		else if(intVal == 0) //opmode is router
		{
			sprintf(buffer, "%s", "3" );
		}
		
#elif defined(CONFIG_POCKET_AP_SUPPORT)
		apmib_get( MIB_WLAN_MODE, (void *)&intVal);
		if(intVal == 0) //wlan is AP mode
 		{
			sprintf(buffer, "%s", "2" );
		} else {
			sprintf(buffer, "%s", "1" );
		}
#else
		sprintf(buffer, "%s", "0");

#endif
		return req_format_write(wp, buffer);
	}
#ifdef HOME_GATEWAY
	else if ( !strcmp(name, "ddnsDomainName")) {
		if ( !apmib_get( MIB_DDNS_DOMAIN_NAME, (void *)&buffer) )
			return -1;
   		return req_format_write(wp, buffer);

	}
	else if ( !strcmp(name, "ddnsUser")) {
		if ( !apmib_get( MIB_DDNS_USER, (void *)&buffer) )
			return -1;
   		return req_format_write(wp, buffer);

	}
	else if ( !strcmp(name, "ddnsPassword")) {
		if ( !apmib_get( MIB_DDNS_PASSWORD, (void *)&buffer) )
			return -1;
   		return req_format_write(wp, buffer);

	}
	/*
	else if(!strcmp(name, "isPocketRouter"))
	{
#if defined(CONFIG_POCKET_ROUTER_SUPPORT) || defined(CONFIG_RTL_8198_AP_ROOT)
		sprintf(buffer, "%s", "1" );
#else
		sprintf(buffer, "%s", "0");
#endif
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "pocketRouter_Mode")) // 0:non-pocketRouter; 3: Router; 2:Bridge AP; 1:Bridge Client
	{
#if defined(CONFIG_POCKET_ROUTER_SUPPORT)
		apmib_get( MIB_OP_MODE, (void *)&intVal);
		if(intVal == 1) //opmode is bridge
		{
			apmib_get( MIB_WLAN_MODE, (void *)&intVal);
			if(intVal == 0) //wlan is AP mode
			{
				sprintf(buffer, "%s", "2" );
			}
			else if(intVal == 1) //wlan is client mode
			{
				sprintf(buffer, "%s", "1" );
			}
			else
			{
				sprintf(buffer, "%s", "0" );
			}
		}
		else if(intVal == 0) //opmode is router
		{
			sprintf(buffer, "%s", "3" );
		}
#elif defined(CONFIG_POCKET_AP_SUPPORT)
		apmib_get( MIB_WLAN_MODE, (void *)&intVal);
		if(intVal == 0) //wlan is AP mode
 		{
			sprintf(buffer, "%s", "2" );
		} else {
			sprintf(buffer, "%s", "1" );
		}
#else
		sprintf(buffer, "%s", "0");

#endif
		return req_format_write(wp, buffer);
	}*/

#if defined(VLAN_CONFIG_SUPPORTED)
	else if(!strcmp(name, "maxWebVlanNum"))
	{
#if defined(CONFIG_RTL_8198_AP_ROOT) && defined(GMII_ENABLED)
		sprintf(buffer, "%d", MAX_IFACE_VLAN_CONFIG-2 );
#else
		sprintf(buffer, "%d", MAX_IFACE_VLAN_CONFIG-1);
#endif
		return req_format_write(wp, buffer);
	}
#endif
	else if(!strcmp(name, "vlanOnOff"))
	{
#if defined(VLAN_CONFIG_SUPPORTED)
		apmib_get( MIB_VLANCONFIG_ENABLED, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
#else
		sprintf(buffer, "%d", 0 );
		return req_format_write(wp, buffer);
#endif
	}	
	else if(!strcmp(name, "rtlMultiLanDev"))
	{
#if defined(CONFIG_RTL_MULTI_LAN_DEV)		
		sprintf(buffer, "%d", 1 );
		return req_format_write(wp, buffer);
#else
		sprintf(buffer, "%d", 0 );
		return req_format_write(wp, buffer);
#endif
	}	
	else if(!strcmp(name, "vlan_menu_onoff"))
	{
#if defined(VLAN_CONFIG_SUPPORTED)
		return req_format_write(wp, "firewall.addItem('VLAN', 'vlan.htm', '', 'Setup VLAN');" );
#else
		return req_format_write(wp,"");
#endif
	}
	else if(!strcmp(name, "8021q_vlan_menu_onoff"))
	{
#if defined(CONFIG_8021Q_VLAN_SUPPORTED)
		return req_format_write(wp, "firewall.addItem('802.1Q VLAN', '8021q_vlan.htm', '', 'Setup 802.1Q VLAN');" );
#else
		return req_format_write(wp,"");
#endif
	}
    else if(!strcmp(name, "vlan_wan_menu_onoff"))
    {
#if defined(CONFIG_RTK_VLAN_WAN_TAG_SUPPORT)
        return req_format_write(wp, "firewall.addItem('VLAN_WAN', 'vlan_wan.htm', '', 'Setup VLAN WAN TAG');" );
#else
        return req_format_write(wp,"");
#endif
    }
#if defined(CONFIG_RTK_VLAN_WAN_TAG_SUPPORT)
	else if(!strcmp(name, "vlan_wan_enable"))
	{
		apmib_get( MIB_VLAN_WAN_ENALE, (void *)&intVal);
		sprintf(buffer, "%s", intVal ? "checked" : "");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_tag"))
	{
		apmib_get( MIB_VLAN_WAN_TAG, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_bridge_enable"))
	{
		apmib_get( MIB_VLAN_WAN_BRIDGE_ENABLE, (void *)&intVal);
		sprintf(buffer, "%s", intVal ? "checked" : "");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_bridge_tag"))
	{
		apmib_get( MIB_VLAN_WAN_BRIDGE_TAG, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_bridge_port_0"))
	{
		apmib_get( MIB_VLAN_WAN_BRIDGE_PORT, (void *)&intVal);
		sprintf(buffer, "%s", (intVal&(1<<3)) ? "checked" : "");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_bridge_port_1"))
	{
		apmib_get( MIB_VLAN_WAN_BRIDGE_PORT, (void *)&intVal);
		sprintf(buffer, "%s", (intVal&(1<<2)) ? "checked" : "");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_bridge_port_2"))
	{
		apmib_get( MIB_VLAN_WAN_BRIDGE_PORT, (void *)&intVal);
		sprintf(buffer, "%s", (intVal&(1<<1)) ? "checked" : "");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_bridge_port_3"))
	{
		apmib_get( MIB_VLAN_WAN_BRIDGE_PORT, (void *)&intVal);
		sprintf(buffer, "%s", (intVal&(1<<0)) ? "checked" : "");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_bridge_port_wifi_root"))
	{
		apmib_get( MIB_VLAN_WAN_BRIDGE_PORT, (void *)&intVal);
		sprintf(buffer, "%s", (intVal&(1<<6)) ? "checked" : "");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_bridge_port_wifi_vap0"))
	{
		apmib_get( MIB_VLAN_WAN_BRIDGE_PORT, (void *)&intVal);
		sprintf(buffer, "%s", (intVal&(1<<7)) ? "checked" : "");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_bridge_port_wifi_vap1"))
	{
		apmib_get( MIB_VLAN_WAN_BRIDGE_PORT, (void *)&intVal);
		sprintf(buffer, "%s", (intVal&(1<<8)) ? "checked" : "");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_bridge_port_wifi_vap2"))
	{
		apmib_get( MIB_VLAN_WAN_BRIDGE_PORT, (void *)&intVal);
		sprintf(buffer, "%s", (intVal&(1<<9)) ? "checked" : "");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_bridge_port_wifi_vap3"))
	{
		apmib_get( MIB_VLAN_WAN_BRIDGE_PORT, (void *)&intVal);
		sprintf(buffer, "%s", (intVal&(1<<10)) ? "checked" : "");
		return req_format_write(wp, buffer);
	}


	else if(!strcmp(name, "vlan_wan_bridge_multicast_enable"))
	{
		apmib_get( MIB_VLAN_WAN_BRIDGE_MULTICAST_ENABLE, (void *)&intVal);
		sprintf(buffer, "%s", intVal ? "checked" : "");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_bridge_multicast_tag"))
	{
		apmib_get( MIB_VLAN_WAN_BRIDGE_MULTICAST_TAG, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_host_enable"))
	{
		apmib_get( MIB_VLAN_WAN_HOST_ENABLE, (void *)&intVal);
		sprintf(buffer, "%s", intVal ? "checked" : "");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_host_tag"))
	{
		apmib_get( MIB_VLAN_WAN_HOST_TAG, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_host_pri"))
	{
		apmib_get( MIB_VLAN_WAN_HOST_PRI, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_wifi_root_enable"))
	{
		apmib_get( MIB_VLAN_WAN_WIFI_ROOT_ENABLE, (void *)&intVal);
		sprintf(buffer, "%s", intVal ? "checked" : "");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_wifi_root_tag"))
	{
		apmib_get( MIB_VLAN_WAN_WIFI_ROOT_TAG, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_wifi_root_pri"))
	{
		apmib_get( MIB_VLAN_WAN_WIFI_ROOT_PRI, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_wifi_vap0_enable"))
	{
		apmib_get( MIB_VLAN_WAN_WIFI_VAP0_ENABLE, (void *)&intVal);
		sprintf(buffer, "%s", intVal ? "checked" : "");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_wifi_vap0_tag"))
	{
		apmib_get( MIB_VLAN_WAN_WIFI_VAP0_TAG, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_wifi_vap0_pri"))
	{
		apmib_get( MIB_VLAN_WAN_WIFI_VAP0_PRI, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_wifi_vap1_enable"))
	{
		apmib_get( MIB_VLAN_WAN_WIFI_VAP1_ENABLE, (void *)&intVal);
		sprintf(buffer, "%s", intVal ? "checked" : "");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_wifi_vap1_tag"))
	{
		apmib_get( MIB_VLAN_WAN_WIFI_VAP1_TAG, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_wifi_vap1_pri"))
	{
		apmib_get( MIB_VLAN_WAN_WIFI_VAP1_PRI, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_wifi_vap2_enable"))
	{
		apmib_get( MIB_VLAN_WAN_WIFI_VAP2_ENABLE, (void *)&intVal);
		sprintf(buffer, "%s", intVal ? "checked" : "");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_wifi_vap2_tag"))
	{
		apmib_get( MIB_VLAN_WAN_WIFI_VAP2_TAG, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_wifi_vap2_pri"))
	{
		apmib_get( MIB_VLAN_WAN_WIFI_VAP2_PRI, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_wifi_vap3_enable"))
	{
		apmib_get( MIB_VLAN_WAN_WIFI_VAP3_ENABLE, (void *)&intVal);
		sprintf(buffer, "%s", intVal ? "checked" : "");
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_wifi_vap3_tag"))
	{
		apmib_get( MIB_VLAN_WAN_WIFI_VAP3_TAG, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "vlan_wan_wifi_vap3_pri"))
	{
		apmib_get( MIB_VLAN_WAN_WIFI_VAP3_PRI, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
#endif

	else if ( !strcmp(name, "wlanMode")) {
		if ( !apmib_get( MIB_WLAN_MODE, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal);
		return req_format_write(wp,buffer);
		return 0;
	}
	else if ( !strcmp(name, "wlanModeByStr")) {
		if ( !apmib_get( MIB_WLAN_MODE, (void *)&intVal) )
			return -1;
        if(intVal==AP_MODE){
		    sprintf(buffer, "%s", "AP");
        }else   if(intVal==CLIENT_MODE){
		    sprintf(buffer, "%s", "STA");
        }else{
		    sprintf(buffer, "%d", intVal);
        }
		return req_format_write(wp,buffer);
	}    
#if defined(GW_QOS_ENGINE)
	else if ( !strcmp(name, "qosEnabled")) {
		if ( !apmib_get( MIB_QOS_ENABLED, (void *)&intVal) )
			return -1;
		if ( intVal == 0 )
			strcpy(buffer, "false");
		else
			strcpy(buffer, "true");
		return req_format_write(wp, buffer);	}
	else if ( !strcmp(name, "qosAutoUplinkSpeed")) {
		if ( !apmib_get( MIB_QOS_AUTO_UPLINK_SPEED, (void *)&intVal) )
			return -1;
		if ( intVal == 0 )
			strcpy(buffer, "false");
		else
			strcpy(buffer, "true");
		return req_format_write(wp, buffer);	}
	else if ( !strcmp(name, "qosManualUplinkSpeed")) {
		if ( !apmib_get( MIB_QOS_MANUAL_UPLINK_SPEED, (void *)&intVal) )
			return -1;
	       sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);	}
	else if ( !strcmp(name, "qosManualDownlinkSpeed")) {
		if ( !apmib_get( MIB_QOS_MANUAL_DOWNLINK_SPEED, (void *)&intVal) )
			return -1;

		sprintf(buffer, "%d", intVal );

		return req_format_write(wp, buffer);	}

#elif defined(QOS_BY_BANDWIDTH)
	else if ( !strcmp(name, "qosEnabled")) {
		if ( !apmib_get( MIB_QOS_ENABLED, (void *)&intVal) )
			return -1;
		if ( intVal == 0 )
			strcpy(buffer, "false");
		else
			strcpy(buffer, "true");
		return req_format_write(wp, buffer);	}
	else if ( !strcmp(name, "qosAutoUplinkSpeed")) {
		if ( !apmib_get( MIB_QOS_AUTO_UPLINK_SPEED, (void *)&intVal) )
			return -1;
		if ( intVal == 0 )
			strcpy(buffer, "false");
		else
			strcpy(buffer, "true");
		return req_format_write(wp, buffer);	}
	else if ( !strcmp(name, "qosManualUplinkSpeed")) {
		if ( !apmib_get( MIB_QOS_MANUAL_UPLINK_SPEED, (void *)&intVal) )
			return -1;

	  sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);	}
	else if ( !strcmp(name, "qosManualDownlinkSpeed")) {
		if ( !apmib_get( MIB_QOS_MANUAL_DOWNLINK_SPEED, (void *)&intVal) )
			return -1;

		sprintf(buffer, "%d", intVal );

		return req_format_write(wp, buffer);	}
#endif

#ifdef DOS_SUPPORT
	else if ( !strcmp(name, "syssynFlood")) {
		if ( !apmib_get( MIB_DOS_SYSSYN_FLOOD, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);	}
	else if ( !strcmp(name, "sysfinFlood")) {
		if ( !apmib_get( MIB_DOS_SYSFIN_FLOOD, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);	}
	else if ( !strcmp(name, "sysudpFlood")) {
		if ( !apmib_get( MIB_DOS_SYSUDP_FLOOD, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);	}
	else if ( !strcmp(name, "sysicmpFlood")) {
		if ( !apmib_get( MIB_DOS_SYSICMP_FLOOD, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);	}
	else if ( !strcmp(name, "pipsynFlood")) {
		if ( !apmib_get( MIB_DOS_PIPSYN_FLOOD, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);	}
	else if ( !strcmp(name, "pipfinFlood")) {
		if ( !apmib_get( MIB_DOS_PIPFIN_FLOOD, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);	}
	else if ( !strcmp(name, "pipudpFlood")) {
		if ( !apmib_get( MIB_DOS_PIPUDP_FLOOD, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);	}
	else if ( !strcmp(name, "pipicmpFlood")) {
		if ( !apmib_get( MIB_DOS_PIPICMP_FLOOD, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);	}
	else if ( !strcmp(name, "blockTime")) {
		if ( !apmib_get( MIB_DOS_BLOCK_TIME, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);	}
#endif // DOS_SUPPORT
	else if ( !strcmp(name, "hostName")) {
		if ( !apmib_get( MIB_HOST_NAME, (void *)&buffer) )
			return -1;
   		return req_format_write(wp, buffer);
	}
#endif // HOME_GATEWAY
#ifdef CONFIG_RTL_8198_AP_ROOT || defined(CONFIG_RTL_8197D_AP)
#if defined(VLAN_CONFIG_SUPPORTED) 
		else if(!strcmp(name, "vlan_menu_onoff"))
		{
			return req_format_write(wp, "menu.addItem('VLAN', 'vlan.htm', '', 'Setup VLAN');" );
		}
		else if(!strcmp(name, "maxWebVlanNum"))
		{
#if defined(CONFIG_RTL_8198_AP_ROOT) && defined(GMII_ENABLED)
			sprintf(buffer, "%d", MAX_IFACE_VLAN_CONFIG-2 );
#else
			sprintf(buffer, "%d", MAX_IFACE_VLAN_CONFIG-1);
#endif
			return req_format_write(wp, buffer);
		}
		else if(!strcmp(name, "vlanOnOff"))
		{
			apmib_get( MIB_VLANCONFIG_ENABLED, (void *)&intVal);
			sprintf(buffer, "%d", intVal );
			return req_format_write(wp, buffer);
		}
		else if ( !strcmp(name, "wlanMode")) {
			if ( !apmib_get( MIB_WLAN_MODE, (void *)&intVal) )
				return -1;
			sprintf(buffer, "%d", intVal);
			return req_format_write(wp,buffer);
			return 0;
		}
		else if ( !strcmp(name, "rf_used")) {
			struct _misc_data_ misc_data;
			if (getMiscData(WLAN_IF, &misc_data) < 0)
				return -1;
			sprintf(buffer, "%d", misc_data.mimo_tr_used);
			req_format_write(wp, buffer);
			return 0;
		}
#endif
#endif
	else if ( !strcmp(name, "rtLogServer")) {
		if ( !apmib_get( MIB_REMOTELOG_SERVER,  (void *)buffer) )
			return -1;
		if (!memcmp(buffer, "\x0\x0\x0\x0", 4))
			return req_format_write(wp, "");
   		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
	else if ( !strcmp(name, "domainName")) {
		if ( !apmib_get( MIB_DOMAIN_NAME, (void *)&buffer) )
			return -1;
   		return req_format_write(wp, buffer);
	}
#if defined(CONFIG_SNMP)
    else if (!strcmp(name, "snmp_name")) {
            if (!apmib_get(MIB_SNMP_NAME, (void *)buffer)) {
                    return -1;
            }
	    translate_control_code(buffer);
            req_format_write(wp, "%s", buffer);
            return 0;
    }
    else if (!strcmp(name, "snmp_location")) {
            if (!apmib_get(MIB_SNMP_LOCATION, (void *)buffer)) {
                    return -1;
            }
	    translate_control_code(buffer);
            req_format_write(wp, "%s", buffer);
            return 0;
    }
    else if (!strcmp(name, "snmp_contact")) {
            if (!apmib_get(MIB_SNMP_CONTACT, (void *)buffer)) {
                    return -1;
            }
	    translate_control_code(buffer);
            req_format_write(wp, "%s", buffer);
            return 0;
    }
    else if (!strcmp(name, "snmp_rwcommunity")) {
            if (!apmib_get(MIB_SNMP_RWCOMMUNITY, (void *)buffer)) {
                    return -1;
            }
            req_format_write(wp, "%s", buffer);
            return 0;
    }
	else if (!strcmp(name, "snmp_rocommunity")) {
            if (!apmib_get(MIB_SNMP_ROCOMMUNITY, (void *)buffer)) {
                    return -1;
            }
            req_format_write(wp, "%s", buffer);
            return 0;
    }
    else if (!strcmp(argv[0], "snmp_trap1")) {
            if (!apmib_get(MIB_SNMP_TRAP_RECEIVER1, (void *)buffer)) {
                    return -1;
            }
            req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)));
            return 0;
    }
    else if (!strcmp(name, "snmp_trap2")) {
            if (!apmib_get(MIB_SNMP_TRAP_RECEIVER2, (void *)buffer)) {
                    return -1;
            }
            req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)));
            return 0;
    }
    else if (!strcmp(name, "snmp_trap3")) {
            if (!apmib_get(MIB_SNMP_TRAP_RECEIVER3, (void *)buffer)) {
                    return -1;
            }
            req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)));
            return 0;
    }
#endif
#ifdef CONFIG_RTK_MESH

    else if ( !strcmp(name, "meshID")) {
            if ( !apmib_get(MIB_WLAN_MESH_ID,  (void *)buffer) )
                    return -1;
            translate_control_code(buffer);
            return req_format_write(wp, "%s", buffer);
    }

#endif // CONFIG_RTK_MESH
#ifdef UNIVERSAL_REPEATER
	else if ( !strcmp(name, "repeaterSSID")) {
		if (wlan_idx == 0)
			intVal = MIB_REPEATER_SSID1;
#if !defined(CONFIG_RTL_TRI_BAND) 
		else
			intVal = MIB_REPEATER_SSID2;
#else
		else if (wlan_idx == 1)
			intVal = MIB_REPEATER_SSID2;
		else if(wlan_idx == 2)
			intVal = MIB_REPEATER_SSID3;
#endif		
		apmib_get(intVal, (void *)buffer);
		translate_control_code(buffer);
		return req_format_write(wp, "%s", buffer);
   	}
#if 0
   	else if ( !strcmp(name, "repeaterEncrypt")) {
 		ENCRYPT_T encrypt;
   		apmib_get( MIB_WLAN_ENCRYPT,  (void *)&encrypt);
		if (encrypt == ENCRYPT_DISABLED)
			strcpy( buffer, "Disabled");
		else if (encrypt == ENCRYPT_WEP) {
			apmib_get(MIB_WLAN_ENABLE_1X, &intVal);
			if (intVal == 0) {
       			apmib_get( MIB_WLAN_WEP,  (void *)&intVal);
				if ( intVal == WEP_DISABLED )
					strcpy( buffer, "Disabled");
				else if ( intVal == WEP64 )
					strcpy( buffer, "WEP 64bits");
				else if ( intVal == WEP128)
					strcpy( buffer, "WEP 128bits");
			}
			else
				strcpy( buffer, "Disabled");
		}
		else {
			apmib_get(MIB_WLAN_WPA_AUTH, &intVal);
			if (intVal == WPA_AUTH_PSK) {
				if (encrypt == ENCRYPT_WPA2 )
					strcpy( buffer, "WPA2" );
				else
					strcpy( buffer, "WPA" );
			}
			else
				strcpy( buffer, "Disabled");
		}
		return req_format_write(wp, buffer);
   	}
#endif
	else if ( !strcmp(name, "repeaterState")) {
		char *pMsg;
		if (wlan_idx == 0)
			strcpy(buffer, "wlan0-vxd");
		else
			strcpy(buffer, "wlan1-vxd");
		getWlBssInfo(buffer, &bss);
		switch (bss.state) {
		case STATE_DISABLED:
			pMsg = "Disabled";
			break;
		case STATE_IDLE:
			pMsg = "Idle";
			break;
		case STATE_STARTED:
			pMsg = "Started";
			break;
		case STATE_CONNECTED:
			pMsg = "Connected";
			break;
		case STATE_WAITFORKEY:
			pMsg = "Waiting for keys";
			break;
		case STATE_SCANNING:
			pMsg = "Scanning";
			break;
		default:
			pMsg=NULL;
		}
		return req_format_write(wp, "%s", pMsg);
	}
 	else if ( !strcmp(name, "repeaterClientnum")) {
		if (wlan_idx == 0)
			strcpy(buffer, "wlan0-vxd");
		else
			strcpy(buffer, "wlan1-vxd");
 		if(getWlStaNum(buffer, &intVal)<0)
 			intVal=0;
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "repeaterSSID_drv")) {
#if defined(CONFIG_RTL_819X) && !defined(CONFIG_WLAN_REPEATER_MODE)// keith. disabled if no this mode in 96c
		return req_format_write(wp, "%s", "e0:00:19:78:01:10");
#else
		if (wlan_idx == 0)
			strcpy(buffer, "wlan0-vxd");
		else
			strcpy(buffer, "wlan1-vxd");
		getWlBssInfo(buffer, &bss);
		memcpy(buffer, bss.ssid, SSID_LEN+1);
		translate_control_code(buffer);
		return req_format_write(wp, "%s", buffer);
#endif
	}
	else if ( !strcmp(name, "repeaterBSSID")) {
		if (wlan_idx == 0)
			strcpy(buffer, "wlan0-vxd");
		else
			strcpy(buffer, "wlan1-vxd");
		getWlBssInfo(buffer, &bss);
		return req_format_write(wp, "%02x:%02x:%02x:%02x:%02x:%02x", bss.bssid[0], bss.bssid[1],
				bss.bssid[2], bss.bssid[3], bss.bssid[4], bss.bssid[5]);
	}
	else if ( !strcmp(name, "wlanRepeaterTxPacketNum")) {
		if (wlan_idx == 0)
			strcpy(buffer, "wlan0-vxd");
		else
			strcpy(buffer, "wlan1-vxd");
		if ( getStats(buffer, &stats) < 0)
			stats.tx_packets = 0;
		sprintf(buffer, "%d", (int)stats.tx_packets);
   		return req_format_write(wp, buffer);

	}
	else if ( !strcmp(name, "wlanRepeaterRxPacketNum")) {
		if (wlan_idx == 0)
			strcpy(buffer, "wlan0-vxd");
		else
			strcpy(buffer, "wlan1-vxd");
		if ( getStats(buffer, &stats) < 0)
			stats.rx_packets = 0;
		sprintf(buffer, "%d", (int)stats.rx_packets);
   		return req_format_write(wp, buffer);
	}
#endif	// UNIVERSAL_REPEATER
// added by rock /////////////////////////////////////////
#ifdef VOIP_SUPPORT
	else if (!strncmp(name, "voip_", 5)) {
		return asp_voip_getInfo(wp, argc, argv);
	}
#else
	else if (!strncmp(name, "voip_", 5)) {
   		return 0;
	}
#endif

#ifdef CONFIG_APP_RTK_VOIP
	else if (!strcmp(name, "isDisplayVOIP")) {
		sprintf(buffer, "1");
		req_format_write(wp, buffer);
		return 0;
	}
#else
	else if (!strcmp(name, "isDisplayVOIP")) {
		sprintf(buffer, "0");
		req_format_write(wp, buffer);
		return 0;
	}
#endif
/////////added by hf_shi/////////////////
#ifdef CONFIG_IPV6
	else if(!strncmp(name, "IPv6_",5)){
			return getIPv6Info(wp, argc, argv);
	}
#else
	else if (!strncmp(name, "IPv6_", 5)) {
   		return 0;
	}
#endif

/*+++++added by Jack for TR-069 configuration+++++*/
#ifdef CONFIG_APP_TR069
	else if(!strcmp(name, "cwmp_tr069_menu")) {
#if 0
		return req_format_write(wp,
				"menu.addItem('TR-069');" \
				"tr069 = new MTMenu();" \
				"tr069.addItem('TR-069 config', 'tr069config.htm', '', 'Setup TR-069 configuration');" \
				"menu.makeLastSubmenu(tr069);");
#else
		return req_format_write(wp, "manage.addItem('TR-069 config', 'tr069config.htm', '', 'Setup TR-069 configuration');" );
#endif
	}else if(!strcmp(name, "tr069_nojs_menu")) {
		return req_format_write(wp,
				"document.write('"\
				"<tr><td><b>cwmp_tr069_menu</b></td></tr>"\
				"<tr><td><a href=\"tr069config.htm\" target=\"view\">TR-069 config</a></td></tr>"\
				"')");
	}
	else if(!strcmp(name, "ipfilter_menu")) {
#ifdef CONFIG_IPV6
		return req_format_write(wp,
				"document.write('"\
				"<tr><td><a href=\"ip6filter.htm\" target=\"view\">IP Filter</a></td></tr>"\
				"')");
#else
		return req_format_write(wp,
				"document.write('"\
				"<tr><td><a href=\"ipfilter.htm\" target=\"view\">IP Filter</a></td></tr>"\
				"')");
#endif
	}
	else if(!strcmp(name, "portfilter_menu")) {
#ifdef CONFIG_IPV6
		return req_format_write(wp,
				"document.write('"\
				"<tr><td><a href=\"portfilter6.htm\" target=\"view\">Port Filter</a></td></tr>"\
				"')");
#else
		return req_format_write(wp,
				"document.write('"\
				"<tr><td><a href=\"portfilter.htm\" target=\"view\">Port Filter</a></td></tr>"\
				"')");
#endif
	//<tr><td><a href="portfilter.htm" target="view">Port Filtering</a></td></tr>

	}
	else if(!strcmp(name, "acs_url")) {
		if ( !apmib_get( MIB_CWMP_ACS_URL, (void *)buffer) )
			return -1;
		return req_format_write(wp, buffer);

	}else if(!strcmp(name, "acs_username")) {
		if ( !apmib_get( MIB_CWMP_ACS_USERNAME, (void *)buffer) )
			return -1;
		return req_format_write(wp, buffer);

	}else if(!strcmp(name, "acs_password")) {
		if ( !apmib_get( MIB_CWMP_ACS_PASSWORD, (void *)buffer) )
			return -1;
		return req_format_write(wp, buffer);

	}else if(!strcmp(name, "tr069-inform-0")) {
		if ( !apmib_get( MIB_CWMP_INFORM_ENABLE, (void *)&intVal) )
			return -1;
		if(intVal == 1){
			return req_format_write(wp, "");
		}else{
			return req_format_write(wp, "checked");
		}
	}else if(!strcmp(name, "tr069-inform-1")) {
		if ( !apmib_get( MIB_CWMP_INFORM_ENABLE, (void *)&intVal) )
			return -1;
		if(intVal == 1){
			return req_format_write(wp, "checked");
		}else{
			return req_format_write(wp, "");
		}
	}else if(!strcmp(name, "inform_interval")) {
		if ( !apmib_get( MIB_CWMP_INFORM_INTERVAL, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);

	}else if(!strcmp(name, "tr069_interval")) {
		if ( !apmib_get( MIB_CWMP_INFORM_ENABLE, (void *)&intVal) )
			return -1;
		if(intVal == 1){
			return req_format_write(wp, "");
		}else{
			return req_format_write(wp, "disabled");
		}
	}else if(!strcmp(name, "conreq_name")) {
		if ( !apmib_get( MIB_CWMP_CONREQ_USERNAME, (void *)buffer) )
			return -1;
		return req_format_write(wp, buffer);

	}else if(!strcmp(name, "conreq_pw")) {
		if ( !apmib_get( MIB_CWMP_CONREQ_PASSWORD, (void *)buffer) )
			return -1;
		return req_format_write(wp, buffer);

	}else if(!strcmp(name, "conreq_path")) {
		if ( !apmib_get( MIB_CWMP_CONREQ_PATH, (void *)buffer) )
			return -1;
		return req_format_write(wp, buffer);

	}else if(!strcmp(name, "conreq_port")) {
		if ( !apmib_get( MIB_CWMP_CONREQ_PORT, (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);

	}else if(!strcmp(name, "tr069-dbgmsg-0")) {
		if ( !apmib_get( MIB_CWMP_FLAG, (void *)&intVal) )
			return -1;
		if(intVal & CWMP_FLAG_DEBUG_MSG){
			 return req_format_write(wp,"");
		}else{
			return req_format_write(wp,"checked");
		}
	}else if(!strcmp(name, "tr069-dbgmsg-1")) {
		if ( !apmib_get( MIB_CWMP_FLAG, (void *)&intVal) )
			return -1;
		if(intVal & CWMP_FLAG_DEBUG_MSG){
			 return req_format_write(wp,"checked");
		}else{
			 return req_format_write(wp,"");
		}
	}else if(!strcmp(name, "tr069-sendgetrpc-0")) {
		if ( !apmib_get( MIB_CWMP_FLAG, (void *)&intVal) )
			return -1;
		if(intVal & CWMP_FLAG_SENDGETRPC){
			return req_format_write(wp,"");
		}else{
			return req_format_write(wp,"checked");
		}
	}else if(!strcmp(name, "tr069-sendgetrpc-1")) {
		if ( !apmib_get( MIB_CWMP_FLAG, (void *)&intVal) )
			return -1;
		if(intVal & CWMP_FLAG_SENDGETRPC){
			return req_format_write(wp,"checked");
		}else{
			return req_format_write(wp,"");
		}
	}else if(!strcmp(name, "tr069-skipmreboot-0")) {
		if ( !apmib_get( MIB_CWMP_FLAG, (void *)&intVal) )
			return -1;
		if(intVal & CWMP_FLAG_SKIPMREBOOT){
			return req_format_write(wp,"");
		}else{
			return req_format_write(wp,"checked");
		}
	}else if(!strcmp(name, "tr069-skipmreboot-1")) {
		if ( !apmib_get( MIB_CWMP_FLAG, (void *)&intVal) )
			return -1;
		if(intVal & CWMP_FLAG_SKIPMREBOOT){
			return req_format_write(wp,"checked");
		}else{
			return req_format_write(wp,"");
		}
	}else if(!strcmp(name, "tr069-autoexec-0")) {
		if ( !apmib_get( MIB_CWMP_FLAG, (void *)&intVal) )
			return -1;
		if(intVal & CWMP_FLAG_AUTORUN){
			return req_format_write(wp,"");
		}else{
			return req_format_write(wp,"checked");
		}
	}else if(!strcmp(name, "tr069-autoexec-1")) {
		if ( !apmib_get( MIB_CWMP_FLAG, (void *)&intVal) )
			return -1;
		if(intVal & CWMP_FLAG_AUTORUN){
			return req_format_write(wp,"checked");
		}else{
			return req_format_write(wp,"");
		}
	}else if(!strcmp(name, "tr069-delay-0")) {
		if ( !apmib_get( MIB_CWMP_FLAG, (void *)&intVal) )
			return -1;
		if(intVal & CWMP_FLAG_DELAY){
			return req_format_write(wp,"");
		}else{
			return req_format_write(wp,"checked");
		}
	}else if(!strcmp(name, "tr069-delay-1")) {
		if ( !apmib_get( MIB_CWMP_FLAG, (void *)&intVal) )
			return -1;
		if(intVal & CWMP_FLAG_DELAY){
			return req_format_write(wp,"checked");
		}else{
			return req_format_write(wp,"");
		}
	}
#else
	else if(!strcmp(name, "cwmp_tr069_menu") || !strcmp(name, "tr069_nojs_menu") ){
		return 0;
	}
#endif /*CONFIG_APP_TR069*/
/*-----end-----*/

#ifdef CONFIG_RTL_WAPI_SUPPORT
	else if(!strcmp(argv[0],"wapiOption"))
	{
		req_format_write(wp,"<option value=\"7\"> WAPI </option>");
		return 0;
	}
	else if(!strcmp(argv[0],"wapiMenu"))
	{
#if defined(CONFIG_RTL_8198C) || defined(CONFIG_RTL_8198) || defined(CONFIG_POCKET_ROUTER_SUPPORT) || defined(CONFIG_RTL_8196C) || defined(CONFIG_RTL_819XD) || defined(CONFIG_RTL_8196E)
		req_format_write(wp,"menu.addItem(\"WAPI\");");
		req_format_write(wp,"wlan_wapi = new MTMenu();");
//#if !defined(CONFIG_RTL_8196C)
		req_format_write(wp,"wlan_wapi.addItem(\"Certification Install\", \"wlwapiinstallcert.htm\", \"\", \"Install Ceritification\");");
#ifdef CONFIG_RTL_WAPI_LOCAL_AS_SUPPORT
		req_format_write(wp,"wlan_wapi.addItem(\"Certification Manage\", \"wlwapiCertManagement.htm\", \"\", \"Manage Ceritification\");");
#endif
//#endif
		req_format_write(wp,"for(i=0; i < wlan_num ; i++){");
		req_format_write(wp,"wlan_name= \"wlan\" +(i+1);");
		req_format_write(wp,"if(wlan_num == 1)");
		req_format_write(wp,"wlan0_wapi = wlan_wapi ;");
		req_format_write(wp,"else{");
		req_format_write(wp,"if(1 == wlan_support_92D){");
		req_format_write(wp,"if(i==0 && wlan1_phyband != \"\"){");
		req_format_write(wp,"wlan_name=wlan_name+\"(\"+wlan1_phyband+\")\";");
		req_format_write(wp,"}else if(i==1 && wlan2_phyband != \"\"){");
		req_format_write(wp,"wlan_name=wlan_name+\"(\"+wlan2_phyband+\")\";");
		req_format_write(wp,"}else{");
		req_format_write(wp,"continue;}}");
		req_format_write(wp,"if(wlBandMode == 3)");	//3:BANDMODESIGNLE
		req_format_write(wp,"wlan_name = \"wlan1\";");
		req_format_write(wp,"wlan_wapi.addItem(wlan_name);");
		req_format_write(wp,"wlan0_wapi= new MTMenu();}");
		req_format_write(wp,"wlan0_wapi.addItem(\"Key Update\", get_form(\"wlwapiRekey.htm\",i), \"\", \"Key update\");");
		req_format_write(wp,"if(wlan_num != 1)");
		req_format_write(wp,"wlan_wapi.makeLastSubmenu(wlan0_wapi);");
		req_format_write(wp,"}");
		req_format_write(wp,"menu.makeLastSubmenu(wlan_wapi);");
#endif
		return 0;
	}
	else if(!strcmp(argv[0],"wapiCertSupport"))
	{
// 8198 and POCKET ROUTER support both wapi psk and wapi cert
// 8196c (not include POCKET ROUTER) only support wapi psk
//#if defined(CONFIG_RTL_8198) || defined(CONFIG_POCKET_ROUTER_SUPPORT)
#if defined(CONFIG_RTL_8198C) ||defined(CONFIG_RTL_8198) || defined(CONFIG_POCKET_ROUTER_SUPPORT) || defined(CONFIG_RTL_8196C) || defined(CONFIG_RTL_819XD) || defined(CONFIG_RTL_8196E) 

#else
		req_format_write(wp,"disabled");
#endif
		return 0;
	}
	else if(!strcmp(argv[0], "wapiUcastTime"))
	{
		if ( !apmib_get(MIB_WLAN_WAPI_UCAST_TIME,  (void*)&intVal))
			return -1;
		req_format_write(wp, "%d",intVal);
		return 0;
	}else if(!strcmp(argv[0], "wapiUcastPackets"))
	{
		if ( !apmib_get(MIB_WLAN_WAPI_UCAST_PACKETS, (void*)&intVal))
			return -1;
		req_format_write(wp, "%d",intVal);
		return 0;
	}	else if(!strcmp(argv[0], "wapiMcastTime"))
	{
		if ( !apmib_get(MIB_WLAN_WAPI_MCAST_TIME,  (void*)&intVal))
			return -1;
		req_format_write(wp, "%d",intVal);
		return 0;
	}
	else if(!strcmp(argv[0], "wapiMcastPackets"))
	{
		if ( !apmib_get(MIB_WLAN_WAPI_MCAST_PACKETS,  (void*)&intVal))
			return -1;
		req_format_write(wp, "%d",intVal);
		return 0;
	}
	else if(!strcmp(argv[0], "wapiPskValue"))
	{
		if ( !apmib_get(MIB_WLAN_WAPI_PSK,  (void*)buffer))
			return -1;
		translate_control_code(buffer);
		req_format_write(wp, "%s",buffer);
		return 0;
	}else if(!strcmp(argv[0], "wapiASIp"))
	{
		if ( !apmib_get(MIB_WLAN_WAPI_ASIPADDR,  (void*)buffer))
			return -1;
		if (!memcmp(buffer, "\x0\x0\x0\x0", 4))
			return req_format_write(wp, "");
   		return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)));
	}
	else if(!strcmp(argv[0], "wapiCertSel"))
	{
		if ( !apmib_get(MIB_WLAN_WAPI_CERT_SEL,  (void*)&intVal))
			return -1;
		req_format_write(wp, "%d",intVal);
		return 0;
	}
	else if(!strcmp(argv[0],"wapiCert"))
	{
		int index;
		int count;
		int i;
		struct stat status;
		char tmpbuf[10];

		CERTS_DB_ENTRY_Tp cert=(CERTS_DB_ENTRY_Tp)malloc(128*sizeof(CERTS_DB_ENTRY_T));
		//Search Index 1--all, 2--serial.no, 3--owner, 4--type, 5--status
		if (!apmib_get(MIB_WLAN_WAPI_SEARCHINDEX,  (void*)&index))
		{
			free(cert);
			return -1;
		}
		if(!apmib_get(MIB_WLAN_WAPI_SEARCHINFO,  (void*)buffer))
		{
			free(cert);
			return -1;
		}

		/*update wapiCertInfo*/
		system("openssl ca -updatedb 2>/dev/null");
		if (stat(WAPI_CERT_CHANGED, &status) == 0) { // file existed
			system("storeWapiFiles -allUser");
		}

		count=searchWapiCert(cert,index,buffer);
		if(count == 0)
			req_format_write(wp, "%s","[]");
		else
		{
			req_format_write(wp, "%s","[");
			for(i=0;i<count;i++)
			{
				sprintf(tmpbuf, "%08X",cert[i].serial);
				req_format_write(wp,"['%s','%s','%d','%d',",cert[i].userName,tmpbuf,cert[i].validDays,cert[i].validDaysLeft);
				if(0 == cert[i].certType)
				{
					req_format_write(wp,"'%s',","X.509");
				}
				if(0==cert[i].certStatus)
				{
					req_format_write(wp,"'%s'","actived");
				}else if(1 ==cert[i].certStatus)
				{
					req_format_write(wp,"'%s'","expired");
				}else if(2 ==cert[i].certStatus)
				{
					req_format_write(wp,"'%s'","revoked");
				}
				if(i ==(count-1))
					req_format_write(wp, "%s","]");
				else
					req_format_write(wp, "%s","],");
			}
			req_format_write(wp, "%s","]");
		}
		free(cert);
		return 0;
	}
else if(!strcmp(argv[0],"caCertExist"))
	{
		 struct stat status;
		 if (stat(CA_CERT, &status) < 0)
		 {
		 	intVal=0;	//CA_CERT not exist
		 }
		 else
		 {
		 	intVal=1;	//CA_CERT exists
		 }
		 req_format_write(wp, "%d",intVal);
		return 0;
	}
	else if(!strcmp(argv[0],"asCerExist"))
	{
		 struct stat status;
		 if (stat(CA_CER, &status) < 0)
		 {
		 	intVal=0;	//AS_CER not exist
		 }
		 else
		 {
		 	intVal=1;	//AS_CER exists
		 }
		 req_format_write(wp, "%d",intVal);
		return 0;
	}
	else if(!strcmp(argv[0],"notSyncSysTime"))
	{
		 struct stat status;
		 time_t  now;
	        struct tm *tnow;

		 if (stat(SYS_TIME_NOT_SYNC_CA, &status) < 0)
		 {
		 	//SYS_TIME_NOT_SYNC_CA not exist

		 	now=time(0);
                    	tnow=localtime(&now);
                 	//printf("now=%ld, %d %d %d %d %d %d, tm_isdst=%d\n",now, 1900+tnow->tm_year,tnow->tm_mon+1,tnow->tm_mday,tnow->tm_hour,tnow->tm_min,tnow->tm_sec, tnow->tm_isdst);//Added for test

			if(1900+tnow->tm_year < 2009)
			{
				intVal=1;	//current year of our system < 2009 which means our system hasn't sync time yet
			}
			else
			{
		 		intVal=0;	//SYS_TIME_NOT_SYNC_CA not exist and current time >= year 2009 which means our system has sync time already
			}
		 }
		 else
		 {
		 	intVal=1;	//SYS_TIME_NOT_SYNC_CA exists which means our system hasn't sync time yet
		 	sprintf(buffer, "rm -f %s 2>/dev/null", SYS_TIME_NOT_SYNC_CA);
			system(buffer);
		 }
		 req_format_write(wp, "%d",intVal);
		return 0;
	}
	else if(!strcmp(argv[0],"wapiLocalAsSupport"))
	{
#ifdef CONFIG_RTL_WAPI_LOCAL_AS_SUPPORT
		req_format_write(wp,"%s","true");
#else
		req_format_write(wp,"%s","false");
#endif
		return 0;
	}
#else
	else if(!strncmp(argv[0],"wapi",4))
	{
		/*if wapi not enabled*/
		return 0;
	}
	else if(!strcmp(argv[0],"wapiLocalAsSupport"))
	{
		req_format_write(wp,"%s","false");
		return 0;
	}
#endif
	else if(!strcmp(argv[0],"isWapiSupport"))
	{
#if defined(CONFIG_RTL_WAPI_SUPPORT)
		sprintf(buffer, "%s", "1" );
#else
		sprintf(buffer, "%s", "0");
#endif
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(argv[0],"wapiLocalAsOption"))
	{
#ifdef CONFIG_RTL_WAPI_LOCAL_AS_SUPPORT
//		req_format_write(wp,"<option value=\"2\"> Use Cert from Local AS </option>");		
		req_format_write(wp,"<option value=\"1\"> Use Cert from Local AS </option>");
#endif
		return 0;
	}
	else if(!strcmp(argv[0],"wapiLocalAsCertsUploadForm"))
	{
#ifdef CONFIG_RTL_WAPI_LOCAL_AS_SUPPORT
		req_format_write(wp,"<form method=\"post\" action=\"/boafrm/formUploadWapiCert\" enctype=\"multipart/form-data\" name=\"uploadCACert\">");
		req_format_write(wp,"<table border=\"0\" cellspacing=\"0\" width=\"500\">");
		req_format_write(wp,"<tr><font size=2></font></tr>");
		req_format_write(wp,"<tr><hr size=1 noshade align=top></tr>");
		req_format_write(wp,"<tr><td width=\"0.55\"><font size=2><b>Certificate Type of Local AS:</b></font></td>");
		req_format_write(wp,"<td width=\"0.45\"><font size=2> <input name=\"cert_type\" type=radio value=0 checked>X.509</font></td></tr>");
		req_format_write(wp,"<tr><td width=\"0.55\"><font size=2><b>CA Certificate from Local AS:</b></font></td>");
		req_format_write(wp,"<td width=\"0.45\"><font size=2><input type=\"file\" name=\"ca_binary\" size=20></font></td></tr></table>");
		req_format_write(wp,"<input onclick=sendClicked(this.form) type=button value=\"Upload\" name=\"send\">&nbsp;&nbsp;");
		req_format_write(wp,"<input type=\"reset\" value=\"Reset\" name=\"reset\">");
		req_format_write(wp,"<input type=\"hidden\" value=\"/wlwapiinstallcert.htm\" name=\"submit-url\">");
		req_format_write(wp,"<input type=\"hidden\" value=\"ca\" name=\"uploadcerttype\">");
		req_format_write(wp,"<input type=\"hidden\" value= \"two_certification\" name=\"auth_mode\"></form>");
		
		
		req_format_write(wp,"<form method=\"post\" action=\"/boafrm/formUploadWapiCert\" enctype=\"multipart/form-data\" name=\"uploadASUCert\"\
		id=\"uploadASUCert_asu\" style=\"display:none\">");


		req_format_write(wp,"<table border=\"0\" cellspacing=\"0\" width=\"500\">");
		req_format_write(wp,"<tr><font size=2></font></tr>");		
		req_format_write(wp,"<tr><td width=\"0.55\"><font size=2><b>ASU Certificate from Local AS:</b></font></td>");
		req_format_write(wp,"<td width=\"0.45\"><font size=2><input type=\"file\" name=\"asu_binary\" size=20></font></td></tr></table>");
		req_format_write(wp,"<input onclick=sendClicked(this.form) type=button value=\"Upload\" name=\"send\">&nbsp;&nbsp;");
		req_format_write(wp,"<input type=\"reset\" value=\"Reset\" name=\"reset\">");
		req_format_write(wp,"<input type=\"hidden\" value=\"/wlwapiinstallcert.htm\" name=\"submit-url\">");
		req_format_write(wp,"<input type=\"hidden\" value=\"asu\" name=\"uploadcerttype\">");
		req_format_write(wp,"<input type=\"hidden\" value= \"two_certification\" name=\"auth_mode\"></form>");

		req_format_write(wp,"<form method=\"post\" action=\"/boafrm/formUploadWapiCert\" enctype=\"multipart/form-data\" name=\"uploadUserCert\">");
		req_format_write(wp,"<table border=\"0\" cellspacing=\"0\" width=\"500\">");
		req_format_write(wp,"<tr><font size=2></font></tr>");
//		req_format_write(wp,"<tr><hr size=1 noshade align=top></tr>");
		req_format_write(wp,"<tr><td width=\"0.55\"><font size=2><b>User Certificate from Local AS:</b></font></td>");
		req_format_write(wp,"<td width=\"0.45\"><font size=2><input type=\"file\" name=\"user_binary\" size=20></font></td></tr></table>");
		req_format_write(wp,"<input onclick=sendClicked(this.form) type=button value=\"Upload\" name=\"send\">&nbsp;&nbsp;");
		req_format_write(wp,"<input type=\"reset\" value=\"Reset\" name=\"reset\">");
		req_format_write(wp,"<input type=\"hidden\" value=\"/wlwapiinstallcert.htm\" name=\"submit-url\">");
		req_format_write(wp,"<input type=\"hidden\" value=\"user\" name=\"uploadcerttype\">");
		req_format_write(wp,"<input type=\"hidden\" value= \"two_certification\" name=\"auth_mode\"></form>");		
#endif
		return 0;
	}
#ifdef CONFIG_RTL_WAPI_SUPPORT
	else if(!strcmp(argv[0],"auth_mode_2or3_certification"))
	{
		if(!apmib_get(MIB_WLAN_WAPI_AUTH_MODE_2or3_CERT,&intVal))
			return -1;

		//printf("val =%d\n",intVal);
		if(intVal !=3)
			req_format_write(wp,"%s","two_certification");
		else
			req_format_write(wp,"%s","three_certification");

		return 0;
	}
#endif

#ifdef CONFIG_RTL_BT_CLIENT
	else if(!strcmp(argv[0],"bt_enabled"))
	{
	     if(!apmib_get(MIB_BT_ENABLED,&intVal))
	                return -1;
	        req_format_write(wp,"%d",intVal);
	        return 0;
	}
	else if(!strcmp(argv[0],"torrents"))
	{
		/*Output torrents*/
		struct torrent_t torrent[20];
		struct ctorrent_t ctorrent[10];
		char tmpbuf[64];
		char tmpbuf1[64];
		int tcounts;
		int ctcounts;
		int i;
		int cindex;
		extern int bt_getTorrents(struct torrent_t *torrentp, int max);
		extern int bt_getClientsInfo(struct ctorrent_t  *ctorrentp, int max);

		memset(torrent,0x0,sizeof(struct torrent_t)*20);
		memset(ctorrent,0x0,sizeof(struct ctorrent_t )*10);

		tcounts=bt_getTorrents(torrent,20);
		ctcounts=bt_getClientsInfo(ctorrent,10);
		/*webWrite format: torrentname,btstatus,size,updownsize,seeder,etaratio,uprate,downrate,index*/
		if(tcounts == 0)
		{
			req_format_write(wp,"%s","[]");
			return 0;
		}
		req_format_write(wp,"%s","[");
		for(i=0;i<tcounts;i++)
		{
			if(0==i)
				req_format_write(wp,"%s","[");
			else
				req_format_write(wp,"%s",",[");
			/*0 not running 1 running 2 start_paused*/
			if(0==torrent[i].status)
			{
				/*not running. no ctorrent.*/
				/*name*/
				req_format_write(wp,"'%s'",torrent[i].name);
				/*status*/
				req_format_write(wp,",'%s'","Not Running");
				/*size*/
				req_format_write(wp,",'%s'","N/A");
				/*up/down size*/
				req_format_write(wp,",'%s'","N/A");
				/*seeder/leecher*/
				req_format_write(wp,",'%s'","N/A");
				/*ETA/RATIO*/
				req_format_write(wp,",'%s'","N/A");
				/*uprate*/
				req_format_write(wp,",'%s'","N/A");
				/*downrate*/
				req_format_write(wp,",'%s'","N/A");
				/*torrent index*/
				req_format_write(wp,",'%d'",torrent[i].index);
				/*ctorrent index*/
				req_format_write(wp,",'%d'",torrent[i].ctorrent);
			}
			else if(1==torrent[i].status)
			{
				req_format_write(wp,"'%s'",torrent[i].name);
				if(ctcounts !=0 && torrent[i].ctorrent != (-1))
				{
					if(ctorrent[torrent[i].ctorrent].paused)
					{
						req_format_write(wp,",'%s'","Paused");
					}
					else
					{
						req_format_write(wp,",'%s'","Running");
					}
				}
				cindex=torrent[i].ctorrent;
				/*get ctorrent to print others*/
				/*size*/
				sprintf(tmpbuf,"%llu",ctorrent[cindex].size);
				req_format_write(wp,",'%s'",tmpbuf);
				/*down/up size*/
				sprintf(tmpbuf,"%llu",ctorrent[cindex].dl_total);
				sprintf(tmpbuf1,"%llu",ctorrent[cindex].ul_total);
				req_format_write(wp,",'%s/%s'",tmpbuf,tmpbuf1);
				/*seeder/leecher*/
				sprintf(tmpbuf,"%u",ctorrent[cindex].seeders);
				sprintf(tmpbuf1,"%u",ctorrent[cindex].leechers);
				req_format_write(wp,",'%s/%s'",tmpbuf,tmpbuf1);
				/*ETA/RATIO*/
				sprintf(tmpbuf,"%llu",ctorrent[cindex].seed_ratio);
				req_format_write(wp,",'%s'",tmpbuf);
				/*uprate*/
				req_format_write(wp,",'%d'",ctorrent[cindex].ul_rate);
				/*downrate*/
				req_format_write(wp,",'%d'",ctorrent[cindex].dl_rate);
				/*torrent index*/
				req_format_write(wp,",'%d'",torrent[i].index);
				/*ctorrent index*/
				req_format_write(wp,",'%d'",cindex);

			}
			else if(2==torrent[i].status)
			{
				req_format_write(wp,"'%s'",torrent[i].name);
				req_format_write(wp,",'%s'","Paused");
				cindex=torrent[i].ctorrent;
				/*get ctorrent to print others*/
				/*size*/
				sprintf(tmpbuf,"%llu",ctorrent[cindex].size);
				req_format_write(wp,",'%s",tmpbuf);
				/*down/up size*/
				sprintf(tmpbuf,"%llu",ctorrent[cindex].dl_total);
				sprintf(tmpbuf1,"%llu",ctorrent[cindex].ul_total);
				req_format_write(wp,",'%s/%s'",tmpbuf,tmpbuf1);
				/*seeder/leecher*/
				sprintf(tmpbuf,"%u",ctorrent[cindex].seeders);
				sprintf(tmpbuf1,"%u",ctorrent[cindex].leechers);
				req_format_write(wp,",'%s/%s'",tmpbuf,tmpbuf1);
				/*ETA/RATIO*/
				sprintf(tmpbuf,"%llu",ctorrent[cindex].seed_ratio);
				req_format_write(wp,",'%s'",tmpbuf);
				/*uprate*/
				req_format_write(wp,",'%d'",ctorrent[cindex].ul_rate);
				/*downrate*/
				req_format_write(wp,",'%d'",ctorrent[cindex].dl_rate);
				/*torrent index*/
				req_format_write(wp,",'%d'",torrent[i].index);
				/*ctorrent index*/
				req_format_write(wp,",'%d'",cindex);
			}
			req_format_write(wp,"%s","]");
		}
		req_format_write(wp,"%s","]");

		for(i=0;i<tcounts;i++)
		{
			if(torrent[i].name)
				free(torrent[i].name);
		}

		for(i=0;i<ctcounts;i++)
		{
			if(ctorrent[i].valid)
			{
				if(ctorrent[i].fname)
					free(ctorrent[i].fname);
				if(ctorrent[i].msg)
					free(ctorrent[i].msg);
			}
		}
		return 0;
	}
	else if(!strcmp(argv[0],"btfiles"))
	{
		char *ptr;
		int index;
		int filecount;
		char tmpbuf[64];
		struct ctfile_t file[30];
		extern int  bt_getDetails(int index, struct ctfile_t *file, int max);

		memset(file,0x0,sizeof(struct ctfile_t)*30);
		ptr=req_get_cstream_var(wp,"ctorrent", "");
		if(ptr)
			index=atoi(ptr);
		else
			return -1;
		/*get index torrent files....*/
		filecount=bt_getDetails(index, file, 30);
		if(0== filecount)
		{
			req_format_write(wp,"%s","[]");
			return 0;
		}
		/*format filename fileno download_percent filesize priority*/
		/*priority is used for indicate if need to download it*/
		req_format_write(wp,"%s","[");
		for(i=0;i<filecount;i++)
		{
			if(0==i)
				req_format_write(wp,"%s","[");
			else
				req_format_write(wp,"%s",",[");

			req_format_write(wp,"'%s'",file[i].filename);
			req_format_write(wp,",'%d'",file[i].fileno);
			req_format_write(wp,",'%d'",file[i].download);
			sprintf(tmpbuf,"%llu",file[i].filesize);
			req_format_write(wp,",'%s'",tmpbuf);
			req_format_write(wp,",'%d'",file[i].priority);

			req_format_write(wp,"%s","]");
	         }
		  req_format_write(wp,"%s","]");
		  for(i=0;i<filecount;i++)
		  {
		  	if(file[filecount].filename)
				free(file[filecount].filename);
		  }
                return 0;
        }
        else if(!strcmp(argv[0],"btclientindex"))
        {
                char *ptr=NULL;
                ptr=req_get_cstream_var(wp,"ctorrent", "");
                if(ptr)
                        req_format_write(wp, "%s",ptr);
                return 0;
        }
        else if(!strcmp(argv[0],"bt_status"))
        {
                return 0;
        }
        else if(!strcmp(argv[0],"bt_limits"))
        {
                return 0;
        }
        else if(!strcmp(argv[0],"BTDDir"))
        {
                if(!apmib_get(MIB_BT_DOWNLOAD_DIR,buffer))
                        return -1;
                req_format_write(wp,"%s",buffer);
                return 0;
        }
        else if(!strcmp(argv[0],"BTUDir"))
        {
                if(!apmib_get(MIB_BT_UPLOAD_DIR,buffer))
                        return -1;
                req_format_write(wp,"%s",buffer);
                return 0;
        }
        else if(!strcmp(argv[0],"BTdlimit"))
        {
                if(!apmib_get(MIB_BT_TOTAL_DLIMIT,&intVal))
                        return -1;
                req_format_write(wp,"%d",intVal);
                return 0;
        }
        else if(!strcmp(argv[0],"BTulimit"))
        {
                if(!apmib_get(MIB_BT_TOTAL_ULIMIT,&intVal))
                        return -1;
                req_format_write(wp,"%d",intVal);
                return 0;
        }
        else if(!strcmp(argv[0],"BTrefreshtime"))
        {
                if(!apmib_get(MIB_BT_REFRESH_TIME,&intVal))
                        return -1;
                req_format_write(wp,"%d",intVal);
                return 0;
        }
	 else if(!strcmp(argv[0],"rtl_bt_menu"))
	 {
	 	req_format_write(wp, "%s","manage.addItem(\"BT Client\", \"bt.htm\", \"\", \"BT Client\");");
		return 0;
	 }
	 else if(!strcmp(argv[0],"is_enabled_bt"))
	 {
	 	req_format_write(wp,"1");
		return 0;
	}
	else if(!strcmp(argv[0],"bt_status"))
	{
	        return 0;
	}
	else if(!strcmp(argv[0],"bt_limits"))
	{
	        return 0;
	}
	else if(!strcmp(argv[0],"BTDDir"))
	{
	        if(!apmib_get(MIB_BT_DOWNLOAD_DIR,buffer))
	                return -1;
	        req_format_write(wp,"%s",buffer);
	        return 0;
	}
	else if(!strcmp(argv[0],"BTUDir"))
	{
	        if(!apmib_get(MIB_BT_UPLOAD_DIR,buffer))
	                return -1;
	        req_format_write(wp,"%s",buffer);
	        return 0;
	}
	else if(!strcmp(argv[0],"BTdlimit"))
	{
	        if(!apmib_get(MIB_BT_TOTAL_DLIMIT,&intVal))
	                return -1;
	        req_format_write(wp,"%d",intVal);
	        return 0;
	}
	else if(!strcmp(argv[0],"BTulimit"))
	{
	        if(!apmib_get(MIB_BT_TOTAL_ULIMIT,&intVal))
	                return -1;
	        req_format_write(wp,"%d",intVal);
	        return 0;
	}
	else if(!strcmp(argv[0],"BTrefreshtime"))
	{
	        if(!apmib_get(MIB_BT_REFRESH_TIME,&intVal))
	                return -1;
	        req_format_write(wp,"%d",intVal);
	        return 0;
	}
	else if(!strcmp(argv[0],"rtl_bt_menu"))
	{
		req_format_write(wp, "%s","manage.addItem(\"BT Client\", \"bt.htm\", \"\", \"BT Client\");");
		return 0;
	}
	 }
#else
	 else if(!strcmp(argv[0],"rtl_bt_menu"))
	 {
	 	return 0;
	 }
	 else if(!strcmp(argv[0],"is_enabled_bt"))
	 {
	 	req_format_write(wp,"0");
		return 0;
	 }
#endif

#ifdef CONFIG_RTL_TRANSMISSION
	else if(!strcmp(argv[0],"bt_enabled"))
	{
		if(!apmib_get(MIB_BT_ENABLED,&intVal))
			return -1;
		if(intVal)
			req_format_write(wp,"checked");
		else
			req_format_write(wp,"");
		return 0;
	}
	else if(!strcmp(argv[0],"BTDDir"))
	{
		if(!apmib_get(MIB_BT_DOWNLOAD_DIR,buffer))
			return -1;
		req_format_write(wp,"%s",buffer);
		return 0;
	}
	else if(!strcmp(argv[0],"BTUDir"))
	{
		if(!apmib_get(MIB_BT_UPLOAD_DIR,buffer))
			return -1;
		req_format_write(wp,"%s",buffer);
		return 0;
	}
	else if(!strcmp(argv[0],"rtl_trans_bt_menu"))
	{
		req_format_write(wp, "%s","manage.addItem(\"BT Client\", \"transmission.htm\", \"\", \"BT Client\");");
		return 0;
	}
#else
	else if(!strcmp(argv[0],"rtl_trans_bt_menu"))
	{
		return 0;
	}
	else if(!strcmp(argv[0],"isEnableBT"))
	{
		sprintf(buffer, "1");
		req_format_write(wp, buffer);
		return 0;
	 }
#endif

#ifdef REBOOT_CHECK
	else if(!strcmp(argv[0],"countDownTime"))
	{
		req_format_write(wp, "%d",countDownTime);
		countDownTime = APPLY_COUNTDOWN_TIME;
		return 0;
	}

	else if(!strcmp(argv[0],"okMsg"))
	{
		req_format_write(wp, "%s", okMsg);
		memset(okMsg,0x00,sizeof(okMsg));
		return 0;
	}

	else if(!strcmp(argv[0],"lastUrl"))
	{
		if(strlen(lastUrl) == 0)
			req_format_write(wp, "%s", "/wizard.htm");
		else
			req_format_write(wp, "%s", lastUrl);

		memset(lastUrl,0x00,sizeof(lastUrl));
		return 0;
	}
#endif
	else if(!strcmp(argv[0],"status_warning"))
	{
#ifdef REBOOT_CHECK
		if(needReboot == 1)
		{
			req_format_write(wp, "%s", "<tr><td></td></tr><tr><td><font size=2><font color='#FF0000'> \
 															Below status shows currnt settings, but does not take effect. \
															</font></td></tr>");
		}
		else
#endif
		{
			req_format_write(wp, "%s", "");
		}

		return 0;
	}
	else if(!strcmp(argv[0],"wlan_onoff_tkip"))
	{
		apmib_get(MIB_WLAN_11N_ONOFF_TKIP, (void *)&intVal);
		req_format_write(wp, "%d",intVal);

		return 0;
	}
	else if(!strcmp(argv[0],"onoff_tkip_comment_start"))
	{
		int wlanMode=0;

		apmib_get(MIB_WLAN_11N_ONOFF_TKIP, (void *)&intVal);
		apmib_get(MIB_WLAN_BAND, (void *)&wlanMode);
		if(intVal == 0 && (wlanMode >= BAND_11N))
			req_format_write(wp, "%s","<!--");
		else
			req_format_write(wp, "%s","");

		return 0;
	}
	else if(!strcmp(argv[0],"onoff_tkip_comment_end"))
	{
		int wlanMode=0;

		apmib_get(MIB_WLAN_11N_ONOFF_TKIP, (void *)&intVal);
		apmib_get(MIB_WLAN_BAND, (void *)&wlanMode);
		if(intVal == 0 && (wlanMode >= BAND_11N))
			req_format_write(wp, "%s","-->");
		else
			req_format_write(wp, "%s","");

		return 0;
	}
	else if(!strcmp(argv[0],"wlanband")) {
		apmib_get(MIB_WLAN_BAND, (void *)&intVal);

		req_format_write(wp, "%d",intVal);

		return 0;
	}
	else if ( !strcmp(name, "opMode")) {
		apmib_get( MIB_OP_MODE, (void *)&intVal);
		req_format_write(wp, "%d",intVal);
		return 0;
	}
	else if ( !strcmp(name, "pocketRouter_html_wan_hide_s")) {
		apmib_get( MIB_OP_MODE, (void *)&intVal);
#if defined(CONFIG_POCKET_ROUTER_SUPPORT)
		if(intVal == 0)
			req_format_write(wp, "%s","");
		else if(intVal == 1)
			req_format_write(wp, "%s","<!--");
#elif defined(CONFIG_POCKET_AP_SUPPORT)
		req_format_write(wp, "%s","<!--");
#elif defined(CONFIG_RTL_8198_AP_ROOT) || defined(CONFIG_RTL_8197D_AP)
		req_format_write(wp, "%s","<!--");
#else
		req_format_write(wp, "%s","");
#endif
		return 0;
	}
	else if ( !strcmp(name, "pocketRouter_html_wan_hide_e")) {
		apmib_get( MIB_OP_MODE, (void *)&intVal);
#if defined(CONFIG_POCKET_ROUTER_SUPPORT)
		if(intVal == 0)
			req_format_write(wp, "%s","");
		else if(intVal == 1)
			req_format_write(wp, "%s","-->");
#elif defined(CONFIG_POCKET_AP_SUPPORT)
		req_format_write(wp, "%s","-->");
#elif defined(CONFIG_RTL_8198_AP_ROOT) || defined(CONFIG_RTL_8197D_AP)
		req_format_write(wp, "%s","-->");
#else
		req_format_write(wp, "%s","");
#endif
		return 0;
	}
	else if ( !strcmp(name, "pocketRouter_html_lan_hide_s")) {
		apmib_get( MIB_OP_MODE, (void *)&intVal);
#if defined(CONFIG_POCKET_ROUTER_SUPPORT)
		if(intVal == 1)
			req_format_write(wp, "%s","");
		else if(intVal == 0)
			req_format_write(wp, "%s","<!--");
#else
		req_format_write(wp, "%s","");
#endif
		return 0;
	}
	else if ( !strcmp(name, "pocketRouter_html_lan_hide_e")) {
		apmib_get( MIB_OP_MODE, (void *)&intVal);
#if defined(CONFIG_POCKET_ROUTER_SUPPORT)
		if(intVal == 1)
			req_format_write(wp, "%s","");
		else if(intVal == 0)
			req_format_write(wp, "%s","-->");
#else
		req_format_write(wp, "%s","");
#endif
		return 0;
	}
	else if(!strcmp(name, "wlan_xTxR")) // 0:non-pocketRouter; 3: Router; 2:Bridge AP; 1:Bridge Client
	{
		int chipVersion = getWLAN_ChipVersion();

#if defined(CONFIG_RTL_8812_SUPPORT)
		return req_format_write(wp, "%s","2*2");
#endif

		if(chipVersion == CHIP_RTL8188C)
			return req_format_write(wp, "%s","1*1");
		else if(chipVersion == CHIP_RTL8192C)
			return req_format_write(wp, "%s","2*2");
#if defined(CONFIG_RTL_92D_SUPPORT)
		else if(chipVersion == CHIP_RTL8192D)
		{
			apmib_get(MIB_WLAN_BAND2G5G_SELECT,(void *)&intVal);
			if(BANDMODEBOTH == intVal)
			{
				return req_format_write(wp, "%s","1*1");
			}
			else
			{
				return req_format_write(wp, "%s","2*2");
			}
		}
#endif
		else if(chipVersion == CHIP_RTL8192E)
			return req_format_write(wp, "%s","2*2");
		else
			return req_format_write(wp, "%s","0*0");
	}
	else if ( !strcmp(name, "ip-lan")) {
		if ( getInAddr(BRIDGE_IF, IP_ADDR, (void *)&intaddr ) )
			return req_format_write(wp, "%s", inet_ntoa(intaddr) );
		else{
			apmib_get( MIB_IP_ADDR,  (void *)buffer);
   			return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)));
		}
	}
	else if ( !strcmp(name, "redirect_ip"))
	{
#ifdef HOME_GATEWAY
		unsigned int wan_access_dut=0;
		apmib_get(MIB_WEB_WAN_ACCESS_ENABLED, (void *)&wan_access_dut);

		if(wan_access_dut == 1)
		{
			struct in_addr lan_ip, lan_mask, logoin_ip, wan_ip, wan_mask;
			unsigned int i_lan_ip, i_lan_mask, i_logoin_ip, i_wan_ip, i_wan_mask;

			char strWanIP[16];
			char strWanMask[16];
			char strWanDefIP[16];
			char strWanHWAddr[18];
			#if defined(CONFIG_RTL_WEB_WAN_ACCESS_PORT)
			int port = 80;
			
			apmib_get( MIB_WEB_WAN_ACCESS_PORT, (void *)&port);
			//printf("%s %d port=%d \n", __func__, __LINE__, port);
			#endif
			
			getWanInfo(strWanIP,strWanMask,strWanDefIP,strWanHWAddr);
			
			inet_aton(strWanIP, &wan_ip);
			inet_aton(strWanMask, &wan_mask);
			inet_aton(wp->remote_ip_addr, &logoin_ip);
			
			apmib_get( MIB_IP_ADDR,  (void *)&lan_ip);
			apmib_get( MIB_SUBNET_MASK, (void *)&lan_mask);
			
			memcpy(&i_lan_ip, &lan_ip, 4);
			memcpy(&i_lan_mask, &lan_mask, 4);
			memcpy(&i_logoin_ip, &logoin_ip, 4);
			memcpy(&i_wan_ip, &wan_ip, 4);
			memcpy(&i_wan_mask, &wan_mask, 4);

			if (i_wan_mask > htonl(0xffffff00))
				i_wan_mask = htonl(0xffffff00);

			/*users should change the LAN IP/SUBNET manually when LAN/WAN IP conflict*/
			if((i_lan_ip & i_lan_mask) == (i_logoin_ip & i_lan_mask)) 
			{
				if ( getInAddr(BRIDGE_IF, IP_ADDR, (void *)&intaddr ) )
					return req_format_write(wp, "%s", inet_ntoa(intaddr) );
				else{
					apmib_get( MIB_IP_ADDR,  (void *)buffer);
		   			return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)));
				}
			}
			else if((i_wan_ip & i_wan_mask) == (i_logoin_ip & i_wan_mask))
			{
				#if defined(CONFIG_RTL_WEB_WAN_ACCESS_PORT)
				return req_format_write(wp, "%s:%d", strWanIP, port);
				#else
				return req_format_write(wp, "%s", strWanIP);
				#endif
				
				
			}
			else
			{
				apmib_get( MIB_IP_ADDR,  (void *)buffer);
	   			return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)));
			}
				
		}
		else
		{
#endif
			//use MIB_IP_ADDR for changing IP subnet from Wizard. users must reset IP addr after LAN/WAN subnet conflict.
			/*
			if ( getInAddr(BRIDGE_IF, IP_ADDR, (void *)&intaddr ) )
				return req_format_write(wp, "%s", inet_ntoa(intaddr) );
			else
			*/
			{
				apmib_get( MIB_IP_ADDR,  (void *)buffer);
	   			return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)));
			}
#ifdef HOME_GATEWAY
		}
#endif		
		
	}
	else if(!strcmp(name, "accessFromWan"))
	{
		struct in_addr logoin_ip, wan_ip, wan_mask;
		unsigned int i_logoin_ip, i_wan_ip, i_wan_mask;
		unsigned int wan_access_dut=0;
		unsigned int op_mode=0;
#ifdef HOME_GATEWAY	
		apmib_get(MIB_WEB_WAN_ACCESS_ENABLED, (void *)&wan_access_dut);
		apmib_get(MIB_OP_MODE, (void *)&op_mode);

		if(wan_access_dut == 1 && op_mode!=BRIDGE_MODE)
		{
			char strWanIP[16];
			char strWanMask[16];
			char strWanDefIP[16];
			char strWanHWAddr[18];
			
			getWanInfo(strWanIP,strWanMask,strWanDefIP,strWanHWAddr);

			inet_aton(wp->remote_ip_addr, &logoin_ip);
			inet_aton(strWanIP, &wan_ip);
			inet_aton(strWanMask, &wan_mask);
			
			memcpy(&i_logoin_ip, &logoin_ip, 4);
			memcpy(&i_wan_ip, &wan_ip, 4);
			memcpy(&i_wan_mask, &wan_mask, 4);

			if (i_wan_mask > htonl(0xffffff00))
				i_wan_mask = htonl(0xffffff00);

			if((i_wan_ip & i_wan_mask) != (i_logoin_ip & i_wan_mask))
				wan_access_dut= 0;
		}
#else
	wan_access_dut=0;
#endif
		sprintf(buffer, "%d", wan_access_dut) ;
		req_format_write(wp, buffer);
		return 0;
	}
#if defined(NEW_SCHEDULE_SUPPORT)
	else if(!strcmp(name, "maxWebWlSchNum"))
	{
		sprintf(buffer, "%d", MAX_SCHEDULE_NUM );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "wlsch_onoff"))
	{
		apmib_get( MIB_WLAN_SCHEDULE_ENABLED, (void *)&intVal);
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
#endif // #if defined(NEW_SCHEDULE_SUPPORT)
	else if(!strcmp(argv[0],"onoff_dual_firmware_start"))
	{
#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE)
		req_format_write(wp, "%s","");
#else
		req_format_write(wp, "%s","<!--");
#endif

		return 0;
	}
	else if(!strcmp(argv[0],"onoff_dual_firmware_end"))
	{
#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE)
		req_format_write(wp, "%s","");
#else
		req_format_write(wp, "%s","-->");
#endif

		return 0;
	}
	else if ( !strcmp(name, "enable_dualFw"))
	{
#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE)
		apmib_get(MIB_DUALBANK_ENABLED, (void *)&intVal);
#else
		intVal = 1;
#endif
		req_format_write(wp, "%d",intVal);

		return 0;
	}
	else if ( !strcmp(name, "currFwBank"))
	{

#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE)
		int active,backup;
		apmib_get(MIB_DUALBANK_ENABLED, (void *)&intVal);
		get_bank_info(intVal ,&active,&backup);
		intVal = active;
#else
		intVal = 1;
#endif
		req_format_write(wp, "%d",intVal);

		return 0;
	}
	else if ( !strcmp(name, "backFwBank"))
	{
#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE)
		int active,backup;
		apmib_get(MIB_DUALBANK_ENABLED, (void *)&intVal);
		get_bank_info(intVal ,&active,&backup);
		intVal = backup;
#else
		intVal = 2;
#endif
		req_format_write(wp, "%d",intVal);

		return 0;
	}
	else if ( !strcmp(name, "wlan_bandMode_menu_onoff"))
	{
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_8881A_SELECTIVE)
		return req_format_write(wp, "wlan.addItem('BandMode', 'wlbandmode.htm', '', 'Setup WLAN Band Mode');" );
#else
		return req_format_write(wp,"");
#endif
	}
	else if(!strcmp(argv[0],"onoff_dmdphy_comment_start"))
	{
#if defined(CONFIG_RTL_92D_DMDP)||defined(CONFIG_RTL_DUAL_PCIESLOT_BIWLAN_D)
			req_format_write(wp, "%s","");
#else
			req_format_write(wp, "%s","<!--");
#endif

		return 0;
	}
	else if(!strcmp(argv[0],"single_band"))
	{
#if defined(CONFIG_RTL_DUAL_PCIESLOT_BIWLAN_D)
		req_format_write(wp, "%s","<input type=\"radio\" value=\"3\" name=\"wlBandMode\" onClick=\"\" DISABLED></input>");
#elif defined(CONFIG_POCKET_AP_SUPPORT) || defined(CONFIG_RTL_8881A_SELECTIVE)
		req_format_write(wp, "%s","<input type=\"radio\" value=\"3\" name=\"wlBandMode\" onClick=\"\" CHECKED></input>");
#else
		req_format_write(wp, "%s","<input type=\"radio\" value=\"3\" name=\"wlBandMode\" onClick=\"\" ></input>");
#endif

		return 0;
	}
	else if(!strcmp(argv[0],"onoff_dmdphy_comment_end"))
	{
#if defined(CONFIG_RTL_92D_DMDP)||defined(CONFIG_RTL_DUAL_PCIESLOT_BIWLAN_D)
			req_format_write(wp, "%s","");
#else
			req_format_write(wp, "%s","-->");
#endif

		return 0;
	}
	else if (!strcmp(argv[0],"initpage")) {
#if defined(CONFIG_POCKET_AP_SUPPORT)
		req_format_write(wp, "%s","status.htm");
#elif defined(CONFIG_RTL_ULINKER)
		apmib_get( MIB_ULINKER_AUTO, (void *)&intVal);
		if(intVal == 1)
			req_format_write(wp, "%s","wizard.htm");
		else
			req_format_write(wp, "%s","ulinker_opmode.htm");
#else
		req_format_write(wp, "%s","wizard.htm");
#endif

		return 0;
	}
	else if (!strcmp(argv[0],"homepage")) {
#if defined(HTTP_FILE_SERVER_SUPPORTED)
		req_format_write(wp, "http_files.htm");
#else
		req_format_write(wp, "home.htm");
#endif
		return 0;
	}
#ifdef HTTP_FILE_SERVER_HTM_UI
	else if(!strcmp(argv[0],"current_directory"))
	{
		//printf("%s:%d current_directory=%s\n",__FUNCTION__,__LINE__,httpfile_dirpath);
		if(httpfile_dirpath && strncmp(httpfile_dirpath,"/var/tmp/usb",strlen("/var/tmp/usb"))==0)
		{
			//printf("%s:%d current_directory=%s\n",__FUNCTION__,__LINE__,httpfile_dirpath);
			req_format_write(wp,"%s",((char*)httpfile_dirpath)+strlen("/var/tmp/usb"));
			//printf("%s:%d current_directory=%s\n",__FUNCTION__,__LINE__,httpfile_dirpath);
			return 0;
		}
		else
			return -1;
	}	
#endif
	else if(!strcmp(argv[0],"info_country"))
	{
		if(sizeof(countryIEArray)==0)
			req_format_write(wp, "%s","[]");
		else
		{
			req_format_write(wp, "%s","[");
			for(i=0;i<sizeof(countryIEArray)/sizeof(COUNTRY_IE_ELEMENT);i++)
			{
				/*country code, abb,5g idx,2g idx,name*/
				req_format_write(wp,"[%d,'%.3s',%d,%d,'%s'",countryIEArray[i].countryNumber,countryIEArray[i].countryA2,
					countryIEArray[i].A_Band_Region,countryIEArray[i].G_Band_Region,countryIEArray[i].countryName);
				if(i ==(sizeof(countryIEArray)/sizeof(COUNTRY_IE_ELEMENT)-1))
					req_format_write(wp, "%s","]");
				else
					req_format_write(wp, "%s","],");
			}
			req_format_write(wp, "%s","]");
		}
		return 0;
	}
	else if(!strcmp(argv[0],"info_2g"))
	{
		if(sizeof(Bandtable_2dot4G)==0)
			req_format_write(wp, "%s","[]");
		else
		{
			req_format_write(wp, "%s","[");
			for(i=0;i<sizeof(Bandtable_2dot4G)/sizeof(REG_DOMAIN_TABLE_ELEMENT_T);i++)
			{
				req_format_write(wp,"[%d,%d,'%s'",Bandtable_2dot4G[i].region,Bandtable_2dot4G[i].channel_set,Bandtable_2dot4G[i].area);
				if(i ==(sizeof(Bandtable_2dot4G)/sizeof(REG_DOMAIN_TABLE_ELEMENT_T)-1))
					req_format_write(wp, "%s","]");
				else
					req_format_write(wp, "%s","],");
			}
			req_format_write(wp, "%s","]");
		}
		return 0;
	}
	else if(!strcmp(argv[0],"info_5g"))
	{
		if(sizeof(Bandtable_5G)==0)
			req_format_write(wp, "%s","[]");
		else
		{
			req_format_write(wp, "%s","[");
			for(i=0;i<sizeof(Bandtable_5G)/sizeof(REG_DOMAIN_TABLE_ELEMENT_T);i++)
			{
				req_format_write(wp,"[%d,%d,'%s'",Bandtable_5G[i].region,Bandtable_5G[i].channel_set,Bandtable_5G[i].area);
				if(i ==(sizeof(Bandtable_5G)/sizeof(REG_DOMAIN_TABLE_ELEMENT_T)-1))
					req_format_write(wp, "%s","]");
				else
					req_format_write(wp, "%s","],");
			}
			req_format_write(wp, "%s","]");
		}
		return 0;
	}
	else if(!strcmp(argv[0],"country_str"))
	{		
		apmib_get(MIB_WLAN_COUNTRY_STRING, (void *)tmpStr);
		req_format_write(wp,"%s",tmpStr);
		return 0;
	}
#if defined(CONFIG_RTL_P2P_SUPPORT)
	else if ( !strcmp(name, "p2p_type"))
	{
		apmib_get( MIB_WLAN_P2P_TYPE, (void *)&intVal);
		
		if(intVal == 4)
			sprintf(buffer, "%d", 1 );
		else
			sprintf(buffer, "%d", 0 );
			
   	return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "p2p_intent"))
	{
		apmib_get( MIB_WLAN_P2P_INTENT, (void *)&intVal);		
		sprintf(buffer, "%d", intVal );
   	return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "p2p_listen_channel"))
	{
		apmib_get( MIB_WLAN_P2P_LISTEN_CHANNEL, (void *)&intVal);		
		sprintf(buffer, "%d", intVal );
   	return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "p2p_op_channel"))
	{
		apmib_get( MIB_WLAN_P2P_OPERATION_CHANNEL, (void *)&intVal);		
		sprintf(buffer, "%d", intVal );
   	return req_format_write(wp, buffer);
	}	
#endif
#if defined(CONFIG_RTL_ETH_802DOT1X_SUPPORT)
	else if(!strcmp(name, "ethdot1x_maxportnum"))
	{
		int opmode = 0;
		
		intVal = MAX_ELAN_DOT1X_PORTNUM - 1;
		
		if ( !apmib_get( MIB_OP_MODE,	(void *)&opmode) )
			return -1;

		if (opmode ==BRIDGE_MODE || opmode == WISP_MODE)
		{
			#if !defined(CONFIG_RTL_IVL_SUPPORT)
			intVal =  MAX_ELAN_DOT1X_PORTNUM;
			#endif
		}

		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "ethdot1x_mode"))
	{
		if ( !apmib_get( MIB_ELAN_DOT1X_MODE,	(void *)&intVal) )
			return -1;

		if (intVal & ETH_DOT1X_PROXY_MODE_BIT)
			sprintf(buffer, "%d", 1 );
		else
			sprintf(buffer, "%d", 0 );
		
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "ethdot1x_type"))
	{
		if ( !apmib_get( MIB_ELAN_DOT1X_PROXY_TYPE,	(void *)&intVal) )
			return -1;

		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "ethdot1x_unicastresp_onoff"))
	{
		if ( !apmib_get( MIB_ELAN_EAPOL_UNICAST_ENABLED,	(void *)&intVal) )
			return -1;

		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "ethdot1x_radius_ip")) {
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_ELAN_RS_IP,  (void *)buffer);
   	return req_format_write(wp, "%s", inet_ntoa(*((struct in_addr *)buffer)) );
	}
  	else if ( !strcmp(name, "ethdot1x_radius_pass")) {
		buffer[0]='\0';
		apmib_get( MIB_ELAN_RS_PASSWORD,  (void *)buffer);
		translate_control_code(buffer);
		return req_format_write(wp, "%s", buffer);
	}
	else if(!strcmp(name, "ethdot1x_radius_port"))
	{
		if ( !apmib_get( MIB_ELAN_RS_PORT,	(void *)&intVal) )
			return -1;

		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "ethdot1x_server_port_number"))
	{
		if ( !apmib_get( MIB_ELAN_DOT1X_SERVER_PORT,	(void *)&intVal) )
			return -1;

		sprintf(buffer, "%d", intVal+1 );
		return req_format_write(wp, buffer);
	}
#endif
	else if(!strcmp(name, "ethdot1x_onoff"))
	{
#if defined(CONFIG_RTL_ETH_802DOT1X_SUPPORT)
		apmib_get( MIB_ELAN_ENABLE_1X, (void *)&intVal);
		/* MIB_ELAN_ENABLE_1X bit0-->proxy/snooping enable/disable
		 * MIB_ELAN_ENABLE_1X bit1-->client mode enable/disable
		 */
		 if (intVal & ETH_DOT1X_PROXY_SNOOPING_MODE_ENABLE_BIT)
			sprintf(buffer, "%d", 1 );
		 else
			 sprintf(buffer, "%d", 0 );
		return req_format_write(wp, buffer);
#else
		sprintf(buffer, "%d", 0 );
		return req_format_write(wp, buffer);
#endif
	}

	else if(!strcmp(name, "tx_restrict"))
	{
		if ( !apmib_get( MIB_WLAN_TX_RESTRICT,  (void *)&intVal) )
			return -1;

		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "rx_restrict"))
	{
		if ( !apmib_get( MIB_WLAN_RX_RESTRICT,  (void *)&intVal) )
			return -1;
		sprintf(buffer, "%d", intVal );
   		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name,"isSupportDhcpPortAndIpBind"))
	{
#ifdef SUPPORT_DHCP_PORT_IP_BIND
		intVal=1;
#else	
		intVal=0;
#endif
		sprintf(buffer, "%d", intVal );
		return req_format_write(wp, buffer);
	}
// add for ds-lite
#ifdef CONFIG_IPV6
#ifdef CONFIG_DSLITE_SUPPORT
	else if(!strcmp(name, "dsliteAftr"))
	{
		if ( !apmib_get(MIB_IPV6_ADDR_AFTR_PARAM,(void *)&ipaddr6))
			return -1 ; 
		sprintf(buffer, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
			ipaddr6.addrIPv6[0], ipaddr6.addrIPv6[1], ipaddr6.addrIPv6[2],
			ipaddr6.addrIPv6[3], ipaddr6.addrIPv6[4], ipaddr6.addrIPv6[5],
			ipaddr6.addrIPv6[6], ipaddr6.addrIPv6[7]);
		req_format_write(wp, "%s", buffer);
		return 0;
	}
	else if(!strcmp(name, "ipv6WanIp"))
	{
		if ( !apmib_get(MIB_IPV6_ADDR_WAN_PARAM,(void *)&ipaddr6))
			return -1 ; 
		sprintf(buffer, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
			ipaddr6.addrIPv6[0], ipaddr6.addrIPv6[1], ipaddr6.addrIPv6[2],
			ipaddr6.addrIPv6[3], ipaddr6.addrIPv6[4], ipaddr6.addrIPv6[5],
			ipaddr6.addrIPv6[6], ipaddr6.addrIPv6[7]);
		req_format_write(wp, "%s", buffer);
		return 0;
	}
	else if(!strcmp(name, "ipv6DefGW"))
	{
		if ( !apmib_get(MIB_IPV6_ADDR_GW_PARAM,(void *)&ipaddr6))
			return -1 ; 
		sprintf(buffer, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
			ipaddr6.addrIPv6[0], ipaddr6.addrIPv6[1], ipaddr6.addrIPv6[2],
			ipaddr6.addrIPv6[3], ipaddr6.addrIPv6[4], ipaddr6.addrIPv6[5],
			ipaddr6.addrIPv6[6], ipaddr6.addrIPv6[7]);
		req_format_write(wp, "%s", buffer);
		return 0;
	}
#else
	else if(!strcmp(name, "dsliteAftr") || !strcmp(name, "ipv6WanIp") || !strcmp(name, "ipv6DefGW"))
	{
		req_format_write(wp, "");
		return 0;
	}
#endif
#else
	else if(!strcmp(name, "dsliteAftr") || !strcmp(name, "ipv6WanIp") || !strcmp(name, "ipv6DefGW"))
	{
		req_format_write(wp, "");
		return 0;
	}
#endif
#ifdef CONFIG_CPU_UTILIZATION
	else if(!strcmp(name, "isCPUdisplayStart") || !strcmp(name, "isCPUdisplayEnd"))
	{
		req_format_write(wp, "");
		return 0;
	}
#else
	else if(!strcmp(name, "isCPUdisplayStart"))
	{
		req_format_write(wp, "<!--");
		return 0;
	}
	else if(!strcmp(name, "isCPUdisplayEnd"))
	{
		req_format_write(wp, "-->");
		return 0;
	}
#endif
	else if(!strcmp(name, "saveConfig"))
	{
#ifdef SHRINK_INIT_TIME
		save_cs_to_file();
#endif
		return 0;
	}

	for(i=0 ;i < wlan_num ; i++){
		sprintf(buffer, "wlan%d-status", i);
		if ( !strcmp(name, buffer )) {
			wlan_idx = i ;
			sprintf(WLAN_IF, "wlan%d", i);
			return req_format_write(wp,"");
		}
	}

 	return -1;
}

/////////////////////////////////////////////////////////////////////////////
#if defined(CONFIG_RTL_819X) && !defined(CONFIG_WLAN_VAP_SUPPORT)// keith. disabled if no this mode in 96c
	#define DEF_MSSID_NUM 0
#else
//		#if defined(CONFIG_RTL8196B)//we disable mssid first for 96b
//	#define DEF_MSSID_NUM 0
//		#else
#ifdef CONFIG_RTL8196B_GW_8M
	#define DEF_MSSID_NUM 1
#else
	#define DEF_MSSID_NUM 4
#endif
//		#endif
#endif //#if defined(CONFIG_RTL_819X) && !defined(CONFIG_WLAN_VAP_SUPPORT)

request inner_req;
char inner_req_buff[MAX_INNER_REQ_BUFF];
int inner_getIndex(char *name)
{
	char *inner_argv[1] = {name};
	
	memset(inner_req_buff, '\0', sizeof(inner_req_buff)); 
	getIndex(&inner_req, 1, inner_argv);

	if (strlen(inner_req_buff)==0)
		sprintf(inner_req_buff, "\"\"");
	return 0;
}

int inner_getInfo(char *name)
{
	char *inner_argv[1] = {name};
	
	memset(inner_req_buff, '\0', sizeof(inner_req_buff)); 
	getInfo(&inner_req, 1, inner_argv);

	if (strlen(inner_req_buff)==0)
		sprintf(inner_req_buff, "\"\"");
	return 0;
}
#ifdef MULTI_WAN_SUPPORT

#if 0
/* test data 4 wan interface */


WanIntfacesType currentIdx=0;
unsigned char upnpEnable=0;
unsigned char igmpProxyEnable=0;
unsigned char wanPingEnable=1;
unsigned char WanWebEnable=1;
unsigned char ipsecPassThrough=0;
unsigned char ipv6PassThrough=0;
WanIntfacesType wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM] = {0};
//wanBindingLanPorts[i]=m means lan[i] bind to wan[m] m=1,2,3,4

//static ip
unsigned char wanStaticIp[4]={192,168,2,100};
unsigned char wanStaticMask[4]={255,255,255,0};
unsigned char wanStaticDfgw[4]={192,168,2,254};
unsigned char wanStaticMtu=1500;

//PPPOE
unsigned char wanPppUserName[MAX_PPP_NAME_LEN+1]={0};
unsigned char wanPppPassWord[MAX_PPP_NAME_LEN+1]={0};
unsigned char wanPppServerName[MAX_PPP_NAME_LEN+1]={0};
unsigned char wanPppAcName[MAX_PPP_NAME_LEN+1]={0};
unsigned char wanPppConnectType=0;
unsigned char wanPppIdleTime=5;
unsigned char wanPppMtu=1452;


//DNS setting
unsigned char wanDNSManual = 0;
unsigned char wanDNS1={0};
unsigned char wanDNS2={0};
unsigned char wanDNS3={0};

//others
unsigned char cloneMacAddr[6]={0};



typedef struct _TestDataType
{
	int enable;
	int wantype;
	int serverType;

	int vlanEnable;
	int vlanId;
	int vlanPri;
	
	
} testDataType;
testDataType testData[WANIFACE_NUM+1]=
{
	{0},
	//enable/wantype/serverType/vlanEnable/vlanId/vlanPri
	{	1,		0,		0,		1,			0,		1},
	{	1,		1,		1,		0,			3,		2},
	{	0,		2,		2,		1,			5,		3},
	{	1,		3,		0,		1,			7,		4}
}; 



int TestApmib_get(int id,void* val)
{
	int index = 0;

	if(id==MIB_WANIFACE_CURRENT_IDX)
	{
		*(int*)val=currentIdx;
		return 0;
	}else
		index = currentIdx;
	switch(id)
	{//global
		case MIB_WANIFACE_TBL_NUM:
			*(int*)val = WANIFACE_NUM;
			return 0;
		case MIB_WANIFACE_BINDING_LAN_PORTS:
		{
			int i=0;
			for(i=0;i<WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM;i++)
			{
				((WanIntfacesType*)val)[i] = wanBindingLanPorts[i];
			}		
			return 0;
		}

		default:
			break;
	}	
	
	if(index<0||index>WANIFACE_NUM)
		return -1;
	switch(id)
	{//wanIntface		
		case MIB_EACHWANIFACE_ENABLED:
			*(int*)val=testData[index].enable;			
			break;
		case MIB_WANIFACE_WAN_TYPE:
			*(int*)val=testData[index].wantype;
			break;
		case MIB_WANIFACE_SERVICE_TYPE:
			*(int*)val=testData[index].serverType;			
			break;		
			
		case MIB_WANIFACE_VLAN:
			*(int*)val=testData[index].vlanEnable;
			break;
		case MIB_WANIFACE_VLANID:
			*(int*)val=testData[index].vlanId;
			printf("%s:%d vlanId=%d\n",__FUNCTION__,__LINE__,*(int*)val);
			break;
		case MIB_WANIFACE_VPRIORITY:
			*(int*)val=testData[index].vlanPri;			
			break;
		
		default:
			*(int*)val=0;
			return -1;
	}
	return 0;
	
}

int TestApmib_set(int id,void* val)
{
	int index = 0;
	if(id==MIB_WANIFACE_CURRENT_IDX)
	{
		currentIdx = *(int*)val;
		return 0;
	}else
		index = currentIdx;

	switch(id)
	{//global
		case MIB_WANIFACE_TBL_NUM:
			*(int*)val = WANIFACE_NUM;
			return 0;
		case MIB_WANIFACE_BINDING_LAN_PORTS:
		{
			int i=0;
			for(i=0;i<WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM;i++)
			{
				wanBindingLanPorts[i]=((WanIntfacesType*)val)[i];
			}			
			return 0;
		}

		default:
			break;
	}	
	if(index<0||index>WANIFACE_NUM)
		return -1;

	
	switch(id)
	{
		case MIB_EACHWANIFACE_ENABLED:
			testData[index].enable=*(int*)val;			
			break;
		case MIB_WANIFACE_WAN_TYPE:
			testData[index].wantype=*(int*)val;
			break;
		case MIB_WANIFACE_SERVICE_TYPE:
			testData[index].serverType=*(int*)val;
			break;		
			
		case MIB_WANIFACE_VLAN:
			testData[index].vlanEnable=*(int*)val;
			break;
		case MIB_WANIFACE_VLANID:
			testData[index].vlanId=*(int*)val;
			break;
		case MIB_WANIFACE_VPRIORITY:
			testData[index].vlanPri=*(int*)val;
			break;
		
		default:
			
			return -1;
	}
	return 0;
}
/* end test data */

/*	add by sen_liu 2012.1.13 for webpage multi_wan get mibInfo*/
typedef enum ValueType_
{
	VALUE_TYPE_INT,
	VALUE_TYPE_STR
} ValueType;


int setMutiWanMibValue(int mib_id,void* val)
{
	
	if(TestApmib_set(mib_id, val)<0)
	{
		printf("%s:%d set waniface value fail!\n",__FUNCTION__,__LINE__);
		return -1;
	}
	return 0;
}


//valType: 1:int 2:str
int getMutiWanValueByMibId(request *wp,ValueType valType,int mib_id)
{
	char buffer[128];
	int val=0;
	char valStr[128];
	
	memset(buffer,"\0",sizeof(char)*128);
	memset(valStr,"\0",sizeof(char)*128);
	switch(valType)
	{
		case VALUE_TYPE_INT:
			if(TestApmib_get(mib_id, (void *)&val)<0)
			{
				printf("%s:%d get waniface entry fail!\n",__FUNCTION__,__LINE__);
				return -1;
			}	
			sprintf(buffer, "%d", val);
			break;
		case VALUE_TYPE_STR:
			if(TestApmib_get(mib_id, (void *)&valStr)<0)
			{
				printf("%s:%d get waniface entry fail!\n",__FUNCTION__,__LINE__);
				return -1;
			}	
			sprintf(buffer, "%s", valStr);
			break;
	}
	
	req_format_write(wp, buffer);
	return 0;
}
#endif

#ifdef CONFIG_RTL_CTRL_MCAST
//extern int setMulticastVlanInfoValue(MULTICAST_VLANINFO_Tp pEntry,int index, int state);
int setMulticastVlanInfoValue(MULTICAST_VLANINFO_Tp pEntry,int index, int state)
{
	MULTICAST_VLANINFO_T entry[4];
	int i=0;
	memset(entry, '\0', sizeof(MULTICAST_VLANINFO_T)*4);

	int idxTbl;
	int idxDelall;
	int idxAdd;
	if(enum_control == state)
	{
		idxTbl = MIB_CONTROL_VLANINFO_TBL;
		idxDelall = MIB_CONTROL_VLANINFO_DELALL;
		idxAdd = MIB_CONTROL_VLANINFO_ADD;
	}
	else if(enum_normal == state)
	{
		idxTbl = MIB_NORMAL_VLANINFO_TBL;
		idxDelall = MIB_NORMAL_VLANINFO_DELALL;
		idxAdd = MIB_NORMAL_VLANINFO_ADD;
	}
	else
	{
		printf("%s:%d state error!\n",__FUNCTION__,__LINE__);
		return -1;
	}

	
//get all entry
	for(i=0;i<4;i++)
	{
		*((char*)(entry+i))=(char)(i+1);
		if(!apmib_get(idxTbl, (void *)(entry+i)))
		{
			printf("%s:%d get waniface entry fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}
	}
	//set change entry
	memcpy(entry+index-1,pEntry,sizeof(MULTICAST_VLANINFO_T));

//delete all entry
	if(!apmib_set(idxDelall,(void *)(entry)))
	{
		printf("%s:%d MIB_WANIFACE_DELALL fail!\n",__FUNCTION__,__LINE__);
			return -1;
	}

//add all entry
	for(i=0;i<4;i++)
	{
		if(!apmib_set(idxAdd,(void*)(entry+i)))
		{
			printf("%s:%d set MIB_WANIFACE_TBL fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}
	}
	return 0;
}
#endif


/* set wan Interface value by index(1,2...)*/
int setMutiWanIntfaceValue(WANIFACE_Tp pEntry,WanIntfacesType index)
{
	WANIFACE_T waniface_mod[2]={0};

	*((char*)waniface_mod) = ((char)index);
	if(!apmib_get(MIB_WANIFACE_TBL, (void *)(waniface_mod)))
	{
		printf("%s:%d get waniface entry fail!\n",__FUNCTION__,__LINE__);
		return -1;
	}

	memcpy(&(waniface_mod[1]),pEntry,sizeof(WANIFACE_T));

	if(!apmib_set(MIB_WANIFACE_MOD,(void*)waniface_mod))
	{
		printf("%s:%d set MIB_WANIFACE_MOD fail!\n",__FUNCTION__,__LINE__);
		return -1;
	}
/*
	WANIFACE_T waniface_entry[WANIFACE_NUM];
	int i=0;
	memset(waniface_entry, '\0', sizeof(WANIFACE_T)*WANIFACE_NUM);

//get all entry
	for(i=0;i<WANIFACE_NUM;i++)
	{
		*((char*)(waniface_entry+i))=(char)(i+1);
		if(!apmib_get(MIB_WANIFACE_TBL, (void *)(waniface_entry+i)))
		{
			printf("%s:%d get waniface entry fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}
	}
	//set change entry
	memcpy(waniface_entry+index-1,pEntry,sizeof(WANIFACE_T));

//delete all entry
	if(!apmib_set(MIB_WANIFACE_DELALL,(void *)(waniface_entry)))
	{
		printf("%s:%d MIB_WANIFACE_DELALL fail!\n",__FUNCTION__,__LINE__);
			return -1;
	}

//add all entry
	for(i=0;i<WANIFACE_NUM;i++)
	{
		if(!apmib_set(MIB_WANIFACE_ADD,(void*)(waniface_entry+i)))
		{
			printf("%s:%d set MIB_WANIFACE_TBL fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}
	}*/
	return 0;
}


/***********************************
	for multi-wan
************************************/

int get_ppp_status(int wan_index)
{
	unsigned char link_file[64];
	sprintf(link_file,"/etc/ppp/link%d",wan_index);	
	
	if(isFileExist(link_file) == 1)
		return 1;
	else
		return 0;
}

int getMultiWanIndex(request *wp, int argc, char **argv)
{
	char *name;
	char buffer[128]={0};
	int wan_index = 0;
	unsigned char wan_num = 0;
	int val =0,i=0;
	WANIFACE_T entry;
	WanIntfacesType wanIdx=WAN1;
	memset(&entry, '\0', sizeof(entry));


	memset(buffer,'\0',sizeof(char)*128);
	name = argv[0];
	wan_num=WANIFACE_NUM;
/*********************************************************************
		for global data
*********************************************************************/	

	if(!strcmp(name, "WANIfaceNum"))
	{
		sprintf(buffer, "%d", wan_num);
		
		req_format_write(wp, buffer);
		return 0;
	}

	if(argc==1)
	{
		if ( !apmib_get( MIB_WANIFACE_CURRENT_IDX, (void *)&wanIdx) )
		{
			fprintf(stderr,"%s:%d get MIB_WANIFACE_CURRENT_IDX fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}
		if(!strcmp(name, "currentWanIdx"))
		{
			//fprintf(stderr,"%s:%d get MIB_WANIFACE_CURRENT_IDX %d!\n",__FUNCTION__,__LINE__,wanIdx);
			if(wanIdx<WAN1||wanIdx>wan_num)
				wanIdx=NO_WAN;
			if(wanIdx>0)
				wanIdx--;
			sprintf(buffer, "%d", wanIdx);
		
			req_format_write(wp, buffer);
			return 0;
		}
	}else if(!strcmp(argv[1],"WAN"))
	{
		//printf("%s,%d argc=%d\n",__FUNCTION__,__LINE__,argc);
		//printf("%s,%d argv[2]=%s\n",__FUNCTION__,__LINE__,argv[2]);
		//return -1;
		wanIdx=atoi(argv[2]);
		//return -1;
	}
	//printf("%s:%d get MIB_WANIFACE_CURRENT_IDX=%d\n",__FUNCTION__,__LINE__,val);
	wan_index = wanIdx;
	//printf("%s:%d in wan_index=%d wan_num=%d\n",__FUNCTION__,__LINE__,wan_index,wan_num);
	
	if(wan_index<WAN1||wan_index>wan_num)//1 - wan_num
	{
		fprintf(stderr,"%s:%d MIB_WANIFACE_CURRENT_IDX illegal!\n",__FUNCTION__,__LINE__);
		return -1;
	}
	

	/************	wan bind lan ports	************/
	if ( !strcmp(name, "wanBindPortArray")) 
	{		
		WanIntfacesType wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM]={0};
		if(!apmib_get(MIB_WANIFACE_BINDING_LAN_PORTS, (void *)&wanBindingLanPorts))
		{
			printf("%s:%d get MIB_WANIFACE_BINDING_LAN_PORTS fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}
		//for(i=0;i<WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM;i++)
		//{
		//	printf("%s:%d wanBindingLanPorts[%d]=%d\n",__FUNCTION__,__LINE__,i,wanBindingLanPorts[i]);
		//}
		//printf("%s:%d wan_index=%d\n",__FUNCTION__,__LINE__,wan_index);

		for(i=0;i<WAN_INTERFACE_LAN_PORT_NUM + WAN_INTERFACE_WLAN_PORT_NUM;i++)
		{
			if(wanBindingLanPorts[i]==wan_index)
				strcat(buffer,"1,");
			else
				strcat(buffer,"0,");
		}
		buffer[strlen(buffer)-1]='\0';
		req_format_write(wp, buffer);

		return 0;
	}

	if ( !strcmp(name, "availableBindingPorts")) 
	{		
		
		WanIntfacesType wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM]={0};
		if(!apmib_get(MIB_WANIFACE_BINDING_LAN_PORTS, (void *)&wanBindingLanPorts))
		{
			printf("%s:%d get MIB_WANIFACE_FREE_BINDING_PORTS fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}
		

		for(i=0;i<WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM;i++)
		{
			if(wanBindingLanPorts[i]==wan_index||wanBindingLanPorts[i]==0)
				strcat(buffer,"1,");
			else
				strcat(buffer,"0,");
		}		
		
		buffer[strlen(buffer)-1]='\0';
		req_format_write(wp, buffer);
		return 0;
	}

	if(!strcmp(name,"wanBindLanPortNum"))
	{
		sprintf(buffer,"%d",WAN_INTERFACE_LAN_PORT_NUM);
		req_format_write(wp, buffer);
		return 0;
	}
	if(!strcmp(name,"wanBindWlanPortNum"))
	{
		sprintf(buffer,"%d",WAN_INTERFACE_WLAN_PORT_NUM);
		req_format_write(wp, buffer);
		return 0;
	}
	
	/*********************************************************************
			for wan interface data
	*********************************************************************/
	
	
	//printf("%s:%d in wan_index=%d wan_num=%d\n",__FUNCTION__,__LINE__,wan_index,wan_num);
	
	

	*((char*)&entry)=(char)wan_index;
	if(!apmib_get(MIB_WANIFACE_TBL, (void *)&entry))
	{
		printf("%s:%d get waniface entry fail!\n",__FUNCTION__,__LINE__);
		return -1;
	}
	//printf("%s:%d get wanType=%d \n",__FUNCTION__,__LINE__,entry.AddressType);

	if(!strcmp(name, "WANInfaceEnabled"))		sprintf(buffer,"%d",entry.enable);
	else if(!strcmp(name, "wanDhcp"))	   		sprintf(buffer,"%d",entry.AddressType);
	else if(!strcmp(name, "wanServerType")) 	sprintf(buffer,"%d",entry.ServiceList);
	
	//static ip
	else if(!strcmp(name, "wan-ip-rom"))  		sprintf(buffer,"%s",inet_ntoa(*((struct in_addr *)entry.ipAddr)));
	else if(!strcmp(name, "wan-mask-rom"))  	sprintf(buffer,"%s",inet_ntoa(*((struct in_addr *)entry.netMask)));
	else if(!strcmp(name, "wan-gateway-rom"))  	sprintf(buffer,"%s",inet_ntoa(*((struct in_addr *)entry.remoteIpAddr)));
	else if(!strcmp(name, "fixedIpMtuSize"))  	sprintf(buffer,"%d",entry.staticIpMtu);

	//dhcp
	else if(!strcmp(name,"hostName")) 			sprintf(buffer,"%s",entry.dhcpHostName);
	else if(!strcmp(name,"dhcpMtuSize")) 		sprintf(buffer,"%d",entry.dhcpMtu);

	//pppoe
	else if(!strcmp(name,"pppUserName")) 		sprintf(buffer,"%s",entry.pppUsername);
	else if(!strcmp(name,"pppPassword")) 		sprintf(buffer,"%s",entry.pppPassword);
	else if(!strcmp(name,"pppServiceName")) 	sprintf(buffer,"%s",entry.pppServiceName);
	else if(!strcmp(name,"pppAcName")) 			sprintf(buffer,"%s",entry.pppACName);
	else if(!strcmp(name,"pppConnectType")) 	sprintf(buffer,"%d",entry.pppCtype);
	else if(!strcmp(name,"pppConnectStatus")) 	{sprintf(buffer,"%d",get_ppp_status(wan_index));}
	else if(!strcmp(name,"wan-ppp-idle")) 		sprintf(buffer,"%d",entry.pppIdleTime);
	else if(!strcmp(name,"pppMtuSize")) 		sprintf(buffer,"%d",entry.pppoeMtu);

#if  defined(SINGLE_WAN_SUPPORT)	
	//pptp
	else if(!strcmp(name, "pptpIp"))  			sprintf(buffer,"%s",inet_ntoa(*((struct in_addr *)entry.pptpIpAddr)));	
	else if(!strcmp(name, "pptpSubnet"))  		sprintf(buffer,"%s",inet_ntoa(*((struct in_addr *)entry.pptpSubnetMask)));	
	else if(!strcmp(name, "pptpDefGw"))  		sprintf(buffer,"%s",inet_ntoa(*((struct in_addr *)entry.pptpGateway)));	
	else if(!strcmp(name, "pptpServerIp"))  	sprintf(buffer,"%s",inet_ntoa(*((struct in_addr *)entry.pptpServerIpAddr)));	
	else if(!strcmp(name, "pptpUserName"))		sprintf(buffer,"%s",entry.pptpUserName);
	else if(!strcmp(name, "pptpPassword")) 		sprintf(buffer,"%s",entry.pptpPassword);
	else if(!strcmp(name, "pptpConnectType")) 	sprintf(buffer,"%d",entry.pptpConnectType);
	else if(!strcmp(name, "pptpIdleTime")) 		sprintf(buffer,"%d",entry.pptpIdleTime);
	else if(!strcmp(name, "pptpMtuSize")) 		sprintf(buffer,"%d",entry.pptpMtuSize);
	else if(!strcmp(name, "pptpSecurity"))		sprintf(buffer,"%d",entry.pptpSecurityEnabled);
	else if(!strcmp(name, "pptpCompress"))		sprintf(buffer,"%d",entry.pptpMppcEnabled);
	else if(!strcmp(name, "pptpConnectStatus")) sprintf(buffer,"%d",get_ppp_status(wan_index));
	//l2tp
	else if(!strcmp(name, "l2tpIp"))  			sprintf(buffer,"%s",inet_ntoa(*((struct in_addr *)entry.l2tpIpAddr)));	
	else if(!strcmp(name, "l2tpSubnet"))  		sprintf(buffer,"%s",inet_ntoa(*((struct in_addr *)entry.l2tpSubnetMask)));	
	else if(!strcmp(name, "l2tpDefGw"))  		sprintf(buffer,"%s",inet_ntoa(*((struct in_addr *)entry.l2tpGateway)));	
	else if(!strcmp(name, "l2tpServerIp"))  	sprintf(buffer,"%s",inet_ntoa(*((struct in_addr *)entry.l2tpServerIpAddr)));	
	else if(!strcmp(name, "l2tpUserName"))		sprintf(buffer,"%s",entry.l2tpUserName);
	else if(!strcmp(name, "l2tpPassword")) 		sprintf(buffer,"%s",entry.l2tpPassword);
	else if(!strcmp(name, "l2tpConnectType")) 	sprintf(buffer,"%d",entry.l2tpConnectType);
	else if(!strcmp(name, "l2tpIdleTime")) 		sprintf(buffer,"%d",entry.l2tpIdleTime);
	else if(!strcmp(name, "l2tpMtuSize")) 		sprintf(buffer,"%d",entry.l2tpMtuSize);
	else if(!strcmp(name, "l2tpConnectStatus")) sprintf(buffer,"%d",get_ppp_status(wan_index));	
#else
	else if(!strcmp(name, "pptpConnectType"))  sprintf(buffer,"0");
	else if(!strcmp(name, "pptpConnectStatus"))  sprintf(buffer,"0");
	else if(!strcmp(name, "pptpSecurity"))  sprintf(buffer,"0");
	else if(!strcmp(name, "pptpCompress"))  sprintf(buffer,"0");
	
	else if(!strcmp(name, "l2tpConnectType"))  sprintf(buffer,"0");
	else if(!strcmp(name, "l2tpConnectStatus"))  sprintf(buffer,"0");	
#endif	
	//bridge
	//dns  
	else if(!strcmp(name,"wanDnsAuto")) 		sprintf(buffer,"%d",entry.dnsAuto);
	else if(!strcmp(name,"wan-dns1")) 			sprintf(buffer,"%s",inet_ntoa(*((struct in_addr *)entry.wanIfDns1)));
	else if(!strcmp(name,"wan-dns2")) 			sprintf(buffer,"%s",inet_ntoa(*((struct in_addr *)entry.wanIfDns2)));
	else if(!strcmp(name,"wan-dns3")) 			sprintf(buffer,"%s",inet_ntoa(*((struct in_addr *)entry.wanIfDns3)));
	//vlan
	else if(!strcmp(name, "vlanEnable"))  		sprintf(buffer,"%d",entry.vlan);
	else if(!strcmp(name, "INPUT_VlanId"))	  	sprintf(buffer,"%d",entry.vlanid);
	else if(!strcmp(name,"INPUT_vlan_priority"))sprintf(buffer,"%d",entry.vlanpriority);
	//others 
	else if(!strcmp(name,"wanMac"))				sprintf(buffer,"%02x%02x%02x%02x%02x%02x",
	(unsigned char)entry.wanMacAddr[0],(unsigned char)entry.wanMacAddr[1],(unsigned char)entry.wanMacAddr[2],
	(unsigned char)entry.wanMacAddr[3],(unsigned char)entry.wanMacAddr[4],(unsigned char)entry.wanMacAddr[5]);
	else if(!strcmp(name,"enableIGMP"))			sprintf(buffer,"%d",entry.enableIGMP);
#if defined CONFIG_ISP_IGMPPROXY_MULTIWAN
	else if(!strcmp(name,"onPhyIf"))			sprintf(buffer,"%d",entry.onPhyIf);
#endif	
	else if(!strcmp(name,"upnpEnabled"))			sprintf(buffer,"%d",entry.enableUPNP);
#ifdef CONFIG_IPV6
	else if(!strcmp(name,"ipv6WanEnabled")) 		sprintf(buffer,"%d",entry.ipv6Enable);
	else if(!strcmp(name,"ipv6Origin"))	  			sprintf(buffer,"%d",entry.ipv6Origin);
	else if(!strcmp(name,"ipv6LinkType")) 			sprintf(buffer,"%d",entry.ipv6LinkType);
//ipv6 wan addr
	else if(!strcmp(name,"wan_ipv6Addr_0"))			sprintf(buffer,"%04x",entry.ipv6Addr.addrIPv6[0]);
	else if(!strcmp(name,"wan_ipv6Addr_1"))			sprintf(buffer,"%04x",entry.ipv6Addr.addrIPv6[1]);
	else if(!strcmp(name,"wan_ipv6Addr_2"))			sprintf(buffer,"%04x",entry.ipv6Addr.addrIPv6[2]);
	else if(!strcmp(name,"wan_ipv6Addr_3"))			sprintf(buffer,"%04x",entry.ipv6Addr.addrIPv6[3]);
	else if(!strcmp(name,"wan_ipv6Addr_4"))			sprintf(buffer,"%04x",entry.ipv6Addr.addrIPv6[4]);
	else if(!strcmp(name,"wan_ipv6Addr_5"))			sprintf(buffer,"%04x",entry.ipv6Addr.addrIPv6[5]);
	else if(!strcmp(name,"wan_ipv6Addr_6"))			sprintf(buffer,"%04x",entry.ipv6Addr.addrIPv6[6]);
	else if(!strcmp(name,"wan_ipv6Addr_7"))			sprintf(buffer,"%04x",entry.ipv6Addr.addrIPv6[7]);
	else if(!strcmp(name,"wan_ipv6Addr_prefixLen"))	sprintf(buffer,"%d",entry.ipv6Addr.prefix_len);
//ipv6 wan gw
	else if(!strcmp(name,"wan_ipv6Gw_0"))			sprintf(buffer,"%04x",entry.ipv6Gw.addrIPv6[0]);
	else if(!strcmp(name,"wan_ipv6Gw_1"))			sprintf(buffer,"%04x",entry.ipv6Gw.addrIPv6[1]);
	else if(!strcmp(name,"wan_ipv6Gw_2"))			sprintf(buffer,"%04x",entry.ipv6Gw.addrIPv6[2]);
	else if(!strcmp(name,"wan_ipv6Gw_3"))			sprintf(buffer,"%04x",entry.ipv6Gw.addrIPv6[3]);
	else if(!strcmp(name,"wan_ipv6Gw_4"))			sprintf(buffer,"%04x",entry.ipv6Gw.addrIPv6[4]);
	else if(!strcmp(name,"wan_ipv6Gw_5"))			sprintf(buffer,"%04x",entry.ipv6Gw.addrIPv6[5]);
	else if(!strcmp(name,"wan_ipv6Gw_6"))			sprintf(buffer,"%04x",entry.ipv6Gw.addrIPv6[6]);
	else if(!strcmp(name,"wan_ipv6Gw_7"))			sprintf(buffer,"%04x",entry.ipv6Gw.addrIPv6[7]);
	else if(!strcmp(name,"wan_ipv6Gw_prefixLen"))	sprintf(buffer,"%d",entry.ipv6Gw.prefix_len);

	else if(!strcmp(name,"wan_ipv6DnsAuto"))		sprintf(buffer,"%d",entry.ipv6_dnsAuto);
//ipv6 wan dns1
	else if(!strcmp(name,"wan_ipv6Dns1_0"))			sprintf(buffer,"%04x",entry.ipv6dns1.addrIPv6[0]);
	else if(!strcmp(name,"wan_ipv6Dns1_1"))			sprintf(buffer,"%04x",entry.ipv6dns1.addrIPv6[1]);
	else if(!strcmp(name,"wan_ipv6Dns1_2"))			sprintf(buffer,"%04x",entry.ipv6dns1.addrIPv6[2]);
	else if(!strcmp(name,"wan_ipv6Dns1_3"))			sprintf(buffer,"%04x",entry.ipv6dns1.addrIPv6[3]);
	else if(!strcmp(name,"wan_ipv6Dns1_4"))			sprintf(buffer,"%04x",entry.ipv6dns1.addrIPv6[4]);
	else if(!strcmp(name,"wan_ipv6Dns1_5"))			sprintf(buffer,"%04x",entry.ipv6dns1.addrIPv6[5]);
	else if(!strcmp(name,"wan_ipv6Dns1_6"))			sprintf(buffer,"%04x",entry.ipv6dns1.addrIPv6[6]);
	else if(!strcmp(name,"wan_ipv6Dns1_7"))			sprintf(buffer,"%04x",entry.ipv6dns1.addrIPv6[7]);
	else if(!strcmp(name,"wan_ipv6Dns1_prefixLen"))	sprintf(buffer,"%d",entry.ipv6dns1.prefix_len);
//ipv6 wan dns2
	else if(!strcmp(name,"wan_ipv6Dns2_0")) 		sprintf(buffer,"%04x",entry.ipv6dns2.addrIPv6[0]);
	else if(!strcmp(name,"wan_ipv6Dns2_1")) 		sprintf(buffer,"%04x",entry.ipv6dns2.addrIPv6[1]);
	else if(!strcmp(name,"wan_ipv6Dns2_2")) 		sprintf(buffer,"%04x",entry.ipv6dns2.addrIPv6[2]);
	else if(!strcmp(name,"wan_ipv6Dns2_3")) 		sprintf(buffer,"%04x",entry.ipv6dns2.addrIPv6[3]);
	else if(!strcmp(name,"wan_ipv6Dns2_4")) 		sprintf(buffer,"%04x",entry.ipv6dns2.addrIPv6[4]);
	else if(!strcmp(name,"wan_ipv6Dns2_5")) 		sprintf(buffer,"%04x",entry.ipv6dns2.addrIPv6[5]);
	else if(!strcmp(name,"wan_ipv6Dns2_6")) 		sprintf(buffer,"%04x",entry.ipv6dns2.addrIPv6[6]);
	else if(!strcmp(name,"wan_ipv6Dns2_7")) 		sprintf(buffer,"%04x",entry.ipv6dns2.addrIPv6[7]);
	else if(!strcmp(name,"wan_ipv6Dns2_prefixLen")) sprintf(buffer,"%d",entry.ipv6dns2.prefix_len);
	else if(!strcmp(name,"mldproxyDisabled"))		sprintf(buffer,"%d",entry.ipv6MldproxyDisabled);
//dhcpv6 client
	else if(!strcmp(name,"ifname_dhcpv6c"))			sprintf(buffer,"%s",entry.dhcp6cCfgParam.interfaceName);
	else if(!strcmp(name,"enable_dhcpv6pd"))		sprintf(buffer,"%d",entry.dhcp6cCfgParam.enabled & DHCP6C_PREFIX_ENABLE);
	else if(!strcmp(name,"interfacename_pd"))		sprintf(buffer,"%s",entry.dhcp6cCfgParam.dhcp6pd.interfaceName);
	else if(!strcmp(name,"sla_len"))				sprintf(buffer,"%d",entry.dhcp6cCfgParam.dhcp6pd.sla_len);
	else if(!strcmp(name,"sla_id"))					sprintf(buffer,"%d",entry.dhcp6cCfgParam.dhcp6pd.sla_id);

//lan
//	else if(!strcmp(name,"ipv6LanPrefixType"))		sprintf(buffer,"%d",entry.ipv6LanPrefixType);
//ipv6 lan addr
	else if(!strcmp(name,"lan_ipv6Addr_0"))			sprintf(buffer,"%04x",entry.ipv6AddrLan.addrIPv6[0]);
	else if(!strcmp(name,"lan_ipv6Addr_1"))			sprintf(buffer,"%04x",entry.ipv6AddrLan.addrIPv6[1]);
	else if(!strcmp(name,"lan_ipv6Addr_2"))			sprintf(buffer,"%04x",entry.ipv6AddrLan.addrIPv6[2]);
	else if(!strcmp(name,"lan_ipv6Addr_3"))			sprintf(buffer,"%04x",entry.ipv6AddrLan.addrIPv6[3]);
	else if(!strcmp(name,"lan_ipv6Addr_4"))			sprintf(buffer,"%04x",entry.ipv6AddrLan.addrIPv6[4]);
	else if(!strcmp(name,"lan_ipv6Addr_5"))			sprintf(buffer,"%04x",entry.ipv6AddrLan.addrIPv6[5]);
	else if(!strcmp(name,"lan_ipv6Addr_6"))			sprintf(buffer,"%04x",entry.ipv6AddrLan.addrIPv6[6]);
	else if(!strcmp(name,"lan_ipv6Addr_7"))			sprintf(buffer,"%04x",entry.ipv6AddrLan.addrIPv6[7]);
	else if(!strcmp(name,"lan_ipv6Addr_prefixLen"))	sprintf(buffer,"%04x",entry.ipv6AddrLan.prefix_len);
//dhcpv6 server	
	else if(!strcmp(name,"dhcpv6s_enabled")) 		sprintf(buffer,"%d",entry.dhcp6sCfgParam.enabled);
	else if(!strcmp(name,"dhcpv6s_dnsaddr")) 		sprintf(buffer,"%s",entry.dhcp6sCfgParam.DNSaddr6);
	else if(!strcmp(name,"dhcpv6s_interfacenameds"))sprintf(buffer,"%s",entry.dhcp6sCfgParam.interfaceNameds);
	else if(!strcmp(name,"dhcpv6s_addrPoolStart")) 	sprintf(buffer,"%s",entry.dhcp6sCfgParam.addr6PoolS);
	else if(!strcmp(name,"dhcpv6s_addrPoolEnd")) 	sprintf(buffer,"%s",entry.dhcp6sCfgParam.addr6PoolE);
//radvd		
	else if(!strcmp(name,"radvdinterfacename"))		sprintf(buffer,"%d",entry.radvdCfgParam.interface.Name);
#endif
	else return -1;
	//fprintf(stderr,"%s:%d name=%s bufer=%s\n",__FUNCTION__,__LINE__,name,buffer);

	
	req_format_write(wp, buffer);
		return 0;
	
}
/* show wan List*/
#define WAN_ITEM_MAX_LENGTH 128
int getWanList(request *wp, int argc, char **argv)
{
	int wan_num=0,i=0;
	int var=0;//,enable=0;
	char bufferWanEnabled[32],bufferWantype[32],bufferServerType[32];
	char buffer[WAN_ITEM_MAX_LENGTH];
	WANIFACE_T entry;

	
	memset(buffer,'\0',sizeof(char)*WAN_ITEM_MAX_LENGTH);
	memset(bufferWanEnabled,'\0',sizeof(char)*32);
	memset(bufferWantype,'\0',sizeof(char)*32);
	memset(bufferServerType,'\0',sizeof(char)*32);
	/*if ( TestApmib_get( MIB_WANIFACE_TBL_NUM, (void *)&wan_num)<0 )
	{
		fprintf(stderr,"%s:%d get MIB_WANIFACE_TBL_NUM fail!\n",__FUNCTION__,__LINE__);
		return -1;
	}*/

	wan_num=WANIFACE_NUM;
	for(i=1;i<=wan_num;i++)
	{
		/*if(!apmib_set(MIB_WANIFACE_CURRENT_IDX, (void*)&i))
		{
			printf("%s:%d set MIB_WANIFACE_CURRENT_IDX fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}*/
		memset(&entry, '\0', sizeof(entry));
		*((char *)&entry) = (char)i;
		if(!apmib_get(MIB_WANIFACE_TBL, (void *)&entry))
		{
			printf("%s:%d get MIB_WANIFACE_TBL fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}
		//enable=entry.enable;
		/*if(entry.connDisable)
			enable = 0;
		else
			enable = 1;*/
		if(entry.enable)
			strcpy(bufferWanEnabled,"Enabled");
		else
			strcpy(bufferWanEnabled,"Disabled");

//fprintf(stderr,"%s:%d get entry=%d,%d,%d \n",__FUNCTION__,__LINE__,entry.enable,entry.AddressType,entry.applicationtype);

		/*if(TestApmib_get(MIB_WANIFACE_WAN_TYPE, (void *)&var)<0)
		{
			printf("%s:%d get MIB_WANIFACE_WAN_TYPE fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}*/
		if(!entry.enable)
			strcpy(bufferWantype,"NONE");
		else
		{
			switch(entry.AddressType)
			{
				case DHCP_DISABLED:
					strcpy(bufferWantype,"Static ip");
					
					break;
				case DHCP_CLIENT:
					strcpy(bufferWantype,"DHCP client");
					
					break;
				case PPPOE:
					strcpy(bufferWantype,"PPPOE");
					
					break;
				case BRIDGE:
					strcpy(bufferWantype,"Bridge");
					
					break;
				default:
					strcpy(bufferWantype,"error");

					break;
			}
		}

		getMultiWanApplicationType(entry.ServiceList,bufferServerType,sizeof(bufferServerType));

		/*if(TestApmib_get(MIB_WANIFACE_SERVICE_TYPE, (void *)&var)<0)
		{
			printf("%s:%d get MIB_WANIFACE_SERVICE_TYPE fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}*/
/*		switch(entry.applicationtype)
		{//TR069_INTERNET(0),INTERNET(1), TR069(2),  Other(3)
			case 0:
				strcpy(bufferServerType,"TR069_INTERNET");
				break;
			case 1:
				strcpy(bufferServerType,"INTERNET");
				
				break;
			case 2:
				strcpy(bufferServerType,"TR069");
				
				break;
			case 3:
				strcpy(bufferServerType,"Other");
				break;
			case 4:
				strcpy(bufferServerType,"Iptv");
				break;
			case 5:
				strcpy(bufferServerType,"Voip");
				break;
			default:
				strcpy(bufferServerType,"Error");
				
				break;
		}
*/
		

		sprintf(buffer,"wanListArray[%d] = new Array(\"%s\",\"WAN%d\",\"%s\",\"%s\");\n",i-1,bufferWanEnabled,i,bufferWantype,bufferServerType);
		req_format_write(wp, buffer);
	}
	return 0;
}
#ifdef CONFIG_IPV6
int ipv6_getWanList(request *wp, int argc, char **argv)
{
	int i=0,wan_num=0,wanEnable=0;
	char bufferWanEnabled[32],bufferLinktype[32],bufferOrigin[32],bufferServerType[32];
	char buffer[WAN_ITEM_MAX_LENGTH];
	IPV6_LINKTYPE_T linkType=IPV6_LINKTYPE_IP;
	IPV6_ORIGIN_T	origin = IPV6_ORIGIN_DHCP;
	unsigned int	ServiceList =0;

	

	wan_num=WANIFACE_NUM;
	for(i=1;i<=wan_num;i++)
	{
		bzero(bufferWanEnabled,sizeof(bufferWanEnabled));
		bzero(bufferLinktype,sizeof(bufferLinktype));
		bzero(bufferOrigin,sizeof(bufferOrigin));
		bzero(bufferServerType,sizeof(bufferServerType));
		bzero(buffer,sizeof(buffer));
	
		if(!apmib_set(MIB_WANIFACE_CURRENT_IDX, (void*)&i))
		{
			printf("%s:%d set MIB_WANIFACE_CURRENT_IDX fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}

		if(!apmib_get(MIB_WANIFACE_IPV6_ENABLE,(void*)&wanEnable))
		{
			printf("%s:%d set MIB_WANIFACE_IPV6_ENABLE fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}
		if(!apmib_get(MIB_WANIFACE_IPV6_LINK_TYPE,(void*)&linkType))
		{
			printf("%s:%d set MIB_WANIFACE_IPV6_ENABLE fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}
		if(!apmib_get(MIB_WANIFACE_IPV6_ORIGIN,(void*)&origin))
		{
			printf("%s:%d set MIB_WANIFACE_IPV6_ENABLE fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}
		if(!apmib_get(MIB_WANIFACE_SERVICELIST,(void*)&ServiceList))
		{
			printf("%s:%d set MIB_WANIFACE_APPLICATIONTYPE fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}

		if(wanEnable)
			strcpy(bufferWanEnabled,"Enabled");
		else
			strcpy(bufferWanEnabled,"Disabled");

		switch(linkType)
		{
			case IPV6_LINKTYPE_PPP:
				strcpy(bufferLinktype,"PPPOE");
				break;
			case IPV6_LINKTYPE_IP:
				strcpy(bufferLinktype,"IP");
				break;
			
			default:
				printf("%s:%d invalid linkType=%d\n",__FUNCTION__,__LINE__,linkType);
				return -1;
		}

		switch(origin)
		{
			case IPV6_ORIGIN_AUTO:
				strcpy(bufferOrigin,"auto config");
				break;
			case IPV6_ORIGIN_DHCP:
				strcpy(bufferOrigin,"dhcp");
				break;
			case IPV6_ORIGIN_STATIC:
				strcpy(bufferOrigin,"static ip");
				break;
			
			default:
				printf("%s:%d invalid origin=%d\n",__FUNCTION__,__LINE__,origin);
				return -1;
		}
		getMultiWanApplicationType(ServiceList,bufferServerType,sizeof(bufferServerType));

/*		switch(applicationtype)
		{//TR069_INTERNET(0),INTERNET(1), TR069(2),  Other(3)
			case 0:
				strcpy(bufferServerType,"TR069_INTERNET");
				break;
			case 1:
				strcpy(bufferServerType,"INTERNET");
				
				break;
			case 2:
				strcpy(bufferServerType,"TR069");
				
				break;
			case 3:
				strcpy(bufferServerType,"Other");
				
				break;
			default:
				strcpy(bufferServerType,"Error");
				
				break;
		}
*/
		sprintf(buffer,"wanListArray[%d] = new Array(\"%s\",\"WAN%d\",\"%s\",\"%s\",\"%s\");\n",i-1,bufferWanEnabled,i,bufferLinktype,bufferOrigin,bufferServerType);
		//printf("%s \n",buffer);
		req_format_write(wp, buffer);		
	}
	
}

int ipv6_getLanList(request *wp, int argc, char **argv)
{
	int i=0,wan_num=0,wanEnable=0;
	char bufferWanEnabled[32],bufferOrigin[32],bufferServerType[32];
	char buffer[WAN_ITEM_MAX_LENGTH];
	IPV6_ORIGIN_T	origin = IPV6_PREFIXTYPE_DHCP;
	unsigned int	ServiceList =0;

	

	wan_num=WANIFACE_NUM;
	for(i=1;i<=wan_num;i++)
	{
		bzero(bufferWanEnabled,sizeof(bufferWanEnabled));
		bzero(bufferOrigin,sizeof(bufferOrigin));
		bzero(bufferServerType,sizeof(bufferServerType));
		bzero(buffer,sizeof(buffer));
	
		if(!apmib_set(MIB_WANIFACE_CURRENT_IDX, (void*)&i))
		{
			printf("%s:%d set MIB_WANIFACE_CURRENT_IDX fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}

		if(!apmib_get(MIB_WANIFACE_IPV6_ENABLE,(void*)&wanEnable))
		{
			printf("%s:%d get MIB_WANIFACE_IPV6_ENABLE fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}
		
		/*if(!apmib_get(MIB_LANIFACE_IPV6_PREFIX_TYPE,(void*)&origin))
		{
			printf("%s:%d get MIB_LANIFACE_IPV6_PREFIX_TYPE fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}*/
		if(!apmib_get(MIB_WANIFACE_SERVICELIST,(void*)&ServiceList))
		{
			printf("%s:%d get MIB_WANIFACE_APPLICATIONTYPE fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}

		if(wanEnable)
			strcpy(bufferWanEnabled,"Enabled");
		else
			strcpy(bufferWanEnabled,"Disabled");

		getMultiWanApplicationType(ServiceList,bufferServerType,sizeof(bufferServerType));
/*
		switch(origin)
		{
			case IPV6_PREFIXTYPE_DHCP:
				strcpy(bufferOrigin,"dhcp server");
				break;
			case IPV6_PREFIXTYPE_RADVD:
				strcpy(bufferOrigin,"radvd");
				break;
			case IPV6_PREFIXTYPE_STATIC:
				strcpy(bufferOrigin,"static ip");
				break;
			
			default:
				printf("%s:%d invalid origin=%d\n",__FUNCTION__,__LINE__,origin);
				return -1;
		}

		switch(applicationtype)
		{//TR069_INTERNET(0),INTERNET(1), TR069(2),  Other(3)
			case 0:
				strcpy(bufferServerType,"TR069_INTERNET");
				break;
			case 1:
				strcpy(bufferServerType,"INTERNET");
				
				break;
			case 2:
				strcpy(bufferServerType,"TR069");
				
				break;
			case 3:
				strcpy(bufferServerType,"Other");
				
				break;
			case 4:
				strcpy(bufferServerType,"Iptv");
				break;
			case 5:
				strcpy(bufferServerType,"Voip");
				break;
			default:
				strcpy(bufferServerType,"Error");
				
				break;
		}
		*/

		sprintf(buffer,"wanListArray[%d] = new Array(\"%s\",\"LAN%d\",\"%s\");\n",i-1,bufferWanEnabled,i,bufferServerType);
		//printf("%s \n",buffer);
		req_format_write(wp, buffer);		
	}
	
}


#endif

int getRealInterface(char *wanIface,char * realIface)
{
	FILE *pMap = NULL;
	char strPppName[8]={0},strWanName[8]={0};
	if((pMap = fopen("/etc/ppp/pppx_map2_ethm","r+"))!= NULL)
	{			
		while( fscanf(pMap,"%s %s",strPppName,strWanName) > 0 )
		{						   
		   if(!strcmp(strWanName,wanIface))
		   {
		   		strcpy(realIface,strPppName);
				break;
		   }	
		   memset(strPppName,0,sizeof(char)*8);
		   memset(strWanName,0,sizeof(char)*8);
		}
		close(pMap);
		return 0;
	}	
	return -1;
}

/*	end	*/
int getWanStatusList(request *wp, int argc, char **argv)
{
	int wan_num=0,i=0,isWanPhy_Link=0;
	char buffer[WAN_ITEM_MAX_LENGTH];
	char strWanCurrentInfo[WAN_ITEM_MAX_LENGTH] = {0};
	char strWanIfName[8]={0};
	char strWanPppIfName[8]={0};
	char strWanIP[16]={0};
	char strWanMask[16]={0};
	char strWanDefGW[16]={0};
	char strWanHWAddr[18]={0};
	char strWanHWAddrTmp[18]={0};
	WANIFACE_T entry;
	memset(buffer,'\0',sizeof(char)*WAN_ITEM_MAX_LENGTH);
	
	wan_num=WANIFACE_NUM;
	for(i=1;i<=wan_num;i++)
	{
		memset(&entry, '\0', sizeof(entry));
		*((char *)&entry) = (char)i;
		if(!apmib_get(MIB_WANIFACE_TBL, (void *)&entry))
		{
			printf("%s:%d get MIB_WANIFACE_TBL fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}
		
		ifGetName(entry.ifIndex,strWanIfName,sizeof(strWanIfName));
		
		switch(entry.AddressType)
		{
			case DHCP_DISABLED:
				if(getMultiWanInfo(strWanIfName,strWanIP,strWanMask,strWanDefGW,strWanHWAddr)<0)
				{
					printf("dev name is null!\n");
				}
				isWanPhy_Link=getWanLink(strWanIfName);
				if(isWanPhy_Link<0)
					strcpy(strWanCurrentInfo,"Fixed IP Disconnected");
				else
					strcpy(strWanCurrentInfo,"Fixed IP Connected");				
				break;
			case DHCP_CLIENT:
				if(getMultiWanInfo(strWanIfName,strWanIP,strWanMask,strWanDefGW,strWanHWAddr)<0)
				{
					printf("dev name is null!\n");
				}
				if(!strcmp(strWanIP,"0.0.0.0"))
					strcpy(strWanCurrentInfo,"Getting IP from DHCP server...");
				else
					strcpy(strWanCurrentInfo,"DHCP Connected");		
				
				break;
			case PPPOE:
				if(getMultiWanInfo(strWanIfName,strWanIP,strWanMask,strWanDefGW,strWanHWAddr)<0)
				{//get hw addr
					printf("dev name is null!\n");
				}
				
				if(getRealInterface(strWanIfName,strWanPppIfName)<0)
					strcpy(strWanCurrentInfo,"PPPOE Disconnected");
				else
				{
					memcpy(strWanIfName,strWanPppIfName,sizeof(char)*8);
					memset(strWanIP,0,sizeof(char)*16);
					memset(strWanMask,0,sizeof(char)*16);
					memset(strWanDefGW,0,sizeof(char)*16);
					if(getMultiWanInfo(strWanIfName,strWanIP,strWanMask,strWanDefGW,strWanHWAddrTmp)<0)
					{
						printf("dev name is null!\n");
					}
					if(!strcmp(strWanIP,"0.0.0.0"))
						strcpy(strWanCurrentInfo,"PPPOE connecting...");
					else
						strcpy(strWanCurrentInfo,"PPPOE Connected");
				}
							
				break;
			case BRIDGE:
				strcpy(strWanCurrentInfo,"WAN Bridge Mode");
				if(getMultiWanInfo(strWanIfName,strWanIP,strWanMask,strWanDefGW,strWanHWAddr)<0)
				{//get hw addr
					printf("dev name is null!\n");
				}
				break;
			default:
				strcpy(strWanCurrentInfo,"Error");

				break;
		}

		sprintf(buffer,"WanStatusListArray[%d] = new Array(%d,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\");\n",i-1,entry.enable,
		strWanCurrentInfo,strWanIP,strWanMask,strWanDefGW,strWanHWAddr);
		req_format_write(wp, buffer);
	}
}

int getWanStatsList(request *wp, int argc, char **argv)
{
	int wan_num=0,i=0,isWanPhy_Link=0;
	char buffer[WAN_ITEM_MAX_LENGTH];
	
	char strWanIfName[8]={0};
	char strWanPppIfName[8]={0};
	struct user_net_device_stats stats;
	WANIFACE_T entry;
	memset(buffer,'\0',sizeof(char)*WAN_ITEM_MAX_LENGTH);
	
	wan_num=WANIFACE_NUM;
	for(i=1;i<=wan_num;i++)
	{
		memset(&entry, '\0', sizeof(entry));
		memset(&stats,0,sizeof(stats));
		*((char *)&entry) = (char)i;
		if(!apmib_get(MIB_WANIFACE_TBL, (void *)&entry))
		{
			printf("%s:%d get MIB_WANIFACE_TBL fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}
		
		ifGetName(entry.ifIndex,strWanIfName,sizeof(strWanIfName));
		//printf("%s:%d strWanIfName=%s\n",__FUNCTION__,__LINE__,strWanIfName);
		switch(entry.AddressType)
		{
			case DHCP_DISABLED:
						
				break;
			case DHCP_CLIENT:
					
				
				break;
			case PPPOE:
				if(getRealInterface(strWanIfName,strWanPppIfName)<0)
				{
				}else
				{
					strcpy(strWanIfName,strWanPppIfName);
				}
				//printf("%s:%d strWanIfName=%s\n",__FUNCTION__,__LINE__,strWanIfName);
						
				break;
			case BRIDGE:
				
				
				break;
			default:
				

				break;
		}
		if ( getStats(strWanIfName, &stats) < 0)
		{
			stats.tx_packets = 0;
			stats.rx_packets = 0;
		}
		//printf("%s:%d tx_packets=%Lu,rx_packets=%Lu \n",__FUNCTION__,__LINE__,stats.tx_packets,stats.rx_packets);
		sprintf(buffer,"WanStatsListArray[%d] = new Array(%d,%Lu,%Lu);\n",i-1,entry.enable,
		stats.tx_packets,stats.rx_packets);
		req_format_write(wp, buffer);
	}
}


#endif//MULTI_WAN_SUPPORT
int getIndex(request *wp, int argc, char **argv)
{
	char *name, buffer[50];
	char WLAN_IF_ori[40], inner_buf[2048];
	int chan, val;
	REG_DOMAIN_T domain;
	WEP_T wep;
	DHCP_T dhcp;
	int pppoeNumber;
#ifdef HOME_GATEWAY
	OPMODE_T opmode=-1;
	char *iface=NULL;
#ifdef VPN_SUPPORT
	IPSECTUNNEL_T entry;
#endif
#endif
#ifdef UNIVERSAL_REPEATER
	int id;
#endif
	char tmpStr[20];
    	int wlan_idx_keep=0;
    	
    	memset(tmpStr ,'\0',20);
	wlan_idx_keep = wlan_idx;

#if 0
	{
		int index ;
		printf("\n");
		for(index=0 ; index < argc ;++index)
		{
			printf("%s ",argv[index]);
		}
		printf("\n");
	}
#endif
	//printf("get parameter=%s\n", argv[0]);
	name = argv[0];
	if (name == NULL) {
   		fprintf(stderr, "Insufficient args\n");
   		return -1;
   	}

	if(!strcmp(name, "ipv6")){
#ifdef CONFIG_IPV6
		sprintf(buffer, "1");		
#else
		sprintf(buffer, "0");
#endif	
		req_format_write(wp, buffer);
		return 0;
	}
	
	if(!strcmp(name, "route_setup_onoff")){
#ifdef ROUTE_SUPPORT
		sprintf(buffer, "1");		
#else
		sprintf(buffer, "0");
#endif	
		req_format_write(wp, buffer);
		return 0;
	}
	if(!strcmp(name,"showLanDevInfoSupport")){
#ifdef GET_LAN_DEV_INFO_SUPPORT
		sprintf(buffer, "1");		
#else
		sprintf(buffer, "0");		
#endif
		//printf("%s.%d. str(%s)\n",__FUNCTION__,__LINE__,buffer);
		req_format_write(wp, buffer);
		return 0;
	}
#ifdef CONFIG_RTL_AIRTIME
	if ( !strcmp(name, "isAirtimeDefined")) {
		sprintf(buffer,"%d", 1);
		req_format_write(wp, buffer);
		return 0;
	}
	if ( !strcmp(name, "airTimeEnabled")) {
		if ( !apmib_get( wlan_idx==0?MIB_AIRTIME_ENABLED:MIB_AIRTIME2_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "airTimeMode")) {
		if ( !apmib_get( wlan_idx==0?MIB_AIRTIME_MODE:MIB_AIRTIME2_MODE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "airTimeIfTime")) {
		int disabled[NUM_VWLAN_INTERFACE+1];
		int vWLanIdxBak;
		int i;

		if ( !apmib_get( wlan_idx==0?MIB_AIRTIME_IFTIME:MIB_AIRTIME2_IFTIME, (void *)tmpStr) )
			return -1;

		vWLanIdxBak = vwlan_idx;//backup
		for (i=0; i<=NUM_VWLAN_INTERFACE; i++) {
			vwlan_idx = i;
			if ( !apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&val) ) {
				vwlan_idx = vWLanIdxBak;//restore
				return -1;
			}
			disabled[i] = val;
		}
		vwlan_idx = vWLanIdxBak;//restore

		memset(inner_buf, '\0', sizeof(inner_buf));
		for (i=0; i<=NUM_VWLAN_INTERFACE; i++) {
			if (disabled[i] == 1)
				sprintf(buffer, "df.time%d.value=%d;\n", i, (unsigned char)0);
			else
				sprintf(buffer, "df.time%d.value=%d;\n", i, (unsigned char)tmpStr[i]);
			strcat(inner_buf, buffer);
		}

		sprintf(buffer, "if (value == 1) {");
		strcat(inner_buf, buffer);
		for (i=0; i<=NUM_VWLAN_INTERFACE; i++) {		
			if (disabled[i] == 1) {
				sprintf(buffer, "df.time%d.val=%d;\n", i, 0);
				strcat(inner_buf, buffer);
				sprintf(buffer, "disableTextField(df.time%d);\n", i);
				strcat(inner_buf, buffer);
			}
			else {
				sprintf(buffer, "enableTextField(df.time%d);\n", i);
				strcat(inner_buf, buffer);
			}
		}
		sprintf(buffer, "}");
		strcat(inner_buf, buffer);

		sprintf(buffer, "else {");
		strcat(inner_buf, buffer);
		for (val=0; val<=NUM_VWLAN_INTERFACE; val++) {
			sprintf(buffer, "disableTextField(df.time%d);\n", val);
			strcat(inner_buf, buffer);
		}
		sprintf(buffer, "}");
		strcat(inner_buf, buffer);
		req_format_write(wp, "%s", inner_buf);
		return 0;
	}
	else if ( !strcmp(name, "airTimeUnallcation")) {
		AIRTIME_T atmEntry;
		int i, entryNum, total = 0;
		apmib_get(wlan_idx==0?MIB_AIRTIME_TBL_NUM:MIB_AIRTIME2_TBL_NUM, (void *)&entryNum);
		for (i=1; i<=entryNum; i++) {
			memset(&atmEntry, 0x00, sizeof(atmEntry));
			*((char *)&atmEntry) = (char)i;
			if (apmib_get(wlan_idx==0?MIB_AIRTIME_TBL:MIB_AIRTIME2_TBL, (void *)&atmEntry)) {
				total += atmEntry.atm_time;
			}
		}
		sprintf(buffer, "%d", 100-total);
		req_format_write(wp, "%s", buffer);
		return 0;
	}
	else if ( !strcmp(name, "airTimeInterfaceHtml")) {
		char ssid[MAX_SSID_LEN];
		int vWLanIdxBak, disabled;
		#define OUTWP(fmt, args...) \
					do { \
					sprintf(buffer, fmt, ## args); \
					strcat(inner_buf, buffer); \
					}while(0);

		if ( !apmib_get( wlan_idx==0?MIB_AIRTIME_IFTIME:MIB_AIRTIME2_IFTIME, (void *)tmpStr) )
			return -1;

		memset(inner_buf, '\0', sizeof(inner_buf));
		vWLanIdxBak = vwlan_idx;
		for (val=0; val<=NUM_VWLAN_INTERFACE; val++) {
			vwlan_idx = val;
			if ( !apmib_get( MIB_WLAN_SSID, (void *)ssid) ) {
				vwlan_idx = vWLanIdxBak;
				return -1;
			}
			if (val==0) {
				sprintf(tmpStr, "<b>ROOT AP</b>");
			}
		#ifdef UNIVERSAL_REPEATER
			else if (val == NUM_VWLAN_INTERFACE) {
				sprintf(tmpStr, "RepeaterClient");
			}
		#endif
			else {
				sprintf(tmpStr, "AP%d", val);
			}
			if ( !apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&disabled) ) {
				vwlan_idx = vWLanIdxBak;
				return -1;
			}
			OUTWP("<tr style=\"display:%s\">\n", disabled==1?"none":"");
			OUTWP("<td width=\"70%%\">");
			OUTWP("<b><font size=2>&nbsp;&nbsp;%s:</font></b>", tmpStr);
			OUTWP("<font size=0>&nbsp;&nbsp;&nbsp;&nbsp;%s</font></td>\n", ssid);
			OUTWP("<td width=\"30%%\" align=\"middle\">");
			OUTWP("<font size=2><input type=\"text\" name=\"time%d\"", val);
			OUTWP(" size=\"5\" maxlength=\"3\">%%</font></td>\n");
			OUTWP("</tr>\n");
		}
      #undef OUTWP
		vwlan_idx = vWLanIdxBak;
      return req_format_write(wp, "%s", inner_buf);
	}
	else if ( !strcmp(name, "airTimeNum")) {
		if ( !apmib_get( wlan_idx==0?MIB_AIRTIME_TBL_NUM:MIB_AIRTIME2_TBL_NUM, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#else
	if ( !strcmp(name, "isAirtimeDefined")) {
		sprintf(buffer,"%d", 0);
		req_format_write(wp, buffer);
		return 0;
	}
#endif /* CONFIG_RTL_AIRTIME */

	if(!strcmp(name, "usb3g")){
#ifdef RTK_USB3G
		sprintf(buffer, "1");	
#else
		sprintf(buffer, "0");
#endif
		req_format_write(wp, buffer);
		return 0;
	}
	if(!strcmp(name, "lte4g_build")){
#ifdef CONFIG_4G_LTE_SUPPORT
		sprintf(buffer, "1");
#else
		sprintf(buffer, "0");
#endif
		req_format_write(wp, buffer);
		return 0;
	}
	if(!strcmp(name, "lte4g_enable")){
		if ( !apmib_get( MIB_LTE4G, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
   	if ( !strcmp(name, "dhcp")) {
 		if ( !apmib_get( MIB_DHCP, (void *)&dhcp) )
			return -1;
		sprintf(buffer, "%d", (int)dhcp);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "no-cache")) {
		memset(inner_buf, '\0', sizeof(inner_buf));
		sprintf(inner_buf, "%s\n%s\n%s\n",
				"<meta http-equiv=\"Pragma\" content=\"no-cache\">",
				"<meta HTTP-equiv=\"Cache-Control\" content=\"no-cache\">",
				"<meta HTTP-EQUIV=\"Expires\" CONTENT=\"Mon, 01 Jan 1990 00:00:01 GMT\">");
		req_format_write(wp, inner_buf);
		return 0;
	}
#if defined(CONFIG_USBDISK_UPDATE_IMAGE)
  	else if ( !strcmp(name, "usb_update_img_enabled")) {
		sprintf(buffer, "%d", 1);
		req_format_write(wp, buffer);
	 	return 0;
  	}
	
#else
  	else if ( !strcmp(name, "usb_update_img_enabled")) {
		sprintf(buffer, "%d", 0);
		req_format_write(wp, buffer);
	 	return 0;
  	}
	
#endif 
	else if (!strcmp(name, "isDisplayTR069")) {
#ifdef CONFIG_APP_TR069
		sprintf(buffer, "1");
#else
		sprintf(buffer, "0");
#endif
		req_format_write(wp, buffer);
		return 0;
	}
#ifdef CONFIG_IEEE80211V
    else if ( !strcmp(name, "80211v_enable")) {        
        if ( !apmib_get( MIB_WLAN_DOT11V_ENABLE, (void *)&val) )
            return -1;
        sprintf(buffer, "%d", val);
        req_format_write(wp, buffer);
        return 0;
    } 
#endif  
#ifdef DOT11K
    else if ( !strcmp(name, "dot11k_enable")) {        
        if ( !apmib_get( MIB_WLAN_DOT11K_ENABLE, (void *)&val) )
            return -1;
        sprintf(buffer, "%d", val);
        req_format_write(wp, buffer);
        return 0;
    } 
#endif    
#ifdef RTK_SMART_ROAMING
    else if ( !strcmp(name, "smart_roaming_enable")) {        
        if ( !apmib_get( MIB_CAPWAP_MODE, (void *)&val) )
            return -1;
        sprintf(buffer, "%d", val);
        req_format_write(wp, buffer);
        return 0;
    }
#endif
	else if (!strcmp(name, "isDisplayOpenvpn")) {
#ifdef CONFIG_APP_OPENVPN
		sprintf(buffer, "1");
#else
		sprintf(buffer, "0");
#endif
		req_format_write(wp, buffer);
		return 0;
	}
    else if ( !strcmp(name, "is_80211v_support")) {
#ifdef CONFIG_IEEE80211V
        sprintf(buffer,"%d", 1);
#else
        sprintf(buffer,"%d", 0);
#endif
        req_format_write(wp, buffer);
    }    
    else if ( !strcmp(name, "is_80211k_support")) {
#ifdef DOT11K
        sprintf(buffer,"%d", 1);
#else
        sprintf(buffer,"%d", 0);
#endif
        req_format_write(wp, buffer);
    }    
else if(!strcmp(name, "is_80211r_support"))
	{
#if defined(FAST_BSS_TRANSITION)
		sprintf(buffer, "%s", "1" );
#else
		sprintf(buffer, "%s", "0");
#endif
		return req_format_write(wp, buffer);
	}
#ifdef FAST_BSS_TRANSITION
	else if(!strcmp(name, "11r_ftkh_num")){
		req_format_write(wp, "%d", MAX_VWLAN_FTKH_NUM);
		return 0;
	}
	else if(!strcmp(name, "ft_enable"))
	{
		if ( !apmib_get( MIB_WLAN_FT_ENABLE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		return req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "_ft_mdid"))
		{
			if ( !apmib_get( MIB_WLAN_FT_MDID, (void *)&buffer) )
				return -1;
			if(buffer[0])
				sprintf(inner_buf, "%s", buffer);
			else
				sprintf(inner_buf, "%s", "0");
			return req_format_write(wp, inner_buf);
		}
	else if(!strcmp(name, "_ft_over_ds"))
		{
			if ( !apmib_get( MIB_WLAN_FT_OVER_DS, (void *)&val) )
				return -1;
			sprintf(buffer, "%d", val);
			return req_format_write(wp, buffer);
		}
	else if(!strcmp(name, "_ft_res_request"))
		{
			if ( !apmib_get( MIB_WLAN_FT_RES_REQUEST, (void *)&val) )
				return -1;
			sprintf(buffer, "%d", val);
			return req_format_write(wp, buffer);
		}
	else if(!strcmp(name, "_ft_r0key_timeout"))
		{
			if ( !apmib_get( MIB_WLAN_FT_R0KEY_TO, (void *)&val) )
				return -1;
			sprintf(buffer, "%d", val);
			return req_format_write(wp, buffer);
		}
	else if(!strcmp(name, "_ft_reasoc_timeout"))
		{
			if ( !apmib_get( MIB_WLAN_FT_REASOC_TO, (void *)&val) )
				return -1;
			sprintf(buffer, "%d", val);
			return req_format_write(wp, buffer);
		}
	else if(!strcmp(name, "_ft_r0kh_id"))
		{
			if ( !apmib_get( MIB_WLAN_FT_R0KH_ID, (void *)&buffer) )
				return -1;
			if(buffer[0])
				sprintf(inner_buf, "%s", buffer);
			else
				sprintf(inner_buf, "%s", "0");
			return req_format_write(wp, inner_buf);
		}
	else if(!strcmp(name, "_ft_push"))
		{
			if ( !apmib_get( MIB_WLAN_FT_PUSH, (void *)&val) )
				return -1;
			sprintf(buffer, "%d", val);
			return req_format_write(wp, buffer);
		}
	else if(!strcmp(name, "_ft_kh_num"))
			{
				if ( !apmib_get( MIB_WLAN_FTKH_NUM, (void *)&val) )
					return -1;
				sprintf(buffer, "%d", val);
				return req_format_write(wp, buffer);
			}
	else if(!strcmp(name, "selectedId"))
		{
			sprintf(buffer, "%d", vwlan_idx);
			return req_format_write(wp, buffer);
		}
#endif
  	else if (!strcmp(name, "is_smart_roaming_support")) {
#ifdef RTK_SMART_ROAMING
			sprintf(buffer,"%d", 1);
#else
			sprintf(buffer,"%d", 0);
#endif
			req_format_write(wp, buffer);
	}
  	else if ( !strcmp(name, "dhcp-current")) {
   		if ( !apmib_get( MIB_DHCP, (void *)&dhcp) )
			return -1;
		if ( dhcp == DHCP_CLIENT && !isDhcpClientExist(BRIDGE_IF))
			dhcp = DHCP_DISABLED;
		sprintf(buffer, "%d", (int)dhcp);
		req_format_write(wp, buffer);
		return 0;
	}
 	else if ( !strcmp(name, "stp")) {
   		if ( !apmib_get( MIB_STP_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wac_interface")) {
   		if ( !apmib_get( MIB_MFI_WAC_BAND, (void *)&val) )
			return -1;
		if(val==5){
			val = 1;
			sprintf(buffer, "%d", val);
		}else {
			val = 0;
			sprintf(buffer, "%d", val);
		}
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "mfi_wac_interface_start")) {
#if defined(CONFIG_APP_APPLE_MFI_WAC) && defined(CONFIG_RTL_8197F)
		req_format_write(wp, "");
#else
		req_format_write(wp, "<!--");
#endif
	}
	else if ( !strcmp(name, "mfi_wac_interface_end")) {
#if defined(CONFIG_APP_APPLE_MFI_WAC) && defined(CONFIG_RTL_8197F)
		req_format_write(wp, "");
#else
        req_format_write(wp, "-->");
#endif
	}else if ( !strcmp(name, "sch_enabled")) {
   		if ( !apmib_get( MIB_WLAN_SCHEDULE_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "isPureAP")) {
#if defined(CONFIG_RTL_AP_PACKAGE)
                sprintf(buffer, "%d", 1);
#else
                sprintf(buffer, "%d", 0);
#endif
                req_format_write(wp, buffer);
                return 0;
        }
#if defined(CONFIG_RTL_8198_AP_ROOT) || defined(HOME_GATEWAY) || defined(CONFIG_RTL_8197D_AP)
	else if ( !strcmp(name, "wanDNS")) {
		DNS_TYPE_T dns;
		apmib_get( MIB_DNS_MODE, (void *)&dns);
		sprintf(buffer, "%d", (int)dns);
		req_format_write(wp, buffer);
		return 0;
	}
 	else if ( !strcmp(name, "ntpEnabled")) {
   		if ( !apmib_get( MIB_NTP_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "DaylightSave")) {
   		if ( !apmib_get( MIB_DAYLIGHT_SAVE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "ntpServerId")) {
   		if ( !apmib_get( MIB_NTP_SERVER_ID, (void *)&val) )
			return -1;
		val = val ? 1 : 0;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wanDhcp")) {
#if defined(CONFIG_RTL_8198_AP_ROOT) || defined(CONFIG_RTL_8197D_AP)
		sprintf(buffer, "%d", 1);
#else
		if ( !apmib_get( MIB_WAN_DHCP, (void *)&dhcp) )
			return -1;
		sprintf(buffer, "%d", (int)dhcp);
#endif
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name,"multiWanSupport"))
	{
		#ifdef MULTI_WAN_SUPPORT
			sprintf(buffer, "%d",1);
			req_format_write(wp, buffer);
		#else
			sprintf(buffer, "%d",0);	
			req_format_write(wp, buffer);
		#endif
		return 0;
	}
	else if(!strcmp(name,"OneWanSptUnderMtl"))
	{
		#if defined(MULTI_WAN_SUPPORT) && defined(SINGLE_WAN_SUPPORT)
			sprintf(buffer, "%d",1);
			req_format_write(wp, buffer);
		#else
			sprintf(buffer, "%d",0);	
			req_format_write(wp, buffer);
		#endif
		return 0;
	}
#if defined(MULTI_WAN_SUPPORT)
	else if(!strcmp(name,"WanEnabled"))
	{		
		WANIFACE_T WanIfaceEntry;
		int wan_index = 0 ;

		if(!argv[1])
			return 0;
		
		if(!strcmp(argv[1],"first"))
			wan_index = 1;
		if(!strcmp(argv[1],"second"))
			wan_index = 2;
		if(!strcmp(argv[1],"third"))
			wan_index = 3;
		if(!strcmp(argv[1],"forth"))
			wan_index = 4;
		
		if(!getWanIfaceEntry(wan_index,&WanIfaceEntry))
		{
			printf("%s.%d.get waniface entry fail\n",__FUNCTION__,__LINE__);
			return 0;
		}		
		//printf("enable(%d)\n",WanIfaceEntry.enable);
		if(WanIfaceEntry.AddressType != BRIDGE)
			sprintf(buffer, "%d",WanIfaceEntry.enable);		
		else
			sprintf(buffer, "%d",0);		
		req_format_write(wp, buffer);
		return 0;
	}	
#endif
	else if (!strcmp(name, "multiPppoe")){
		#ifdef  MULTI_PPPOE
			sprintf(buffer, "%d",1);
			req_format_write(wp, buffer);
		#else
			sprintf(buffer, "%d",0);	
			req_format_write(wp, buffer);
		#endif
		return 0;
	}
	else if (!strcmp(name, "pppoeNo"))
	{
		if ( !apmib_get( MIB_PPP_CONNECT_COUNT, (void *)&pppoeNumber) )
			goto FMGET_FAIL;	
		sprintf(buffer, "%d", pppoeNumber);
		
		req_format_write(wp, buffer);
		return 0;
	
	}		
	else if (!strcmp(name, "subnet1"))
	{
		if ( !apmib_get( MIB_SUBNET1_COUNT, (void *)&val) )
			return -1;	
		sprintf(buffer, "%d", val);
		
		req_format_write(wp, buffer);
		return 0;	
	}	
	else if (!strcmp(name, "subnet2"))
	{
		if ( !apmib_get( MIB_SUBNET2_COUNT, (void *)&val) )
			return -1;	
		sprintf(buffer, "%d", val);
		
		req_format_write(wp, buffer);
		return 0;		
	}
	else if (!strcmp(name, "subnet3"))
	{
		if ( !apmib_get( MIB_SUBNET3_COUNT, (void *)&val) )
			return -1;	
		sprintf(buffer, "%d", val);
		
		req_format_write(wp, buffer);
		return 0;	

	}	
	else if (!strcmp(name, "subnet4"))
	{
		if ( !apmib_get( MIB_SUBNET4_COUNT, (void *)&val) )
			return -1;	
		sprintf(buffer, "%d", val);
		
		req_format_write(wp, buffer);
		return 0;	

	}	
	else if ( !strcmp(name, "pppConnectType2")){
		PPP_CONNECT_TYPE_T type;
		if ( !apmib_get( MIB_PPP_CONNECT_TYPE2, (void *)&type) )
			return -1;
		sprintf(buffer, "%d", (int)type);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "pppConnectType3")) {
		PPP_CONNECT_TYPE_T type;
		if ( !apmib_get( MIB_PPP_CONNECT_TYPE3, (void *)&type) )
			return -1;
		sprintf(buffer, "%d", (int)type);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "pppConnectType4")) {
		PPP_CONNECT_TYPE_T type;
		if ( !apmib_get( MIB_PPP_CONNECT_TYPE4, (void *)&type) )
			return -1;
		sprintf(buffer, "%d", (int)type);
		req_format_write(wp, buffer);
		return 0;
	}	
#ifdef HOME_GATEWAY
	else if ( !strcmp(name, "pppConnectStatus2")){
#ifdef MULTI_PPPOE		
		PPPoE_Number = 2;
#endif
		sprintf(buffer, "%d", isConnectPPP());
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "pppConnectStatus3")) {
#ifdef MULTI_PPPOE		
		PPPoE_Number = 3;
#endif
		sprintf(buffer, "%d", isConnectPPP());
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "pppConnectStatus4")) {
#ifdef MULTI_PPPOE
		PPPoE_Number = 4;
#endif
		sprintf(buffer, "%d", isConnectPPP());
		req_format_write(wp, buffer);
		return 0;
	}
#endif
	else if( !strcmp(name, "enableGetServIpByDomainName"))
	{
#ifdef CONFIG_GET_SERVER_IP_BY_DOMAIN
		sprintf(buffer, "%d", 1);
#else
		sprintf(buffer, "%d", 0);
#endif
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name, "pptpGetServIpByDomainName"))
	{
#ifdef CONFIG_GET_SERVER_IP_BY_DOMAIN
		if(!apmib_get(MIB_PPTP_GET_SERV_BY_DOMAIN,(void*)&val))
			return -1;
#else
		val =0;
#endif
		sprintf(buffer,"%d",val);
		req_format_write(wp, buffer);
	}
	else if(!strcmp(name, "l2tpGetServIpByDomainName"))
	{
#ifdef CONFIG_GET_SERVER_IP_BY_DOMAIN
		if(!apmib_get(MIB_L2TP_GET_SERV_BY_DOMAIN,(void*)&val))
			return -1;
#else
		val=0;
#endif
		sprintf(buffer,"%d",val);
		req_format_write(wp, buffer);
	} 

	else if ( !strcmp(name, "wanDhcp-current")) {
#if defined(CONFIG_RTL_8198_AP_ROOT) || defined(CONFIG_RTL_8197D_AP)
		memset(buffer,0x00,sizeof(buffer));
		apmib_get( MIB_WAN_DHCP, (void *)&dhcp);
		sprintf(buffer, "%d", dhcp);
#else
		int wispWanId=0;
		if ( !apmib_get( MIB_WAN_DHCP, (void *)&dhcp) )
			return -1;
  		if ( !apmib_get( MIB_OP_MODE, (void *)&opmode) )
			return -1;
		if( !apmib_get(MIB_WISP_WAN_ID, (void *)&wispWanId))
			return -1;
		if(opmode == WISP_MODE) {
			if(0 == wispWanId)
				iface = "wlan0";
			else if(1 == wispWanId)
				iface = "wlan1";
		}
		else
			iface = WAN_IF;
		if ( dhcp == DHCP_CLIENT && !isDhcpClientExist(iface))
			dhcp = DHCP_DISABLED;
		sprintf(buffer, "%d", (int)dhcp);
#endif
		req_format_write(wp, buffer);
		return 0;
	}
#if defined(HOME_GATEWAY)
#ifdef ROUTE_SUPPORT
	else if ( !strcmp(name, "nat_enabled")) {
		if ( !apmib_get( MIB_NAT_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#endif
	else if ( !strcmp(name, "pppConnectType")) {
		PPP_CONNECT_TYPE_T type;
		if ( !apmib_get( MIB_PPP_CONNECT_TYPE, (void *)&type) )
			return -1;
		sprintf(buffer, "%d", (int)type);
		req_format_write(wp, buffer);
		return 0;
	}
#if defined(CONFIG_DYNAMIC_WAN_IP)
	else if ( !strcmp(name, ("pptp_wan_ip_mode"))) {
		WAN_IP_TYPE_T wanIpType;
		if ( !apmib_get( MIB_PPTP_WAN_IP_DYNAMIC,  (void *)&wanIpType) )
			return -1;
   		sprintf(buffer, "%d", (int)wanIpType );
		req_format_write(wp, buffer);
		return 0;
	}
#endif
	else if ( !strcmp(name, ("pptpConnectType")) ) {
		PPP_CONNECT_TYPE_T type;
		if ( !apmib_get( MIB_PPTP_CONNECTION_TYPE, (void *)&type) )
			return -1;
		sprintf(buffer, "%d", (int)type);
		req_format_write(wp, buffer);
		return 0;
	}
#if defined(CONFIG_DYNAMIC_WAN_IP)
	else if ( !strcmp(name, ("l2tp_wan_ip_mode"))) {
		WAN_IP_TYPE_T wanIpType;
		if ( !apmib_get( MIB_L2TP_WAN_IP_DYNAMIC,  (void *)&wanIpType) )
			return -1;
   		sprintf(buffer, "%d", (int)wanIpType );
		req_format_write(wp, buffer);
		return 0;
	}
#endif
	else if ( !strcmp(name, ("l2tpConnectType")) ) {
		PPP_CONNECT_TYPE_T type;
		if ( !apmib_get( MIB_L2TP_CONNECTION_TYPE, (void *)&type) )
			return -1;
		sprintf(buffer, "%d", (int)type);
		req_format_write(wp, buffer);
		return 0;
	}
#ifdef RTK_USB3G
	else if ( !strcmp(name, "USB3GConnectType")) {
        PPP_CONNECT_TYPE_T type;
        buffer[0]='\0';
		if ( !apmib_get( MIB_USB3G_CONN_TYPE, (void *)buffer) )
			return -1;
		req_format_write(wp, buffer);
		return 0;
	}
#else
	else if ( !strcmp(name, "USB3GConnectType")) {
		buffer[0]='\0';
		req_format_write(wp, buffer);
		return 0;
	}
#endif /* #ifdef RTK_USB3G */
    else if ( !strcmp(name, "pppConnectStatus")) {
#ifdef MULTI_PPPOE		
		PPPoE_Number = 1;
#endif
		sprintf(buffer, "%d", isConnectPPP());
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "portFwNum")) {
		if ( !apmib_get( MIB_PORTFW_TBL_NUM, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "ipFilterNum")) {
		if ( !apmib_get( MIB_IPFILTER_TBL_NUM, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "portFilterNum")) {
		if ( !apmib_get( MIB_PORTFILTER_TBL_NUM, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "macFilterNum")) {
		if ( !apmib_get( MIB_MACFILTER_TBL_NUM, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "urlFilterNum")) {
		if ( !apmib_get( MIB_URLFILTER_TBL_NUM, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "triggerPortNum")) {
		if ( !apmib_get( MIB_TRIGGERPORT_TBL_NUM, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}

#if defined(GW_QOS_ENGINE) || defined(QOS_BY_BANDWIDTH)
	else if ( !strcmp(name, "qosEnabled")) {
		if ( !apmib_get( MIB_QOS_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "qosAutoUplinkSpeed")) {
		if ( !apmib_get( MIB_QOS_AUTO_UPLINK_SPEED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "qosRuleNum")) {
		if ( !apmib_get( MIB_QOS_RULE_TBL_NUM, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "qosAutoDownlinkSpeed")) {
		if ( !apmib_get( MIB_QOS_AUTO_DOWNLINK_SPEED, (void *)&val) )
			return -1;

		if(val == 0)
			sprintf(buffer, "%s", "");
		else
			sprintf(buffer, "%s", "checked");

		return req_format_write(wp, buffer);
	}
#endif

#ifdef ROUTE_SUPPORT
	else if ( !strcmp(name, "staticRouteNum")) {
		if ( !apmib_get( MIB_STATICROUTE_TBL_NUM, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#endif
	else if ( !strcmp(name, "portFwEnabled")) {
		if ( !apmib_get( MIB_PORTFW_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "ipFilterEnabled")) {
		if ( !apmib_get( MIB_IPFILTER_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "portFilterEnabled")) {
		if ( !apmib_get( MIB_PORTFILTER_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "macFilterEnabled")) {
		if ( !apmib_get( MIB_MACFILTER_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "triggerPortEnabled")) {
		if ( !apmib_get( MIB_TRIGGERPORT_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#ifdef ROUTE_SUPPORT
	else if ( !strcmp(name, "staticRouteEnabled")) {
		if ( !apmib_get( MIB_STATICROUTE_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#endif
	else if ( !strcmp(name, "dmzEnabled")) {
		if ( !apmib_get( MIB_DMZ_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#ifdef _ALPHA_DUAL_WAN_SUPPORT_
	else if ( !strcmp(name, "pppoeWithDhcpEnabled")) {
		if ( !apmib_get( MIB_PPPOE_DHCP_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val);
		req_format_write(wp, buffer);
		return 0;
	}
#endif
	else if ( !strcmp(name, "upnpEnabled")) {
		if ( !apmib_get( MIB_UPNP_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "igmpproxyDisabled")) {
		if ( !apmib_get( MIB_IGMP_PROXY_DISABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val);
		req_format_write(wp, buffer);
		return 0;
	}
#ifdef ROUTE_SUPPORT
	else if ( !strcmp(name, "ripEnabled")) {
                if ( !apmib_get( MIB_RIP_ENABLED, (void *)&val) )
                        return -1;
                sprintf(buffer, "%d", (int)val);
                req_format_write(wp, buffer);
                return 0;
        }
	else if ( !strcmp(name, "ripLanTx")) {
                if ( !apmib_get( MIB_RIP_LAN_TX, (void *)&val) )
                        return -1;
                sprintf(buffer, "%d", (int)val);
                req_format_write(wp, buffer);
                return 0;
        }
	else if ( !strcmp(name, "ripLanRx")) {
                if ( !apmib_get( MIB_RIP_LAN_RX, (void *)&val) )
                        return -1;
                sprintf(buffer, "%d", (int)val);
                req_format_write(wp, buffer);
                return 0;
        }
	else if ( !strcmp(name, "rip6Support")) {
#ifdef RIP6_SUPPORT
                sprintf(buffer, "%d", 1);
#else
                sprintf(buffer, "%d", 0);
#endif
                req_format_write(wp, buffer);
                return 0;
        }
#ifdef RIP6_SUPPORT
	else if ( !strcmp(name, "rip6Enabled")) {
                if ( !apmib_get( MIB_RIP6_ENABLED, (void *)&val) )
                        return -1;
                sprintf(buffer, "%d", (int)val);
				//printf("rip6Enabled: %d\n", val);
                req_format_write(wp, buffer);
                return 0;
        }
#endif
#if 0 //unused
	else if ( !strcmp(name, "ripWanTx")) {
                if ( !apmib_get( MIB_RIP_WAN_TX, (void *)&val) )
                        return -1;
                sprintf(buffer, "%d", (int)val);
                req_format_write(wp, buffer);
                return 0;
        }
	else if ( !strcmp(name, "ripWanRx")) {
                if ( !apmib_get( MIB_RIP_WAN_RX, (void *)&val) )
                        return -1;
                sprintf(buffer, "%d", (int)val);
                req_format_write(wp, buffer);
                return 0;
        }
#endif
#endif //ROUTE
#endif	//HOME_GATEWAY
#endif	//CONFIG_RTL_8198_AP_ROOT && VLAN_CONFIG_SUPPORT
#ifdef HOME_GATEWAY
#ifdef VPN_SUPPORT
		else if ( !strcmp(name, "ipsecTunnelNum")) {
                if ( !apmib_get( MIB_IPSECTUNNEL_TBL_NUM, (void *)&val) )
                        return -1;
                sprintf(buffer, "%d", val);
                req_format_write(wp, buffer);
                return 0;
        }
        else if ( !strcmp(name, "ipsecVpnEnabled")) {
                if ( !apmib_get( MIB_IPSECTUNNEL_ENABLED, (void *)&val) )
                        return -1;
                sprintf(buffer, "%d", val);
                req_format_write(wp, buffer);
                return 0;
        }
	else if ( !strcmp(name, "ipsecNattEnabled")) {
                if ( !apmib_get( MIB_IPSEC_NATT_ENABLED, (void *)&val) )
                        return -1;
                sprintf(buffer, "%d", val);
                req_format_write(wp, buffer);
                return 0;
        }
	else if ( !strcmp(name, "tunnelEnabled")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", 1); // default
		else
	        	sprintf(buffer, "%d", entry.enable );
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "ipsecLocalType")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", SUBNET_ADDR); // subnet Address default
		else
	        	sprintf(buffer, "%d", entry.lcType);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "ipsecRemoteType")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", SUBNET_ADDR); // subnet Address default
		else
	        	sprintf(buffer, "%d", entry.rtType);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "ipsecKeyMode")) {
#if 0		//sc_yang
		int val ;
               if ((val= getVpnKeyMode()) != -1){
                       sprintf(buffer, "%d", (int) val ) ;
               } else{
#endif
		if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", IKE_MODE); // IKE mode
		else
			sprintf(buffer, "%d", entry.keyMode);

		req_format_write(wp, buffer);
		return 0;
	}
/*
	else if ( !strcmp(name, "ipsecEspAh")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", ESP_PROTO); // ESP
		else
	        	sprintf(buffer, "%d", entry.espAh);
		req_format_write(wp, buffer);
		return 0;
	}
*/
	else if ( !strcmp(name, "ipsecEspEncr")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", TRI_DES_ALGO); // 3DES
		else
	        	sprintf(buffer, "%d", entry.espEncr);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "ipsecEspAuth")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", MD5_ALGO); // MD5
		else
	        	sprintf(buffer, "%d", entry.espAuth);
		req_format_write(wp, buffer);
		return 0;
	}
	/*else if ( !strcmp(name, "ipsecAhAuth")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", MD5_ALGO); // MD5
		else
	        	sprintf(buffer, "%d", entry.ahAuth);
		req_format_write(wp, buffer);
		return 0;
	}*/
	else if ( !strcmp(name, "vpnConnectionType")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", RESPONDER); // responder
		else
	        	sprintf(buffer, "%d", entry.conType);
		req_format_write(wp, buffer);
		return 0;
	}
	else if( !strcmp(name, "ikeConnectStatus")){
                if ( getIpsecInfo(&entry) < 0){
			sprintf(buffer, "%d", 0);
		}
		else{
			if ( getConnStat(entry.connName) < 0)
				sprintf(buffer, "%d", 0);
			else
				sprintf(buffer, "%d",1);
		}
		req_format_write(wp, buffer);
		return 0;
	}
	else if( !strcmp(name, "ipsecLocalIdType")){
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", 0);
		else
			sprintf(buffer, "%d",entry.lcIdType);
		req_format_write(wp, buffer);
		return 0;
	}
	else if( !strcmp(name, "ipsecRemoteIdType")){
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", 0);
		else
			sprintf(buffer, "%d",entry.rtIdType);

		req_format_write(wp, buffer);
		return 0;
	}
	else if( !strcmp(name, "ipsecAuthType")) {
                if ( getIpsecInfo(&entry) < 0)
			sprintf(buffer, "%d", 0);
		else
			sprintf(buffer, "%d", entry.authType);

		req_format_write(wp, buffer);
		return 0;
	}
#endif
#endif
	else if ( !strcmp(name, "channel")) {
		if ( !apmib_get( MIB_WLAN_CHANNEL, (void *)&chan) )
			return -1;
		sprintf(buffer, "%d", chan);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "regDomain")) {
		if ( !apmib_get( MIB_HW_REG_DOMAIN, (void *)&domain) )
			return -1;
		sprintf(buffer, "%d", (int)domain);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "urlFilterUserModeSupport")) {
#ifdef URL_FILTER_USER_MODE_SUPPORT
			val = 1;
#else
			val = 0;
#endif
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
		}
	else if ( !strcmp(name, "usrSpecificUrlCommand_start")) {
#ifndef URL_FILTER_USER_MODE_SUPPORT
		req_format_write(wp, "<!--");
#else
		req_format_write(wp, "");
#endif
		}
	else if( !strcmp(name,"usrSpecificUrlCommand_end")){
#ifndef URL_FILTER_USER_MODE_SUPPORT
		req_format_write(wp, "-->");
#else
		req_format_write(wp, "");
#endif
		}
	else if ( !strcmp(name, "wep")) {
		if ( !apmib_get( MIB_WLAN_WEP, (void *)&wep) )
			return -1;
		sprintf(buffer, "%d", (int)wep);
		req_format_write(wp, buffer);
   	    	return 0;
	}
	else if ( !strcmp(name, "defaultKeyId")) {
		if ( !apmib_get( MIB_WLAN_WEP_DEFAULT_KEY, (void *)&val) )
			return -1;
		val++;
		sprintf(buffer, "%d", (int)val) ;
		 req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "keyType")) {
		if ( !apmib_get( MIB_WLAN_WEP_KEY_TYPE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		 req_format_write(wp, buffer);
		return 0;
	}
  	else if ( !strcmp(name, "authType")) {
		if ( !apmib_get( MIB_WLAN_AUTH_TYPE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "operRate")) {
		if ( !apmib_get( MIB_WLAN_SUPPORTED_RATES, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "basicRate")) {
		if ( !apmib_get( MIB_WLAN_BASIC_RATES, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "preamble")) {
		if ( !apmib_get( MIB_WLAN_PREAMBLE_TYPE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "hiddenSSID")) {
		if ( !apmib_get( MIB_WLAN_HIDDEN_SSID, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wmFilterNum")) {
		if ( !apmib_get( MIB_WLAN_MACAC_NUM, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wlanDisabled")) {
		if ( !apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wlanAcNum")) {
		if ( !apmib_get( MIB_WLAN_MACAC_NUM, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wlanAcEnabled")) {
		if ( !apmib_get( MIB_WLAN_MACAC_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}

#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_) // below code copy above ACL code
	else if ( !strcmp(name, "meshAclNum")) {
		if ( !apmib_get( MIB_WLAN_MESH_ACL_NUM, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "meshAclEnabled")) {
		if ( !apmib_get( MIB_WLAN_MESH_ACL_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#endif

	else if ( !strcmp(name, "rateAdaptiveEnabled")) {
		if ( !apmib_get( MIB_WLAN_RATE_ADAPTIVE_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wlanMode")) {
		if ( !apmib_get( MIB_WLAN_MODE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wlanMode_rpt")) {
#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)

		sprintf(tmpStr,"wlan%d-vxd",wlan_idx);
		SetWlan_idx(tmpStr);
		apmib_get( MIB_WLAN_MODE, (void *)&val);
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		wlan_idx = wlan_idx_keep;
		sprintf(tmpStr,"wlan%d",wlan_idx);
		SetWlan_idx(tmpStr);
#else
		sprintf(buffer, "%d", 0);
		req_format_write(wp, buffer);
#endif
		return 0;
	}
	else if ( !strcmp(name, "networkType")) {
		if ( !apmib_get( MIB_WLAN_NETWORK_TYPE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "networkType_rpt")) {
#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)
//		char tmpStr[10];
		sprintf(tmpStr,"wlan%d-vxd",wlan_idx);
		SetWlan_idx(tmpStr);
		apmib_get( MIB_WLAN_NETWORK_TYPE, (void *)&val);
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		wlan_idx = wlan_idx_keep;
		sprintf(tmpStr,"wlan%d",wlan_idx);
		SetWlan_idx(tmpStr);
#else
		sprintf(buffer, "%d", 0);
		req_format_write(wp, buffer);
#endif
		return 0;
	}
	else if ( !strcmp(name, "lockdown_stat")) {	/* WPS2DOTX for brute force attack mitigation;unlock*/

		#define WSCD_LOCK_STAT		("/tmp/wscd_lock_stat")

		struct stat lockdown_status;
		if (stat(WSCD_LOCK_STAT, &lockdown_status) == 0) {
			//printf("[%s %d] %s exist\n",__FUNCTION__,__LINE__,WSCD_LOCK_STAT);
			val=1;
		}else{
			val=0;
		}
		
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);		
		return 0;
	}/* WPS2DOTX support*/
	else if ( !strcmp(name, "iappDisabled")) {
#ifndef CONFIG_IAPP_SUPPORT
		val = -2;
#else
		if ( !apmib_get( MIB_WLAN_IAPP_DISABLED, (void *)&val) )
			return -1;
#endif
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "protectionDisabled")) {
		if ( !apmib_get( MIB_WLAN_PROTECTION_DISABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "encrypt")) {
		if ( !apmib_get( MIB_WLAN_ENCRYPT, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "encrypt_rpt")) {
#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)
//		char tmpStr[10];
		sprintf(tmpStr,"wlan%d-vxd",wlan_idx);
		SetWlan_idx(tmpStr);
		apmib_get( MIB_WLAN_ENCRYPT, (void *)&val);
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		wlan_idx = wlan_idx_keep;
		sprintf(tmpStr,"wlan%d",wlan_idx);
		SetWlan_idx(tmpStr);
#else
		sprintf(buffer, "%d", 0);
		req_format_write(wp, buffer);
#endif
		return 0;
	}
	else if ( !strcmp(name, "enable1X")) {
		if ( !apmib_get( MIB_WLAN_ENABLE_1X, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "enable1x_rpt")) {
#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)
//		char tmpStr[10];
		sprintf(tmpStr,"wlan%d-vxd",wlan_idx);
		SetWlan_idx(tmpStr);
		apmib_get( MIB_WLAN_ENABLE_1X, (void *)&val);
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		wlan_idx = wlan_idx_keep;
		sprintf(tmpStr,"wlan%d",wlan_idx);
		SetWlan_idx(tmpStr);
#else
		sprintf(buffer, "%d", 0);
		req_format_write(wp, buffer);
#endif
		return 0;
	}
	else if ( !strcmp(name, "enableSuppNonWpa")) {
		if ( !apmib_get( MIB_WLAN_ENABLE_SUPP_NONWPA, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "suppNonWpa")) {
		if ( !apmib_get( MIB_WLAN_SUPP_NONWPA, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wpaAuth")) {
		if ( !apmib_get( MIB_WLAN_WPA_AUTH, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wpa_auth_rpt")) {
#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)
//		char tmpStr[10];
		sprintf(tmpStr,"wlan%d-vxd",wlan_idx);
		SetWlan_idx(tmpStr);
		apmib_get( MIB_WLAN_WPA_AUTH, (void *)&val);
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		wlan_idx = wlan_idx_keep;
		sprintf(tmpStr,"wlan%d",wlan_idx);
		SetWlan_idx(tmpStr);
#else
		sprintf(buffer, "%d", 0);
		req_format_write(wp, buffer);
#endif
		return 0;
	}
	else if ( !strcmp(name, "clientModeSupport1X")) {
#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
		sprintf(buffer, "%d", 1);
#else
		sprintf(buffer, "%d", 0);
#endif
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "eapType")) {
#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
		if ( !apmib_get( MIB_WLAN_EAP_TYPE, (void *)&val) )
			return -1;
#else
		val=0;
#endif
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "eapInsideType")) {
#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
		if ( !apmib_get( MIB_WLAN_EAP_INSIDE_TYPE, (void *)&val) )
			return -1;
#else
		val=0;
#endif
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}

	else if ( !strcmp(name, "wpaCipher")) {
		if ( !apmib_get( MIB_WLAN_WPA_CIPHER_SUITE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#ifdef CONFIG_IEEE80211W
	else if ( !strcmp(name, "wpa11w")) {
		if ( !apmib_get( MIB_WLAN_IEEE80211W, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wpa2EnableSHA256")) {
		if ( !apmib_get( MIB_WLAN_SHA256_ENABLE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#endif
	else if ( !strcmp(name, "wpa2Cipher")) {
		if ( !apmib_get( MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "pskFormat")) {
		if ( !apmib_get( MIB_WLAN_PSK_FORMAT, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "accountRsEnabled")) {
		if ( !apmib_get( MIB_WLAN_ACCOUNT_RS_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "accountRsUpdateEnabled")) {
		if ( !apmib_get( MIB_WLAN_ACCOUNT_RS_UPDATE_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "enableMacAuth")) {
		if ( !apmib_get( MIB_WLAN_MAC_AUTH_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "rsRetry")) {
		if ( !apmib_get( MIB_WLAN_RS_MAXRETRY, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "accountRsRetry")) {
		if ( !apmib_get( MIB_WLAN_ACCOUNT_RS_MAXRETRY, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wlanWdsEnabled")) {
		if ( !apmib_get( MIB_WLAN_WDS_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wlanWdsNum")) {
		if ( !apmib_get( MIB_WLAN_WDS_NUM, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wdsEncrypt")) {
		if ( !apmib_get( MIB_WLAN_WDS_ENCRYPT, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wdsWepFormat")) {
		if ( !apmib_get( MIB_WLAN_WDS_WEP_FORMAT, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wdsPskFormat")) {
		if ( !apmib_get( MIB_WLAN_WDS_PSK_FORMAT, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		 req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "RFType")) {
		if ( !apmib_get( MIB_HW_RF_TYPE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		 req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "band")) {
		if ( !apmib_get( MIB_WLAN_BAND, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		 req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "fixTxRate")) {
		if ( !apmib_get( MIB_WLAN_FIX_RATE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		 req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "preAuth")) {
		if ( !apmib_get( MIB_WLAN_WPA2_PRE_AUTH, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		 req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "turboMode")) {
		if ( !apmib_get( MIB_WLAN_TURBO_MODE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		 req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "RFPower")) {
		if ( !apmib_get( MIB_WLAN_RFPOWER_SCALE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		 req_format_write(wp, buffer);
		return 0;
	}


#ifdef WLAN_EASY_CONFIG
	else if ( !strcmp(name, "autoCfgEnabled")) {
		if ( !apmib_get( MIB_WLAN_EASYCFG_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "autoCfgMode")) {
		if ( !apmib_get( MIB_WLAN_EASYCFG_MODE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "autoCfgKeyInstall")) {
		char tmpbuf[100];
		if ( !apmib_get( MIB_WLAN_EASYCFG_KEY, (void *)&tmpbuf) )
			return -1;
		if (strlen(tmpbuf))
			val = 1;
		else
			val = 0;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "autoCfgDigestInstall")) {
		char tmpbuf[100];
		int is_adhoc;
		if ( !apmib_get( MIB_WLAN_MODE, (void *)&val) )
			return -1;
		if (val == CLIENT_MODE) {
			apmib_get( MIB_WLAN_NETWORK_TYPE, (void *)&is_adhoc );
			if (is_adhoc) {
				apmib_get( MIB_WLAN_EASYCFG_MODE, (void *)&val);
				if (!(val & MODE_QUESTION))
					val = 2;
				else {
					apmib_get( MIB_WLAN_EASYCFG_DIGEST, (void *)&tmpbuf);
					if (strlen(tmpbuf))
						val = 1;
					else
						val = 0;
				}
			}
			else
				val = 2;
		}
		else {
			if ( !apmib_get( MIB_WLAN_EASYCFG_MODE, (void *)&val) )
				return -1;
			if (!(val & MODE_QUESTION))
				val = 2;
			else {
				if ( !apmib_get( MIB_WLAN_EASYCFG_DIGEST, (void *)&tmpbuf) )
					return -1;
				if (strlen(tmpbuf))
					val = 1;
				else
					val = 0;
			}
		}
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "autoCfgWlanMode")) {
		if ( !apmib_get( MIB_WLAN_EASYCFG_WLAN_MODE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#endif // WLAN_EASY_CONFIG
#ifdef HOME_GATEWAY
	else if ( !strcmp(name, "ddnsEnabled")) {
		if ( !apmib_get( MIB_DDNS_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "ddnsType")) {
		if ( !apmib_get( MIB_DDNS_TYPE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "webWanAccess")) {
		if ( !apmib_get( MIB_WEB_WAN_ACCESS_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	#if defined(CONFIG_RTL_WEB_WAN_ACCESS_PORT)
	else if ( !strcmp(name, "webAccessPort")) {
		if ( !apmib_get( MIB_WEB_WAN_ACCESS_PORT, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wanAccessPortEnable")) {
		#if defined(CONFIG_RTL_WEB_WAN_ACCESS_PORT)
		sprintf(buffer,"%d",1); 
		#else
		sprintf(buffer,"%d",0);
		#endif
		req_format_write(wp, buffer);
		return 0;
	}
	#endif
	else if ( !strcmp(name, "pingWanAccess")) {
		if ( !apmib_get( MIB_PING_WAN_ACCESS_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "VPNPassThruIPsec")) {
		if ( !apmib_get( MIB_VPN_PASSTHRU_IPSEC_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "VPNPassThruPPTP")) {
		if ( !apmib_get( MIB_VPN_PASSTHRU_PPTP_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "VPNPassThruL2TP")) {
		if ( !apmib_get( MIB_VPN_PASSTHRU_L2TP_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(argv[0], "ppoepassthrouh")) {
		if ( !apmib_get( MIB_CUSTOM_PASSTHRU_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", ((int)val& 0x2)?1:0) ;
		req_format_write(wp, buffer);
		return 0;
        }
	#if !defined (CONFIG_IPV6)
        else if ( !strcmp(argv[0], "ipv6passthrouh")) {
		if ( !apmib_get( MIB_CUSTOM_PASSTHRU_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", ((int)val& 0x1)?1:0) ;
		req_format_write(wp, buffer);
		return 0;
        }
	#endif

	else if ( !strcmp(name, "urlFilterEnabled")) {
		if ( !apmib_get( MIB_URLFILTER_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "urlFilterMode")) {
		if ( !apmib_get( MIB_URLFILTER_MODE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}

#endif
	else if ( !strcmp(name, "wispWanId")) {
		if ( !apmib_get( MIB_WISP_WAN_ID, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "opMode")) {
		if ( !apmib_get( MIB_OP_MODE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name,"wlan_num"))
	{
		sprintf(buffer, "%d", wlan_num);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "show_wlan_num")) {
#if defined(CONFIG_RTL_92D_DMDP)||defined(CONFIG_RTL_DUAL_PCIESLOT_BIWLAN_D)
		apmib_get(MIB_WLAN_BAND2G5G_SELECT, (void *)&val);

		if(BANDMODEBOTH == val)
		{
			sprintf(buffer, "%d", wlan_num);
		}
		else
		{
			sprintf(buffer, "%d", wlan_num);
			//sprintf(buffer, "%d", wlan_num-1);
		}
#else
		sprintf(buffer, "%d", wlan_num);
#endif
		req_format_write(wp, buffer);
		return 0;
	}
#ifdef MBSSID
	else if ( !strcmp(name, "vwlan_num")) {
		sprintf(buffer, "%d", vwlan_num);
		req_format_write(wp, buffer);
		return 0;
	}
#endif
	else if ( !strcmp(name, "wlan_idx")) {
		sprintf(buffer, "%d", wlan_idx);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wlanMacClone")) {
		if ( !apmib_get( MIB_WLAN_MACCLONE_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "isWispDisplay")) {
#if defined(CONFIG_RTL_819X) && !defined(CONFIG_WLAN_CLIENT_MODE)// keith. disabled if no this mode in 96c
		sprintf(buffer,"%d", 0);
#else
		sprintf(buffer,"%d", 1);
#endif
		req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "isRepeaterDisplay")) {
#if !defined(UNIVERSAL_REPEATER) || defined(CONFIG_RTL_819X) && !defined(CONFIG_WLAN_CLIENT_MODE)// keith. disabled if no this mode in 96c
		sprintf(buffer,"%d", 0);
#else
		sprintf(buffer,"%d", 1);
#endif
		req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "isWDSDefined")) {
#if defined(CONFIG_WLAN_WDS_SUPPORT)
		sprintf(buffer,"%d", 1);
#else
		sprintf(buffer,"%d", 0);
#endif
		req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "isOtg_Auto")) {
#if defined(CONFIG_RTL_ULINKER)
		apmib_get( MIB_ULINKER_AUTO, (void *)&val);
		sprintf(buffer, "%d", (int)val) ;
#else
		sprintf(buffer,"%d", 0);
#endif
		req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "ulinker_opMode")) {
		sprintf(buffer, "%d", 2) ;
#if defined(CONFIG_RTL_ULINKER)
		int opMode, wlanMode, rpt_enabled;
		apmib_get( MIB_OP_MODE, (void *)&opMode);
		apmib_get( MIB_WLAN_MODE, (void *)&wlanMode);
		if(wlan_idx == 0)
			apmib_get( MIB_REPEATER_ENABLED1, (void *)&rpt_enabled);
#if !defined(CONFIG_RTL_TRI_BAND) 
		else 
			apmib_get( MIB_REPEATER_ENABLED2, (void *)&rpt_enabled);
#else						
		else (wlan_idx == 1)
			apmib_get( MIB_REPEATER_ENABLED2, (void *)&rpt_enabled);
		else (wlan_idx == 2)
			apmib_get( MIB_REPEATER_ENABLED3, (void *)&rpt_enabled);
#endif
		//0:AP; 1:Client; 2:Router; 3:RPT; 4:WISP-RPT
		if(opMode == GATEWAY_MODE)
		{
			sprintf(buffer, "%d", 2) ;
		}
		else if(opMode == WISP_MODE)
		{
			sprintf(buffer, "%d", 4) ;
		}
		else
		{
			if(wlanMode == AP_MODE)
			{	
				if(rpt_enabled == 1)
					sprintf(buffer, "%d", 3);
				else
					sprintf(buffer, "%d", 0);
			}
			else
				sprintf(buffer, "%d", 1);
				
		}

#endif
		req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "isP2PSupport")) {	// P2P_SUPPORT
#if defined(CONFIG_RTL_P2P_SUPPORT)
		sprintf(buffer,"%d", 1);
#else
		sprintf(buffer,"%d", 0);
#endif				
		req_format_write(wp, buffer);
	}
#if 0
	else if ( !strcmp(name, "isWlanMenuStart")) {
#if defined(CONFIG_NET_RADIO) // keith. disabled if no wlan
		sprintf(buffer,"%s", "");
#else
		sprintf(buffer,"%s", "/*");
#endif
		req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "isWlanMenuEnd")) {
#if defined(CONFIG_NET_RADIO) // keith. disabled if no wlan
		sprintf(buffer,"%d", "");
#else
		sprintf(buffer,"%s", "*/");
#endif
		req_format_write(wp, buffer);
	}
#endif

#ifdef RTK_CROSSBAND_REPEATER
	else if( !strcmp(name, "crossband_enable")){
		if(!apmib_get( MIB_WLAN_CROSSBAND_ENABLE, (void *)&val) )
		{	
			return -1;
		}
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#endif
	else if( !strcmp(name, "isCrossbandDefined")){
#ifdef RTK_CROSSBAND_REPEATER
		sprintf(buffer, "%d", 1);
#else
		sprintf(buffer, "%d", 0);
#endif
		req_format_write(wp, buffer);
	}

#ifdef STA_CONTROL
    else if ( !strcmp(name, "staControlEnabled")) {        
        if ( !apmib_get( MIB_WLAN_STACTRL_ENABLE, (void *)&val) )
            return -1;
        sprintf(buffer, "%d", val);
        req_format_write(wp, buffer);
        return 0;
    }
    else if ( !strcmp(name, "staControlPrefer")) {
        if ( !apmib_get( MIB_WLAN_STACTRL_PREFER, (void *)&val) )
            return -1;
        sprintf(buffer, "%d", val);
        req_format_write(wp, buffer);
        return 0;
    }    
#endif
    else if ( !strcmp(name, "isStaControlDefined")) {
#ifdef STA_CONTROL
        sprintf(buffer,"%d", 1);
#else
        sprintf(buffer,"%d", 0);
#endif
        req_format_write(wp, buffer);
    }

#ifdef CONFIG_RTK_MESH
	else if ( !strcmp(name, "wlanMeshEnabled")) {
				//new feature:Mesh enable/disable
                if ( !apmib_get( MIB_WLAN_MESH_ENABLE, (void *)&val) )
                        return -1;
                sprintf(buffer, "%d", val);
                req_format_write(wp, buffer);
                return 0;
        }
        else if ( !strcmp(name, "meshRootEnabled")) {
                if ( !apmib_get( MIB_WLAN_MESH_ROOT_ENABLE, (void *)&val) )
                        return -1;
                sprintf(buffer, "%d", val);
                req_format_write(wp, buffer);
                return 0;
        }
		else if ( !strcmp(name, "meshEncrypt")) {
			if ( !apmib_get( MIB_WLAN_MESH_ENCRYPT, (void *)&val) )
				return -1;
			sprintf(buffer, "%d", val);
			req_format_write(wp, buffer);
			return 0;
		}
		else if ( !strcmp(name, "meshPskFormat")) {
			if ( !apmib_get( MIB_WLAN_MESH_PSK_FORMAT, (void *)&val) )
				return -1;
			sprintf(buffer, "%d", val);
			req_format_write(wp, buffer);
			return 0;
		}
	 	else if ( !strcmp(name, "meshPskValue")) {
			int i;
			buffer[0]='\0';
			if ( !apmib_get(MIB_WLAN_MESH_WPA_PSK,  (void *)buffer) )
				return -1;
			for (i=0; i<strlen(buffer); i++)
				buffer[i]='*';
			buffer[i]='\0';
	   		return req_format_write(wp, buffer);
		}
		else if ( !strcmp(name, "meshWpaAuth")) {
			if ( !apmib_get( MIB_WLAN_MESH_WPA_AUTH, (void *)&val) )
				return -1;
			sprintf(buffer, "%d", val);
			req_format_write(wp, buffer);
			return 0;
		}
		else if ( !strcmp(name, "meshWpa2Cipher")) {
			if ( !apmib_get( MIB_WLAN_MESH_WPA2_CIPHER_SUITE, (void *)&val) )
				return -1;
			sprintf(buffer, "%d", val);
			req_format_write(wp, buffer);
			return 0;
		}
#ifdef RTK_MESH_METRIC_REFINE
		else if ( !strcmp(name, "meshCrossbandEnabled")) {
			if ( !apmib_get( MIB_WLAN_MESH_CROSSBAND_ENABLED, (void *)&val) )
				return -1;
			sprintf(buffer, "%d", val);
			req_format_write(wp, buffer);
			return 0;
		}
#endif
		else if ( !strcmp(name, "isMeshCrossbandDefined")) {
#ifdef RTK_MESH_METRIC_REFINE
        sprintf(buffer,"%d", 1);		
#else
        sprintf(buffer,"%d", 0);		
#endif
        req_format_write(wp, buffer);
    	}

#ifdef _MESH_ACL_ENABLE_
	else if ( !strcmp(name, "meshAclEnabled")) {
		if ( !apmib_get( MIB_WLAN_MESH_ACL_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#endif
#endif // CONFIG_RTK_MESH
	//indispensable!! MESH related , no matter mesh enable or not
	else if ( !strcmp(name, "isMeshDefined")) {
#ifdef CONFIG_RTK_MESH
		sprintf(buffer,"%d", 1);
#else
		sprintf(buffer,"%d", 0);
#endif
		req_format_write(wp, buffer);
	}
	//indispensable!! MESH related , no matter mesh enable or not
	else if ( !strcmp(name, "isNewMeshUI")) {
#ifdef CONFIG_NEW_MESH_UI
		sprintf(buffer,"%d", 1);
#else
		sprintf(buffer,"%d", 0);
#endif
		req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "sambaEnabled")) {
#ifdef CONFIG_APP_SAMBA
		sprintf(buffer,"%d", 1);
#else
		sprintf(buffer,"%d", 0);
#endif
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "rtLogEnabled")) {
		if ( !apmib_get( MIB_REMOTELOG_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "logEnabled")) {
		if ( !apmib_get( MIB_SCRLOG_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
#ifdef TLS_CLIENT
	else if ( !strcmp(name, "rootIdx")) {
		if ( !apmib_get( MIB_ROOT_IDX, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "userIdx")) {
		if ( !apmib_get( MIB_USER_IDX, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "rootNum")) {
		if ( !apmib_get( MIB_CERTROOT_TBL_NUM, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "userNum")) {
		if ( !apmib_get( MIB_CERTUSER_TBL_NUM, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}

#endif

#ifdef UNIVERSAL_REPEATER
	else if ( !strcmp(name, "multiRepeaterEnabled")) {
#if defined(RTL_MULTI_REPEATER_MODE_SUPPORT)
        sprintf(buffer,"%d", 1);
#else
        sprintf(buffer,"%d", 0);
#endif				
        req_format_write(wp, buffer);       
		return 0;
	}
	else if ( !strcmp(name, "multiAPRepeaterStr")) {
#if defined(RTL_MULTI_REPEATER_MODE_SUPPORT)
        sprintf(buffer,"%s", "MultipleAP-MultipleRepeater");
#else
        sprintf(buffer,"%s", "MultipleAP");
#endif				
        req_format_write(wp, buffer);       
		return 0;
	}        
	else if ( !strcmp(name, "repeaterEnabled")) {
		if (wlan_idx == 0)
			id = MIB_REPEATER_ENABLED1;
#if !defined(CONFIG_RTL_TRI_BAND) 
		else
			id = MIB_REPEATER_ENABLED2;
#else
		else if (wlan_idx == 1)
			id = MIB_REPEATER_ENABLED2;
		else if (wlan_idx == 2)
			id = MIB_REPEATER_ENABLED3;		
#endif

		if ( !apmib_get( id, (void *)&val) )
				return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "isRepeaterEnabled")) {
#if 1
		int intVal, intVal2;
		if (wlan_idx == 0)
			apmib_get(MIB_REPEATER_ENABLED1, (void *)&intVal);
#if !defined(CONFIG_RTL_TRI_BAND) 
		else
			apmib_get(MIB_REPEATER_ENABLED2, (void *)&intVal);
#else
		else if (wlan_idx == 1)
			apmib_get(MIB_REPEATER_ENABLED2, (void *)&intVal);
		else if (wlan_idx == 2)
			apmib_get(MIB_REPEATER_ENABLED3, (void *)&intVal);
#endif

		apmib_get(MIB_WLAN_NETWORK_TYPE, (void *)&intVal2);
		apmib_get(MIB_WLAN_MODE, (void *)&val);

		if (intVal != 0 && val != WDS_MODE && !(val==CLIENT_MODE && intVal2==ADHOC))
		{
			val = 1;
		}
		else
		{
			val = 0;
		}

#else
		if (wlan_idx == 0)
			strcpy(buffer, "wlan0-vxd");
		else
			strcpy(buffer, "wlan1-vxd");
		if ( isVxdInterfaceExist(buffer))
			val = 1;
		else
			val = 0;
#endif
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "repeaterMode")) {
		if ( !apmib_get( MIB_WLAN_MODE, (void *)&val) )
			return -1;
		if (val == AP_MODE || val == AP_WDS_MODE || val == AP_MESH_MODE || val == MESH_MODE)
			val = CLIENT_MODE;
		else
			val = AP_MODE;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#endif // UNIVERSAL_REPEATER
	else if ( !strcmp(name, "WiFiTest")) {
		apmib_get( MIB_WIFI_SPECIFIC, (void *)&val);
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#ifdef HOME_GATEWAY
#ifdef DOS_SUPPORT
	else if ( !strcmp(name, "dosEnabled")) {
		if ( !apmib_get( MIB_DOS_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#endif
	else if ( !strcmp(name, "pptpSecurity")) {
		if ( !apmib_get( MIB_PPTP_SECURITY_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "pptpCompress")) {
		if ( !apmib_get( MIB_PPTP_MPPC_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
#endif

#ifdef WIFI_SIMPLE_CONFIG
	else if ( !strcmp(name, "wscDisable")) {
		apmib_get(MIB_WLAN_WSC_DISABLE, (void *)&val);
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wscConfig")) {
		apmib_get(MIB_WLAN_WSC_CONFIGURED, (void *)&val);
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wscRptConfig"))
	{
#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)
		SetWlan_idx("wlan0-vxd");
		apmib_get(MIB_WLAN_WSC_CONFIGURED, (void *)&val);
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		wlan_idx = wlan_idx_keep;
		sprintf(tmpStr,"wlan%d",wlan_idx);
		SetWlan_idx(tmpStr);

		//SetWlan_idx("wlan0");
#else
		sprintf(buffer, "%d", 0);
		req_format_write(wp, buffer);
#endif
		return 0;
	}
	else if ( !strcmp(name, "wps_by_reg")) {
		apmib_get(MIB_WLAN_WSC_CONFIGBYEXTREG, (void *)&val);
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wps_auth")) {
		apmib_get(MIB_WLAN_WSC_AUTH, (void *)&val);
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wpsRpt_auth")) {
#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)
		SetWlan_idx("wlan0-vxd");
		apmib_get(MIB_WLAN_WSC_AUTH, (void *)&val);
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		wlan_idx = wlan_idx_keep;

		sprintf(tmpStr,"wlan%d",wlan_idx);
		SetWlan_idx(tmpStr);

//		SetWlan_idx("wlan0");
#else
		sprintf(buffer, "%d", 0);
		req_format_write(wp, buffer);
#endif
		return 0;
	}
	else if ( !strcmp(name, "wps_enc")) {
		apmib_get(MIB_WLAN_WSC_ENC, (void *)&val);
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wpsRpt_enc")) {
#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)
		SetWlan_idx("wlan0-vxd");
		apmib_get(MIB_WLAN_WSC_ENC, (void *)&val);
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		wlan_idx = wlan_idx_keep;

		sprintf(tmpStr,"wlan%d",wlan_idx);
		SetWlan_idx(tmpStr);

//		SetWlan_idx("wlan0");
#else
		sprintf(buffer, "%d", 0);
		req_format_write(wp, buffer);
#endif
		return 0;
	}
#endif // WIFI_SIMPLE_CONFIG
#ifdef WLAN_HS2_CONFIG
	else if ( !strcmp(name, "hs2Enabled")) {
		if ( !apmib_get( MIB_WLAN_HS2_ENABLE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#else
	else if ( !strcmp(name, "hs2Enabled")) {
		sprintf(buffer, "%d", -2);
		req_format_write(wp, buffer);
		return 0;
	}
#endif
	else if ( !strcmp(name, "pmfEnabled")) {
#ifdef CONFIG_IEEE80211W
		sprintf(buffer, "%d", 1);		
#else
		sprintf(buffer, "%d", 0);			
#endif
		req_format_write(wp, buffer);
		return 0;
	}
// for WMM
	else if ( !strcmp(name, "wmmEnabled")) {
		if ( !apmib_get(MIB_WLAN_WMM_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
//for 11N
	else if ( !strcmp(name, "ChannelBonding")) {
		if ( !apmib_get(MIB_WLAN_CHANNEL_BONDING, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "ControlSideBand")) {
		if ( !apmib_get(MIB_WLAN_CONTROL_SIDEBAND, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "aggregation")) {
		if ( !apmib_get(MIB_WLAN_AGGREGATION, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "shortGIEnabled")) {
		if ( !apmib_get(MIB_WLAN_SHORT_GI, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "static_dhcp")) {
		if ( !apmib_get(MIB_DHCPRSVDIP_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wlanAccess")) {
		if ( !apmib_get(MIB_WLAN_ACCESS, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "rf_used")) {
		struct _misc_data_ misc_data;
		if (getMiscData(WLAN_IF, &misc_data) < 0)
		{			
			sprintf(buffer, "%d", 0);
		}
		else
		{
			sprintf(buffer, "%d", misc_data.mimo_tr_used);
		}
		req_format_write(wp, buffer);
		return 0;
	}	else if ( !strcmp(name, "block_relay")) {
		if ( !apmib_get( MIB_WLAN_BLOCK_RELAY, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}	else if ( !strcmp(name, "tx_stbc")) {
		if ( !apmib_get( MIB_WLAN_STBC_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}	else if ( !strcmp(name, "tx_ldpc")) {
		if ( !apmib_get( MIB_WLAN_LDPC_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
		
	}
	else if ( !strcmp(name, "coexist")) {
		if ( !apmib_get( MIB_WLAN_COEXIST_ENABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	//### add by sen_liu 2011.3.29 TX Beamforming added to mib in 92D
	else if ( !strcmp(name, "tx_beamforming")) {
		if ( !apmib_get( MIB_WLAN_TX_BEAMFORMING, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}//### end
	else if ( !strcmp(name, "txbf_mu")) {
		if ( !apmib_get( MIB_WLAN_TXBF_MU, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "mc2u_disable")) {
		if ( !apmib_get( MIB_WLAN_MC2U_DISABLED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "is_tdls_support")) {
#ifdef TDLS_SUPPORT 
       	 sprintf(buffer,"%d", 1);
#else
      		  sprintf(buffer,"%d", 0);
#endif
      		  req_format_write(wp, buffer);
    	}
#ifdef TDLS_SUPPORT	
	else if ( !strcmp(name, "tdls_prohibited")) {
		if ( !apmib_get( MIB_WLAN_TDLS_PROHIBITED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "tdls_cs_prohibited")) {
		if ( !apmib_get( MIB_WLAN_TDLS_CS_PROHIBITED, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#endif	
#ifdef FAST_BSS_TRANSITION	
	else if ( !strcmp(name, "80211r_enable")) {
		if ( !apmib_get( MIB_WLAN_FT_ENABLE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "ft_over_ds")) {
		if ( !apmib_get( MIB_WLAN_FT_OVER_DS, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#endif
	else if(!strcmp(name,"lowestMlcstRate")){
		if ( !apmib_get( MIB_WLAN_LOWEST_MLCST_RATE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name,"vlan_bridge_feature")){	
	#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT) || defined(CONFIG_RTL_HW_VLAN_SUPPORT) //mark_hwv
		sprintf(buffer,"%d",1);	
	#else
		sprintf(buffer,"%d",0);
	#endif
		req_format_write(wp, buffer);
		return 0;
	}

	else if ( !strcmp(name,"portfw_extend")){	
#if defined(CONFIG_RTL_PORTFW_EXTEND)
		sprintf(buffer,"%d",1);	
	#else
		sprintf(buffer,"%d",0);
	#endif
		req_format_write(wp, buffer);
		return 0;
	}
	else if (!strcmp(name,"hw_vlan_support")){
	#if defined(CONFIG_RTL_HW_VLAN_SUPPORT)
		sprintf(buffer,"%d",1);
	#else
		sprintf(buffer,"%d",0);
	#endif
		req_format_write(wp, buffer);
		return 0;
	}
	else if (!strcmp(name,"rtk_vlan_support")){
	#if defined(VLAN_CONFIG_SUPPORTED)
		sprintf(buffer,"%d",1);
	#else
		sprintf(buffer,"%d",0);
	#endif
		req_format_write(wp, buffer);
		return 0;
	}
	else if (!strcmp(name,"linux_vlan_support")){
	#if defined(CONFIG_8021Q_VLAN_SUPPORTED)
		sprintf(buffer,"%d",1);
	#else
		sprintf(buffer,"%d",0);
	#endif
		req_format_write(wp, buffer);
		return 0;
	}
	
#ifdef MBSSID
	else if ( !strcmp(name, "mssid_idx")) {
		sprintf(buffer, "%d", mssid_idx);
		req_format_write(wp, buffer);
		return 0;
	}
#endif
	else if ( !strcmp(name, "wlan_mssid_num")) {
		sprintf(buffer, "%d", DEF_MSSID_NUM);
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wlan_root_mssid_rpt_num")) {
#if defined(UNIVERSAL_REPEATER) 		
		sprintf(buffer, "%d", NUM_VWLAN+1+1); /// 1:root ; 1:rpt
#else
		sprintf(buffer, "%d", NUM_VWLAN+1); /// 1:root ;
#endif
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wapiAuth")) {
#ifdef CONFIG_RTL_WAPI_SUPPORT
		if ( !apmib_get(MIB_WLAN_WAPI_AUTH, (void *)&val) )
			return -1;
#else
		val=0;
#endif
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wapiPskFormat")) {
#ifdef CONFIG_RTL_WAPI_SUPPORT
		if ( !apmib_get( MIB_WLAN_WAPI_PSK_FORMAT, (void *)&val) )
			return -1;
#else
		val=0;
#endif
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}

#ifdef CONFIG_RTL_WAPI_SUPPORT
	else if ( !strcmp(name, "wapiUcastReKeyType")) {
		if ( !apmib_get(MIB_WLAN_WAPI_UCASTREKEY, (void *)&val) )
			return -1;
		if(0 == val)
		{
			/*default should be off*/
			val = 1;
		}
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wapiMcastReKeyType")) {
		if ( !apmib_get(MIB_WLAN_WAPI_MCASTREKEY, (void *)&val) )
			return -1;
		if(0 == val)
		{
			/*default should be off*/
			val = 1;
		}
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name,"wapiSearchIndex")) {
		if(!apmib_get(MIB_WLAN_WAPI_SEARCHINDEX,(void *)&val))
			return -1;
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
#else
	else if(!strncmp(name,"wapi",4)){
		/*wapi not support*/
		return 0;
	}
#endif
	else if ( !strcmp(name, "isSupportNewWlanSch")) {
#if defined(NEW_SCHEDULE_SUPPORT)
		sprintf(buffer,"%d", 1);
#else
		sprintf(buffer,"%d", 0);
#endif
		req_format_write(wp, buffer);
	}
	else if ( !strcmp(name, "clientModeSupportWapi")) {
#ifdef CONFIG_RTL_WAPI_SUPPORT
		sprintf(buffer, "%d", 1);
#else
		sprintf(buffer, "%d", 0);
#endif
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "is_rpt_wps_support")) {
#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)
		sprintf(buffer,"%d", 1);
#else
		sprintf(buffer,"%d", 0);
#endif
		req_format_write(wp, buffer);
	}
else if ( !strcmp(name, "wps_either_ap_or_vxd")) {
#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT) && defined(CONFIG_WPS_EITHER_AP_OR_VXD)
		sprintf(buffer,"%d", 1);
#else
		sprintf(buffer,"%d", 0);
#endif
		req_format_write(wp, buffer);
	}
#if defined(CONFIG_SNMP)
    else if (!strcmp(name, "snmp_enabled")) {
            if (!apmib_get(MIB_SNMP_ENABLED, (void *)&val)) {
                    return -1;
            }
			sprintf(buffer,"%d", val);
            req_format_write(wp, buffer);
            return 0;
    }
#endif
	else if(!strcmp(name,"snmp_support")){
#ifdef CONFIG_SNMP
		req_format_write(wp, "1");
#else
		req_format_write(wp, "0");
#endif
	return 0;
	}
	else if(!strcmp(name,"wlanBand2G5GSelect")) {
		apmib_get(MIB_WLAN_BAND2G5G_SELECT,(void *)&val);
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name,"Band2G5GSupport")) {
		/* here already set wlan_idx and vwlan_idx */
//printf("\r\n wlan_idx=[%u],vwlan_idx=[%u],__[%s-%u]\r\n",wlan_idx,vwlan_idx,__FILE__,__LINE__);
		apmib_get(MIB_WLAN_PHY_BAND_SELECT, (void *)&val);
		sprintf(buffer, "%d", (int)val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name,"wlan_support_92D"))
	{
#if defined(CONFIG_RTL_92D_SUPPORT)
		sprintf(buffer, "%d", 1) ;
#else
		sprintf(buffer, "%d", 0) ;
#endif
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name,"wlan_support_8812e")) //8812
	{
#if defined(CONFIG_RTL_8812_SUPPORT)
		sprintf(buffer, "%d", 1) ;
#else
		sprintf(buffer, "%d", 0) ;
#endif
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name,"wlan_support_8822be")) //8812
	{
#if defined(CONFIG_WLAN_HAL_8822BE)
		sprintf(buffer, "%d", 1) ;
#else
		sprintf(buffer, "%d", 0) ;
#endif
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name,"wlan_support_ac2g")) //ac2g
	{
#if defined(CONFIG_RTL_AC2G_256QAM) || defined(CONFIG_WLAN_HAL_8814AE)
		sprintf(buffer, "%d", 1) ;
#else
		sprintf(buffer, "%d", 0) ;
#endif
		req_format_write(wp, buffer);
		return 0;
	}
    else if(!strcmp(name,"wlan_support_8192f")) //8192f
    {
#if defined(CONFIG_RTL_8812AR_VN_SUPPORT)
        sprintf(buffer, "%d", 1) ;
#else
        sprintf(buffer, "%d", 0) ;
#endif
        req_format_write(wp, buffer);
        return 0;
    }    
	else if(!strcmp(name,"wlan_mode_2x2"))
	{
	int isWlanMode2x2=0;
#if defined(CONFIG_RTL_92D_SUPPORT)//support 92d
	#if defined(CONFIG_RTL_DUAL_PCIESLOT_BIWLAN_D)//support 92C+92D
		apmib_get(MIB_WLAN_PHY_BAND_SELECT, (void *)&val);
	if((int)val == 2)//5G 92D
		isWlanMode2x2=1;

	#else//only support 92D
		isWlanMode2x2=1;
	#endif
	#endif
#if defined(CONFIG_RTL8192E) || defined(CONFIG_RTL_8812_SUPPORT)  
	isWlanMode2x2=1;
#endif
		sprintf(buffer, "%d", isWlanMode2x2) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name,"wlan_support_92D_concurrent"))
	{
//### edit by sen_liu 2011.4.7 #if #else # endif bug
#if defined(CONFIG_RTL_DUAL_PCIESLOT_BIWLAN_D)	//92D + 92C
	sprintf(buffer, "%d", 2) ;
#else
	#if defined(CONFIG_RTL_92D_DMDP) //92D
		sprintf(buffer, "%d", 1) ;
	#else //92C
		sprintf(buffer, "%d", 0) ;
	#endif
#endif
//### end
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strncmp(name, "tri_band_support", strlen("tri_band_support")) )
	{
#if defined(CONFIG_RTL_TRI_BAND) 
		sprintf(buffer, "%d", 1) ;
#else
		sprintf(buffer, "%d", 0) ;
#endif
		req_format_write(wp, buffer);
		return 0;
	}	
#if defined(CONFIG_RTL_TRI_BAND) 
	else if ( !strncmp(name, "wlan_phyband", strlen("wlan_phyband")) ) 	//wlan idx: 0, 1, 2
	{
		int wl_idx = atoi(argv[1]);
		char wl_ifname[20];
		sprintf(wl_ifname, "%s%d", "wlan", wl_idx);
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_8812_SUPPORT)
		int wlanBand2G5GSelect;
		apmib_get(MIB_WLAN_BAND2G5G_SELECT, (void *)&wlanBand2G5GSelect);
		memset(buffer, 0x00, sizeof(buffer));
		if(SetWlan_idx(wl_ifname))
		{
			apmib_get(MIB_WLAN_PHY_BAND_SELECT, (void *)&val);
			if(val == PHYBAND_5G && (wlanBand2G5GSelect==BANDMODE5G || wlanBand2G5GSelect==BANDMODEBOTH || wlanBand2G5GSelect==BANDMODESINGLE ))
				sprintf(buffer, "%s", "5GHz") ;
			else if(val == PHYBAND_2G && (wlanBand2G5GSelect==BANDMODE2G || wlanBand2G5GSelect==BANDMODEBOTH || wlanBand2G5GSelect==BANDMODESINGLE ))
				sprintf(buffer, "%s", "2.4GHz") ;
			else
				sprintf(buffer, "%s", "") ;
		}
#else
		sprintf(buffer, "%s", "") ;
#endif
		req_format_write(wp, buffer);
		return 0;
	}	
#endif
	else if ( !strcmp(name, "wlan1_phyband"))
	{
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_8812_SUPPORT)
		int wlanBand2G5GSelect;
		apmib_get(MIB_WLAN_BAND2G5G_SELECT, (void *)&wlanBand2G5GSelect);
		memset(buffer, 0x00, sizeof(buffer));
		if(SetWlan_idx("wlan0"))
		{
			apmib_get(MIB_WLAN_PHY_BAND_SELECT, (void *)&val);
			if(val == PHYBAND_5G && (wlanBand2G5GSelect==BANDMODE5G || wlanBand2G5GSelect==BANDMODEBOTH || wlanBand2G5GSelect==BANDMODESINGLE))
				sprintf(buffer, "%s", "5GHz") ;
			else if(val == PHYBAND_2G && (wlanBand2G5GSelect==BANDMODE2G || wlanBand2G5GSelect==BANDMODEBOTH || wlanBand2G5GSelect==BANDMODESINGLE))
				sprintf(buffer, "%s", "2.4GHz") ;
			else
				sprintf(buffer, "%s", "") ;
		}
#else
		sprintf(buffer, "%s", "") ;
#endif
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "wlan2_phyband"))
	{
#if defined(CONFIG_RTL_92D_SUPPORT)
		int wlanBand2G5GSelect;
		apmib_get(MIB_WLAN_BAND2G5G_SELECT, (void *)&wlanBand2G5GSelect);
		memset(buffer, 0x00, sizeof(buffer));
		if(SetWlan_idx("wlan1"))
		{
			apmib_get(MIB_WLAN_PHY_BAND_SELECT, (void *)&val);
			if(val == PHYBAND_5G && (wlanBand2G5GSelect==BANDMODE5G || wlanBand2G5GSelect==BANDMODEBOTH))
				sprintf(buffer, "%s", "5GHz") ;
			else if(val == PHYBAND_2G && (wlanBand2G5GSelect==BANDMODE2G || wlanBand2G5GSelect==BANDMODEBOTH))
				sprintf(buffer, "%s", "2.4GHz") ;
			else
				sprintf(buffer, "%s", "") ;
		}
#else
		sprintf(buffer, "%s", "") ;
#endif
		req_format_write(wp, buffer);
		return 0;
	}
	else if ( !strcmp(name, "maxWebVlanNum"))
	{
#if defined(CONFIG_RTL_8198_AP_ROOT) && defined(GMII_ENABLED)
		sprintf(buffer, "%d", MAX_IFACE_VLAN_CONFIG-2 );
#else
		sprintf(buffer, "%d", MAX_IFACE_VLAN_CONFIG-1);
#endif
		req_format_write(wp, buffer);
		return 0;
	}
    else if(!strcmp(argv[0], "2G_ssid"))
    {
      char ssid[MAX_SSID_LEN*6];
			int ori_wlan_idx = wlan_idx;
			short wlanif;
			unsigned char wlanIfStr[10];

			memset(ssid,0x00,sizeof(ssid));

			wlanif = whichWlanIfIs(PHYBAND_2G);

			if(wlanif >= 0)
			{
				memset(wlanIfStr,0x00,sizeof(wlanIfStr));
				sprintf((char *)wlanIfStr, "wlan%d",wlanif);

				if(SetWlan_idx((char *)wlanIfStr))
				{
					apmib_get(MIB_WLAN_SSID, (void *)ssid);
				}
				wlan_idx = ori_wlan_idx;
			}
			else
			{
				;//ssid is empty
			}
#ifdef CONFIG_RTL_8812_SUPPORT
			apmib_get(MIB_WLAN_SSID, (void *)ssid);
#endif
			translate_control_code(ssid);
	//		req_format_write(wp, ssid);
	//		return 0;
			return req_format_write(wp, "%s", ssid);
    }
    else if(!strcmp(argv[0], "5G_ssid"))
    {
      char ssid[MAX_SSID_LEN*6];
			int ori_wlan_idx = wlan_idx;
			short wlanif;
			unsigned char wlanIfStr[10];

			memset(ssid,0x00,sizeof(ssid));

			wlanif = whichWlanIfIs(PHYBAND_5G);

			if(wlanif >= 0)
			{
				memset(wlanIfStr,0x00,sizeof(wlanIfStr));
				sprintf((char *)wlanIfStr, "wlan%d",wlanif);

				if(SetWlan_idx((char *)wlanIfStr))
				{
					apmib_get(MIB_WLAN_SSID, (void *)ssid);
				}
				wlan_idx = ori_wlan_idx;
			}
			else
			{
				;//ssid is empty
			}
#ifdef CONFIG_RTL_8812_SUPPORT
			apmib_get(MIB_WLAN_SSID, (void *)ssid);
#endif
			translate_control_code(ssid);
	//		req_format_write(wp, ssid);
	//		return 0;
			return req_format_write(wp, "%s", ssid);
    }
    else if(!strcmp(argv[0], "dsf_enable"))
    {
#if defined(CONFIG_RTL_DFS_SUPPORT)
			sprintf(buffer, "%d", 1);
#else
			sprintf(buffer, "%d", 0);
#endif

			req_format_write(wp, buffer);

			return 0;
    }
     else if(!strcmp(name,"set_wlanindex"))
     {
     		if(argc > 1)
     		{
     			wlan_idx=atoi(argv[argc-1]);
			req_format_write(wp, "");
			return 0;
     		}
		else
			return -1;
     }
   	else if ( !strcmp(name, "vlan_val_init")) {
		int i=0, j=0, ret_i=0, ret_j=0, wlan_idx_ori, vwlan_idx_ori;

		memset(WLAN_IF_ori, '\0', sizeof(WLAN_IF_ori));
		memset(inner_buf, '\0', sizeof(inner_buf));
		
		wlan_idx_ori = wlan_idx;
		vwlan_idx_ori = vwlan_idx;
		strcpy(WLAN_IF_ori, WLAN_IF);

		for (i=0; i <wlan_num; i++)
		{
			sprintf(WLAN_IF, "wlan%d", i);
			SetWlan_idx(WLAN_IF);		
			if ( !apmib_get( MIB_WLAN_MODE, (void *)&val) ) {
				ret_i = -1;
				break;
			}
			sprintf(buffer, "wlanMode[%d]=%d;\n", wlan_idx, val);
			strcat(inner_buf, buffer);

			if ( !apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&val) ) {
				ret_i = -1;
				break;
			}
			sprintf(buffer, "wlanDisabled[%d]=%d;\n", wlan_idx, val);
			strcat(inner_buf, buffer);

			for (j=0; j<DEF_MSSID_NUM; j++)
			{
				sprintf(WLAN_IF, "wlan%d-va%d", wlan_idx, vwlan_idx);
				SetWlan_idx(WLAN_IF);
				if ( !apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&val) ) {
					ret_j = -1;
					break;
				}
				sprintf(buffer, "mssid_disable[%d][%d]=%d;\n", wlan_idx, vwlan_idx-1, val);
				strcat(inner_buf, buffer);
			}

			if (ret_j !=0) {
				ret_i = -1;
				break;
			}
		}

		wlan_idx  = wlan_idx_ori;
		vwlan_idx = vwlan_idx_ori;
		strcpy(WLAN_IF, WLAN_IF_ori);

		if (ret_i == 0)
			req_format_write(wp, inner_buf);

		return ret_i;
	}
	else if ( !strcmp(name, "wizard_wlband_init")) {
		int i=0, wlan_idx_ori, vwlan_idx_ori;

		memset(WLAN_IF_ori, '\0', sizeof(WLAN_IF_ori));
		memset(inner_buf, '\0', sizeof(inner_buf));

		wlan_idx_ori = wlan_idx;
		vwlan_idx_ori = vwlan_idx;
		strcpy(WLAN_IF_ori, WLAN_IF);	

		for (i=0; i <wlan_num; i++)
		{
			char tmpbuf[256];
			int val=0;
			sprintf(WLAN_IF, "wlan%d", i);
			SetWlan_idx(WLAN_IF);

			inner_getIndex("wlanDisabled");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "wlanDisabled", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("RFType");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "RFType", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("wlanMode");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "APMode", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("band");
			val = atoi(inner_req_buff);
			if (val > 0) val=val-1;

			sprintf(tmpbuf, "%s[%d]=%d;\n", "bandIdx", wlan_idx, val);
			strcat(inner_buf, tmpbuf);
			sprintf(tmpbuf, "%s[%d]=%d;\n", "bandIdxClient", wlan_idx, val);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("networkType");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "networkType", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("regDomain");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "regDomain", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("channel");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "defaultChan", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("band");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "usedBand", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getInfo("ssid");
			translate_control_code_sprintf(tmpbuf);			
			sprintf(tmpbuf, "%s[%d]='%s';\n", "ssid", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("encrypt");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "encrypt", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("wep");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "wep", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("defaultKeyId");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "defaultKeyId", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("pskFormat");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "defPskFormat", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("wlanMacClone");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "macClone", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("wpaCipher");
			sprintf(tmpbuf, "%s[%d]='%s';\n", "wpaCipher", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);
			
			inner_getIndex("wpa2Cipher");
			sprintf(tmpbuf, "%s[%d]='%s';\n", "wpa2Cipher", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);
			
			inner_getInfo("pskValue");
			sprintf(tmpbuf, "%s[%d]='%s';\n", "pskValue", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("keyType");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "keyType", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("wapiAuth");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "defWapiAuth", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("wapiPskFormat");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "defWapiPskFormat", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getInfo("wapiPskValue");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "defWapiPskValue", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getInfo("wapiASIp");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "defWapiASIP", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getInfo("wapiCertSel");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "defWapiCertSel", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("ChannelBonding");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "init_bound", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("ControlSideBand");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "init_sideband", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);

			inner_getIndex("Band2G5GSupport");
			sprintf(tmpbuf, "%s[%d]=%s;\n", "wlanBand2G5G", wlan_idx, inner_req_buff);
			strcat(inner_buf, tmpbuf);
		}
		
		wlan_idx  = wlan_idx_ori;
		vwlan_idx = vwlan_idx_ori;
		strcpy(WLAN_IF, WLAN_IF_ori);
		
		req_format_write(wp, inner_buf);

		return 0;
	}
#ifdef ADVANCED_IPQOS
	else if(!strcmp(name,"advancedIpqosComment_start"))
	{
		req_format_write(wp, "");
	}
	else if(!strcmp(name,"advancedIpqosComment_end"))
	{
		req_format_write(wp, "");
	}
	else if(!strcmp(name,"noAdvancedIpqosComment_start"))
	{
		req_format_write(wp, "<!--");
	}
	else if(!strcmp(name,"noAdvancedIpqosComment_end"))
	{
		req_format_write(wp, "-->");
	}
	else if(!strcmp(name,"advancedIpqosEnable"))
	{
		req_format_write(wp, "1");
	}
	else if(!strcmp(name,"vlanSupport"))
	{
		#ifdef CONFIG_8021Q_VLAN_SUPPORTED
		int intVal;
		if(apmib_get(MIB_VLAN_ENABLED, (void *)&intVal)){
			if(intVal)
				req_format_write(wp, "1");
			else
				req_format_write(wp, "0");
		}
		else
			req_format_write(wp, "0");
		#else
			req_format_write(wp, "0");
		#endif
	}
	else if(!strcmp(name,"staticIpSupport"))
	{
		int dhcp;
		if(apmib_get( MIB_WAN_DHCP, (void *)&dhcp)){
			if(dhcp == DHCP_DISABLED)
				req_format_write(wp, "1");
			else
				req_format_write(wp, "0");
		}
		else
			req_format_write(wp, "0");
	}
	else if(!strcmp(name,"hwQosSupport")){
		#ifdef CONFIG_RTL_HW_QOS_SUPPORT
			req_format_write(wp, "1");
		#else
			req_format_write(wp, "0");
		#endif
	}		
	else if(!strcmp(name,"hwQosSupport_start"))
	{
		#ifndef CONFIG_RTL_HW_QOS_SUPPORT
		req_format_write(wp, "<!--");
		#else
		req_format_write(wp, "");
		#endif
	}
	else if(!strcmp(name,"hwQosSupport_end"))
	{
		#ifndef CONFIG_RTL_HW_QOS_SUPPORT
		req_format_write(wp, "-->");
		#else
		req_format_write(wp, "");
		#endif
	}
	else if(!strcmp(name,"vlanSupport_start"))
	{
		#ifdef CONFIG_8021Q_VLAN_SUPPORTED
		int intVal;
		if(apmib_get(MIB_VLAN_ENABLED, (void *)&intVal)){
			if(intVal)
				req_format_write(wp, "");
			else
				req_format_write(wp, "<!--");
		}
		else
			req_format_write(wp, "<!--");
		#else
			req_format_write(wp, "<!--");
		#endif
	}
	else if(!strcmp(name,"vlanSupport_end"))
	{
		#ifdef CONFIG_8021Q_VLAN_SUPPORTED
		int intVal;
		if(apmib_get(MIB_VLAN_ENABLED, (void *)&intVal)){
			if(intVal)
				req_format_write(wp, "");
			else
				req_format_write(wp, "-->");
		}
		else
			req_format_write(wp, "-->");
		#else
			req_format_write(wp, "-->");
		#endif
	}
	else if(!strcmp(name,"staticIp_start"))
	{
		int dhcp;
		if(apmib_get( MIB_WAN_DHCP, (void *)&dhcp)){
			if(dhcp == DHCP_DISABLED)
				req_format_write(wp, "");
			else
				req_format_write(wp, "<!--");
		}
		else
			req_format_write(wp, "<!--");
	}
	else if(!strcmp(name,"staticIp_end"))
	{
		int dhcp;
		if(apmib_get( MIB_WAN_DHCP, (void *)&dhcp)){
			if(dhcp == DHCP_DISABLED)
				req_format_write(wp, "");
			else
				req_format_write(wp, "-->");
		}
		else
			req_format_write(wp, "-->");
	}
#else
	else if(!strcmp(name,"advancedIpqosComment_start"))
	{
		req_format_write(wp, "<!--");
	}
	else if(!strcmp(name,"advancedIpqosComment_end"))
	{
		req_format_write(wp, "-->");
	}
	else if(!strcmp(name,"noAdvancedIpqosComment_start"))
	{
		req_format_write(wp, "");
	}
	else if(!strcmp(name,"noAdvancedIpqosComment_end"))
	{
		req_format_write(wp, "");
	}
	else if(!strcmp(name,"advancedIpqosEnable"))
	{
		req_format_write(wp, "0");
	}
	else if(!strcmp(name,"vlanSupport"))
	{
		req_format_write(wp, "0");
	}
	else if(!strcmp(name,"staticIpSupport"))
	{
		req_format_write(wp, "0");
	}
	else if(!strcmp(name,"hwQosSupport")){		// for advanced feature
		req_format_write(wp, "0");
	}	
	else if(!strcmp(name,"hwQosSupport_start"))
	{
		req_format_write(wp, "<!--");
	}
	else if(!strcmp(name,"hwQosSupport_end"))
	{
		req_format_write(wp, "-->");
	}
	else if(!strcmp(name,"vlanSupport_start"))
	{
		req_format_write(wp, "<!--");
	}
	else if(!strcmp(name,"vlanSupport_end"))
	{
		req_format_write(wp, "-->");
	}
	else if(!strcmp(name,"staticIp_start"))
	{
		req_format_write(wp, "<!--");
	}
	else if(!strcmp(name,"staticIp_end"))
	{
		req_format_write(wp, "-->");
	}
#endif
	else if(!strcmp(name,"is_l7_qos_support"))
	{
#if defined(CONFIG_NETFILTER_XT_MATCH_LAYER7)
		sprintf(buffer, "%d", 1) ;
#else
		sprintf(buffer, "%d", 0) ;
#endif
		req_format_write(wp, buffer);
		return 0;
	}
#if defined(CONFIG_RTL_ETH_802DOT1X_CLIENT_MODE_SUPPORT)
	else if(!strcmp(name,"wan_eth_dot1x_enabled"))
	{
		#if 0
		int eth_1x_mode;
		apmib_get(MIB_ELAN_ENABLE_1X,(void *)&val);
		apmib_get(MIB_ELAN_DOT1X_MODE,(void *)&eth_1x_mode);
		if(val && (eth_1x_mode & ETH_DOT1X_CLIENT_MODE))
			sprintf(buffer, "%d", 1) ;
		else
			sprintf(buffer, "%d", 0) ;
		#else
		/* MIB_ELAN_ENABLE_1X bit0-->proxy/snooping enable/disable
		 * MIB_ELAN_ENABLE_1X bit1-->client mode enable/disable
		 */
		apmib_get(MIB_ELAN_ENABLE_1X,(void *)&val);
		if (val & ETH_DOT1X_CLIENT_MODE_ENABLE_BIT)
		{
			sprintf(buffer, "%d", 1) ;
		}
		else
		{
			sprintf(buffer, "%d", 0) ;
		}
		#endif
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name,"wan_eth_dot1x_eap_type"))
	{
		apmib_get(MIB_ELAN_EAP_TYPE,(void *)&val);
		sprintf(buffer, "%d", val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name,"eth_eap_inside_type"))
	{
		apmib_get(MIB_ELAN_EAP_INSIDE_TYPE,(void *)&val);
		sprintf(buffer, "%d", val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name,"eth_eap_phase2_type"))
	{
		apmib_get(MIB_ELAN_EAP_PHASE2_TYPE,(void *)&val);
		sprintf(buffer, "%d", val) ;
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name,"eth_eap_user_id"))
	{
		apmib_get(MIB_ELAN_EAP_USER_ID,(void *)buffer);
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name,"eth_eap_user_name"))
	{
		apmib_get(MIB_ELAN_RS_USER_NAME,(void *)buffer);
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name,"eth_eap_user_password"))
	{
		apmib_get(MIB_ELAN_RS_USER_PASSWD,(void *)buffer);
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name,"eth_eap_user_key"))
	{
		apmib_get(MIB_ELAN_RS_USER_CERT_PASSWD,(void *)buffer);
		req_format_write(wp, buffer);
		return 0;
	}
#endif

#ifdef CONFIG_IPV6
#ifdef CONFIG_DSLITE_SUPPORT
	else if ( !strcmp(name, "dsliteMode")) {
		if ( !apmib_get( MIB_DSLITE_MODE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#else
	else if ( !strcmp(name, "dsliteMode")) {
		sprintf(buffer, "0");
		req_format_write(wp, buffer);
		return 0;
	}
#endif
#else
	else if ( !strcmp(name, "dsliteMode")) {
		sprintf(buffer, "0");
		req_format_write(wp, buffer);
		return 0;
	}
#endif
#ifdef CONFIG_CPU_UTILIZATION
	else if (!strcmp(name, "isDisplayCPU")) {
		sprintf(buffer, "1");
		req_format_write(wp, buffer);
		return 0;
	}
	else if (!strcmp(name, "CPUnumber")) {
		FILE *fh;
	  	char buf[64], tmp[3];
		char *p, *q;
		int cpu_num=-1;

		fh = fopen("/proc/cpuinfo", "r");
		if (!fh) {
//			printf("Warning: cannot open /proc/cpuinfo\n");
			return req_format_write(wp, "Warning: cannot open /proc/cpuinfo");
		}
		
		while(!feof(fh))
		{
			fgets(buf, sizeof buf, fh);

			if(strncmp(buf, "processor", strlen("processor")) == 0)
			{
				p = buf + 9;
				q = tmp;
				while(*p)
				{
					if(*p >= '0' && *p <= '9')
					{
						*q = *p;
						q++;
					}
					p++;
				}
				*q='\0';
				cpu_num = atoi(tmp);
			}
		}

		fclose(fh);

		cpu_num++;
		sprintf(buffer, "%d", cpu_num);
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name, "CPUsample"))
	{
		if ( !apmib_get( MIB_CPU_UTILIZATION_INTERVAL, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name, "CPUenable"))
	{
		if ( !apmib_get( MIB_ENABLE_CPU_UTILIZATION, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#else
	else if (!strcmp(name, "isDisplayCPU")) {
		sprintf(buffer, "0");
		req_format_write(wp, buffer);
		return 0;
	}
#endif

	else if ( !strcmp(name, "DiskWebSupportOnOff")) {
		#if defined(SAMBA_WEB_SUPPORT)
		sprintf(buffer, "%d", 1);
		#else
		sprintf(buffer, "%d", 0);
		#endif
		req_format_write(wp, buffer);
		return 0;
	}
	else if(!strcmp(name,"isSupportWeave"))
	{
	#ifdef CONFIG_APP_WEAVE
		sprintf(buffer, "1");
	#else
		sprintf(buffer, "0");
	#endif
		req_format_write(wp, buffer);
		return 0;
	}
#ifdef CONFIG_APP_WEAVE
	else if(!strcmp(name,"weave_registered"))
	{
		if ( !apmib_get( MIB_WEAVE_REGISTERED, (void *)&val) )
			return -1;
		if(val)
			req_format_write(wp, "1");
		else
			req_format_write(wp, "0");
		return 0;
	}
	else if(!strcmp(name,"weave_device_id"))
	{
		char weaveConfig[MAX_WEAVE_CONFIG_LENGTH]={0};
		char * device_id=NULL;
		int index_=0;
		if ( !apmib_get( MIB_WEAVE_CONFIG, (void *)weaveConfig) )
			return -1;
		//fprintf(stderr,"%s:%dweaveConfig=%s\n",__FUNCTION__,__LINE__,weaveConfig);
		if((device_id=strstr(weaveConfig,"\"cloud_id\":\ \""))==NULL){
			printf("Invalid weave config!!\n");
			return -1;
		}
		//fprintf(stderr,"%s:%d device_id=%s\n",__FUNCTION__,__LINE__,device_id);
		device_id+=strlen("\"cloud_id\":\ \"");
		if(device_id[0]=="\""){
			printf("Invalid weave config!!\n");
			return -1;
		}
		//fprintf(stderr,"%s:%d device_id=%s\n",__FUNCTION__,__LINE__,device_id);
		while((device_id[index_]!='\0') && (device_id[index_]!='\"')) {			
		//fprintf(stderr,"%s:%d index_=%d device_id=%c\n",__FUNCTION__,__LINE__,index_,device_id[index_]);
			index_++;
		}
		//fprintf(stderr,"%s:%d index_=%d device_id=%s\n",__FUNCTION__,__LINE__,index_,device_id);
		device_id[index_]='\0';
		//fprintf(stderr,"%s:%d device_id=%s\n",__FUNCTION__,__LINE__,device_id);
		
		req_format_write(wp,"%s", device_id);
		
		return 0;
	}
#endif
	else if(!strcmp(name,"isSupportZigBee"))
	{
#ifdef CONFIG_APP_ZIGBEE
		sprintf(buffer, "1");
#else
		sprintf(buffer, "0");
#endif
		req_format_write(wp, buffer);
		return 0;
	}
#ifdef SAMBA_WEB_SUPPORT
	else if ( !strcmp(name, "StorageAnonAccessEnable")) {
		if ( !apmib_get( MIB_STORAGE_ANON_ENABLE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		//printf("%s %d buffer=%s \n", __FUNCTION__, __LINE__, buffer);
		req_format_write(wp, buffer);
		return 0;
	}
/*	else if ( !strcmp(name, "StorageAnonAccessFtpEnable")) {
		if ( !apmib_get( MIB_STORAGE_ANON_FTP_ENABLE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}*/
	else if ( !strcmp(name, "StorageAnonAccessDiskEnable")) {
		if ( !apmib_get( MIB_STORAGE_ANON_DISK_ENABLE, (void *)&val) )
			return -1;
		sprintf(buffer, "%d", val);
		req_format_write(wp, buffer);
		return 0;
	}
#endif

#ifdef CONFIG_RTL_TRANSMISSION
	else if(!strcmp(name,"isEnableBT"))
	{
		sprintf(buffer, "1");
		req_format_write(wp, buffer);
		return 0;
	}
#else
	else if(!strcmp(name,"isEnableBT"))
	{
		sprintf(buffer, "0");
		req_format_write(wp, buffer);
		return 0;
	}
#endif
	else if( !strcmp(name, "diagnostic_enable")) {
#ifdef	SYS_DIAGNOSTIC
		return req_format_write(wp, "%s", "1");
#endif
		return req_format_write(wp, "%s", "0");
	}
#ifdef SUPER_NAME_SUPPORT

	else if( !strcmp(name,"userDegreeFlag"))
		return aspUserFlag(wp);

	else if( !strcmp(name,"supportSuperUser"))
	{
		sprintf(buffer, "1");
		req_format_write(wp, buffer);
		return 0;
	}
	
	else if( !strcmp(name,"opnode.htm")){
		sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
		req_format_write(wp, buffer);
		return 0;
	}			
	else if( !strcmp(name,"wlbasic.htm")){
		sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
		req_format_write(wp, buffer);
		return 0;
	}	
	else if( !strcmp(name,"wladvanced.htm")){
		sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
		req_format_write(wp, buffer);
		return 0;
	}
	else if( !strcmp(name,"wlsecurity.htm")){
		sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
		req_format_write(wp, buffer);
		return 0;
	}
	else if( !strcmp(name,"wlactrl.htm")){
		sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
		req_format_write(wp, buffer);
		return 0;
	}
	else if( !strcmp(name,"wlwds.htm")){
		sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
		req_format_write(wp, buffer);
		return 0;
	}
		else if( !strcmp(name,"wlsurvey.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"wlwps.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"wlp2p.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"wlsch.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"rsCertInstall.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"tcpiplan.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"tcpip_multiwanlist.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"tcpipwan.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"ipv6_wan.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"dhcp6s.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"radvd.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"tunnel6.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"portfilter6.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"ip6filter.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"portfilter.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}	
		else if( !strcmp(name,"ipfilter.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"macfilter.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"portfw.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"urlfilter.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"dmz.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"vlan.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"8021q_vlan.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"ip6_qos.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"ip_qos.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"status.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"stats.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"ddns.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			//sprintf(buffer,"%d",0);
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"cpuShow.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}

		else if( !strcmp(name,"ntp.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"dos.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"tr069config.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"syslog.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"upload.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"saveconf.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"password.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"super_password.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"route.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"sub_menu_wlan.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"sub_menu_tcpip.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"sub_menu_ipv6.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"sub_menu_firewall.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		else if( !strcmp(name,"sub_menu_mng.htm")){
			sprintf(buffer,"%d",getHtmlAuth(name,wp->auth_flag));
			req_format_write(wp, buffer);
			return 0;
		}
		
#endif
		else if( !strcmp(name,"sub_menu_wlan_support")){
#ifdef CONFIG_WLAN_UI_SUPPORT
				sprintf(buffer,"%d", 1);
#else
				sprintf(buffer,"%d", 0);
#endif
				req_format_write(wp, buffer);

				return 0;
		}

#ifdef MULTI_WAN_SUUPORT
		else if(!strcmp(name,"WANInfaceEnabled")){
			getMultiWanIndex(wp,argc,argv);
			return 0;
		}
#endif

	else
	{
FMGET_FAIL:

		sprintf(buffer, "%d", 0) ;
		req_format_write(wp, buffer);
		return -1;
	}

	return 0;
}

#ifdef MBSSID

int getVirtualIndex(request *wp, int argc, char **argv)
{
	int ret, old;
	char WLAN_IF_old[40];

	old = vwlan_idx;
	vwlan_idx = atoi(argv[--argc]);
#if defined(CONFIG_RTL_ULINKER)
	if(vwlan_idx == 5) //vxd
		vwlan_idx = NUM_VWLAN_INTERFACE;
#endif

	if (vwlan_idx > NUM_VWLAN_INTERFACE) {
		//fprintf(stderr, "###%s:%d wlan_idx=%d vwlan_idx=%d###\n", __FILE__, __LINE__, wlan_idx, vwlan_idx);
		req_format_write(wp, "0");
		vwlan_idx = old;
		return 0;
	}
	
//#if defined(CONFIG_RTL_8196B)
//	if (vwlan_idx == 5) { //rtl8196b support repeater mode only first, no mssid
//#else
	if (vwlan_idx > 0) {
//#endif
		strcpy(WLAN_IF_old, WLAN_IF);
		sprintf(WLAN_IF, "%s-va%d", WLAN_IF_old, vwlan_idx-1);
	}

	ret = getIndex(wp, argc, argv);

//#if defined(CONFIG_RTL_8196B)
//	if (vwlan_idx == 5)
//#else
	if (vwlan_idx > 0)
//#endif
		strcpy(WLAN_IF, WLAN_IF_old);

	vwlan_idx = old;
	return ret;
}

int getVirtualInfo(request *wp, int argc, char **argv)
{
	int ret, old;
	char WLAN_IF_old[40];

	old = vwlan_idx;
	vwlan_idx = atoi(argv[--argc]);
#if defined(CONFIG_RTL_ULINKER)
		if(vwlan_idx == 5) //vxd
		vwlan_idx = NUM_VWLAN_INTERFACE;
#endif	

	if (vwlan_idx > NUM_VWLAN_INTERFACE) {
		//fprintf(stderr, "###%s:%d wlan_idx=%d vwlan_idx=%d###\n", __FILE__, __LINE__, wlan_idx, vwlan_idx);
		req_format_write(wp, "0");
		vwlan_idx = old;
		return 0;
	}
		
//#if defined(CONFIG_RTL_8196B)
//	if (vwlan_idx == 5) { //rtl8196b support repeater mode only first, no mssid
//#else
	if (vwlan_idx > 0) {
//#endif
		strcpy(WLAN_IF_old, WLAN_IF);
		sprintf(WLAN_IF, "%s-va%d", WLAN_IF_old, vwlan_idx-1);
	}

	ret = getInfo(wp, argc, argv);

//#if defined(CONFIG_RTL_8196B)
//	if (vwlan_idx == 5)
//#else
	if (vwlan_idx > 0)
//#endif
		strcpy(WLAN_IF, WLAN_IF_old);

	vwlan_idx = old;
	return ret;
}
#endif
#ifdef FAST_BSS_TRANSITION
void multilang(request *wp, int argc, char **argv)
{
	return req_format_write(wp,"%s",argv[0]);
}
#endif

#ifdef HOME_GATEWAY
/////////////////////////////////////////////////////////////////////////////
int isConnectPPP()
{
	struct stat status;
#ifdef MULTI_PPPOE
	if(PPPoE_Number == 1)
	{
		if ( stat("/etc/ppp/link", &status) < 0)
			return 0;
	}
	else if(PPPoE_Number == 2)
	{
		if ( stat("/etc/ppp/link2", &status) < 0)
			return 0;
	}
	else if(PPPoE_Number ==3)
	{
		if ( stat("/etc/ppp/link3", &status) < 0)
			return 0;	
	}
	else if(PPPoE_Number ==4)
	{
		if ( stat("/etc/ppp/link4", &status) < 0)
			return 0;
	}
	else
	{
		if ( stat("/etc/ppp/link", &status) < 0)
			return 0;		
	}
#else
	if ( stat("/etc/ppp/link", &status) < 0)
		return 0;
#endif

	return 1;
}
#endif
int getDHCPModeCombobox(request *wp, int argc, char **argv)
{
	int val = 0;
	int lan_dhcp_mode=0;
	int operation_mode=0;
	apmib_get( MIB_WLAN_MODE, (void *)&val);
	apmib_get(MIB_DHCP,(void *)&lan_dhcp_mode);
	apmib_get( MIB_OP_MODE, (void *)&operation_mode);
#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT)
        if((operation_mode==1 && (val==0 ||val==1)) || (operation_mode==0)){
	       if(lan_dhcp_mode == 0){
	 		return req_format_write(wp,"<option selected value=\"0\">Disabled</option>"
	 							"<option value=\"1\">Client</option>"
	 							 "<option value=\"2\">Server</option>"
	 							  "<option value=\"15\">Auto</option>");
	      	  }
		if(lan_dhcp_mode == 1){
	 		return req_format_write(wp,"<option  value=\"0\">Disabled</option>"
	 							"<option selected value=\"1\">Client</option>"
	 							 "<option value=\"2\">Server</option>"
	 							  "<option value=\"15\">Auto</option>");
	      	  }
		if(lan_dhcp_mode == 2){
	 		return req_format_write(wp,"<option  value=\"0\">Disabled</option>"
	 							"<option  value=\"1\">Client</option>"
	 							 "<option selected value=\"2\">Server</option>"
	 							  "<option value=\"15\">Auto</option>");
	      	  }
	       if(lan_dhcp_mode == 15){
	 		return req_format_write(wp,"<option  value=\"0\">Disabled</option>"
	 							"<option  value=\"1\">Client</option>"
	 							 "<option value=\"2\">Server</option>"
	 							 "<option selected value=\"15\">Auto</option>");
	      	  }
    	}
#elif defined(CONFIG_RTL_ULINKER)
		if((operation_mode==1 && (val==0 ||val==1)) || (operation_mode==0) || (operation_mode==2)){
		   if(lan_dhcp_mode == 0){
			return req_format_write(wp,"<option selected value=\"0\">Disabled</option>"
								"<option value=\"1\">Client</option>"
								 "<option value=\"2\">Server</option>"
								  "<option value=\"19\">Auto</option>");
			  }
		if(lan_dhcp_mode == 1){
			return req_format_write(wp,"<option  value=\"0\">Disabled</option>"
								"<option selected value=\"1\">Client</option>"
								 "<option value=\"2\">Server</option>"
								  "<option value=\"19\">Auto</option>");
			  }
		if(lan_dhcp_mode == 2){
			return req_format_write(wp,"<option  value=\"0\">Disabled</option>"
								"<option  value=\"1\">Client</option>"
								 "<option selected value=\"2\">Server</option>"
								  "<option value=\"19\">Auto</option>");
			  }
		   if(lan_dhcp_mode == 19){
			return req_format_write(wp,"<option  value=\"0\">Disabled</option>"
								"<option  value=\"1\">Client</option>"
								 "<option value=\"2\">Server</option>"
								 "<option selected value=\"19\">Auto</option>");
			  }
		}
#else
 	if(lan_dhcp_mode == 0){
 		return req_format_write(wp,"<option selected value=\"0\">Disabled</option>"
 							"<option value=\"1\">Client</option>"
 							 "<option value=\"2\">Server</option>");
      	  }
	if(lan_dhcp_mode == 1){
 		return req_format_write(wp,"<option  value=\"0\">Disabled</option>"
 							"<option selected value=\"1\">Client</option>"
 							 "<option value=\"2\">Server</option>");
      	  }
	if(lan_dhcp_mode == 2){
 		return req_format_write(wp,"<option  value=\"0\">Disabled</option>"
 							"<option  value=\"1\">Client</option>"
 							 "<option selected value=\"2\">Server</option>");
      	  }
#endif
	return 0;
}
#ifdef FAST_BSS_TRANSITION
void SSID_select(request *wp, int argc, char **argv)
{
	int wlan_disable=0,wlan_mode=0,i=0;
	
	char ssid[MAX_SSID_LEN*6]={0};
	apmib_get(MIB_WLAN_WLAN_DISABLED,(void*)&wlan_disable);
	apmib_get(MIB_WLAN_MODE,(void*)&wlan_mode);
	apmib_get(MIB_WLAN_SSID,(void*)ssid);
	translate_control_code(ssid);

	if(wlan_disable)
		return req_format_write(wp, "<option selected value=\"0\">wlan disabled</option>");
	if(wlan_mode==AP_MODE || wlan_mode==AP_WDS_MODE ||wlan_mode==AP_MESH_MODE)
	{
		req_format_write(wp, "<option value=0>Root AP - %s</option>\n", ssid);
	}
	
	apmib_save_wlanIdx();
	
	for (i=1; i<NUM_VWLAN_INTERFACE+1; i++) 
	{
			vwlan_idx=i;
			apmib_get(MIB_WLAN_WLAN_DISABLED,(void*)&wlan_disable);
			if(!wlan_disable)
			{
				apmib_get(MIB_WLAN_SSID,(void*)ssid);
				
				translate_control_code(ssid);
				if(i==NUM_VWLAN_INTERFACE)
					req_format_write(wp, "<option value=%d>wlan%d repeater - %s</option>\n",i,wlan_idx, ssid);
				else
					req_format_write(wp, "<option value=%d>wlan%d - %s</option>\n",i,wlan_idx, ssid);
			}
			
	}
	apmib_recov_wlanIdx();
}
void wlFtKhList(request *wp, int argc, char **argv)
{
    int nBytesSent=0, entryNum, i, j, intfIndex=-1;
    FTKH_T ftkh_entry={0};
    int colspan;


    char strSsid[MAX_SSID_LEN],strAddr[18], strId[49], strKey[33];

    // show title
    nBytesSent += req_format_write(wp, "<tr>"
            "<td align=center width=\"14%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
            "<td align=center width=\"44%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
            "<td align=center width=\"30%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
            #ifdef DOT11K		
            "<td align=center width=\"30%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
            "<td align=center width=\"30%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
            #endif		
            "<td align=center width=\"7%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td></tr>\n",
            "MAC address", ("NAS identifier"),("128-bit key / passphrase"), 
            #ifdef DOT11K		
            ("Op Class"), ("Channel"), 
            #endif				
            ("Select"));

    #ifdef DOT11K       
    colspan = 6;
    #else
    colspan = 4;
    #endif
    apmib_get(MIB_WLAN_SSID,(void*)&strSsid);
    nBytesSent += req_format_write(wp, "<tr>"
            "<td align=left width=\"100%%\" colspan=\"%d\" bgcolor=\"#A0A0A0\"><font size=\"2\"><b>%s</b></td></tr>\n",
            colspan, strSsid);

    apmib_get(MIB_WLAN_FTKH_NUM,(void*)&entryNum);
    for(i=1;i<=entryNum;i++)
    {
        *((char*)(&ftkh_entry))=(char)i;
        apmib_get(MIB_WLAN_FTKH,(void*)&ftkh_entry);
        //printf("%s:%d mac=%02x%02x%02x%02x%02x%02x kh_nas_id=%s\n",__FUNCTION__,__LINE__,ftkh_entry.macAddr[0],ftkh_entry.macAddr[1],ftkh_entry.macAddr[2]
        //	,ftkh_entry.macAddr[3],ftkh_entry.macAddr[4],ftkh_entry.macAddr[5],ftkh_entry.nas_id);
        snprintf(strAddr, sizeof(strAddr), "%02x:%02x:%02x:%02x:%02x:%02x",
            ftkh_entry.macAddr[0], ftkh_entry.macAddr[1], ftkh_entry.macAddr[2],
            ftkh_entry.macAddr[3], ftkh_entry.macAddr[4], ftkh_entry.macAddr[5]);
            snprintf(strId, sizeof(strId), "%s", ftkh_entry.nas_id);
            snprintf(strKey, sizeof(strKey), "%s", ftkh_entry.key);

        nBytesSent += req_format_write(wp, "<tr>"
                "<td align=center width=\"16%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
                "<td align=center width=\"25%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
                "<td align=center width=\"25%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"    
                #ifdef DOT11K					
                "<td align=center width=\"12%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n"
                "<td align=center width=\"12%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n"
                #endif			
                "<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"kh_entry_%d\" value=\"ON\"></td></tr>\n",
                strAddr, strId, strKey, 
                #ifdef DOT11K
                ftkh_entry.opclass, ftkh_entry.channel,
                #endif
                i);
    }


    return nBytesSent;
}

#endif

int getModeCombobox(request *wp, int argc, char **argv)
{
	int val = 0;
	int opmode;
	int wlanBandMode;    
	apmib_get( MIB_OP_MODE, (void *)&opmode);
    apmib_get( MIB_WLAN_BAND, (void *)&wlanBandMode);

	if ( !apmib_get( MIB_WLAN_MODE, (void *)&val) )
			return -1;

#ifdef CONFIG_RTK_MESH
#ifdef CONFIG_NEW_MESH_UI
	  if ( val == 0 ) {
      	  	return req_format_write(wp, "<option selected value=\"0\">AP</option>"
   	  	 "<option value=\"1\">Client</option>"
   	  	 "<option value=\"2\">WDS</option>"
   	  	 "<option value=\"3\">AP+WDS</option>"
   	  	 "<option value=\"4\">AP+MESH</option>"
   	  	 "<option value=\"5\">MESH</option>"  );
      	  }
	  if ( val == 1 ) {
     	  	 return req_format_write(wp,"<option value=\"0\">AP</option>"
   	  	 "<option selected value=\"1\">Client </option>"
   	  	 "<option value=\"2\">WDS</option>"
   	  	 "<option value=\"3\">AP+WDS</option>"
   	  	 "<option value=\"4\">AP+MESH</option>"
   	  	 "<option value=\"5\">MESH</option>"  );
      	  }
	  if ( val == 2 ) {
     	  	 return req_format_write(wp,"<option value=\"0\">AP</option>"
   	  	 "<option value=\"1\">Client </option>"
 	  	 "<option selected value=\"2\">WDS</option>"
   	  	 "<option value=\"3\">AP+WDS</option>"
   	  	 "<option value=\"4\">AP+MESH</option>"
   	  	 "<option value=\"5\">MESH</option>"  );
   	  }
	  if ( val == 3 ) {
     	  	 return req_format_write(wp,"<option value=\"0\">AP</option>"
   	  	 "<option value=\"1\">Client </option>"
 	  	 "<option  value=\"2\">WDS</option>"
   	  	 "<option selected value=\"3\">AP+WDS</option>"
   	  	 "<option value=\"4\">AP+MESH</option>"
   	  	 "<option value=\"5\">MESH</option>"  );
   	  }
   	  if ( val == 4 ) {
		 return req_format_write(wp,"<option value=\"0\">AP</option>"
   	  	 "<option value=\"1\">Client</option>"
   	  	 "<option value=\"2\">WDS</option>"
   	  	 "<option value=\"3\">AP+WDS</option>"
   	  	 "<option selected value=\"4\">AP+MESH</option>"
   	  	 "<option value=\"5\">MESH</option>"  );
   	  }
   	  if ( val == 5 ) {
		 return req_format_write(wp,"<option value=\"0\">AP</option>"
   	  	 "<option value=\"1\">Client</option>"
   	  	 "<option value=\"2\">WDS</option>"
   	  	 "<option value=\"3\">AP+WDS</option>"
   	  	 "<option value=\"4\">AP+MESH</option>"
   	  	 "<option selected value=\"5\">MESH</option>"  );
   	  }
	  else
	  return 0;

#else
  	if ( val == 0 ) {
      	  	return req_format_write(wp, "<option selected value=\"0\">AP</option>"
   	  	 "<option value=\"1\">Client</option>"
   	  	 "<option value=\"2\">WDS</option>"
   	  	 "<option value=\"3\">AP+WDS</option>"
   	  	 "<option value=\"4\">AP+MPP</option>"
   	  	 "<option value=\"5\">MPP</option>"
   	  	 "<option value=\"6\">MAP</option>"
   	  	 "<option value=\"7\">MP</option>" );
      	  }
	  if ( val == 1 ) {
     	  	 return req_format_write(wp,"<option value=\"0\">AP</option>"
   	  	 "<option selected value=\"1\">Client </option>"
   	  	 "<option value=\"2\">WDS</option>"
   	  	 "<option value=\"3\">AP+WDS</option>"
   	  	 "<option value=\"4\">AP+MPP</option>"
   	  	 "<option value=\"5\">MPP</option>"
   	  	 "<option value=\"6\">MAP</option>"
   	  	 "<option value=\"7\">MP</option>"  );
      	  }
	  if ( val == 2 ) {
     	  	 return req_format_write(wp,"<option value=\"0\">AP</option>"
   	  	 "<option value=\"1\">Client </option>"
 	  	 "<option selected value=\"2\">WDS</option>"
   	  	 "<option value=\"3\">AP+WDS</option>"
   	  	 "<option value=\"4\">AP+MPP</option>"
   	  	 "<option value=\"5\">MPP</option>"
   	  	 "<option value=\"6\">MAP</option>"
   	  	 "<option value=\"7\">MP</option>"  );
   	  }
	  if ( val == 3 ) {
     	  	 return req_format_write(wp,"<option value=\"0\">AP</option>"
   	  	 "<option value=\"1\">Client </option>"
 	  	 "<option  value=\"2\">WDS</option>"
   	  	 "<option selected value=\"3\">AP+WDS</option>"
   	  	 "<option value=\"4\">AP+MPP</option>"
   	  	 "<option value=\"5\">MPP</option>"
   	  	 "<option value=\"6\">MAP</option>"
   	  	 "<option value=\"7\">MP</option>"  );
   	  }
   	  if ( val == 4 ) {
		 return req_format_write(wp,"<option value=\"0\">AP</option>"
   	  	 "<option value=\"1\">Client</option>"
   	  	 "<option value=\"2\">WDS</option>"
   	  	 "<option value=\"3\">AP+WDS</option>"
   	  	 "<option selected value=\"4\">AP+MPP</option>"
   	  	 "<option value=\"5\">MPP</option>"
   	  	 "<option value=\"6\">MAP</option>"
   	  	 "<option value=\"7\">MP</option>"  );
   	  }
   	  if ( val == 5 ) {
		 return req_format_write(wp,"<option value=\"0\">AP</option>"
   	  	 "<option value=\"1\">Client</option>"
   	  	 "<option value=\"2\">WDS</option>"
   	  	 "<option value=\"3\">AP+WDS</option>"
   	  	 "<option value=\"4\">AP+MPP</option>"
   	  	 "<option selected value=\"5\">MPP</option>"
   	  	 "<option value=\"6\">MAP</option>"
   	  	 "<option value=\"7\">MP</option>"  );
   	  }
   	   if ( val == 6 ) {
		 return req_format_write(wp,"<option value=\"0\">AP</option>"
   	  	 "<option value=\"1\">Client</option>"
   	  	 "<option value=\"2\">WDS</option>"
   	  	 "<option value=\"3\">AP+WDS</option>"
   	  	 "<option value=\"4\">AP+MPP</option>"
   	  	 "<option value=\"5\">MPP</option>"
   	  	 "<option selected value=\"6\">MAP</option>"
   	  	 "<option value=\"7\">MP</option>"  );
   	  }
   	   if ( val == 7 ) {
		 return req_format_write(wp,"<option value=\"0\">AP</option>"
   	  	 "<option value=\"1\">Client</option>"
   	  	 "<option value=\"2\">WDS</option>"
   	  	 "<option value=\"3\">AP+WDS</option>"
   	  	 "<option value=\"4\">AP+MPP</option>"
   	  	 "<option value=\"5\">MPP</option>"
   	  	 "<option value=\"6\">MAP</option>"
   	  	 "<option selected  value=\"7\">MP</option>" );
   	}
	else
   	return 0;
#endif
#else

  	if ( val == 0 ) {
  		char tmp[300];
  		memset(tmp,0x00,sizeof(tmp));
  		sprintf(tmp,"%s","<option selected value=\"0\">AP</option>");
#if defined(CONFIG_RTL_819X) && !defined(CONFIG_WLAN_CLIENT_MODE)// keith. disabled if no this mode in 96c


#else

#if defined(CONFIG_POCKET_ROUTER_SUPPORT)
	if(opmode == BRIDGE_MODE && val == CLIENT_MODE)
	{
   		strcat(tmp,"<option value=\"1\">Client</option>");
	}
	else
	{

	}
#else
   	  strcat(tmp,"<option value=\"1\">Client</option>");
#endif //#if defined(CONFIG_POCKET_ROUTER_SUPPORT)

#endif

#if defined(CONFIG_RTL_819X) && !defined(CONFIG_WLAN_WDS_SUPPORT)// keith. disabled if no this mode in 96c
#else
   	  strcat(tmp,"<option value=\"2\">WDS</option>"
   	  	 "<option value=\"3\">AP+WDS</option>"    );
#endif

#ifdef CONFIG_RTL_P2P_SUPPORT
      SDEBUG("\n");
      if((wlanBandMode&BAND_11A)==0)    // not include 5G
   	  strcat(tmp,"<option value=\"8\">P2P</option> ");
#endif
      return req_format_write(wp,tmp);
      	  }

	  if ( val == 1 ) {
	  	char tmp[300];
  		memset(tmp,0x00,sizeof(tmp));
  		sprintf(tmp,"%s","<option value=\"0\">AP</option>");
#if defined(CONFIG_RTL_819X) && !defined(CONFIG_WLAN_CLIENT_MODE)// keith. disabled if no this mode in 96c
#else
   	  strcat(tmp,"<option selected value=\"1\">Client</option>");
#endif

#if defined(CONFIG_RTL_819X) && !defined(CONFIG_WLAN_WDS_SUPPORT)// keith. disabled if no this mode in 96c
#else
   	  strcat(tmp,"<option value=\"2\">WDS</option>"
   	  	 "<option value=\"3\">AP+WDS</option>"     );
#endif
#ifdef CONFIG_RTL_P2P_SUPPORT
      SDEBUG("\n");
      if((wlanBandMode&BAND_11A)==0)    // not include 5G
   	  strcat(tmp,"<option value=\"8\">P2P</option> ");
#endif

      return req_format_write(wp,tmp);
      	  }

	  if ( val == 2 ) {
		char tmp[300];
  		memset(tmp,0x00,sizeof(tmp));
  		sprintf(tmp,"%s","<option value=\"0\">AP</option>");
#if defined(CONFIG_RTL_819X) && !defined(CONFIG_WLAN_CLIENT_MODE)// keith. disabled if no this mode in 96c
#else

#if defined(CONFIG_POCKET_ROUTER_SUPPORT)
	if(opmode == BRIDGE_MODE && val == CLIENT_MODE)
	{
   	  strcat(tmp,"<option value=\"1\">Client</option>");
	}
	else
	{

	}
#else
	strcat(tmp,"<option value=\"1\">Client</option>");
#endif //#if defined(CONFIG_POCKET_ROUTER_SUPPORT)

#endif

#if defined(CONFIG_RTL_819X) && !defined(CONFIG_WLAN_WDS_SUPPORT)// keith. disabled if no this mode in 96c
#else
   	  strcat(tmp,"<option selected value=\"2\">WDS</option>"
   	  	 "<option value=\"3\">AP+WDS</option>"    );
#endif
#ifdef CONFIG_RTL_P2P_SUPPORT
      SDEBUG("\n");
      if((wlanBandMode&BAND_11A)==0)    // not include 5G
   	  strcat(tmp,"<option value=\"8\">P2P</option> ");
#endif
      return req_format_write(wp,tmp);
   	  }
	  if ( val == 3 ) {
		char tmp[300];
  		memset(tmp,0x00,sizeof(tmp));
  		sprintf(tmp,"%s","<option value=\"0\">AP</option>");
#if defined(CONFIG_RTL_819X) && !defined(CONFIG_WLAN_CLIENT_MODE)// keith. disabled if no this mode in 96c
#else

#if defined(CONFIG_POCKET_ROUTER_SUPPORT)
	if(opmode == BRIDGE_MODE && val == CLIENT_MODE)
	{
   	  strcat(tmp,"<option value=\"1\">Client</option>");
	}
	else
	{

	}
#else
	strcat(tmp,"<option value=\"1\">Client</option>");
#endif //#if defined(CONFIG_POCKET_ROUTER_SUPPORT)

#endif

#if defined(CONFIG_RTL_819X) && !defined(CONFIG_WLAN_WDS_SUPPORT)// keith. disabled if no this mode in 96c
#else
   	  strcat(tmp,"<option value=\"2\">WDS</option>"
   	  	 "<option selected value=\"3\">AP+WDS</option>"   );
#endif
#ifdef CONFIG_RTL_P2P_SUPPORT
      SDEBUG("\n");
      if((wlanBandMode&BAND_11A)==0)    // not include 5G
   	  strcat(tmp,"<option value=\"8\">P2P</option> ");
#endif
      return req_format_write(wp,tmp);
   	  } 
#ifdef CONFIG_RTL_P2P_SUPPORT
	  if ( val == 8 ) {
		char tmp[300];
  		memset(tmp,0x00,sizeof(tmp));
  		sprintf(tmp,"%s","<option value=\"0\">AP</option>");
#if defined(CONFIG_RTL_819X) && !defined(CONFIG_WLAN_CLIENT_MODE)// keith. disabled if no this mode in 96c			 	
#else			 	

#if defined(CONFIG_POCKET_ROUTER_SUPPORT)
	if(opmode == BRIDGE_MODE && val == CLIENT_MODE)
	{
   	  strcat(tmp,"<option value=\"1\">Client</option>");
	}
	else
	{

	}
#else
	strcat(tmp,"<option value=\"1\">Client</option>");
#endif //#if defined(CONFIG_POCKET_ROUTER_SUPPORT)

#endif   	  	 

#if defined(CONFIG_RTL_819X) && !defined(CONFIG_WLAN_WDS_SUPPORT)// keith. disabled if no this mode in 96c
#else
   	  strcat(tmp,"<option value=\"2\">WDS</option>"
   	  	 "<option value=\"3\">AP+WDS</option>"   );
#endif
      SDEBUG("\n");
      if((wlanBandMode&BAND_11A)==0)    // not include 5G
   	  strcat(tmp,"<option selected value=\"8\">P2P</option> ");

      return req_format_write(wp,tmp);
   	  }
#endif	  
	  else
   	  	return 0;
#endif
}

