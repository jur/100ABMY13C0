/*
*/

/* System include files */
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <netdb.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <ctype.h>

#include <net/if.h>
#include <stddef.h>		/* offsetof */
#include <net/if_arp.h>
#include <linux/if_ether.h>
#include "apmib.h"
#include "mibtbl.h"
#include "sysconf.h"
#include "sys_utility.h"

#ifndef IPV6_ADDR_LINKLOCAL
#define IPV6_ADDR_LINKLOCAL   0x0020U
#endif

extern char wan_interface[16];
void set_dhcp6s();
void set_dnsv6();
void set_radvd();
void set_ecmh();
void set_dhcp6c();
void set_wanv6();
void set_lanv6();
void set_wanipv6();
void set_lanipv6();
void setbridgeipv6();
#ifdef CONFIG_IPV6_DNSV6_MODE_SUPPORT
int get_linklocal_addr(addr6CfgParam_t *addr6_dns, char *ifaname);
#endif



int checkDnsAddrIsExist(char *dnsAddr, char * dnsFileName)
{
	char  line_buf[128];		
	FILE *fp=NULL;
	if((fp=fopen(dnsFileName, "r"))==NULL)
	{
//		printf("Open file : %s fails!\n",dnsFileName);
		return 0;
	}
	while(fgets(line_buf, 128, fp)) 
	{			
		if(strstr(line_buf, dnsAddr)!=NULL)
		{
			fclose(fp);
			return 1;
		}			
	}
	fclose(fp);
	return 0;
}
void set_dhcp6s()
{
	dhcp6sCfgParam_t dhcp6sCfgParam;
	char tmpStr[256];
	char tmpBuf[256];
	int fh;
	int pid=-1;
	int dnsMode;
	int dnsforward=0;
	if ( !apmib_get(MIB_IPV6_DHCPV6S_PARAM,(void *)&dhcp6sCfgParam)){
		printf("get MIB_IPV6_DHCPV6S_PARAM failed\n");
		return;  
	}
	
	if(!dhcp6sCfgParam.enabled){
		return;
	}
	
#ifdef CONFIG_SIXRD_SUPPORT
	int val;

	if(!apmib_get(MIB_IPV6_ORIGIN_TYPE,&val)){
		fprintf(stderr, "Read MIB_IPV6_ORIGIN_TYPE Error\n");
		return -1;			
	}
	if(val == 2) //if 6rd, dhcp6s start with 6rd prefix delegation
		goto start_dhcp6s;
#endif

#ifdef CONFIG_IPV6_CE_ROUTER_SUPPORT
		if(dhcp6sCfgParam.addr6PrefixMode== 1)
			return;
#endif
	
	if(isFileExist(DHCP6S_CONF_FILE) == 0) {
		/*create config file*/
		fh = open(DHCP6S_CONF_FILE, O_RDWR|O_CREAT|O_TRUNC, S_IRWXO|S_IRWXG);	
		if (fh < 0) {
			fprintf(stderr, "Create %s file error!\n", DHCP6S_CONF_FILE);
			return;
		}
		printf("create dhcp6s.conf\n");
#ifdef	CONFIG_IPV6_DNSV6_MODE_SUPPORT
		apmib_get(MIB_IPV6_DNS_FORWARD, (void *)&dnsforward);	
		if(dnsforward==0)  // dns relay mode
		{
			addr6CfgParam_t addr6_dns;
			if(get_linklocal_addr(&addr6_dns,dhcp6sCfgParam.interfaceNameds)>0)
			{
				snprintf(tmpBuf, sizeof(tmpBuf), "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x", 
				addr6_dns.addrIPv6[0], addr6_dns.addrIPv6[1], addr6_dns.addrIPv6[2], addr6_dns.addrIPv6[3], 
				addr6_dns.addrIPv6[4], addr6_dns.addrIPv6[5], addr6_dns.addrIPv6[6], addr6_dns.addrIPv6[7]);
				if(strstr(tmpBuf, "0000:0000:0000:0000:0000:0000:0000:0000")==NULL) 
				{
					sprintf(tmpStr, "option domain-name-servers %s;\n", tmpBuf);
					write(fh, tmpStr, strlen(tmpStr));
				}
			}
		}
		else
		{
			sprintf(tmpStr, "option domain-name-servers %s;\n", dhcp6sCfgParam.DNSaddr6);
			write(fh, tmpStr, strlen(tmpStr));
		}
#else
		sprintf(tmpStr, "option domain-name-servers %s;\n", dhcp6sCfgParam.DNSaddr6);
		write(fh, tmpStr, strlen(tmpStr));
#endif
#ifdef	CONFIG_IPV6_DNSV6_MODE_SUPPORT
		memset(tmpBuf, 0, sizeof(tmpBuf));
		apmib_get(MIB_DOMAIN_NAME, (void *)tmpBuf);	
		sprintf(tmpStr, "option domain-name \"%s.com\";\n", tmpBuf);
		write(fh, tmpStr, strlen(tmpStr));
#endif
		sprintf(tmpStr, "interface %s {\n", dhcp6sCfgParam.interfaceNameds);
		write(fh, tmpStr, strlen(tmpStr));
		sprintf(tmpStr, "  address-pool pool1 3600;\n");
		write(fh, tmpStr, strlen(tmpStr));
		sprintf(tmpStr, "};\n");
		write(fh, tmpStr, strlen(tmpStr));
		sprintf(tmpStr, "pool pool1 {\n");
		write(fh, tmpStr, strlen(tmpStr));
#ifdef CONFIG_IPV6_CE_ROUTER_SUPPORT
		if(dhcp6sCfgParam.addr6PrefixMode == 2)
		{
			addr6CfgParam_t ula_prefix;

			if ( !apmib_get(MIB_IPV6_ADDR_ULA_PARAM,(void *)&ula_prefix))
			{
				fprintf(stderr, "get mib %d error!\n", MIB_IPV6_ADDR_ULA_PARAM);
				return;
			}

			sprintf(tmpStr, "  range %04x:%04x:%04x:%04x:%s "	\
							"to %04x:%04x:%04x:%04x:%s ;\n",
							ula_prefix.addrIPv6[0],
							ula_prefix.addrIPv6[1],
							ula_prefix.addrIPv6[2],
							ula_prefix.addrIPv6[3],
							dhcp6sCfgParam.addr6PoolS,
							ula_prefix.addrIPv6[0],
							ula_prefix.addrIPv6[1],
							ula_prefix.addrIPv6[2],
							ula_prefix.addrIPv6[3],
							dhcp6sCfgParam.addr6PoolE);
		}
		else if(dhcp6sCfgParam.addr6PrefixMode == 0)
#endif
		{
		sprintf(tmpStr, "  range %s to %s ;\n", dhcp6sCfgParam.addr6PoolS, dhcp6sCfgParam.addr6PoolE);
		}
		write(fh, tmpStr, strlen(tmpStr));
		sprintf(tmpStr, "};\n");
		write(fh, tmpStr, strlen(tmpStr));

		close(fh);
	}

	/*start daemon*/
start_dhcp6s:
	if(isFileExist(DHCP6S_PID_FILE)) {
		pid=getPid_fromFile(DHCP6S_PID_FILE);
		if(dhcp6sCfgParam.enabled == 1){
			sprintf(tmpStr, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-9", tmpStr, NULL_STR);
			unlink(DHCP6S_PID_FILE);
			RunSystemCmd(NULL_FILE, "/bin/dhcp6s", dhcp6sCfgParam.interfaceNameds, NULL_STR);
		}
		else 
			RunSystemCmd(NULL_FILE, "kill", "-9", tmpStr, NULL_STR);
			
	}else {
		if(dhcp6sCfgParam.enabled == 1)
			RunSystemCmd(NULL_FILE, "/bin/dhcp6s", dhcp6sCfgParam.interfaceNameds, NULL_STR);
	}
		
	return;
}

void set_dnsv6()
{
	dnsv6CfgParam_t dnsCfgParam;
	int pid = -1;
	int fh, dnsMode;
	char tmpStr[128];
	char tmpBuff[32];
	char tmpChar;
	char addr[256];
	char cmdBuffer[256];
	char dns[64], dnstmp[64];
	
//	char def_wan_ifname[]="eth1";
	char *def_wan_ifname = "eth1";
	
//	char hostfile[]="/var/dnsmasq_hostfile";
	FILE *fp=NULL;

#ifdef TR181_SUPPORT
	DNS_CLIENT_SERVER_T entry[2]={0};
	int y;
#endif

#ifdef CONFIG_SIXRD_SUPPORT
	int val;
	if(!apmib_get(MIB_IPV6_ORIGIN_TYPE,&val)){
		fprintf(stderr, "get mib %d error!\n", MIB_IPV6_ORIGIN_TYPE);
		return;
	}
	if(val == IPV6_ORIGIN_6RD)
	{
		def_wan_ifname = "tun6rd";
	}
#endif


	system("rm -f /var/dnsmasq.conf 2> /dev/null");		
	system("rm -f /var/dnsmasq_resolv.conf 2> /dev/null");
//	system("rm -f /var/dnsmasq_hostfile 2> /dev/null");
	if ( !apmib_get(MIB_IPV6_DNSV6_PARAM,(void *)&dnsCfgParam))
	{
		printf("get MIB_IPV6_DNSV6_PARAM failed\n");
		return;  
	}
	
	if(!isFileExist(DNSV6_CONF_FILE))
	{
		/*create config file*/
		fh = open(DNSV6_CONF_FILE, O_RDWR|O_CREAT|O_TRUNC, S_IRWXO|S_IRWXG);	
		if (fh < 0) 
		{
			fprintf(stderr, "Create %s file error!\n", DNSV6_CONF_FILE);
			return;
		}

		printf("create dnsmasq.conf\n");

		apmib_get(MIB_ELAN_MAC_ADDR,  (void *)tmpBuff);
		if(!memcmp(tmpBuff, "\x00\x00\x00\x00\x00\x00", 6))
		apmib_get(MIB_HW_NIC0_ADDR,  (void *)tmpBuff);
		sprintf(cmdBuffer, "%02x%02x%02x%02x%02x%02x", (unsigned char)tmpBuff[0], (unsigned char)tmpBuff[1], 
		(unsigned char)tmpBuff[2], (unsigned char)tmpBuff[3], (unsigned char)tmpBuff[4], (unsigned char)tmpBuff[5]);

		tmpChar=cmdBuffer[1];

		switch(tmpChar) 
		{
			case '0':
			case '1':
			case '4':
			case '5':
			case '8':
			case '9':
			case 'c':
			case 'd':
			tmpChar = (char)((int)tmpChar+2);
			break;
			default:
			break;
		}
		sprintf(addr, "Fe80::%c%c%c%c:%c%cFF:FE%c%c:%c%c%c%c", 
		cmdBuffer[0], tmpChar, cmdBuffer[2], cmdBuffer[3],
		cmdBuffer[4], cmdBuffer[5],
		cmdBuffer[6], cmdBuffer[7],
		cmdBuffer[8],cmdBuffer[9],cmdBuffer[10],cmdBuffer[11]
		);

		sprintf(tmpStr, "domain-needed\n");
		write(fh, tmpStr, strlen(tmpStr));
		sprintf(tmpStr, "bogus-priv\n");
		write(fh, tmpStr, strlen(tmpStr));	
//		sprintf(tmpStr, "addn-hosts=%s\n", hostfile);
//		write(fh, tmpStr, strlen(tmpStr));
		sprintf(tmpStr, "address=/%s/%s\n",dnsCfgParam.routerName,addr);
		write(fh, tmpStr, strlen(tmpStr));

//		close(fh);	
	}	
	
	if(!apmib_get(MIB_IPV6_DNS_AUTO,  (void *)&dnsMode))
	{
		printf("get MIB_IPV6_DNS_AUTO failed\n");
		return; 
	}	
	if(dnsMode==0)  //Set DNS Manually 
	{
		addr6CfgParam_t addr6_dns;
		
		if(!apmib_get(MIB_IPV6_ADDR_DNS_PARAM,  (void *)&addr6_dns))
		{
			printf("get MIB_IPV6_ADDR_DNS_PARAM failed\n");
			return;
		}
		snprintf(dns, sizeof(dns), "nameserver %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n", 
		addr6_dns.addrIPv6[0], addr6_dns.addrIPv6[1], addr6_dns.addrIPv6[2], addr6_dns.addrIPv6[3], 
		addr6_dns.addrIPv6[4], addr6_dns.addrIPv6[5], addr6_dns.addrIPv6[6], addr6_dns.addrIPv6[7]);

		if(strstr(dns, "0000:0000:0000:0000:0000:0000:0000:0000")==NULL)	
		{
			if(isFileExist(DNSV6_RESOLV_FILE))
			{
				if(!checkDnsAddrIsExist(dns, DNSV6_RESOLV_FILE))
					write_line_to_file(DNSV6_RESOLV_FILE, 2, dns);
			}
			else
				write_line_to_file(DNSV6_RESOLV_FILE, 1, dns);
#ifdef TR181_SUPPORT
			snprintf(dnstmp, sizeof(dnstmp), "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x", 
				addr6_dns.addrIPv6[0], addr6_dns.addrIPv6[1], addr6_dns.addrIPv6[2], addr6_dns.addrIPv6[3], 
				addr6_dns.addrIPv6[4], addr6_dns.addrIPv6[5], addr6_dns.addrIPv6[6], addr6_dns.addrIPv6[7]);

			y = 9;
			*((char*)entry)=(char)y;
			if(apmib_get(MIB_DNS_CLIENT_SERVER_TBL,(void*)entry)==0)
			{
				printf("get MIB_DNS_CLIENT_SERVER_TBL fail!\n");
				return;
			}
			memcpy(&(entry[1]), &(entry[0]), sizeof(DNS_CLIENT_SERVER_T));
			entry[1].index = 8;
			entry[1].enable = 1;
			entry[1].status = 1;
			strcpy(entry[1].ipAddr, dnstmp);
//			strcpy(entry[1].interface, def_wan_ifname);
			entry[1].type = 5; //static
			if(apmib_set(MIB_DNS_CLIENT_SERVER_MOD,(void*)entry)==0)
			{
				printf("set MIB_DNS_CLIENT_SERVER_TBL fail!\n");
				return;
			}

#endif
		}			
		if(isFileExist(DNSV6_RESOLV_FILE))
		{			
			sprintf(tmpStr, "resolv-file=%s\n", DNSV6_RESOLV_FILE);
			write(fh, tmpStr, strlen(tmpStr));
		}	
	}
	else  //Attain DNS Automatically 
	{
#ifdef TR181_SUPPORT
		FILE *dnsfile = NULL;
		//wait for file DNSV6_ADDR_FILE create and open it
		int intT=0;
		while(intT<5)
		{
			dnsfile = fopen(DNSV6_ADDR_FILE, "r");
			if (dnsfile == NULL) {
//				fprintf(stderr, "Can't open %s -- %d!\n", DNSV6_ADDR_FILE, intT);
			}
			else
				break;
			intT++;
			sleep(1);
		}
		if(intT>=5)
		{
			printf("Open %s failed!\n", DNSV6_ADDR_FILE);
			close(fh);	
			return;
		}
//		read(fh_dnsfile, dns, 64);
//		close(fh_dnsfile);
		y = 7;
		while(fgets(dns, 64, dnsfile) != NULL && y<9)
		{
			if(strstr(dns, "nameserver")==NULL)
			{
				printf("Error nameserver in %s.\n", DNSV6_ADDR_FILE);
				return;
			}
			int i=10, j=0;
			while(dns[i]!='\0' && dns[i]!='\n' && i<64)
			{
				if(dns[i]==' ')
				{
					i++;
					continue;
				}
				dnstmp[j] = dns[i];
				i++;
				j++;
			}
			dnstmp[j]='\0';
//			printf("The ip address is %s...\n", dnstmp);
				
			*((char*)entry)=(char)y;
			if(apmib_get(MIB_DNS_CLIENT_SERVER_TBL,(void*)entry)==0)
			{
				printf("get MIB_DNS_CLIENT_SERVER_TBL fail!\n");
				return;
			}
			memcpy(&(entry[1]), &(entry[0]), sizeof(DNS_CLIENT_SERVER_T));
			entry[1].index = y-1;
			entry[1].enable = 1;
			entry[1].status = 1;
			strcpy(entry[1].ipAddr, dnstmp);
	//		strcpy(entry[1].interface, def_wan_ifname);
			entry[1].type = 2; //DHCPv6
			if(apmib_set(MIB_DNS_CLIENT_SERVER_MOD,(void*)entry)==0)
			{
				printf("set MIB_DNS_CLIENT_SERVER_TBL fail!\n");
				return;
			}
			y++;
		}
#endif
#ifdef	CONFIG_IPV6_DNSV6_MODE_SUPPORT
	if(isFileExist(RESOLV_CONF_FILE))
	{			
		sprintf(tmpStr, "resolv-file=%s\n", RESOLV_CONF_FILE);
		write(fh, tmpStr, strlen(tmpStr));
	}
#else
	if(isFileExist(DNSV6_RESOLV_FILE))
	{			
		sprintf(tmpStr, "resolv-file=%s\n", DNSV6_RESOLV_FILE);
		write(fh, tmpStr, strlen(tmpStr));
	}	
#endif
	}
	
	close(fh);		
//	system("echo fe80::287:42ff:fe16:9541 rlx.realsil >>/var/dnsmasq_hostfile");
	if(isFileExist(DNSV6_PID_FILE)) 
	{
		pid=getPid_fromFile(DNSV6_PID_FILE);
		if(pid>0)
		{
			sprintf(tmpStr, "kill -9 %d", pid);
			system(tmpStr);
			unlink(DNSV6_PID_FILE);
		}
//		if(dnsCfgParam.enabled == 1) 
		{
#if 0
			if(isFileExist(DNRD_PID_FILE)){
			pid=getPid_fromFile(DNRD_PID_FILE);
			sprintf(tmpStr, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-9", tmpStr, NULL_STR);
			unlink(DNRD_PID_FILE);				
			}
#endif
			sprintf(tmpStr, "dnsmasq -C /var/dnsmasq.conf -O %s", def_wan_ifname);
			system(tmpStr);
		}
	} 
	else
	{
//		if(dnsCfgParam.enabled == 1) 
		{
#if 0
			if(isFileExist(DNRD_PID_FILE)) {
			pid=getPid_fromFile(DNRD_PID_FILE);
			sprintf(tmpStr, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-9", tmpStr, NULL_STR);
			unlink(DNRD_PID_FILE);			
			}
#endif
			sprintf(tmpStr, "dnsmasq -C /var/dnsmasq.conf -O %s", def_wan_ifname);
			system(tmpStr);
		}
	}
	return;
}
#ifdef CONFIG_IPV6_DNSV6_MODE_SUPPORT
int get_linklocal_addr(addr6CfgParam_t *addr6_dns, char *ifaname)
{
	FILE *fp;
	char str_addr[40];
	unsigned int plen, scope, dad_status, if_idx;
	char devname[IFNAMSIZ];
	if ((fp = fopen("/proc/net/if_inet6", "r")) == NULL)
	{
		printf("can't open %s: %s", "/proc/net/if_inet6",
			strerror(errno));
		return (-1);	
	}
	
	while (fscanf(fp, "%32s %x %02x %02x %02x %15s\n",
		      str_addr, &if_idx, &plen, &scope, &dad_status,
		      devname) != EOF)
	{
		if (scope == IPV6_ADDR_LINKLOCAL &&
		    strcmp(devname, ifaname) == 0)
		{
			struct in6_addr addr;
			unsigned int ap;
			int i;
			for (i=0; i<8; i++)
			{
				sscanf(str_addr + i * 4, "%04x", &ap);
				addr6_dns->addrIPv6[i] = ap;
			}
			addr6_dns->prefix_len=plen;
			fclose(fp);
			return 1;	
		}
	}
	printf("no linklocal address configured for %s", ifaname);
	fclose(fp);
	return (-1);	
}
#endif
void set_radvd()
{
	radvdCfgParam_t radvdCfgParam;
#ifdef CONFIG_APP_RADVD_WAN
	radvdCfgParam_t radvdCfgParam_wan;
#endif
	int fh;
	char tmpStr[256];
	char tmpBuf[256];
	unsigned short tmpNum[8];
	int dnsMode;
	int dnsforward;
	FILE *fp=NULL;

#ifdef CONFIG_IPV6_CE_ROUTER_SUPPORT
	addr6CfgParam_t ula_prefix;
#endif
	if ( !apmib_get(MIB_IPV6_RADVD_PARAM,(void *)&radvdCfgParam)){
		printf("get MIB_IPV6_RADVD_PARAM failed\n");
		return;  
	}

	if(!radvdCfgParam.enabled)
		return;

#ifdef CONFIG_SIXRD_SUPPORT
	int val;

	if(!apmib_get(MIB_IPV6_ORIGIN_TYPE,&val)){
		fprintf(stderr, "Read MIB_IPV6_ORIGIN_TYPE Error\n");
		return -1;			
	}
	if(val == 2) //if 6rd, radvd start with 6rd prefix delegation
		goto start_radvd;
#endif

#ifdef CONFIG_IPV6_CE_ROUTER_SUPPORT
	if(radvdCfgParam.interface.prefix[0].prefix_mode == 1)
		return;
#endif

#if defined(CONFIG_IPV6_CE_ROUTER_SUPPORT)
    dhcp6sCfgParam_t  dhcp6sCfgParam;
    int dhcp6s_iana_support = 0;
		if ( !apmib_get(MIB_IPV6_DHCPV6S_PARAM,(void *)&dhcp6sCfgParam)){
			fprintf(stderr,"get MIB_IPV6_DHCPV6S_PARAM failed\n");
			return;  
		}
        dhcp6s_iana_support= dhcp6sCfgParam.enabled;
#endif
	if(!isFileExist(RADVD_CONF_FILE)){
		/*create config file*/
//		printf("%s-%d: create configure file: %s.\n", __FUNCTION__, __LINE__, RADVD_CONF_FILE);
		fh = open(RADVD_CONF_FILE, O_RDWR|O_CREAT|O_TRUNC, S_IRWXO|S_IRWXG);	
		if (fh < 0) {
			fprintf(stderr, "Create %s file error!\n", RADVD_CONF_FILE);
			return;
		}
		printf("create radvd.conf\n");
		sprintf(tmpStr, "interface %s\n", radvdCfgParam.interface.Name);
		write(fh, tmpStr, strlen(tmpStr));
		sprintf(tmpStr, "{\n");
		write(fh, tmpStr, strlen(tmpStr));
		sprintf(tmpStr, "AdvSendAdvert on;\n");
		write(fh, tmpStr, strlen(tmpStr));
		sprintf(tmpStr, "MaxRtrAdvInterval %d;\n", radvdCfgParam.interface.MaxRtrAdvInterval);
		write(fh, tmpStr, strlen(tmpStr));
		sprintf(tmpStr, "MinRtrAdvInterval %d;\n", radvdCfgParam.interface.MinRtrAdvInterval);
		write(fh, tmpStr, strlen(tmpStr));
		sprintf(tmpStr, "MinDelayBetweenRAs %d;\n", radvdCfgParam.interface.MinDelayBetweenRAs);
		write(fh, tmpStr, strlen(tmpStr));
		if(radvdCfgParam.interface.AdvManagedFlag > 0) {
#if defined(CONFIG_IPV6_CE_ROUTER_SUPPORT)
            if(dhcp6s_iana_support)
			{
#endif
			sprintf(tmpStr, "AdvManagedFlag on;\n");
			write(fh, tmpStr, strlen(tmpStr));			
#if defined(CONFIG_IPV6_CE_ROUTER_SUPPORT)
			}else{
				sprintf(tmpStr, "AdvManagedFlag off;\n");
				write(fh, tmpStr, strlen(tmpStr));			
			}
#endif
		}
		if(radvdCfgParam.interface.AdvOtherConfigFlag > 0){
			sprintf(tmpStr, "AdvOtherConfigFlag on;\n");
			write(fh, tmpStr, strlen(tmpStr));	
		}
		sprintf(tmpStr, "AdvLinkMTU %d;\n", radvdCfgParam.interface.AdvLinkMTU);
		write(fh, tmpStr, strlen(tmpStr));
		sprintf(tmpStr, "AdvReachableTime %u;\n", radvdCfgParam.interface.AdvReachableTime);
		write(fh, tmpStr, strlen(tmpStr));
		sprintf(tmpStr, "AdvRetransTimer %u;\n", radvdCfgParam.interface.AdvRetransTimer);
		write(fh, tmpStr, strlen(tmpStr));
		sprintf(tmpStr, "AdvCurHopLimit %d;\n", radvdCfgParam.interface.AdvCurHopLimit);
		write(fh, tmpStr, strlen(tmpStr));
		sprintf(tmpStr, "AdvDefaultLifetime %d;\n", radvdCfgParam.interface.AdvDefaultLifetime);
		write(fh, tmpStr, strlen(tmpStr));
		sprintf(tmpStr, "AdvDefaultPreference %s;\n", radvdCfgParam.interface.AdvDefaultPreference);
		write(fh, tmpStr, strlen(tmpStr));
		if(radvdCfgParam.interface.AdvSourceLLAddress > 0) {
			sprintf(tmpStr, "AdvSourceLLAddress on;\n");
			write(fh, tmpStr, strlen(tmpStr));			
		}		
		if(radvdCfgParam.interface.UnicastOnly > 0){
			sprintf(tmpStr, "UnicastOnly on;\n");
			write(fh, tmpStr, strlen(tmpStr));	
		}
		

		/*prefix 1*/
		if(radvdCfgParam.interface.prefix[0].enabled > 0){
#ifdef CONFIG_IPV6_CE_ROUTER_SUPPORT
			if(radvdCfgParam.interface.prefix[0].prefix_mode == 2)
			{
				if ( !apmib_get(MIB_IPV6_ADDR_ULA_PARAM,(void *)&ula_prefix))
				{
					fprintf(stderr, "get mib %d error!\n", MIB_IPV6_ADDR_ULA_PARAM);
					return;
				}

				sprintf(tmpStr, "prefix %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/64\n",
						ula_prefix.addrIPv6[0],
						ula_prefix.addrIPv6[1],
						ula_prefix.addrIPv6[2],
						ula_prefix.addrIPv6[3],
						0,
						0,
						0,
						0);
				write(fh, tmpStr, strlen(tmpStr));
			}
			else if(radvdCfgParam.interface.prefix[0].prefix_mode == 0)
#endif
			{
			memcpy(tmpNum,radvdCfgParam.interface.prefix[0].Prefix, sizeof(radvdCfgParam.interface.prefix[0].Prefix));
			sprintf(tmpBuf, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x", tmpNum[0], tmpNum[1], 
				tmpNum[2], tmpNum[3], tmpNum[4], tmpNum[5],tmpNum[6],tmpNum[7]);
			strcat(tmpBuf, "\0");
			sprintf(tmpStr, "prefix %s/%d\n", tmpBuf, radvdCfgParam.interface.prefix[0].PrefixLen);			
			write(fh, tmpStr, strlen(tmpStr));
			}
			sprintf(tmpStr, "{\n");
			write(fh, tmpStr, strlen(tmpStr));
			if(radvdCfgParam.interface.prefix[0].AdvOnLinkFlag > 0){
				sprintf(tmpStr, "AdvOnLink on;\n");
				write(fh, tmpStr, strlen(tmpStr));					
			}
			if(radvdCfgParam.interface.prefix[0].AdvAutonomousFlag > 0){
				sprintf(tmpStr, "AdvAutonomous on;\n");
				write(fh, tmpStr, strlen(tmpStr));					
			}
			sprintf(tmpStr, "AdvValidLifetime %u;\n", radvdCfgParam.interface.prefix[0].AdvValidLifetime);
			write(fh, tmpStr, strlen(tmpStr));					
			sprintf(tmpStr, "AdvPreferredLifetime %u;\n", radvdCfgParam.interface.prefix[0].AdvPreferredLifetime);
			write(fh, tmpStr, strlen(tmpStr));	
			if(radvdCfgParam.interface.prefix[0].AdvRouterAddr > 0){
				sprintf(tmpStr, "AdvRouterAddr on;\n");
				write(fh, tmpStr, strlen(tmpStr));						
			}
			if(radvdCfgParam.interface.prefix[0].if6to4[0]){
				sprintf(tmpStr, "Base6to4Interface %s;\n", radvdCfgParam.interface.prefix[0].if6to4);
				write(fh, tmpStr, strlen(tmpStr));						
			}
			sprintf(tmpStr, "};\n");
			write(fh, tmpStr, strlen(tmpStr));						
		}

		/*prefix 2*/
		if(radvdCfgParam.interface.prefix[1].enabled > 0){
			memcpy(tmpNum,radvdCfgParam.interface.prefix[1].Prefix, sizeof(radvdCfgParam.interface.prefix[1].Prefix));
			sprintf(tmpBuf, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x", tmpNum[0], tmpNum[1], 
				tmpNum[2], tmpNum[3], tmpNum[4], tmpNum[5],tmpNum[6],tmpNum[7]);
			strcat(tmpBuf, "\0");
			sprintf(tmpStr, "prefix %s/%d\n", tmpBuf, radvdCfgParam.interface.prefix[1].PrefixLen);
			write(fh, tmpStr, strlen(tmpStr));
			sprintf(tmpStr, "{\n");
			write(fh, tmpStr, strlen(tmpStr));
			if(radvdCfgParam.interface.prefix[1].AdvOnLinkFlag > 0){
				sprintf(tmpStr, "AdvOnLink on;\n");
				write(fh, tmpStr, strlen(tmpStr));					
			}
			if(radvdCfgParam.interface.prefix[1].AdvAutonomousFlag > 0){
				sprintf(tmpStr, "AdvAutonomous on;\n");
				write(fh, tmpStr, strlen(tmpStr));					
			}
			sprintf(tmpStr, "AdvValidLifetime %u;\n", radvdCfgParam.interface.prefix[1].AdvValidLifetime);
			write(fh, tmpStr, strlen(tmpStr));					
			sprintf(tmpStr, "AdvPreferredLifetime %u;\n", radvdCfgParam.interface.prefix[1].AdvPreferredLifetime);
			write(fh, tmpStr, strlen(tmpStr));	
			if(radvdCfgParam.interface.prefix[1].AdvRouterAddr > 0){
				sprintf(tmpStr, "AdvRouterAddr on;\n");
				write(fh, tmpStr, strlen(tmpStr));						
			}
			if(radvdCfgParam.interface.prefix[1].if6to4[0]){
				sprintf(tmpStr, "Base6to4Interface %s;\n", radvdCfgParam.interface.prefix[1].if6to4);
				write(fh, tmpStr, strlen(tmpStr));						
			}
			sprintf(tmpStr, "};\n");
			write(fh, tmpStr, strlen(tmpStr));						
		}

		if(radvdCfgParam.interface.prefix[0].if6to4[0] ||
		   radvdCfgParam.interface.prefix[1].if6to4[0])
		{
			sprintf(tmpStr, "route 2000::/3\n");
			write(fh, tmpStr, strlen(tmpStr));
			sprintf(tmpStr, "{\n");
			write(fh, tmpStr, strlen(tmpStr));
			sprintf(tmpStr, "AdvRoutePreference medium;\n");
			write(fh, tmpStr, strlen(tmpStr));
			sprintf(tmpStr, "AdvRouteLifetime 1800;\n");
			write(fh, tmpStr, strlen(tmpStr));
			sprintf(tmpStr, "};\n");
			write(fh, tmpStr, strlen(tmpStr));
		}

#ifdef CONFIG_IPV6_CE_ROUTER_SUPPORT
		if(radvdCfgParam.interface.prefix[0].prefix_mode == 2)
		{
			sprintf(tmpStr, "route %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/64\n",
					ula_prefix.addrIPv6[0],
					ula_prefix.addrIPv6[1],
					ula_prefix.addrIPv6[2],
					ula_prefix.addrIPv6[3],
					0,
					0,
					0,
					0);
			write(fh, tmpStr, strlen(tmpStr));
			sprintf(tmpStr, "{\n");
			write(fh, tmpStr, strlen(tmpStr));
			sprintf(tmpStr, "AdvRoutePreference medium;\n");
			write(fh, tmpStr, strlen(tmpStr));
			sprintf(tmpStr, "AdvRouteLifetime 1800;\n");
			write(fh, tmpStr, strlen(tmpStr));
			sprintf(tmpStr, "};\n");
			write(fh, tmpStr, strlen(tmpStr));
		}
#endif

		
#ifdef SUPPORT_RDNSS_OPTION
		//add RDNSS 
		apmib_get(MIB_IPV6_DNS_AUTO, (void *)&dnsMode);		
		if(dnsMode==0)  //Set DNS Manually 
		{
			addr6CfgParam_t addr6_dns;
			
			apmib_get(MIB_IPV6_ADDR_DNS_PARAM,  (void *)&addr6_dns);
				
			snprintf(tmpBuf, sizeof(tmpBuf), "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x", 
			addr6_dns.addrIPv6[0], addr6_dns.addrIPv6[1], addr6_dns.addrIPv6[2], addr6_dns.addrIPv6[3], 
			addr6_dns.addrIPv6[4], addr6_dns.addrIPv6[5], addr6_dns.addrIPv6[6], addr6_dns.addrIPv6[7]);

			if(strstr(tmpBuf, "0000:0000:0000:0000:0000:0000:0000:0000")==NULL)	
			{
				//add RDNSS 
				sprintf(tmpStr, "RDNSS %s\n",tmpBuf);
				write(fh, tmpStr, strlen(tmpStr));
				sprintf(tmpStr, "{\n");
				write(fh, tmpStr, strlen(tmpStr));
				sprintf(tmpStr, "AdvRDNSSLifetime %d;\n", radvdCfgParam.interface.MaxRtrAdvInterval);
				write(fh, tmpStr, strlen(tmpStr));
				sprintf(tmpStr, "};\n");
				write(fh, tmpStr, strlen(tmpStr));
			}			
		}
		else
		{
#ifdef	CONFIG_IPV6_DNSV6_MODE_SUPPORT
	apmib_get(MIB_IPV6_DNS_FORWARD, (void *)&dnsforward);	
	if(dnsforward==0)	 //dns relay mode
	{
		addr6CfgParam_t addr6_dns;
		if(get_linklocal_addr(&addr6_dns,radvdCfgParam.interface.Name)>0)
		{
			snprintf(tmpBuf, sizeof(tmpBuf), "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x", 
			addr6_dns.addrIPv6[0], addr6_dns.addrIPv6[1], addr6_dns.addrIPv6[2], addr6_dns.addrIPv6[3], 
			addr6_dns.addrIPv6[4], addr6_dns.addrIPv6[5], addr6_dns.addrIPv6[6], addr6_dns.addrIPv6[7]);
			if(strstr(tmpBuf, "0000:0000:0000:0000:0000:0000:0000:0000")==NULL) 
			{
				//add RDNSS 
				sprintf(tmpStr, "RDNSS %s\n",tmpBuf);
				write(fh, tmpStr, strlen(tmpStr));
				sprintf(tmpStr, "{\n");
				write(fh, tmpStr, strlen(tmpStr));
				sprintf(tmpStr, "AdvRDNSSLifetime %d;\n", radvdCfgParam.interface.MaxRtrAdvInterval);
				write(fh, tmpStr, strlen(tmpStr));
				sprintf(tmpStr, "};\n");
				write(fh, tmpStr, strlen(tmpStr));
			}
		}
	}
#else
	if(isFileExist(DNSV6_ADDR_FILE))
	{
		if((fp=fopen("/var/dns6.conf","r"))!=NULL)
		{				
			memset(tmpStr, 0, sizeof(tmpStr));
			while(fgets(tmpBuf, sizeof(tmpBuf), fp))
			{
				tmpBuf[strlen(tmpBuf)-1]=0;
				strcat(tmpStr, tmpBuf+strlen("nameserver")+1);
				strcat(tmpStr, " ");					
			}
			if(strlen(tmpStr>1)==0)
			{
				tmpStr[strlen(tmpStr)-1]=0;				
				sprintf(tmpBuf, "RDNSS %s\n",tmpStr);
				write(fh, tmpBuf, strlen(tmpBuf));
				sprintf(tmpBuf, "{\n");
				write(fh, tmpBuf, strlen(tmpStr));
				sprintf(tmpBuf, "AdvRDNSSLifetime %d;\n", radvdCfgParam.interface.MaxRtrAdvInterval);
				write(fh, tmpBuf, strlen(tmpBuf));
				sprintf(tmpBuf, "};\n");
				write(fh, tmpBuf, strlen(tmpBuf));	
			}
			fclose(fp);	
		}
	}
#endif
	}
#endif
#ifdef SUPPORT_DNSSL_OPTION
		//add DNSSL
		memset(tmpBuf, 0, sizeof(tmpBuf));
		apmib_get(MIB_DOMAIN_NAME, (void *)tmpBuf);	
		if(strlen(tmpBuf)>0)
		{
			sprintf(tmpStr, "DNSSL %s.com %s.com.cn\n", tmpBuf, tmpBuf);
			write(fh, tmpStr, strlen(tmpStr));
			sprintf(tmpStr, "{\n");
			write(fh, tmpStr, strlen(tmpStr));
			sprintf(tmpStr, "AdvDNSSLLifetime %d;\n", radvdCfgParam.interface.MaxRtrAdvInterval);
			write(fh, tmpStr, strlen(tmpStr));
			sprintf(tmpStr, "};\n");
			write(fh, tmpStr, strlen(tmpStr));		
		}
#endif

		sprintf(tmpStr, "};\n");
		write(fh, tmpStr, strlen(tmpStr));

#ifdef CONFIG_APP_RADVD_WAN
			if (apmib_get(MIB_IPV6_RADVD_PARAM_WAN,(void *)&radvdCfgParam_wan)&& radvdCfgParam_wan.enabled)
			{
				sprintf(tmpStr, "interface %s\n", radvdCfgParam_wan.interface.Name);
				write(fh, tmpStr, strlen(tmpStr));
				sprintf(tmpStr, "{\n");
				write(fh, tmpStr, strlen(tmpStr));
				sprintf(tmpStr, "AdvSendAdvert on;\n");
				write(fh, tmpStr, strlen(tmpStr));
				sprintf(tmpStr, "MaxRtrAdvInterval %d;\n", radvdCfgParam_wan.interface.MaxRtrAdvInterval);
				write(fh, tmpStr, strlen(tmpStr));
				sprintf(tmpStr, "MinRtrAdvInterval %d;\n", radvdCfgParam_wan.interface.MinRtrAdvInterval);
				write(fh, tmpStr, strlen(tmpStr));
				sprintf(tmpStr, "MinDelayBetweenRAs %d;\n", radvdCfgParam_wan.interface.MinDelayBetweenRAs);
				write(fh, tmpStr, strlen(tmpStr));
				if(radvdCfgParam_wan.interface.AdvManagedFlag > 0) {
					sprintf(tmpStr, "AdvManagedFlag on;\n");
					write(fh, tmpStr, strlen(tmpStr));			
				}
				if(radvdCfgParam_wan.interface.AdvOtherConfigFlag > 0){
					sprintf(tmpStr, "AdvOtherConfigFlag on;\n");
					write(fh, tmpStr, strlen(tmpStr));	
				}
				sprintf(tmpStr, "AdvLinkMTU %d;\n", radvdCfgParam_wan.interface.AdvLinkMTU);
				write(fh, tmpStr, strlen(tmpStr));
				sprintf(tmpStr, "AdvReachableTime %u;\n", radvdCfgParam_wan.interface.AdvReachableTime);
				write(fh, tmpStr, strlen(tmpStr));
				sprintf(tmpStr, "AdvRetransTimer %u;\n", radvdCfgParam_wan.interface.AdvRetransTimer);
				write(fh, tmpStr, strlen(tmpStr));
				sprintf(tmpStr, "AdvCurHopLimit %d;\n", radvdCfgParam_wan.interface.AdvCurHopLimit);
				write(fh, tmpStr, strlen(tmpStr));
				sprintf(tmpStr, "AdvDefaultLifetime %d;\n", radvdCfgParam_wan.interface.AdvDefaultLifetime);
				write(fh, tmpStr, strlen(tmpStr));
				sprintf(tmpStr, "AdvDefaultPreference %s;\n", radvdCfgParam_wan.interface.AdvDefaultPreference);
				write(fh, tmpStr, strlen(tmpStr));
				if(radvdCfgParam_wan.interface.AdvSourceLLAddress > 0) {
					sprintf(tmpStr, "AdvSourceLLAddress on;\n");
					write(fh, tmpStr, strlen(tmpStr));			
				}		
				if(radvdCfgParam_wan.interface.UnicastOnly > 0){
					sprintf(tmpStr, "UnicastOnly on;\n");
					write(fh, tmpStr, strlen(tmpStr));	
				}
				
		
				/*prefix 1*/
				if(radvdCfgParam_wan.interface.prefix[0].enabled > 0){
					{
					memcpy(tmpNum,radvdCfgParam_wan.interface.prefix[0].Prefix, sizeof(radvdCfgParam_wan.interface.prefix[0].Prefix));
					sprintf(tmpBuf, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x", tmpNum[0], tmpNum[1], 
						tmpNum[2], tmpNum[3], tmpNum[4], tmpNum[5],tmpNum[6],tmpNum[7]);
					strcat(tmpBuf, "\0");
					sprintf(tmpStr, "prefix %s/%d\n", tmpBuf, radvdCfgParam_wan.interface.prefix[0].PrefixLen); 		
					write(fh, tmpStr, strlen(tmpStr));
					}
					sprintf(tmpStr, "{\n");
					write(fh, tmpStr, strlen(tmpStr));
					if(radvdCfgParam_wan.interface.prefix[0].AdvOnLinkFlag > 0){
						sprintf(tmpStr, "AdvOnLink on;\n");
						write(fh, tmpStr, strlen(tmpStr));					
					}
					if(radvdCfgParam_wan.interface.prefix[0].AdvAutonomousFlag > 0){
						sprintf(tmpStr, "AdvAutonomous on;\n");
						write(fh, tmpStr, strlen(tmpStr));					
					}
					sprintf(tmpStr, "AdvValidLifetime %u;\n", radvdCfgParam_wan.interface.prefix[0].AdvValidLifetime);
					write(fh, tmpStr, strlen(tmpStr));					
					sprintf(tmpStr, "AdvPreferredLifetime %u;\n", radvdCfgParam_wan.interface.prefix[0].AdvPreferredLifetime);
					write(fh, tmpStr, strlen(tmpStr));	
					if(radvdCfgParam_wan.interface.prefix[0].AdvRouterAddr > 0){
						sprintf(tmpStr, "AdvRouterAddr on;\n");
						write(fh, tmpStr, strlen(tmpStr));						
					}
					if(radvdCfgParam_wan.interface.prefix[0].if6to4[0]){
						sprintf(tmpStr, "Base6to4Interface %s;\n", radvdCfgParam_wan.interface.prefix[0].if6to4);
						write(fh, tmpStr, strlen(tmpStr));						
					}
					sprintf(tmpStr, "};\n");
					write(fh, tmpStr, strlen(tmpStr));						
				}
		
				/*prefix 2*/
				if(radvdCfgParam_wan.interface.prefix[1].enabled > 0){
					memcpy(tmpNum,radvdCfgParam_wan.interface.prefix[1].Prefix, sizeof(radvdCfgParam_wan.interface.prefix[1].Prefix));
					sprintf(tmpBuf, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x", tmpNum[0], tmpNum[1], 
						tmpNum[2], tmpNum[3], tmpNum[4], tmpNum[5],tmpNum[6],tmpNum[7]);
					strcat(tmpBuf, "\0");
					sprintf(tmpStr, "prefix %s/%d\n", tmpBuf, radvdCfgParam_wan.interface.prefix[1].PrefixLen);
					write(fh, tmpStr, strlen(tmpStr));
					sprintf(tmpStr, "{\n");
					write(fh, tmpStr, strlen(tmpStr));
					if(radvdCfgParam_wan.interface.prefix[1].AdvOnLinkFlag > 0){
						sprintf(tmpStr, "AdvOnLink on;\n");
						write(fh, tmpStr, strlen(tmpStr));					
					}
					if(radvdCfgParam_wan.interface.prefix[1].AdvAutonomousFlag > 0){
						sprintf(tmpStr, "AdvAutonomous on;\n");
						write(fh, tmpStr, strlen(tmpStr));					
					}
					sprintf(tmpStr, "AdvValidLifetime %u;\n", radvdCfgParam_wan.interface.prefix[1].AdvValidLifetime);
					write(fh, tmpStr, strlen(tmpStr));					
					sprintf(tmpStr, "AdvPreferredLifetime %u;\n", radvdCfgParam_wan.interface.prefix[1].AdvPreferredLifetime);
					write(fh, tmpStr, strlen(tmpStr));	
					if(radvdCfgParam_wan.interface.prefix[1].AdvRouterAddr > 0){
						sprintf(tmpStr, "AdvRouterAddr on;\n");
						write(fh, tmpStr, strlen(tmpStr));						
					}
					if(radvdCfgParam_wan.interface.prefix[1].if6to4[0]){
						sprintf(tmpStr, "Base6to4Interface %s;\n", radvdCfgParam_wan.interface.prefix[1].if6to4);
						write(fh, tmpStr, strlen(tmpStr));						
					}
					sprintf(tmpStr, "};\n");
					write(fh, tmpStr, strlen(tmpStr));						
				}
		
				if(radvdCfgParam_wan.interface.prefix[0].if6to4[0] ||
				   radvdCfgParam_wan.interface.prefix[1].if6to4[0])
				{
					sprintf(tmpStr, "route 2000::/3\n");
					write(fh, tmpStr, strlen(tmpStr));
					sprintf(tmpStr, "{\n");
					write(fh, tmpStr, strlen(tmpStr));
					sprintf(tmpStr, "AdvRoutePreference medium;\n");
					write(fh, tmpStr, strlen(tmpStr));
					sprintf(tmpStr, "AdvRouteLifetime 1800;\n");
					write(fh, tmpStr, strlen(tmpStr));
					sprintf(tmpStr, "};\n");
					write(fh, tmpStr, strlen(tmpStr));
				}
				sprintf(tmpStr, "};\n");
				write(fh, tmpStr, strlen(tmpStr));
			}
#endif
		
		close(fh);		
	}


start_radvd:
	printf("%s %d: start radvd...\n", __FUNCTION__, __LINE__);
	if(isFileExist(RADVD_PID_FILE)){
		if(radvdCfgParam.enabled == 1) {
			system("killall radvd 2> /dev/null");
			system("rm -f /var/run/radvd.pid 2> /dev/null");
			system("echo 1 > /proc/sys/net/ipv6/conf/all/forwarding");
			system("radvd -C /var/radvd.conf");
				
		} else {	
			system("killall radvd 2> /dev/null");
			system("rm -f /var/run/radvd.pid 2> /dev/null");
		}
	} else{
		if(radvdCfgParam.enabled == 1) {
			system("echo 1 > /proc/sys/net/ipv6/conf/all/forwarding");
			system("radvd -C /var/radvd.conf");	
		}
	}
	
	return;
}

void set_ecmh()
{	
		
	if(isFileExist(ECMH_PID_FILE)){
		system("killall ecmh 2> /dev/null");		
	}
	
	system("ecmh");		
	
	return;
}
void start_mldproxy(char *wan_iface, char *lan_iface)
{
	int intValue=0;
	int opmode=-1;
	apmib_get(MIB_MLD_PROXY_DISABLED, (void *)&intValue);
	RunSystemCmd(NULL_FILE, "killall", "-9", "mldproxy", NULL_STR);
	apmib_get(MIB_OP_MODE,(void *)&opmode);
	
	if(intValue==0) {
		//RunSystemCmd(NULL_FILE, "mldproxy", wan_iface, lan_iface, NULL_STR);
		if(opmode==GATEWAY_MODE){
			RunSystemCmd(NULL_FILE, "mldproxy", wan_iface, lan_iface, NULL_STR);
		}	
		else if(opmode==WISP_MODE){
			//RunSystemCmd(NULL_FILE, "mldproxy", "wlan0", lan_iface, NULL_STR);
		}	
		else if(opmode==BRIDGE_MODE){
		}
				
	}
	
}


void set_basicv6() 
{
	addrIPv6CfgParam_t addrIPv6CfgParam;
	char tmpStr[256];
	
	if ( !apmib_get(MIB_IPV6_ADDR_PARAM,(void *)&addrIPv6CfgParam)){
		printf("get MIB_IPV6_ADDR_PARAM failed\n");
		return;        
	}
	if(addrIPv6CfgParam.enabled == 1) {
		/*
		/bin/ifconfig br0 $ADDR1/$PREFIX1
        /bin/ifconfig eth1 $ADDR2/$PREFIX2
        */
		sprintf(tmpStr,"/bin/ifconfig br0 %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%d",
			addrIPv6CfgParam.addrIPv6[0][0],addrIPv6CfgParam.addrIPv6[0][1],addrIPv6CfgParam.addrIPv6[0][2],addrIPv6CfgParam.addrIPv6[0][3],
			addrIPv6CfgParam.addrIPv6[0][4],addrIPv6CfgParam.addrIPv6[0][5],addrIPv6CfgParam.addrIPv6[0][6],addrIPv6CfgParam.addrIPv6[0][7],
			addrIPv6CfgParam.prefix_len[0]);
		//RunSystemCmd(NULL_FILE, tmpStr, NULL_STR);
		system(tmpStr);
		//printf("the cmd for ipv6 is %s\n", tmpStr);
		
		sprintf(tmpStr,"/bin/ifconfig eth1 %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%d",
			addrIPv6CfgParam.addrIPv6[1][0],addrIPv6CfgParam.addrIPv6[1][1],addrIPv6CfgParam.addrIPv6[1][2],addrIPv6CfgParam.addrIPv6[1][3],
			addrIPv6CfgParam.addrIPv6[1][4],addrIPv6CfgParam.addrIPv6[1][5],addrIPv6CfgParam.addrIPv6[1][6],addrIPv6CfgParam.addrIPv6[1][7],
			addrIPv6CfgParam.prefix_len[1]);
		//RunSystemCmd(NULL_FILE, tmpStr, NULL_STR);
		system(tmpStr);
		//printf("the cmd for ipv6 is %s\n", tmpStr);
	}



	
}

void set_dhcp6c()
{
		
		char tmpStr[256];
		int fh;
		int val;
#ifdef CONFIG_DSLITE_SUPPORT
		int enable_dslite;
#endif
		FILE *fp = NULL;
		int pid=-1;
		struct duid_t dhcp6c_duid;
		uint16 len;
		char filename[64];
		char pidname[64];
		struct sockaddr hwaddr={0};
		int dhcpMode=0;
		int dhcpPdEnable=0,dhcpRapidCommitEnable=0;
		int opmode=-1;
		char br_interface[16]={0};
		sprintf(br_interface,"%s","br0");
		if ( !apmib_get(MIB_OP_MODE,(void *)&opmode)){
			fprintf(stderr, "get mib %d error!\n", MIB_OP_MODE);
			return;	
		}
		if ( !apmib_get(MIB_IPV6_DHCP_MODE,(void *)&dhcpMode)){
			fprintf(stderr, "get mib %d error!\n", MIB_IPV6_DHCP_MODE);
			return;
		}	
		if(dhcpMode==IPV6_DHCP_STATELESS)
		{
			printf("wan Stateless Address Auto Configuration!\n");
			sprintf(tmpStr,"echo 2 >/proc/sys/net/ipv6/conf/%s/accept_ra",wan_interface);
			system(tmpStr);
			return;
		}
		printf("wan Stateful Address Auto Configuration!\n");
#ifdef TR181_SUPPORT
		{
			char value[64]={0};
			if(apmib_get(MIB_IPV6_DHCPC_IFACE,(void*)&value)==0)
			{
				printf("get MIB_IPV6_DHCPC_IFACE fail!\n");
				return -1;
			}
			if(value[0])
				strcpy(wan_interface,value);
		}
#endif

 		/*for test use fixed duid of 0003000100e04c8196c9*/
		if(!isFileExist(DHCP6C_DUID_FILE)){
			/*create config file*/
			fp=fopen(DHCP6C_DUID_FILE,"w+");
			if(fp==NULL){
				fprintf(stderr, "Create %s file error!\n", DHCP6C_DUID_FILE);
				return;
			}
			
			dhcp6c_duid.duid_type=htons(0x0003);
			dhcp6c_duid.hw_type=htons(0x0001);
			//if ( getInAddr(wan_interface, HW_ADDR_T, (void *)&hwaddr )==0)
			if ( getInAddr("eth1", HW_ADDR_T, (void *)&hwaddr )==0)
			{
					fprintf(stderr, "Read hwaddr Error\n");
					return;	
			}
			memcpy(dhcp6c_duid.mac,hwaddr.sa_data,6);

			len=sizeof(dhcp6c_duid);
			if ((fwrite(&len, sizeof(len), 1, fp)) != 1) {
				fprintf(stderr, "write %s file error!\n", DHCP6C_DUID_FILE);
			}
			else if(fwrite(&dhcp6c_duid,sizeof(dhcp6c_duid),1,fp)!=1)
				fprintf(stderr, "write %s file error!\n", DHCP6C_DUID_FILE);
			
			fclose(fp);
		}

		if ( !apmib_get(MIB_IPV6_DHCP_PD_ENABLE,(void *)&dhcpPdEnable)){
			fprintf(stderr, "get mib %d error!\n", MIB_IPV6_DHCP_PD_ENABLE);
			return;
		}	

		
		sprintf(filename,DHCP6C_CONF_FILE);
		if(isFileExist(filename) == 0)
		/*create config file*/
			fh = open(filename, O_RDWR|O_CREAT|O_TRUNC, S_IRWXO|S_IRWXG);	
		else
			fh = open(filename, O_RDWR|O_TRUNC, S_IRWXO|S_IRWXG);	
	
		if (fh < 0){
			fprintf(stderr, "Create %s file error!\n", filename);
			return;
		}

		if(!apmib_get(MIB_IPV6_LINK_TYPE,(void *)&val)){	
			fprintf(stderr, "get mib %d error!\n", MIB_IPV6_LINK_TYPE);
			close(fh);
			return;			
		}
			
		if(val == IPV6_LINKTYPE_PPP)
			sprintf(wan_interface,"ppp0");
	if(opmode == BRIDGE_MODE)
		sprintf(tmpStr, "interface %s {\n",br_interface);				
	else
		sprintf(tmpStr, "interface %s {\n",wan_interface);				
		write(fh, tmpStr, strlen(tmpStr));
		if(dhcpPdEnable){
			sprintf(tmpStr, "	send ia-pd %d;\n",100);
			write(fh, tmpStr, strlen(tmpStr));
		}
#ifdef TR181_SUPPORT
		if(!apmib_get(MIB_IPV6_DHCPC_REQUEST_ADDR,(void *)&val)){	
			fprintf(stderr, "get mib MIB_IPV6_DHCPC_REQUEST_ADDR error!\n");
			return;			
		}
		if(val)
		{
#endif
		sprintf(tmpStr, "	send ia-na %d;\n",101);
		write(fh, tmpStr, strlen(tmpStr));
#ifdef TR181_SUPPORT
		}
#endif

		if(!apmib_get(MIB_IPV6_DHCP_RAPID_COMMIT_ENABLE,(void *)&dhcpRapidCommitEnable)){	
			fprintf(stderr, "get mib %d error!\n", MIB_IPV6_LINK_TYPE);
			close(fh);
			return;			
		}
		if(dhcpRapidCommitEnable){
			sprintf(tmpStr, "	send rapid-commit;\n");
			write(fh, tmpStr, strlen(tmpStr));	
		}
#ifndef TR181_SUPPORT
		/*dns*/
		sprintf(tmpStr, "	request domain-name-servers;\n");
		write(fh, tmpStr, strlen(tmpStr));
#ifdef	CONFIG_IPV6_DNSV6_MODE_SUPPORT
		sprintf(tmpStr, "	request domain-name;\n");
		write(fh, tmpStr, strlen(tmpStr));
#endif
#else
		{
			int i=0;
			DHCPV6C_SENDOPT_T entryTmp={0};
			for(i=1;i<=IPV6_DHCPC_SENDOPT_NUM;i++)
			{
				*((char *)&entryTmp) = (char)i;
				if ( !apmib_get(MIB_IPV6_DHCPC_SENDOPT_TBL, (void *)&entryTmp)){
					printf("get MIB_IPV6_DHCPC_SENDOPT_TBL fail!\n");
					return;
				}
				if(entryTmp.enable)
				{
					bzero(tmpStr,sizeof(tmpStr));
					switch(entryTmp.tag)
					{
						case 21:
							sprintf(tmpStr, "	request sip-domain-name;\n");
							break;
						case 22:
							sprintf(tmpStr, "	request sip-server-address;\n");
							break;
						case 23:
							sprintf(tmpStr, "	request domain-name-servers;\n");
							break;
						case 27:
							sprintf(tmpStr, "	request nis-server-address;\n");
							break;
						case 28:
							sprintf(tmpStr, "	request nisp-server-address;\n");
							break;
						case 29:
							sprintf(tmpStr, "	request nis-domain-name;\n");
							break;
						case 30:
							sprintf(tmpStr, "	request nisp-domain-name;\n");
							break;
						case 33:
							sprintf(tmpStr, "	request bcmcs-domain-name;\n");
							break;
						case 34:
							sprintf(tmpStr, "	request bcmcs-server-address;\n");
							break;
						default:
							break;
					}
					if(tmpStr[0])
						write(fh, tmpStr, strlen(tmpStr));
				}
			}
		}
#endif

#ifdef CONFIG_DSLITE_SUPPORT
		if(!apmib_get(MIB_WAN_DHCP,&enable_dslite)){	
			fprintf(stderr, "get mib %d error!\n", MIB_WAN_DHCP);
			close(fh);
			return;
		}

		if(enable_dslite == 17)
		{
			if(!apmib_get(MIB_DSLITE_MODE,&val)){	
				fprintf(stderr, "get mib %d error!\n", MIB_DSLITE_MODE);
				close(fh);
				return;
			}
			if(val == 0)
			{
				sprintf(tmpStr, "	request address-family-transition-router;\n");
				write(fh, tmpStr, strlen(tmpStr));
			}
//			sprintf(tmpStr, "	script \"%s\";\n", DSLITE_SCRIPT);
//			write(fh, tmpStr, strlen(tmpStr));
		}
#endif

		sprintf(tmpStr, "	script \"%s\";\n", DHCP6C_SCRIPT);
		write(fh, tmpStr, strlen(tmpStr));
//		system("cp /bin/dhcp6cRcv.sh /var/dhcp6cRcv.sh");
		sprintf(tmpStr, "};\n\n");
		write(fh, tmpStr, strlen(tmpStr));

		if(dhcpPdEnable){
			sprintf(tmpStr, "id-assoc pd %d {\n",100);
			write(fh, tmpStr, strlen(tmpStr));	
#ifdef TR181_SUPPORT
			sprintf(tmpStr, "	suggest-t {\n");
			write(fh, tmpStr, strlen(tmpStr));
			if(!apmib_get(MIB_IPV6_DHCPC_SUGGESTEDT1,(void *)&val)){	
				fprintf(stderr, "get mib MIB_IPV6_DHCPC_SUGGESTEDT1 error!\n");
				return;			
			}
			if(val >0)
			{
				sprintf(tmpStr, "		t1 %d;\n", val);
				write(fh, tmpStr, strlen(tmpStr));
			}
			if(!apmib_get(MIB_IPV6_DHCPC_SUGGESTEDT2,(void *)&val)){	
				fprintf(stderr, "get mib MIB_IPV6_DHCPC_SUGGESTEDT2 error!\n");
				return;			
			}
			if(val > 0)
			{
				sprintf(tmpStr, "		t2 %d;\n", val);
				write(fh, tmpStr, strlen(tmpStr));
			}
			sprintf(tmpStr, "	};\n");
			write(fh, tmpStr, strlen(tmpStr));
#endif
			sprintf(tmpStr, "		prefix-interface br0 {\n");
			write(fh, tmpStr, strlen(tmpStr));				
			sprintf(tmpStr, "			sla-id 0;\n");
			write(fh, tmpStr, strlen(tmpStr));
			sprintf(tmpStr, "			sla-len 0;\n");
			write(fh, tmpStr, strlen(tmpStr));
			sprintf(tmpStr, "		};\n");
			write(fh, tmpStr, strlen(tmpStr));
			sprintf(tmpStr, "};\n\n");
			write(fh, tmpStr, strlen(tmpStr));	
		}						
			
		/*ia-na*/
		sprintf(tmpStr, "id-assoc na %d {\n",101);
		write(fh, tmpStr, strlen(tmpStr));
#ifdef TR181_SUPPORT
		sprintf(tmpStr, "	suggest-t {\n");
		write(fh, tmpStr, strlen(tmpStr));

		if(!apmib_get(MIB_IPV6_DHCPC_SUGGESTEDT1,(void *)&val)){	
			fprintf(stderr, "get mib MIB_IPV6_DHCPC_SUGGESTEDT1 error!\n");
			return;			
		}
		if(val >0)
		{
			sprintf(tmpStr, "		t1 %d;\n", val);
			write(fh, tmpStr, strlen(tmpStr));
		}
		if(!apmib_get(MIB_IPV6_DHCPC_SUGGESTEDT2,(void *)&val)){	
			fprintf(stderr, "get mib MIB_IPV6_DHCPC_SUGGESTEDT2 error!\n");
			return;			
		}
		if(val > 0)
		{
			sprintf(tmpStr, "		t2 %d;\n", val);
			write(fh, tmpStr, strlen(tmpStr));
		}
		sprintf(tmpStr, "	};\n");
		write(fh, tmpStr, strlen(tmpStr));
#endif
		sprintf(tmpStr, "};\n\n");
		write(fh, tmpStr, strlen(tmpStr));	

		close(fh);

		/* create script dhcp6c.sh */
		sprintf(tmpStr, "cp /bin/dhcp6cRcv.sh %s", DHCP6C_SCRIPT);
		system(tmpStr);
#ifdef CONFIG_DSLITE_SUPPORT
		if(enable_dslite == 17)
		{
			sprintf(tmpStr, "echo \"sleep 1\" >> %s", DHCP6C_SCRIPT);
			system(tmpStr);
			sprintf(tmpStr, "echo \"%s\" >> %s", DSLITE_SCRIPT, DHCP6C_SCRIPT);
			system(tmpStr);
		}
#endif
		
		sprintf(pidname,DHCP6C_PID_FILE);
		if(isFileExist(pidname)){
			pid=getPid_fromFile(pidname);
			if(pid>0){
				sprintf(tmpStr, "%d", pid);
				RunSystemCmd(NULL_FILE, "kill", "-9", tmpStr, NULL_STR);
			}
			unlink(pidname);
		}	
		/*start daemon*/
		if(opmode == BRIDGE_MODE)
			sprintf(tmpStr, "dhcp6c -c %s -p %s %s ", DHCP6C_CONF_FILE,DHCP6C_PID_FILE,br_interface);
		else
		sprintf(tmpStr, "dhcp6c -c %s -p %s %s ", DHCP6C_CONF_FILE,DHCP6C_PID_FILE,wan_interface);
		/*Use system() instead of RunSystemCmd() to avoid stderr closing, 
		process itself will redirect stderr when it wants to run as deamon() */
		system(tmpStr);
		printf("%s\n",tmpStr);
	return;
}

void set_wanv6()
{
		char tmpStr[256];
		char gateway[64];
		addr6CfgParam_t	addr6_wan;
		addr6CfgParam_t addr6_gw;
		int val;
		
		if(!apmib_get(MIB_IPV6_LINK_TYPE,&val)){	
			fprintf(stderr, "get mib %d error!\n", MIB_IPV6_LINK_TYPE);
			return;			
		}

		if(val == IPV6_LINKTYPE_PPP)
			return;

		/*disable proc of forwarding to enable RA process in kernel*/
		sprintf(tmpStr,"echo 0 > /proc/sys/net/ipv6/conf/%s/forwarding 2> /dev/null",wan_interface);
		system(tmpStr);

		if(!apmib_get(MIB_IPV6_ORIGIN_TYPE,&val)){	
			fprintf(stderr, "get mib %d error!\n", MIB_IPV6_ORIGIN_TYPE);
			return;			
		}

		switch(val){
	
			case IPV6_ORIGIN_DHCP:
				system("echo 2 > /proc/sys/net/ipv6/conf/eth1/accept_ra");
#ifdef CONFIG_IPV6_CE_ROUTER_SUPPORT
				system("ipv6_manage_inet start&");
#else
				set_dhcp6c();	
#endif
#ifdef	CONFIG_IPV6_DNSV6_MODE_SUPPORT
				system("ipv6_manage_inet start&");
#endif
				break;
	
			case IPV6_ORIGIN_STATIC:
				/*ifconfig ipv6 address*/
				if ( !apmib_get(MIB_IPV6_ADDR_WAN_PARAM,(void *)&addr6_wan)){
					fprintf(stderr, "get mib %d error!\n", MIB_IPV6_ADDR_WAN_PARAM);
					return ;        
				}

				sprintf(tmpStr,"ifconfig eth1 %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%d",
					addr6_wan.addrIPv6[0],addr6_wan.addrIPv6[1],addr6_wan.addrIPv6[2],
					addr6_wan.addrIPv6[3],addr6_wan.addrIPv6[4],addr6_wan.addrIPv6[5],
					addr6_wan.addrIPv6[6],addr6_wan.addrIPv6[7],addr6_wan.prefix_len);
				system(tmpStr);

				if ( !apmib_get(MIB_IPV6_ADDR_GW_PARAM,(void *)&addr6_gw)){
					fprintf(stderr, "get mib %d error!\n", MIB_IPV6_ADDR_GW_PARAM);
					return ;
				}
				/*route -A inet6 add 3ffe:501:ffff::/64 gw fe80::0200:00ff:fe00:a0a0 dev br0*/
				sprintf(tmpStr,"route -A inet6 del default dev eth1 2> /dev/null");
				system(tmpStr);

				sprintf(gateway,"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
					addr6_gw.addrIPv6[0],addr6_gw.addrIPv6[1],addr6_gw.addrIPv6[2],
					addr6_gw.addrIPv6[3],addr6_gw.addrIPv6[4],addr6_gw.addrIPv6[5],
					addr6_gw.addrIPv6[6],addr6_gw.addrIPv6[7]);
				sprintf(tmpStr,"route -A inet6 add default gw %s dev eth1",gateway);
				system(tmpStr);
				break;

#ifdef CONFIG_SIXRD_SUPPORT
			case IPV6_ORIGIN_6RD:
#if defined(CONFIG_RTL_8198C) ||defined(CONFIG_RTL_8197F)
#if defined(CONFIG_RTL_HW_DSLITE_SUPPORT)
				system("echo 1 > /proc/6rd_hw_fw");
#endif
#endif
				break;
#endif
				
			default:
				break;
		}	

		return;			
	
}

#ifdef CONFIG_IPV6_CE_ROUTER_SUPPORT
void create_eui64(char *mac, char *eui)
{
	memcpy(eui, mac, 3);
	memcpy(eui+5, mac+3, 3);
	eui[3] = 0xFF;
	eui[4] = 0xFE;
	eui[0] ^= 2;
}
#endif

void set_lanv6()
{
	addr6CfgParam_t addr6;
	char tmpBuf[128];
	
	if ( !apmib_get(MIB_IPV6_ADDR_LAN_PARAM,&addr6)){
		fprintf(stderr, "get mib %d error!\n", MIB_IPV6_ADDR_LAN_PARAM);
		return ;        
	}
	sprintf(tmpBuf,"ifconfig br0 add %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%d 2> /dev/null",
					addr6.addrIPv6[0],addr6.addrIPv6[1],addr6.addrIPv6[2],
					addr6.addrIPv6[3],addr6.addrIPv6[4],addr6.addrIPv6[5],
					addr6.addrIPv6[6],addr6.addrIPv6[7],addr6.prefix_len);
#if defined(RTL_8367R_IPV6READY_LOGO)
	 sleep(1);
#endif
	system(tmpBuf);

#ifdef CONFIG_IPV6_CE_ROUTER_SUPPORT
	int enable_ula;
	addr6CfgParam_t ula_prefix;
	unsigned char mac[6];
	unsigned char eui[8];

	if(!apmib_get(MIB_IPV6_ULA_ENABLE,(void *)&enable_ula))
	{
		fprintf(stderr, "get mib %d error!\n", MIB_IPV6_ULA_ENABLE);
		return;
	}

	if(enable_ula)
	{
		if ( !apmib_get(MIB_IPV6_ADDR_ULA_PARAM,(void *)&ula_prefix))
		{
			fprintf(stderr, "get mib %d error!\n", MIB_IPV6_ADDR_ULA_PARAM);
			return;
		}
		if ( !apmib_get(MIB_HW_NIC0_ADDR,  (void *)mac) )
		{
			fprintf(stderr, "get mib %d error!\n", MIB_HW_NIC0_ADDR);
			return;
		}
		create_eui64(mac, eui);

		sprintf(tmpBuf,"ifconfig br0 add %04x:%04x:%04x:%04x:%02x%02x:%02x%02x:%02x%02x:%02x%02x/64 2> /dev/null",
					ula_prefix.addrIPv6[0],ula_prefix.addrIPv6[1],
					ula_prefix.addrIPv6[2],ula_prefix.addrIPv6[3],
					eui[0],eui[1],eui[2],eui[3],
					eui[4],eui[5],eui[6],eui[7]);
		system(tmpBuf);
	}
#endif
	return;
}

unsigned long do_ioctl_get_ipaddress(char *dev)
{
	struct ifreq ifr;
	int fd;
	unsigned long ip;
//	struct in_addr tmp_addr;
	       
	strcpy(ifr.ifr_name, dev);
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (ioctl(fd, SIOCGIFADDR, &ifr)) {
		perror("ioctl error");
		return 0;
	}
	close(fd);
	memcpy(&ip,ifr.ifr_addr.sa_data + 2,4);
//	tmp_addr.s_addr=ip;
//	printf("%s : %s\n", dev, inet_ntoa(tmp_addr));
	return ip;
}

int get_gateway(unsigned long *p)    
{    
	FILE *fp;    
	char buf[256]; // 128 is enough for linux    
	char iface[16];    
	unsigned long dest_addr, gate_addr;    
	*p = 0;    
	fp = fopen("/proc/net/route", "r");    
	if (fp == NULL)    
		return -1;    
	/* Skip title line */    
	fgets(buf, sizeof(buf), fp);    
	while (fgets(buf, sizeof(buf), fp)) {    
		if (sscanf(buf, "%s\t%lX\t%lX", iface, &dest_addr, &gate_addr) != 3 ||    
			dest_addr != 0)    
			continue;    
		*p = gate_addr;    
		break;    
	}
//	struct in_addr tmp_addr;
//	tmp_addr.s_addr = gate_addr;
//	printf("address is %s\n", inet_ntoa((struct in_addr)tmp_addr));

	fclose(fp);    
	return 0;    
}   


void set_6to4tunnel()
{
	tunnelCfgParam_t tunnelCfgParam;
	char tmpBuf[256];
	unsigned char wanIP[50], GW[50];
	struct in_addr tmp_addr;

	apmib_get(MIB_IPV6_TUNNEL_PARAM,(void *)&tunnelCfgParam);

	if(!tunnelCfgParam.enabled)
		return;

	//tunnel add 
	usleep(100); //wait for get wan IP and gateway
	tmp_addr.s_addr = 0;

	tmp_addr.s_addr = do_ioctl_get_ipaddress("eth1");

	if(tmp_addr.s_addr == 0)
	{
		printf("Set 6to4 tunnel fail: Can't get wan IP!\n");
		return;
	}

	sprintf(wanIP,"%s",inet_ntoa(tmp_addr));
	tmp_addr.s_addr = 0;

	get_gateway(&(tmp_addr.s_addr));
	if(tmp_addr.s_addr == 0)
	{
		printf("Set 6to4 tunnel fail: Can't get default gateway!\n");
		return;
	}

	sprintf(GW,"%s",inet_ntoa(tmp_addr));

	//add iptables
	bzero(tmpBuf,sizeof(tmpBuf));
	sprintf(tmpBuf,"iptables -A INPUT -p 41 -i eth1 -d %s -j ACCEPT",wanIP);
	system(tmpBuf);

	//create tunnel
	bzero(tmpBuf,sizeof(tmpBuf));
	sprintf(tmpBuf,"ip tunnel add tun mode sit remote any local %s",wanIP);
	system("ip tunnel del tun 2> /dev/null");
	system(tmpBuf);	

	bzero(tmpBuf,sizeof(tmpBuf));
	sprintf(tmpBuf,"ifconfig tun up");
	system(tmpBuf);

	char *p1,*p2,*p3,*p4;
	p1=strtok(wanIP,".");
	p2=strtok(NULL,".");
	p3=strtok(NULL,".");
	p4=strtok(NULL,".");

	bzero(tmpBuf,sizeof(tmpBuf));
	sprintf(tmpBuf,"ifconfig tun 2002:%02x%02x:%02x%02x::1\/16",atoi(p1),atoi(p2),atoi(p3),atoi(p4));
	system(tmpBuf);

	//br0
	bzero(tmpBuf,sizeof(tmpBuf));
	sprintf(tmpBuf,"ifconfig br0 2002:%02x%02x:%02x%02x:1::1\/64",atoi(p1),atoi(p2),atoi(p3),atoi(p4));
	system(tmpBuf);

	//add route
	bzero(tmpBuf,sizeof(tmpBuf));
	sprintf(tmpBuf,"route -A inet6 add 2000::/16 gw ::%s dev tun",GW);
	system(tmpBuf);
}

#ifdef CONFIG_DSLITE_SUPPORT
void set_dslite()
{
	int val;
	char tmpStr[256], filename[64];
	int fh;
	FILE *fp;
	char local_addr6[40], remote_addr6[40], *line = NULL;
	addr6CfgParam_t ipaddr6;
	
	if(!apmib_get(MIB_WAN_DHCP,&val)){	
		fprintf(stderr, "get mib %d error!\n", MIB_WAN_DHCP);
		return;
	}
	if(val != 17) //not DS-Lite mode
		return;

	if(!apmib_get(MIB_DSLITE_MODE,&val)){	
		fprintf(stderr, "get mib %d error!\n", MIB_DSLITE_MODE);
		return;
	}
	if(val != 1) //AFTR not set manually
		return;

	if(!apmib_get(MIB_IPV6_ORIGIN_TYPE,&val)){	
		fprintf(stderr, "get mib %d error!\n", MIB_IPV6_ORIGIN_TYPE);
		return;			
	}
	switch(val)
	{
		case IPV6_ORIGIN_DHCP:
			local_addr6[0] = '\0';
			int len, i = 0;
			while(i < 10)
			{
				fp = fopen("/var/dhcp6addr.conf", "r");
				printf("open file %d\n", i);
				if(fp != NULL)
					break;
				sleep(1); //wait until get ipv6 address from dhcp6 server
				i++;
			}
			if(fp != NULL)
			{
				if(getline(&line, &len, fp) != -1)
				{
				//	printf("address is %s\n", line);
					memcpy(local_addr6, line, 40);
					local_addr6[39] = '\0';
				}
				else
				{
					printf("Read data failed\n");
				}
				fclose(fp);
				if(line)
					free(line);
			}
			else
			{
				printf("Can't open file /var/dhcp6addr.conf\n");
				printf("Can't get IPv6 address from dhcpv6 server\n");
			}
			
			break;
	
		case IPV6_ORIGIN_STATIC:
			if ( !apmib_get(MIB_IPV6_ADDR_WAN_PARAM,(void *)&ipaddr6)){
				fprintf(stderr, "get mib %d error!\n", MIB_IPV6_ADDR_WAN_PARAM);
				return ;        
			}
			sprintf(local_addr6,"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
				ipaddr6.addrIPv6[0],ipaddr6.addrIPv6[1],ipaddr6.addrIPv6[2],
				ipaddr6.addrIPv6[3],ipaddr6.addrIPv6[4],ipaddr6.addrIPv6[5],
				ipaddr6.addrIPv6[6],ipaddr6.addrIPv6[7]);
			local_addr6[39]='\0';

			break;
		default:
			break;
	}

	/* get aftr addr */
	if ( !apmib_get(MIB_IPV6_ADDR_AFTR_PARAM,(void *)&ipaddr6)){
		fprintf(stderr, "get mib %d error!\n", MIB_IPV6_ADDR_AFTR_PARAM);
		return ; 
	}
	sprintf(remote_addr6,"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
		ipaddr6.addrIPv6[0],ipaddr6.addrIPv6[1],ipaddr6.addrIPv6[2],
		ipaddr6.addrIPv6[3],ipaddr6.addrIPv6[4],ipaddr6.addrIPv6[5],
		ipaddr6.addrIPv6[6],ipaddr6.addrIPv6[7]);
	remote_addr6[39]='\0';

	/* create /var/ds-lite.script */
	sprintf(filename,DSLITE_SCRIPT);
	if(isFileExist(filename) == 0)
		fh = open(filename, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU|S_IRWXO|S_IRWXG);	
	else
		fh = open(filename, O_RDWR|O_TRUNC, S_IRWXU|S_IRWXO|S_IRWXG);	
		
	if (fh < 0){
		fprintf(stderr, "Create %s file error!\n", filename);
		return;
	}
	
	sprintf(tmpStr, "#!/bin/sh\n\n");
	write(fh, tmpStr, strlen(tmpStr));
	sprintf(tmpStr, "ip -6 tunnel del ds-lite 2> /dev/null\n");
	write(fh, tmpStr, strlen(tmpStr));
	sprintf(tmpStr, "ip -6 tunnel add ds-lite mode ipip6 remote %s local %s dev eth1\n",
		remote_addr6, local_addr6);
	write(fh, tmpStr, strlen(tmpStr));
	sprintf(tmpStr, "ip link set ds-lite up\n");
	write(fh, tmpStr, strlen(tmpStr));
	sprintf(tmpStr, "ip addr add 192.0.0.2 peer 192.0.0.1 dev ds-lite\n");
	write(fh, tmpStr, strlen(tmpStr));
	sprintf(tmpStr, "ip route add default via 192.0.0.1 dev ds-lite\n");
	write(fh, tmpStr, strlen(tmpStr));	
	close(fh);

	if(local_addr6[0])
	{
		sprintf(tmpStr, DSLITE_SCRIPT); //run script and create IPv4 in IPv6 tunnel
		system(tmpStr);
	}
	else
	{
//		printf("Can't get local ipv6 address!\n");
	}

	RunSystemCmd("/proc/sys/net/ipv4/ip_forward", "echo", "1", NULL_STR);

	return;
}
#endif

#ifdef CONFIG_MAP_E_SUPPORT
int ffs(int x)
{
	int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}

void set_map_e()
{
//	printf("=========set MAP-E==========\n");

	int enable, mode;
	unsigned int val;
	struct in_addr pri, pub, pri_mask, pub_mask;
	addr6CfgParam_t prefix, br;
	unsigned int pri_mask_len, pub_mask_len;
	unsigned int psid;
	unsigned int psid_offset, psid_len, ea_len;
	char tmpStr[256], tmp1[16], tmp2[16];

	/* enable MAP-E */
	if(!apmib_get(MIB_MAPE_ENABLE,(void *)(&enable))){
		fprintf(stderr, "get mib %d error!\n", MIB_MAPE_ENABLE);
		return;
	}
	if(enable == 0) // 0 is disable
		return;

	/* configurate mode */
	if(!apmib_get(MIB_MAPE_MODE,(void *)(&mode))){
		fprintf(stderr, "get mib %d error!\n", MIB_MAPE_MODE);
		return;
	}
	if(mode == 0) // 0 is auto configure
		return;

	/* IPv4 private address */
	if(!apmib_get(MIB_IP_ADDR,(void *)(&pri))){
		fprintf(stderr, "get mib %d error!\n", MIB_IP_ADDR);
		return;
	}
	if(!apmib_get(MIB_SUBNET_MASK,(void *)(&pri_mask))){
		fprintf(stderr, "get mib %d error!\n", MIB_SUBNET_MASK);
		return;
	}
	memcpy(&val, &pri_mask, 4);
	pri_mask_len = 32 - ffs(val) + 1;

	/* IPv4 public address */
	if(!apmib_get(MIB_IPV4_MAPE,(void *)(&pub))){
		fprintf(stderr, "get mib %d error!\n", MIB_IPV4_MAPE);
		return;
	}
	if(!apmib_get(MIB_IPV4_MAPE_PREFIX_LEN,(void *)(&pub_mask_len))){
		fprintf(stderr, "get mib %d error!\n", MIB_IPV4_MAPE_PREFIX_LEN);
		return;
	}
	val = 0xffffffff << (32 - pub_mask_len);
	memcpy(&pub_mask, &val, 4);

	/* IPv6 prefix */
	if(!apmib_get(MIB_IPV6_MAPE_PREFIX,(void *)(&prefix))){
		fprintf(stderr, "get mib %d error!\n", MIB_IPV6_MAPE_PREFIX);
		return;
	}

	/* BR address */
	if(!apmib_get(MIB_IPV6_MAPE_BR_ADDR,(void *)(&br))){
		fprintf(stderr, "get mib %d error!\n", MIB_IPV6_MAPE_BR_ADDR);
		return;
	}

	/* PSID */
	if(!apmib_get(MIB_MAPE_PSID_OFFSET,(void *)(&psid_offset))){
		fprintf(stderr, "get mib %d error!\n", MIB_MAPE_PSID_OFFSET);
		return;
	}
	if(!apmib_get(MIB_MAPE_PSID_LEN,(void *)(&psid_len))){
		fprintf(stderr, "get mib %d error!\n", MIB_MAPE_PSID_LEN);
		return;
	}
	if(!apmib_get(MIB_MAPE_PSID,(void *)(&psid))){
		fprintf(stderr, "get mib %d error!\n", MIB_MAPE_PSID);
		return;
	}

	sprintf(tmp1, "%s", inet_ntoa(pub));
	sprintf(tmp2, "%s", inet_ntoa(pub_mask));
	memset(tmpStr,0,sizeof(tmpStr));
	sprintf(tmpStr, "ifconfig %s add %s netmask %s",
		wan_interface, tmp1, tmp2);
//	printf("%s\n", tmpStr);
	system(tmpStr);

	sprintf(tmpStr, "route -A inet add default dev %s", wan_interface);
//	printf("%s\n", tmpStr);
	system(tmpStr);

	system("insmod /lib/modules/ivi.ko");

	sprintf(tmpStr, "ivictl -r -d -P %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%d -E",
		br.addrIPv6[0], br.addrIPv6[1], br.addrIPv6[2], br.addrIPv6[3],
		br.addrIPv6[4], br.addrIPv6[5], br.addrIPv6[6], br.addrIPv6[7],
		br.prefix_len);
//	printf("%s\n", tmpStr);
	system(tmpStr);

	sprintf(tmp2, "%s", inet_ntoa(pri));
	val = 1 << psid_len;
	sprintf(tmpStr, "ivictl -s -i br0 -I %s -H -N -a %s/%d -A %s/%d " \
		"-P %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%d "\
		"-z %d -R %d -o %d -c 1400 -E",
		wan_interface, tmp2, pri_mask_len,
		tmp1, pub_mask_len,
		prefix.addrIPv6[0], prefix.addrIPv6[1],
		prefix.addrIPv6[2], prefix.addrIPv6[3],
		prefix.addrIPv6[4], prefix.addrIPv6[5],
		prefix.addrIPv6[6], prefix.addrIPv6[7],
		prefix.prefix_len, psid_offset,
		val, psid);
//	printf("%s\n", tmpStr);
	system(tmpStr);

	sprintf(tmpStr, "echo \"run\" > %s", MAP_E_FILE);
	system(tmpStr);
	
}
#endif

void setbridgeipv6()
{
#if defined(CONFIG_IPV6)
	set_lanipv6();
#endif
}

void set_lanipv6()
{
	int val;
#if defined(CONFIG_IPV6)
	if(!apmib_get(MIB_IPV6_WAN_ENABLE,&val)){		
		fprintf(stderr, "get mib %d error!\n", MIB_IPV6_WAN_ENABLE);
		return ;			
	}
	else if(val==0)
		return;
	set_lanv6();
#endif
	return;
}
void set_wanipv6()
{
    printf("+++%s+++%d\n",__FUNCTION__,__LINE__);
	int val;
	
#ifdef CONFIG_APP_TR069
	int dns_disable=0;
	apmib_get(MIB_CWMP_DNS_DISABLE,(void*)&dns_disable);
#endif
#if defined(CONFIG_IPV6)
	printf("Start setting IPv6[IPv6]\n");

	if(!apmib_get(MIB_IPV6_WAN_ENABLE,&val)){		
		fprintf(stderr, "get mib %d error!\n", MIB_IPV6_WAN_ENABLE);
		return ;			
	}
	else if(val==0)
		return;

#ifdef CONFIG_DSLITE_SUPPORT
	if(!apmib_get(MIB_WAN_DHCP,&val)){	
		fprintf(stderr, "get mib %d error!\n", MIB_WAN_DHCP);
	}
	if(val == 17) //DS-Lite mode
		RunSystemCmd("proc/dslite_hw_fw", "echo", "1", NULL_STR);
#endif
	
	RunSystemCmd("/proc/sys/net/ipv6/conf/all/forwarding", "echo", "1", NULL_STR);
	
	set_wanv6();
//	printf("Start dhcpv6[IPv6]\n");
	set_dhcp6s();

#ifdef TR181_SUPPORT
	DNS_CLIENT_SERVER_T entry[2]={0};
	int dnsEnable, x, y=0;
	
	if ( !apmib_get(MIB_DNS_CLIENT_ENABLE,(void *)&dnsEnable)){
			fprintf(stderr,"get MIB_DNS_CLIENT_ENABLE failed\n");
			return;  
	}

	for(x=6; x<10; x++)
	{
		y = x+1;
		*((char*)entry)=(char)y;
		if(apmib_get(MIB_DNS_CLIENT_SERVER_TBL,(void*)entry)==0)
		{
			printf("%s(%d): get MIB_DNS_CLIENT_SERVER_TBL fail!\n",__FUNCTION__,__LINE__);
			return;
		}
		memcpy(&(entry[1]), &(entry[0]), sizeof(DNS_CLIENT_SERVER_T));
		entry[1].index = x;
		entry[1].enable = 0;
		entry[1].status = 0;
//		strcpy(entry[1].alias, "");
//		strcpy(entry[1].ipAddr, "");
//		strcpy(entry[1].interface, "");
//		entry[1].type = 0; //Unknown
		if(apmib_set(MIB_DNS_CLIENT_SERVER_MOD,(void*)entry)==0)
		{
			printf("%s(%d): set MIB_DNS_CLIENT_SERVER_TBL fail!\n",__FUNCTION__,__LINE__);
			return;
		}
	}
	if(dnsEnable !=0)
#endif
#ifdef CONFIG_APP_TR069
	if(dns_disable==0)
#endif
	{
	printf("Start dnsv6[IPv6]\n");
	set_dnsv6();
	}

	set_6to4tunnel();


	//printf("Start radvd[IPv6]\n");
	set_radvd();
	
	//printf("Start ECMH[IPv6]\n");
	set_ecmh();
	//printf("Start mldproxy[IPv6]\n");
	start_mldproxy("eth1","br0");

#ifdef CONFIG_IPV6_RA_RDNSS_SUPPORT
	system("rdnssd -p /var/rdnssd.pid -r /var/rdnssd.conf");
#endif

#ifdef CONFIG_DSLITE_SUPPORT
	//printf("Start ds-lite[IPv6]\n");
	set_dslite();
#endif
#ifdef CONFIG_APP_NDP_PROXY
	system("ndppd&");
#endif
#ifdef CONFIG_MAP_E_SUPPORT
	set_map_e();
#endif
#endif

	return;
}





