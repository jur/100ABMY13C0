/*
 *      Utiltiy function for setting wlan application 
 *
 */

/*-- System inlcude files --*/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <arpa/inet.h>
#include <unistd.h>
    
#include "apmib.h"
#include "sysconf.h"
#include "sys_utility.h"
#include <dirent.h>


//#define SDEBUG(fmt, args...) printf("[%s %d]"fmt,__FUNCTION__,__LINE__,## args)
#define SDEBUG(fmt, args...) {}

extern int SetWlan_idx(char * wlan_iface_name);
//extern int wlan_idx;	// interface index 
//extern int vwlan_idx;	// initially set interface index to root   
extern int apmib_initialized;

#define IWCONTROL_PID_FILE "/var/run/iwcontrol.pid"
#define PATHSEL_PID_FILE1 "/var/run/pathsel-wlan0.pid"
#define PATHSEL_PID_FILE2 "/var/run/pathsel-wlan1.pid"
#define PATHSEL_PID_FILE3 "/var/run/pathsel-wlan-msh.pid"

#ifdef DOT11K
#define DOT11K_DAEMON_PID_FILE_PREFIX "dot11k-"
#endif

#ifdef FAST_BSS_TRANSITION
#define FT_DAEMON_PID_FILE "/var/run/ft.pid"
#endif

#ifdef CONFIG_IAPP_SUPPORT
#define IAPP_PID_FILE "/var/run/iapp.pid"
#endif

#ifdef RTK_CROSSBAND_REPEATER
#define CROSSBAND_PID_FILE "/var/run/crossband.pid"
#endif

#ifdef SHRINK_INIT_TIME
#define WLAN_APP_CLEAN_FILE "/var/tmp/clean_wlanapp_done"
#endif
#define MESH_PATHSEL "/bin/pathsel" 
#define MAX_CHECK_PID_NUM 10
#ifdef CONFIG_RTL_WAPI_SUPPORT
#define MAX_WAPI_CONF_NUM 10
int asIpExist(WAPI_ASSERVER_CONF_T *pwapiconf, const unsigned char wapiCertSel, char *asipaddr, int *index)
{
	int j;
	if(NULL == pwapiconf || NULL == asipaddr || NULL == index )
		return 0;
	for(j=0;j<MAX_WAPI_CONF_NUM;j++)
	{
		if(pwapiconf[j].valid){
			if((!memcmp(pwapiconf[j].wapi_asip,asipaddr,4)) && (pwapiconf[j].wapi_cert_sel == wapiCertSel)){
				*index=j;
				return 1;
			}
		} else {
			*index = j;
			break;
		}		
	}
	return 0;
}
#endif

static int start_wsc_deamon(char * wlan_interface, int mode, int WSC_UPNP_Enabled, char * bridge_iface) {
    char *cmd_opt[16]={0};
    int cmd_cnt = 0;
    char tempbuf[40];
    char * arg_buff[40];
    char *token=NULL,*token1=NULL, *savestr1=NULL;
    int wps_debug=0, use_iwcontrol=1;
    char wsc_pin_local[16]={0},wsc_pin_peer[16]={0};
    FILE *fp;
    char wscFifoFile[40];
    char wscFifoFile1[40];
    char wscConfFile[40];    
    int wait_fifo=0;

    if(wlan_interface == NULL) {
        return;
    }
    WSC_THREE_DEBUG("enter start_wsc_deamon, ifname(%s)\n", wlan_interface);
    memset(wscFifoFile, 0, sizeof(wscFifoFile));
    memset(wscFifoFile1, 0, sizeof(wscFifoFile1));
    memset(cmd_opt, 0x00, 16);
    cmd_cnt=0;
    wps_debug=0;
    use_iwcontrol=1;


    sprintf(arg_buff,"%s", wlan_interface);
    token = strtok_r(arg_buff," ", &savestr1);
    if(token)
        token1 = strtok_r(NULL," ", &savestr1);

    
    cmd_opt[cmd_cnt++] = "wscd";
    if(isFileExist("/var/wps/simplecfgservice.xml")==0){ //file is not exist
        if(isFileExist("/var/wps"))
            RunSystemCmd(NULL_FILE, "rm", "/var/wps", "-rf", NULL_STR);
        RunSystemCmd(NULL_FILE, "mkdir", "/var/wps", NULL_STR); 
        system("cp /etc/simplecfg*.xml /var/wps");
    }
    
    if(mode == 1) /*cleint*/
    {
        WSC_UPNP_Enabled=0;
        cmd_opt[cmd_cnt++] = "-mode";
        cmd_opt[cmd_cnt++] = "2";
        
    }else{
        cmd_opt[cmd_cnt++] = "-start";
    }
    
    if(WSC_UPNP_Enabled==1){
        RunSystemCmd(NULL_FILE, "route", "del", "-net", "239.255.255.250", "netmask", "255.255.255.255", bridge_iface, NULL_STR); 
        RunSystemCmd(NULL_FILE, "route", "add", "-net", "239.255.255.250", "netmask", "255.255.255.255", bridge_iface, NULL_STR); 
    }
    
    sprintf(wscConfFile,"/var/wsc-%s", token);
    if(token1) {
        strcat(wscConfFile, "-");            
        strcat(wscConfFile, token1);
    }
    strcat(wscConfFile, ".conf");

    WSC_THREE_DEBUG("---> updateWscConf\n");
    RunSystemCmd(NULL_FILE, "flash", "upd-wsc-conf", "/etc/wscd.conf", wscConfFile, wlan_interface, NULL_STR); 
    WSC_THREE_DEBUG("<--- updateWscConf done !!!\n");
    
    cmd_opt[cmd_cnt++] = "-c";
    cmd_opt[cmd_cnt++] = wscConfFile;
    
#ifdef CONFIG_RTL_TRI_BAND_SUPPORT
	if(token[4] == '0' && token[5]=='-' && token[7]=='a') // wlan0-va0
		cmd_opt[cmd_cnt++] = "-w3";
	else if(token[4] == '2')
	    cmd_opt[cmd_cnt++] = "-w3";
	else
#endif
    if(token[4] == '0')
        cmd_opt[cmd_cnt++] = "-w";    
    else if(token[4] == '1')
        cmd_opt[cmd_cnt++] = "-w2";

    cmd_opt[cmd_cnt++] = token;

    if(token1) {
        if(token1[4] == '0')
            cmd_opt[cmd_cnt++] = "-w";       
        else
            cmd_opt[cmd_cnt++] = "-w2";
        cmd_opt[cmd_cnt++] = token1;    
    }
    
    if(wps_debug==1){
        /* when you would like to open debug, you should add define in wsc.h for debug mode enable*/
        cmd_opt[cmd_cnt++] = "-debug";
    }
    if(use_iwcontrol==1){
#ifdef CONFIG_RTL_TRI_BAND_SUPPORT
	if(token[4] == '0' && token[5]=='-' && token[7]=='a') // wlan0-va0
		cmd_opt[cmd_cnt++] = "-fi3";
	else if(token[4] == '2')
	    cmd_opt[cmd_cnt++] = "-fi3";
	else
#endif		
        if(token[4] == '0')
            cmd_opt[cmd_cnt++] = "-fi";
        else if(token[4] == '1')
            cmd_opt[cmd_cnt++] = "-fi2";
        sprintf(wscFifoFile,"/var/wscd-%s.fifo",token);        
        cmd_opt[cmd_cnt++] = wscFifoFile;

        if(token1) {
            if(token1[4] == '0')
                cmd_opt[cmd_cnt++] = "-fi";
            else
                cmd_opt[cmd_cnt++] = "-fi2";
            sprintf(wscFifoFile1,"/var/wscd-%s.fifo",token1);        
            cmd_opt[cmd_cnt++] = wscFifoFile1;
        }
    }
    if(isFileExist("/var/wps_start_pbc")){
        cmd_opt[cmd_cnt++] = "-start_pbc";
        unlink("/var/wps_start_pbc");
    }
    if(isFileExist("/var/wps_start_pin")){
        cmd_opt[cmd_cnt++] = "-start";
        unlink("/var/wps_start_pin");
    }
    if(isFileExist("/var/wps_local_pin")){
        fp=fopen("/var/wps_local_pin", "r");
        if(fp != NULL){
            fscanf(fp, "%s", tempbuf);
            fclose(fp);
        }
        sprintf(wsc_pin_local, "%s", tempbuf);
        cmd_opt[cmd_cnt++] = "-local_pin";
        cmd_opt[cmd_cnt++] = wsc_pin_local;
        unlink("/var/wps_local_pin");
    }
    if(isFileExist("/var/wps_peer_pin")){
        fp=fopen("/var/wps_peer_pin", "r");
        if(fp != NULL){
            fscanf(fp, "%s", tempbuf);
            fclose(fp);
        }
        sprintf(wsc_pin_peer, "%s", tempbuf);
        cmd_opt[cmd_cnt++] = "-peer_pin";
        cmd_opt[cmd_cnt++] = wsc_pin_peer;
        unlink("/var/wps_peer_pin");
    }

    cmd_opt[cmd_cnt++] = "-daemon";
    
    cmd_opt[cmd_cnt++] = 0;

	printf("do cmd to start wps...........\n");
	printf("/******************************/\n");
    for (wps_debug=0; wps_debug<cmd_cnt-1;wps_debug++)
       printf("%s ", cmd_opt[wps_debug]);
	printf("\n");
	printf("/******************************/\n");
    printf("\n");
	
    DoCmd(cmd_opt, NULL_FILE);  // important !!!


    if(use_iwcontrol) {
        wait_fifo=5;
        do{        
            if(isFileExist(wscFifoFile) && (wscFifoFile1[0] == 0 || isFileExist(wscFifoFile1)))
            {
                wait_fifo=0;				
            }else{
                wait_fifo--;				
                sleep(1);
            }
            
        }while(wait_fifo !=0);
    }
    
    return use_iwcontrol;
}


#ifdef DOT11K
#define MAX_DEAMON_NUM 5
static int start_11k_deamon(char * wlanInterface) {
    char arg_buff[200];
    char *token=NULL, *savestr1=NULL;
    int wlan_disabled, dot11k_enable, mode;
    char ssid[33];    
    int i,ssid_idx = 0;
    char ssid_array[MAX_DEAMON_NUM][33];
    char dot11k_interface[MAX_DEAMON_NUM][20];    
    char sys_cmd[30];
    
    
    sprintf(arg_buff, "%s", wlanInterface);    
    token = strtok_r(arg_buff," ", &savestr1);
    do{
        if (token == NULL){/*check if the first arg is NULL*/
            break;
        }else{            
           SetWlan_idx(token);
           apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disabled);
           apmib_get(MIB_WLAN_DOT11K_ENABLE,(void*)&dot11k_enable);
           apmib_get(MIB_WLAN_MODE, (void *)&mode);
           apmib_get(MIB_WLAN_SSID, (void *)&ssid);
           if(!wlan_disabled && dot11k_enable && (mode == AP_MODE || mode == AP_MESH_MODE || mode == AP_WDS_MODE)) {
                for(i = 0; i < ssid_idx; i++) {
                    if(strcmp(ssid, ssid_array[i]) == 0) 
                        break;
                }

                if(i == ssid_idx) { /* not found*/
                    if(ssid_idx < MAX_DEAMON_NUM) {
                        sprintf(
ssid_array[ssid_idx], "%s", ssid);
                        sprintf( dot11k_interface[ssid_idx], "-i %s", token);
                        ssid_idx++;
                    }
                }
                else {
                    strcat(dot11k_interface[i]," -i ");
                    strcat(dot11k_interface[i], token);
                }
           }

        }
        token = strtok_r(NULL, " ", &savestr1);
    }while(token != NULL);


    for(i = 0; i < ssid_idx; i++) {
        sprintf(sys_cmd, "dot11k_deamon %s", dot11k_interface[i]);        
        system(sys_cmd);
    }
}


#endif

#ifdef RTK_CROSSBAND_REPEATER
static int start_crossbandDaemon(){
	int crossband_enable1 = 0, crossband_activate = 0; 

	SetWlan_idx("wlan0");
	apmib_get(MIB_WLAN_CROSSBAND_ENABLE, (void *)&crossband_enable1);
	apmib_get(MIB_WLAN_CROSSBAND_ACTIVATE, (void *)&crossband_activate);
	
	if (crossband_enable1 == 1 && crossband_activate == 1//&& crossband_enable2 == 1
		){
  		printf("Starting crossband_daemon...\n");
		system("/bin/crossband_daemon");		
	}
	else{
		printf( "Not starting crossband!\n");
	}
}
#endif


#ifdef SHRINK_INIT_TIME
int cleanWlan_Applications(char* wlanInterface)
{
	char *token=NULL, *savestr1=NULL;
	char tmpBuff[100], tmpBuff1[100], arg_buff[200];
	char iface_name[16];
	int wlan0_mode=1, wlan1_mode=1, both_band_ap=0;
	int pid=-1;
	char strPID[10];	

	if(isFileExist(WLAN_APP_CLEAN_FILE)){
		unlink(WLAN_APP_CLEAN_FILE);
		return;
	}

	if(isFileExist(IWCONTROL_PID_FILE)){
		pid=getPid_fromFile(IWCONTROL_PID_FILE);
		if(pid != -1){
			sprintf(strPID, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
		}
		unlink(IWCONTROL_PID_FILE);
	}

//for mesh========================================================
#if defined(CONFIG_RTK_MESH)    
	if(isFileExist(PATHSEL_PID_FILE1)){
		pid=getPid_fromFile(PATHSEL_PID_FILE1);
		if(pid != -1){
			sprintf(strPID, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-9", strPID,NULL_STR);
		}
		unlink(PATHSEL_PID_FILE1);
		RunSystemCmd(NULL_FILE, "brctl", "meshsignaloff",NULL_STR);
		
	}
	if(isFileExist(PATHSEL_PID_FILE2)){
		pid=getPid_fromFile(PATHSEL_PID_FILE2);
		if(pid != -1){
			sprintf(strPID, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-9", strPID,NULL_STR);
		}
		unlink(PATHSEL_PID_FILE2);
		RunSystemCmd(NULL_FILE, "brctl", "meshsignaloff",NULL_STR);
		
	}
	if(isFileExist(PATHSEL_PID_FILE3)){
		pid=getPid_fromFile(PATHSEL_PID_FILE3);
		if(pid != -1){
			sprintf(strPID, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-9", strPID,NULL_STR);
		}
		unlink(PATHSEL_PID_FILE3);
		RunSystemCmd(NULL_FILE, "brctl", "meshsignaloff",NULL_STR);
		
	}	 
#endif
#ifdef FAST_BSS_TRANSITION
	if(isFileExist(FT_DAEMON_PID_FILE)){
		pid=getPid_fromFile(FT_DAEMON_PID_FILE);
		if(pid != -1){
			sprintf(strPID, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-9", strPID,NULL_STR);
		}
		unlink(FT_DAEMON_PID_FILE);
	}
#endif
#ifdef DOT11K
    {
        struct dirent **namelist;
        int numdir;
        numdir = scandir("/var/run/", &namelist, NULL, NULL);
        if(numdir >=0) {
            while (numdir--) {
                if (0 ==memcmp(namelist[numdir]->d_name, DOT11K_DAEMON_PID_FILE_PREFIX, strlen(DOT11K_DAEMON_PID_FILE_PREFIX))) {
                    sprintf(tmpBuff, "/var/run/%s", namelist[numdir]->d_name);
				pid=getPid_fromFile(tmpBuff);
				if(pid != -1){
					sprintf(strPID, "%d", pid);
					RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
				}               
				unlink(tmpBuff);			
			}					

                free(namelist[numdir]);
                namelist[numdir] = NULL;
            }
            free(namelist);
            namelist = NULL;
        }
    }
#endif

#ifdef RTK_CROSSBAND_REPEATER
	if(isFileExist(CROSSBAND_PID_FILE)){
		pid=getPid_fromFile(CROSSBAND_PID_FILE);
		if(pid != -1){
			sprintf(strPID, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-9", strPID,NULL_STR);
		}
			unlink(CROSSBAND_PID_FILE);
	}
#endif

	system("killall wscd");
	system("killall iapp");
	system("killall auth");

	unlink("/var/run/wscd*");
	unlink("/var/run/iapp*");
	
#ifdef WLAN_HS2_CONFIG
	if(isFileExist("/tmp/hs2_pidname")){
		FILE *fp;
		char line[100];
		fp = fopen("/tmp/hs2_pidname", "r");
		if (fp != NULL) {
			fgets(line, 100, fp);
			fclose(fp);
			//line[strlen(line)-1]='\0';
			if(isFileExist(line)){
				pid = getPid_fromFile(line);
				if (pid != -1){
					printf("kill hs2:%d\n",pid);
					sprintf(strPID, "%d", pid);
					RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
				}
				unlink(line);
			}
		}
		unlink("/tmp/hs2_pidname");
	}
#endif
	//RunSystemCmd(NULL_FILE, "rm", "-f", "/var/*.fifo", NULL_STR);
	system("rm -f /var/*.fifo");

#ifdef CONFIG_APP_SIMPLE_CONFIG 
	system("killall simple_config >/dev/null 2>&1");
#endif

	RunSystemCmd(NULL_FILE, "echo 0 >", WLAN_APP_CLEAN_FILE);
	return 0;
}
#endif


#ifdef FAST_BSS_TRANSITION
#define FT_ENABLE 1
#define FIRST_CMD 1
#define FT_CONFIG_ERROR -1
//#define CONFIG_11R_PSK
static int config_ft_daemon()
{
#ifdef CONFIG_11R_PSK
	char tmpBuff[100]={0};
	char CmdBuff1[100]={0};
	char CmdBuff2[100]={0};
	FTKH_T khEntryTmp={0};
	int ft_enable = 0, idx;
	int num_5g, num_2g;

	SetWlan_idx("wlan0");
	if ( !apmib_get(MIB_WLAN_FT_ENABLE, (void *)&ft_enable)) {
  		printf( "Get MIB_WLAN_FT_ENABLE table entry error!\n");
		return FT_CONFIG_ERROR;
	}
	
	if ( !apmib_get(MIB_WLAN_FTKH_NUM, (void *)&num_5g)) {
	  	//printf( "Get MIB_WLAN_FTKH_NUM table entry error!\n");
		return FT_CONFIG_ERROR;
	}
	
	if(ft_enable == FT_ENABLE)
	{	
		//printf(" (%s)line=%d,wlan0 ft_ table num  = %d\n", __FUNCTION__, __LINE__, num_5g);
		for (idx = 1; idx <= num_5g; idx++)
		{
			*((char *)&khEntryTmp) = (char)idx;
			if (!apmib_get(MIB_WLAN_FTKH,(void*)&khEntryTmp)) {
	              	printf( "Get MIB_WLAN_FTKH table entry error!\n");
				return FT_CONFIG_ERROR;
			}
			sprintf(tmpBuff,"%02x:%02x:%02x:%02x:%02x:%02x",
				khEntryTmp.macAddr[0], khEntryTmp.macAddr[1], khEntryTmp.macAddr[2],
				khEntryTmp.macAddr[3], khEntryTmp.macAddr[4], khEntryTmp.macAddr[5]);
			//printf("@@=> %s, %s, %s\n", tmpBuff, khEntryTmp.nas_id, khEntryTmp.key);
			
			if(idx == FIRST_CMD)
				sprintf(CmdBuff1, "echo r0kh=%s %s %s wlan0 > /tmp/ft.conf",  tmpBuff, khEntryTmp.nas_id, khEntryTmp.key);
			else
				sprintf(CmdBuff1, "echo r0kh=%s %s %s wlan0 >> /tmp/ft.conf",  tmpBuff, khEntryTmp.nas_id, khEntryTmp.key);
			system(CmdBuff1);	
				
			sprintf(CmdBuff2, "echo r1kh=%s %s %s wlan0 >> /tmp/ft.conf",  tmpBuff, tmpBuff, khEntryTmp.key);
			system(CmdBuff2);	
			
			SDEBUG("Cmd1=> %s\n", CmdBuff1);
			SDEBUG("Cmd2=> %s\n", CmdBuff2);
		}
	}

	SetWlan_idx("wlan1");
	if ( !apmib_get(MIB_WLAN_FT_ENABLE, (void *)&ft_enable)) {
  		printf( "Get MIB_WLAN_FT_ENABLE table entry error!\n");
		return FT_CONFIG_ERROR;
	}

	if ( !apmib_get(MIB_WLAN_FTKH_NUM, (void *)&num_2g)) {
	  	//printf( "Get MIB_WLAN_FTKH_NUM table entry error!\n");
		return FT_CONFIG_ERROR;
	}
	
	if(ft_enable == FT_ENABLE)
	{	
		//printf(" (%s)line=%d,wlan1 ft_ table num  = %d\n", __FUNCTION__, __LINE__, num_2g);
		for (idx = 1; idx <= num_2g; idx++)
		{
			*((char *)&khEntryTmp) = (char)idx;
			if (!apmib_get(MIB_WLAN_FTKH,(void*)&khEntryTmp)) {
	              	printf( "Get MIB_WLAN_FTKH table entry error!\n");
				return FT_CONFIG_ERROR;
			}
			sprintf(tmpBuff,"%02x:%02x:%02x:%02x:%02x:%02x",
				khEntryTmp.macAddr[0], khEntryTmp.macAddr[1], khEntryTmp.macAddr[2],
				khEntryTmp.macAddr[3], khEntryTmp.macAddr[4], khEntryTmp.macAddr[5]);
			//printf("@@=> %s, %s, %s\n", tmpBuff, khEntryTmp.nas_id, khEntryTmp.key);
		
			if(idx == FIRST_CMD && (num_5g == 0))
				sprintf(CmdBuff1, "echo r0kh=%s %s %s wlan1 > /tmp/ft.conf",  tmpBuff, khEntryTmp.nas_id, khEntryTmp.key);
			else
				sprintf(CmdBuff1, "echo r0kh=%s %s %s wlan1 >> /tmp/ft.conf",  tmpBuff, khEntryTmp.nas_id, khEntryTmp.key);
			system(CmdBuff1);	
			
			sprintf(CmdBuff2, "echo r1kh=%s %s %s wlan1 >> /tmp/ft.conf",  tmpBuff, tmpBuff, khEntryTmp.key);
			system(CmdBuff2);
			
			SDEBUG("Cmd1=> %s\n", CmdBuff1);
			SDEBUG("Cmd2=> %s\n", CmdBuff2);
		}
	}
#endif	
	return 0;
}
#endif

int setWlan_Applications(char *action, char *argv)
{
	int pid=-1;
	char strPID[10];
	char iface_name[16];
	char tmpBuff[100], tmpBuff1[100], arg_buff[200],wlan_wapi_asipaddr[100];
	int wlan_wapi_cert_sel;
	int _enable_1x=0, _use_rs=0;
	int wlan_mode_root=0,wlan_disabled_root=0, wlan_wpa_auth_root=0, wlan1_wpa_auth_root=0;
	int wlan0_mode=1, wlan1_mode=1, both_band_ap=0;
#if defined(CONFIG_RTL_TRI_BAND_SUPPORT)
	int wlan2_mode=1;
#endif
	int wlan_wsc_disabled_root=0, wlan_network_type_root=0, wlan0_wsc_disabled_vxd=1, wlan1_wsc_disabled_vxd=1;
	int wlan_1x_enabled_root=0, wlan_encrypt_root=0, wlan_mac_auth_enabled_root=0,wlan_wapi_auth=0;
	int wlan_disabled=0, wlan_mode=0, wlan_wds_enabled=0, wlan_wds_num=0;
	int wlan_encrypt=0, wlan_wds_encrypt=0;
	int wlan_wpa_auth=0;
	int wlan_1x_enabled=0,wlan_mac_auth_enabled=0;
	int wlan_root_auth_enable=0, wlan_vap_auth_enable=0;
	int wlan_network_type=0, wlan_wsc_disabled=0, wlan_hidden_ssid_enabled=0,wlan0_hidden_ssid_enabled=0,wlan1_hidden_ssid_enabled=0;
	char tmp_iface[30]={0}, wlan_role[30]={0}, wlan_vap[30]={0}, wlan_vxd[30]={0};
	char valid_wlan_interface[200]={0}, all_wlan_interface[200]={0};
	int vap_not_in_pure_ap_mode=0, deamon_created=0;
	int isWLANEnabled=0, isAP=0, intValue=0;
	char bridge_iface[30]={0};
	char *token=NULL, *savestr1=NULL;	
	int WSC=1, WSC_UPNP_Enabled=0;
#ifdef CONFIG_RTL_TRI_BAND_SUPPORT
    int three_wps_icons=0;
    char wpsVapPidFile[50]={0}, wpsWlan2PidFile[50]={0};	
#endif
	char *cmd_opt[16]={0};
	int cmd_cnt = 0;
	int check_cnt = 0;
	//Added for virtual wlan interface
	int i=0, wlan_encrypt_virtual=0;
	char wlan_vname[16];
#ifdef CONFIG_RTL_WAPI_SUPPORT
	/*assume MAX 10 configuration*/
	char wlan_name[10];
	int wlan_index;
	int index;
	int apAsAS;
	int wlanBand2G5GSelect;
	WAPI_ASSERVER_CONF_T wapiconf[MAX_WAPI_CONF_NUM];
	int wlan_wapi_auth_root;
#endif

#if defined(FOR_DUAL_BAND)
	int wlan_wsc1_disabled = 1 ;
	int wlan1_disabled_root = 1;
#endif	

#if defined(CONFIG_RTK_MESH)
	char wlan_mesh_interface[22]={0};
	int num_wlan_mesh_interface=0;
    int wlan0_mesh_enabled=0,wlan1_mesh_enabled=0;
  #if defined(CONFIG_RTL_TRI_BAND_SUPPORT)
	int wlan2_mesh_enabled=0;
  #endif
#endif
#ifdef WLAN_HS2_CONFIG
	int wait_fifo=0;
	FILE *fp;
	int wlan0_hs2_enable = 0, wlan0_va0_hs2_enable = 0, wlan0_va1_hs2_enable = 0, wlan0_va2_hs2_enable = 0, wlan0_va3_hs2_enable = 0;	
	int wlan1_hs2_enable = 0, wlan1_va0_hs2_enable = 0, wlan1_va1_hs2_enable = 0, wlan1_va2_hs2_enable = 0, wlan1_va3_hs2_enable = 0;	
	int wlan0_hs2_conf_enable = 0;//, wlan0_va0_hs2_conf_enable = 0, wlan0_va1_hs2_conf_enable = 0, wlan0_va2_hs2_conf_enable = 0, wlan0_va3_hs2_conf_enable = 0;	
	int wlan1_hs2_conf_enable = 0;//, wlan1_va0_hs2_conf_enable = 0, wlan1_va1_hs2_conf_enable = 0, wlan1_va2_hs2_conf_enable = 0, wlan1_va3_hs2_conf_enable = 0;	
	
#endif
#ifdef CONFIG_IAPP_SUPPORT
	char iapp_interface[200]= {0};
	int wlan_iapp_disabled=0,wlan_iapp_disabled_root=0, isIAPPEnabled=0;
#endif

#ifdef CONFIG_RTL_TRI_BAND_SUPPORT
	apmib_get(MIB_THREE_WPS_ICONS, &three_wps_icons);
	sprintf(wpsVapPidFile, "/var/run/wscd-%s.pid", WSCD_VAP_INTERFACE);
	sprintf(wpsWlan2PidFile, "/var/run/wscd-%s.pid", WSCD_WLAN2_INTERFACE);	
#endif

#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)
	int isRptEnabled1=0;
	int isRptEnabled2=0;

	SetWlan_idx("wlan0-vxd");
	apmib_get(MIB_REPEATER_ENABLED1, (void *)&isRptEnabled1);
	apmib_get(MIB_WLAN_WSC_DISABLE, (void *)&wlan0_wsc_disabled_vxd);
				
#if defined(FOR_DUAL_BAND)		
	SetWlan_idx("wlan1-vxd");
	apmib_get(MIB_REPEATER_ENABLED2, (void *)&isRptEnabled2);
	apmib_get( MIB_WLAN_WSC_DISABLE, (void *)&wlan1_wsc_disabled_vxd);
#endif
#endif //#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)

#ifdef WLAN_HS2_CONFIG
	SetWlan_idx("wlan0");
	apmib_get( MIB_WLAN_HS2_ENABLE, (void *)&wlan0_hs2_conf_enable);
	printf("wlan0_hs2_conf_enable=%d\n",wlan0_hs2_conf_enable);
	SetWlan_idx("wlan1");
	apmib_get( MIB_WLAN_HS2_ENABLE, (void *)&wlan1_hs2_conf_enable);
	printf("wlan1_hs2_conf_enable=%d\n",wlan1_hs2_conf_enable);
#endif


#if defined(FOR_DUAL_BAND)
	SetWlan_idx("wlan0");
	apmib_get(MIB_WLAN_MODE, (void *)&wlan0_mode);
	apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disabled_root);
	apmib_get(MIB_WLAN_WSC_DISABLE, (void *)&wlan_wsc_disabled);
	apmib_get(MIB_WLAN_HIDDEN_SSID, (void *)&wlan0_hidden_ssid_enabled);
	apmib_get(MIB_WLAN_WPA_AUTH, (void *)&wlan_wpa_auth_root);
	SetWlan_idx("wlan1");
	apmib_get( MIB_WLAN_MODE, (void *)&wlan1_mode);
	apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&wlan1_disabled_root);
	apmib_get(MIB_WLAN_WSC_DISABLE, (void *)&wlan_wsc1_disabled);
	apmib_get(MIB_WLAN_HIDDEN_SSID, (void *)&wlan1_hidden_ssid_enabled);
	apmib_get(MIB_WLAN_WPA_AUTH, (void *)&wlan1_wpa_auth_root);
	if( ((wlan0_mode == AP_MODE) || (wlan0_mode == AP_WDS_MODE) || (wlan0_mode == AP_MESH_MODE)) && ((wlan1_mode == 0) || (wlan1_mode == AP_WDS_MODE) || (wlan1_mode == AP_MESH_MODE))
		&& (wlan_disabled_root == 0) && (wlan1_disabled_root == 0) && (wlan_wsc_disabled == 0) && (wlan_wsc1_disabled == 0) && (wlan0_hidden_ssid_enabled ==0) && (wlan1_hidden_ssid_enabled ==0)
		&&(wlan_wpa_auth_root != WPA_AUTH_AUTO)&&(wlan1_wpa_auth_root != WPA_AUTH_AUTO) )
	{
#if defined(CONFIG_WPS_EITHER_AP_OR_VXD)
		if ( (isRptEnabled1 == 1 && wlan_disabled_root == 0 && wlan_wsc_disabled == 0) 
			|| (isRptEnabled2 == 1 && wlan1_disabled_root == 0 && wlan_wsc1_disabled == 0))
			both_band_ap = 0;
		else
			both_band_ap = 1;
#else
		both_band_ap = 1;	
#endif
	}

	SetWlan_idx("wlan0");
#endif

#ifdef CONFIG_RTL_P2P_SUPPORT							
	int p2p_mode=0;
#endif

	token=NULL;
	savestr1=NULL;	     
	sprintf(arg_buff, "%s", argv);	

    SDEBUG("arg_buff=[%s]\n",arg_buff);
	
	token = strtok_r(arg_buff," ", &savestr1);
	do{
		if (token == NULL){/*check if the first arg is NULL*/
			break;
		}else{        
			sprintf(iface_name, "%s", token);                                           		
			if(strncmp(iface_name, "wlan", 4)==0){//wlan iface   
				if(all_wlan_interface[0]==0x0){
					sprintf(all_wlan_interface, "%s",iface_name); 
				}else{
					sprintf(tmp_iface, " %s", iface_name);
					strcat(all_wlan_interface, tmp_iface);
				}
			}else{
				sprintf(bridge_iface, "%s", iface_name);
			}
		}
		token = strtok_r(NULL, " ", &savestr1);
	}while(token !=NULL);
	//printf("bridge_iface=%s\n", bridge_iface);
#ifdef SHRINK_INIT_TIME
	cleanWlan_Applications(all_wlan_interface);
#else
	if(isFileExist(IWCONTROL_PID_FILE)){
		pid=getPid_fromFile(IWCONTROL_PID_FILE);
		if(pid != -1){
			sprintf(strPID, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
		}
		unlink(IWCONTROL_PID_FILE);
	}
	
	printf("[%s %d]all_wlan_interface=[%s]\n" , __FUNCTION__, __LINE__, all_wlan_interface);
//for mesh========================================================
#if defined(CONFIG_RTK_MESH)    
	if(isFileExist(PATHSEL_PID_FILE1)){
		pid=getPid_fromFile(PATHSEL_PID_FILE1);
		if(pid != -1){
			sprintf(strPID, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-9", strPID,NULL_STR);
		}
		unlink(PATHSEL_PID_FILE1);
		RunSystemCmd(NULL_FILE, "brctl", "meshsignaloff",NULL_STR);
		
	}
	if(isFileExist(PATHSEL_PID_FILE2)){
		pid=getPid_fromFile(PATHSEL_PID_FILE2);
		if(pid != -1){
			sprintf(strPID, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-9", strPID,NULL_STR);
		}
		unlink(PATHSEL_PID_FILE2);
		RunSystemCmd(NULL_FILE, "brctl", "meshsignaloff",NULL_STR);
		
	}
	if(isFileExist(PATHSEL_PID_FILE3)){
		pid=getPid_fromFile(PATHSEL_PID_FILE3);
		if(pid != -1){
			sprintf(strPID, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-9", strPID,NULL_STR);
		}
		unlink(PATHSEL_PID_FILE3);
		RunSystemCmd(NULL_FILE, "brctl", "meshsignaloff",NULL_STR);
		
	}    
#endif

	token=NULL;
	savestr1=NULL;	     
	sprintf(arg_buff, "%s", all_wlan_interface);
	token = strtok_r(arg_buff," ", &savestr1);
	do{
		if (token == NULL){/*check if the first arg is NULL*/
			break;
		}else{                
			sprintf(iface_name, "%s", token);    	
			if(strncmp(iface_name, "wlan", 4)==0){//wlan iface   
				sprintf(tmpBuff, "/var/run/auth-%s.pid",iface_name);
				if(isFileExist(tmpBuff)){
					pid=getPid_fromFile(tmpBuff);
						if(pid != -1){
							sprintf(strPID, "%d", pid);
							RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
						}
					unlink(tmpBuff);
					sprintf(tmpBuff1, "/var/run/auth-%s-vxd.pid",iface_name);
					if(isFileExist(tmpBuff1)){
					pid=getPid_fromFile(tmpBuff1);
						if(pid != -1){
							sprintf(strPID, "%d", pid);
							RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
						}
					unlink(tmpBuff1);
					}
				}
    			RunSystemCmd("/proc/gpio", "echo", "0", NULL_STR);///is it need to do this for other interface??????except wps
			}			
		}	
		token = strtok_r(NULL, " ", &savestr1);
	}while(token !=NULL);
	
    /* release WPS */
    do{
        if(isFileExist("/var/run/wscd-wlan0.pid"))
        {
            pid=getPid_fromFile("/var/run/wscd-wlan0.pid");
            if(pid != -1){
                sprintf(strPID, "%d", pid);
                RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
            }
            else
                break;
            unlink("/var/run/wscd-wlan0.pid");
            sleep(1);                   
        }
        else if(isFileExist("/var/run/wscd-wlan1.pid"))
        {
            pid=getPid_fromFile("/var/run/wscd-wlan1.pid");
            if(pid != -1){
                sprintf(strPID, "%d", pid);
                RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
            }
            else
                break;
            unlink("/var/run/wscd-wlan1.pid");
            sleep(1);                   
        }
        else if(isFileExist("/var/run/wscd-wlan0-vxd.pid"))
        {
            pid=getPid_fromFile("/var/run/wscd-wlan0-vxd.pid");
            if(pid != -1){
                sprintf(strPID, "%d", pid);
                RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
            }
            else
                break;
            unlink("/var/run/wscd-wlan0-vxd.pid");
            sleep(1);                   
        }       
        else if(isFileExist("/var/run/wscd-wlan1-vxd.pid"))
        {
            pid=getPid_fromFile("/var/run/wscd-wlan1-vxd.pid");
            if(pid != -1){
                sprintf(strPID, "%d", pid);
                RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
            }
            else
                break;
            unlink("/var/run/wscd-wlan1-vxd.pid");
            sleep(1);                   
        }             
        else if(isFileExist("/var/run/wscd-wlan0-wlan1.pid"))
        {
            pid=getPid_fromFile("/var/run/wscd-wlan0-wlan1.pid");
            if(pid != -1){
                sprintf(strPID, "%d", pid);
                RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
            }
            else
                break;
            unlink("/var/run/wscd-wlan0-wlan1.pid");
            sleep(1);                   
        }
        else if(isFileExist("/var/run/wscd-wlan0-wlan1-c.pid"))
        {
            pid=getPid_fromFile("/var/run/wscd-wlan0-wlan1-c.pid");
            if(pid != -1){
                sprintf(strPID, "%d", pid);
                RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
            }
            else
                break;
            unlink("/var/run/wscd-wlan0-wlan1-c.pid");
            sleep(1);                   
        }
#ifdef CONFIG_RTL_TRI_BAND_SUPPORT
		else if(isFileExist(wpsVapPidFile))
		{
			WSC_THREE_DEBUG("%s EXIST!!!\n", wpsVapPidFile);
			pid=getPid_fromFile(wpsVapPidFile);
            if(pid != -1){
				WSC_THREE_DEBUG("pid(%d)!!!\n",pid);
                sprintf(strPID, "%d", pid);
                RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
            }
            else
                break;
            unlink(wpsVapPidFile);
            sleep(1);
		}
		else if(isFileExist(wpsWlan2PidFile))
		{
			WSC_THREE_DEBUG("%s EXIST!!!\n", wpsWlan2PidFile);
			pid=getPid_fromFile(wpsWlan2PidFile);
            if(pid != -1){
				WSC_THREE_DEBUG("pid(%d)!!!\n",pid);
                sprintf(strPID, "%d", pid);
                RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
            }
            else
                break;
            unlink(wpsWlan2PidFile);
            sleep(1);
		}
#endif
        else
            break;
    }while(find_pid_by_name("wscd") > 0);



#ifdef DOT11K
    {
        struct dirent **namelist;
        int numdir;
        numdir = scandir("/var/run/", &namelist, NULL, NULL);
        if(numdir >=0) {
            while (numdir--) {
                if (0 == memcmp(namelist[numdir]->d_name, DOT11K_DAEMON_PID_FILE_PREFIX, strlen(DOT11K_DAEMON_PID_FILE_PREFIX))) {
                    sprintf(tmpBuff, "/var/run/%s", namelist[numdir]->d_name);
				pid=getPid_fromFile(tmpBuff);
				if(pid != -1){
					sprintf(strPID, "%d", pid);
					RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
				}
				unlink(tmpBuff);			
			}					

                free(namelist[numdir]);
                namelist[numdir] = NULL;
            }
            free(namelist);
            namelist = NULL;
        }
		}	
#endif

#ifdef FAST_BSS_TRANSITION
	if(isFileExist(FT_DAEMON_PID_FILE)){
		pid=getPid_fromFile(FT_DAEMON_PID_FILE);
		if(pid != -1){
			sprintf(strPID, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-9", strPID,NULL_STR);
		}
		unlink(FT_DAEMON_PID_FILE);	
	}
	int ret = config_ft_daemon();
	if(ret == FT_CONFIG_ERROR)
		printf( "config_ft_daemon() error!!\n");
	else
		system("ftd -br br0 -w wlan0 wlan1 -pid /var/run/ft.pid -c /tmp/ft.conf &");
#endif
	
#ifdef CONFIG_IAPP_SUPPORT	
	if(isFileExist(IAPP_PID_FILE)){
		pid=getPid_fromFile(IAPP_PID_FILE);
		if(pid != -1){
			sprintf(strPID, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
		}
		unlink(IAPP_PID_FILE);
	}
#endif

#ifdef RTK_CROSSBAND_REPEATER
	if(isFileExist(CROSSBAND_PID_FILE)){
		pid=getPid_fromFile(CROSSBAND_PID_FILE);
		if(pid != -1){
			sprintf(strPID, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
		}
		unlink(CROSSBAND_PID_FILE);
	}
#endif

#ifdef WLAN_HS2_CONFIG
    if(isFileExist("/tmp/hs2_pidname")){
        FILE *fp;
        char line[100];
        fp = fopen("/tmp/hs2_pidname", "r");
        if (fp != NULL) {
            fgets(line, 100, fp);
            fclose(fp);
            //line[strlen(line)-1]='\0';
            if(isFileExist(line)){
                pid = getPid_fromFile(line);
                if (pid != -1){
                    printf("kill hs2:%d\n",pid);
                    sprintf(strPID, "%d", pid);
                    RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
                }
                unlink(line);
            }
        }
		unlink("/tmp/hs2_pidname");
    }
#endif
	//RunSystemCmd(NULL_FILE, "rm", "-f", "/var/*.fifo", NULL_STR);
	system("rm -f /var/*.fifo");

#ifdef CONFIG_APP_SIMPLE_CONFIG 
	system("killall simple_config >/dev/null 2>&1");
#endif
#endif
	if(!strcmp(action, "kill"))
		return 0;
	
	printf("Init Wlan application.......\n");	
	//get root setting first//no this operate in script

#if defined(FOR_DUAL_BAND)
    SetWlan_idx("wlan1");
    apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&wlan1_disabled_root);
    apmib_get(MIB_WLAN_WSC_DISABLE, (void *)&wlan_wsc1_disabled);
#endif

    
    if(SetWlan_idx("wlan0")){
        apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disabled_root);
        apmib_get( MIB_WLAN_MODE, (void *)&wlan_mode_root);
#ifdef CONFIG_IAPP_SUPPORT
        apmib_get( MIB_WLAN_IAPP_DISABLED, (void *)&wlan_iapp_disabled_root);
#endif

        apmib_get( MIB_WLAN_WSC_DISABLE, (void *)&wlan_wsc_disabled_root);
        apmib_get( MIB_WLAN_ENABLE_1X, (void *)&wlan_1x_enabled_root);
        apmib_get( MIB_WLAN_ENCRYPT, (void *)&wlan_encrypt_root);

#ifdef CONFIG_RTL_WAPI_SUPPORT
        apmib_get(MIB_WLAN_WAPI_AUTH, (void *)&wlan_wapi_auth_root);
#endif

        apmib_get( MIB_WLAN_MAC_AUTH_ENABLED, (void *)&wlan_mac_auth_enabled_root);
        apmib_get( MIB_WLAN_NETWORK_TYPE, (void *)&wlan_network_type_root);
        apmib_get( MIB_WLAN_WPA_AUTH, (void *)&wlan_wpa_auth_root);
        apmib_get( MIB_WLAN_WSC_UPNP_ENABLED, (void *)&WSC_UPNP_Enabled);




        // For WAPI.now not support  VAP
        //		apmib_get(MIB_WLAN_WAPI_AUTH, (void *)&wlan_wapi_auth);
        //		memset(wlan_wapi_asipaddr,0x00,sizeof(wlan_wapi_asipaddr));
        //		apmib_get(MIB_WLAN_WAPI_ASIPADDR,  (void*)wlan_wapi_asipaddr);

    }

	token=NULL;
	savestr1=NULL;
	sprintf(arg_buff, "%s", all_wlan_interface);
	token = strtok_r(arg_buff," ", &savestr1);
	do{
		_enable_1x=0;
		_use_rs=0;

		if (token == NULL){/*check if the first arg is NULL*/
			break;
		}else{  			
			sprintf(iface_name, "%s", token); 
			if(strncmp(iface_name, "wlan", 4)==0){//wlan iface   
					
				if(strlen(iface_name)>=9){
					wlan_vap[0]=iface_name[6];
					wlan_vap[1]=iface_name[7];	
				}else{
					wlan_vap[0]=0;
				}
				
				if(SetWlan_idx(iface_name)){
					
					apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disabled);
					apmib_get( MIB_WLAN_MODE, (void *)&wlan_mode); 
					apmib_get( MIB_WLAN_WDS_ENABLED, (void *)&wlan_wds_enabled);
					apmib_get( MIB_WLAN_WDS_NUM, (void *)&wlan_wds_num);
					apmib_get( MIB_WLAN_ENCRYPT, (void *)&wlan_encrypt);
					apmib_get( MIB_WLAN_WPA_AUTH, (void *)&wlan_wpa_auth);

#ifdef WLAN_HS2_CONFIG
					//printf("iface_name=%s\nwlan_disabled=%d\nwlan_encrypt=%d\nwlan_wpa_auth=%d\n",iface_name,wlan_disabled,wlan_encrypt,wlan_wpa_auth);
					
					if (!strcmp(iface_name, "wlan0")) {
						if (wlan0_hs2_conf_enable == 0)
							wlan0_hs2_enable = 0;
						else if ((wlan_disabled == 0) && (wlan_encrypt == 4) && (wlan_wpa_auth == 1)) 
							wlan0_hs2_enable = 1;
						else
							wlan0_hs2_enable = 0;
					}
					else if (!strcmp(iface_name, "wlan0-va0")) {
                        if ((wlan_disabled == 0) && (wlan_encrypt == 4) && (wlan_wpa_auth == 1))
                            wlan0_va0_hs2_enable = 0;
                        else
                            wlan0_va0_hs2_enable = 0;
                    }
					else if (!strcmp(iface_name, "wlan0-va1")) {
                        if ((wlan_disabled == 0) && (wlan_encrypt == 4) && (wlan_wpa_auth == 1))
                            wlan0_va1_hs2_enable = 0;
                        else
                            wlan0_va1_hs2_enable = 0;
                    }
					else if (!strcmp(iface_name, "wlan0-va2")) {
                        if ((wlan_disabled == 0) && (wlan_encrypt == 4) && (wlan_wpa_auth == 1))
                            wlan0_va2_hs2_enable = 0;
                        else
                            wlan0_va2_hs2_enable = 0;
                    }
					else if (!strcmp(iface_name, "wlan0-va3")) {
                        if ((wlan_disabled == 0) && (wlan_encrypt == 4) && (wlan_wpa_auth == 1))
                            wlan0_va3_hs2_enable = 0;
                        else
                            wlan0_va3_hs2_enable = 0;
                    }					
					else if (!strcmp(iface_name, "wlan1")) {
                        if (wlan1_hs2_conf_enable == 0)
							wlan1_hs2_enable = 0;
						else if ((wlan_disabled == 0) && (wlan_encrypt == 4) && (wlan_wpa_auth == 1))
                            wlan1_hs2_enable = 1;
                        else
                            wlan1_hs2_enable = 0;
                    }
					else if (!strcmp(iface_name, "wlan1-va0")) {
                        if ((wlan_disabled == 0) && (wlan_encrypt == 4) && (wlan_wpa_auth == 1))
                            wlan1_va0_hs2_enable = 0;
                        else
                            wlan1_va0_hs2_enable = 0;
                    }
					else if (!strcmp(iface_name, "wlan1-va1")) {
                        if ((wlan_disabled == 0) && (wlan_encrypt == 4) && (wlan_wpa_auth == 1))
                            wlan1_va1_hs2_enable = 0;
                        else
                            wlan1_va1_hs2_enable = 0;
                    }
					else if (!strcmp(iface_name, "wlan1-va2")) {
                        if ((wlan_disabled == 0) && (wlan_encrypt == 4) && (wlan_wpa_auth == 1))
                            wlan1_va2_hs2_enable = 0;
                        else
                            wlan1_va2_hs2_enable = 0;
                    }
					else if (!strcmp(iface_name, "wlan1-va3")) {
                        if ((wlan_disabled == 0) && (wlan_encrypt == 4) && (wlan_wpa_auth == 1))
                            wlan1_va3_hs2_enable = 0;
                        else
                            wlan1_va3_hs2_enable = 0;
                    }
					else
						printf("unknown hs2 iface name:%s\n", iface_name);
#endif
					/* WDS */
					if(wlan_disabled==0 && (wlan_mode ==2 || wlan_mode ==3) && (wlan_wds_enabled !=0) &&(wlan_wds_num!=0)){
						apmib_get( MIB_WLAN_WDS_ENCRYPT, (void *)&wlan_wds_encrypt);
						if(wlan_wds_encrypt==3 || wlan_wds_encrypt==4){
							sprintf(tmpBuff, "/var/wpa-wds-%s.conf",iface_name);//encrytp conf file
							RunSystemCmd(NULL_FILE, "flash", "wpa", iface_name, tmpBuff, "wds", NULL_STR); 
							RunSystemCmd(NULL_FILE, "auth", iface_name, bridge_iface, "wds", tmpBuff, NULL_STR); 
							sprintf(tmpBuff1, "/var/run/auth-%s.pid",iface_name);//auth pid file
							check_cnt = 0;
							do{
								if(isFileExist(tmpBuff1)){//check pid file is exist or not
									break;
								}else{
									sleep(1);
								}
								check_cnt++;
							}while(check_cnt < MAX_CHECK_PID_NUM);
						}
						
					}

 					/* ENCRYPT */
					if(wlan_encrypt < 2){
						apmib_get( MIB_WLAN_ENABLE_1X, (void *)&wlan_1x_enabled);
						apmib_get( MIB_WLAN_MAC_AUTH_ENABLED, (void *)&wlan_mac_auth_enabled);
						if(wlan_1x_enabled != 0 || wlan_mac_auth_enabled != 0){
							_enable_1x=1;
							_use_rs=1;
						}
					}else{
						if(wlan_encrypt != 7){	// not wapi
							_enable_1x=1;
							if(wlan_wpa_auth ==1){
								_use_rs=1;
							}
						}
					}

					if(_enable_1x !=0 && wlan_disabled==0){
						
						sprintf(tmpBuff, "/var/wpa-%s.conf",iface_name);//encrytp conf file
						
						RunSystemCmd(NULL_FILE, "flash", "wpa", iface_name, tmpBuff, NULL_STR); 
						//wait for generate 1x config file
						sleep(2);	
						
						if(wlan_mode==1){//client mode
							apmib_get( MIB_WLAN_NETWORK_TYPE, (void *)&wlan_network_type);
							if(wlan_network_type==0){
								sprintf(wlan_role, "%s", "client-infra");
							}else{
								sprintf(wlan_role, "%s", "client-adhoc");
							}
						}else{
							sprintf(wlan_role, "%s", "auth");
						}
						
						if(wlan_vap[0]=='v' && wlan_vap[1]=='a'){
							if(wlan_mode_root != 0 && wlan_mode_root != 3){
								vap_not_in_pure_ap_mode=1;
							}
						}						
						if(wlan_mode != 2 && vap_not_in_pure_ap_mode==0){
							
							if(wlan_wpa_auth != 2 || _use_rs !=0 ){
								deamon_created=1;
								RunSystemCmd(NULL_FILE, "auth", iface_name, bridge_iface, wlan_role, tmpBuff, NULL_STR); 
								
								if(wlan_vap[0]=='v' && wlan_vap[1]=='a')
									wlan_vap_auth_enable=1;
								else
									wlan_root_auth_enable=1;
							}
						}
					}
					
					if(wlan_vap[0]=='v' && wlan_vap[1]=='x' && wlan_disabled==0){
						if(strcmp(wlan_role, "auth") || (!strcmp(wlan_role, "auth") && (_use_rs !=0)))
						{
							strcat(wlan_vxd," ");
							strcat(wlan_vxd,iface_name);
						}
					}
					
					if(wlan_vap[0]=='v' && wlan_vap[1]=='a'){ // vap
						if(wlan_disabled==0){
							if(wlan_vap_auth_enable==1
#ifdef CONFIG_IAPP_SUPPORT
								|| wlan_iapp_disabled_root==0 
#endif
							){
								if(valid_wlan_interface[0]==0){
									sprintf(valid_wlan_interface, "%s",iface_name); 
								}else{
									sprintf(tmp_iface, " %s", iface_name);
									strcat(valid_wlan_interface, tmp_iface);
								}
							}
						}
					}else{						
						if(wlan_vap[0] !='v' && wlan_vap[1] !='x'){ // root							
#ifdef CONFIG_IAPP_SUPPORT
							apmib_get( MIB_WLAN_IAPP_DISABLED, (void *)&wlan_iapp_disabled);
#endif
							apmib_get( MIB_WLAN_WSC_DISABLE, (void *)&wlan_wsc_disabled); 
							if(wlan_disabled==0 && (wlan_root_auth_enable==1  || wlan_wsc_disabled==0
#ifdef CONFIG_IAPP_SUPPORT
								|| wlan_iapp_disabled==0
#endif
								)){
								if(valid_wlan_interface[0]==0){
									sprintf(valid_wlan_interface, "%s",iface_name); 
								}else{
									sprintf(tmp_iface, " %s", iface_name);
									strcat(valid_wlan_interface, tmp_iface);
								}
							}
						}
					}
						
						if((wlan_vap[0] !='v' && wlan_vap[1] !='a') && (wlan_vap[0] !='v' && wlan_vap[1] !='x')){
							 if(wlan_disabled==0)
							 	isWLANEnabled=1;
							 if(wlan_mode ==0 || wlan_mode ==3 || wlan_mode ==4 || wlan_mode ==6)
							 	isAP=1;
#ifdef CONFIG_IAPP_SUPPORT
							 if(wlan_iapp_disabled==0)
							 	isIAPPEnabled=1;
#endif
						}
				}	
			}
		}
		token = strtok_r(NULL, " ", &savestr1);
	}while(token !=NULL);

	printf("valid_wlan_interface:%s (%d)\n", valid_wlan_interface, __LINE__);
	
// for init IAPP........
#ifdef CONFIG_IAPP_SUPPORT		
	if(isWLANEnabled==1 && isAP==1&& isIAPPEnabled==1){
#if defined(CONFIG_RTL_ULINKER)
		//fixme: disable iapp temporary
#else
		token=NULL;
		savestr1=NULL;	     
		sprintf(arg_buff, "%s", valid_wlan_interface);
		token = strtok_r(arg_buff," ", &savestr1);
		do{
			if (token == NULL)
				break;
			if(!strcmp(token, "wlan0") //root if
				|| !strcmp(token, "wlan1")
#ifdef CONFIG_RTL_TRI_BAND_SUPPORT
				|| !strcmp(token, "wlan2")
#endif
			)
			{
				SetWlan_idx(token);
				apmib_get( MIB_WLAN_MODE, (void *)&wlan_mode_root);
				apmib_get( MIB_WLAN_IAPP_DISABLED,(void *)&wlan_iapp_disabled_root);
				if((wlan_mode_root ==0 || wlan_mode_root ==3 || wlan_mode_root ==4 || wlan_mode_root ==6) && wlan_iapp_disabled_root==0){
					if(iapp_interface[0]==0){
						sprintf(iapp_interface, "%s",token); 
					}else{
						sprintf(tmp_iface, " %s", token);
						strcat(iapp_interface, tmp_iface);
					}
				}
					
			}
			else{
				if(iapp_interface[0]==0){
					sprintf(iapp_interface, "%s",token); 
				}else{
					sprintf(tmp_iface, " %s", token);
					strcat(iapp_interface, tmp_iface);
				}
			}
			token = strtok_r(NULL, " ", &savestr1);
		}while(token != NULL);

		printf("start IAPP:%s %s (%s-%d)\n", bridge_iface, iapp_interface,  __FUNCTION__,__LINE__);
		sprintf(tmpBuff, "iapp %s %s",bridge_iface, iapp_interface);
		system(tmpBuff);
		
		deamon_created=1;
        if(isFileExist(RESTART_IAPP))
            unlink(RESTART_IAPP);
        RunSystemCmd(RESTART_IAPP, "echo", tmpBuff, NULL_STR);
#endif
	}
#endif
	
//for mesh========================================================
#if defined(CONFIG_RTK_MESH)
	SetWlan_idx("wlan0");
	apmib_get(MIB_WLAN_MODE, (void *)&wlan0_mode);
	apmib_get(MIB_WLAN_MESH_ENABLE,(void *)&wlan0_mesh_enabled);
    if(wlan0_mode != AP_MESH_MODE && wlan0_mode != MESH_MODE) {
        wlan0_mesh_enabled = 0;
    }

#if defined(FOR_DUAL_BAND)
    SetWlan_idx("wlan1");
    apmib_get(MIB_WLAN_MODE, (void *)&wlan1_mode);
    apmib_get(MIB_WLAN_MESH_ENABLE,(void *)&wlan1_mesh_enabled);
    if(wlan1_mode != AP_MESH_MODE && wlan1_mode != MESH_MODE){
        wlan1_mesh_enabled = 0;
    }
#endif

#if defined(CONFIG_RTL_TRI_BAND_SUPPORT)
    SetWlan_idx("wlan2");
    apmib_get(MIB_WLAN_MODE, (void *)&wlan2_mode);
    apmib_get(MIB_WLAN_MESH_ENABLE,(void *)&wlan2_mesh_enabled);
    if(wlan2_mode != AP_MESH_MODE && wlan2_mode != MESH_MODE){
        wlan2_mesh_enabled = 0;
    }
#endif

	num_wlan_mesh_interface=if_readlist_proc(wlan_mesh_interface, "msh", 0);

    #if defined(CONFIG_RTL_MESH_CROSSBAND) || !defined(CONFIG_RTL_MESH_SINGLE_IFACE)
    if(wlan0_mesh_enabled && num_wlan_mesh_interface>0) {
        system("pathsel -i wlan0-msh0 -P -d");
    }

    if(wlan1_mesh_enabled && num_wlan_mesh_interface>1) {
        system("pathsel -i wlan1-msh0 -P -d");
    }

#if defined(CONFIG_RTL_TRI_BAND_SUPPORT)
    if(wlan2_mesh_enabled && num_wlan_mesh_interface>2) {
        system("pathsel -i wlan2-msh0 -P -d");
    }
#endif
    #else
	if(wlan0_mesh_enabled || wlan1_mesh_enabled
#if defined(CONFIG_RTL_TRI_BAND_SUPPORT)
        || wlan2_mesh_enabled
#endif
    ){
        system("pathsel -i wlan-msh -P -d");
	}
    #endif

#endif


//for 802.11k========================================================
#if defined(DOT11K)
    start_11k_deamon(all_wlan_interface);
#endif

//========================================================
//for HS2
#ifdef WLAN_HS2_CONFIG
	printf("hs2_wlan0_enable= %d,%d,%d,%d,%d\n", wlan0_hs2_enable, wlan0_va0_hs2_enable, wlan0_va1_hs2_enable, wlan0_va2_hs2_enable, wlan0_va3_hs2_enable);
	printf("hs2_wlan1_enable= %d,%d,%d,%d,%d\n", wlan1_hs2_enable, wlan1_va0_hs2_enable, wlan1_va1_hs2_enable, wlan1_va2_hs2_enable, wlan1_va3_hs2_enable);

	if (isFileExist("/bin/hs2")) {		
		//memset(tmpBuff, 0x00, 100);
		memset(cmd_opt, 0x00, 16);
        cmd_cnt=0;
        cmd_opt[cmd_cnt++] = "hs2";

		strcat(tmpBuff, "hs2 ");
		if (isFileExist("/etc/hs2_wlan0.conf") && (wlan0_hs2_enable == 1)) {			
			//strcat(tmpBuff, "-c /tmp/hs2_wlan0.conf ");			
			cmd_opt[cmd_cnt++] = "-c";
			cmd_opt[cmd_cnt++] = "/etc/hs2_wlan0.conf";
			//cmd_opt[cmd_cnt++] = "/etc/hs2_wlan0.conf";
		}	
		if (isFileExist("/etc/hs2_wlan0_va0.conf") && (wlan0_va0_hs2_enable == 1)) {
            //strcat(tmpBuff, "-c /tmp/hs2_wlan0_va0.conf ");         
            cmd_opt[cmd_cnt++] = "-c";
            cmd_opt[cmd_cnt++] = "/etc/hs2_wlan0_va0.conf";
        }   
		if (isFileExist("/etc/hs2_wlan0_va1.conf") && (wlan0_va1_hs2_enable == 1)) {
            //strcat(tmpBuff, "-c /tmp/hs2_wlan0_va1.conf ");         
            cmd_opt[cmd_cnt++] = "-c";
            cmd_opt[cmd_cnt++] = "/etc/hs2_wlan0_va1.conf";
        }   
		if (isFileExist("/etc/hs2_wlan0_va2.conf") && (wlan0_va2_hs2_enable == 1)) {
            //strcat(tmpBuff, "-c /tmp/hs2_wlan0_va2.conf ");         
            cmd_opt[cmd_cnt++] = "-c";
            cmd_opt[cmd_cnt++] = "/etc/hs2_wlan0_va2.conf";
        }   
		if (isFileExist("/etc/hs2_wlan0_va3.conf") && (wlan0_va3_hs2_enable == 1)) {
            //strcat(tmpBuff, "-c /tmp/hs2_wlan0_va3.conf ");         
            cmd_opt[cmd_cnt++] = "-c";
            cmd_opt[cmd_cnt++] = "/etc/hs2_wlan0_va3.conf";
        }   
		if (isFileExist("/etc/hs2_wlan1.conf") && (wlan1_hs2_enable == 1)) {
			//strcat(tmpBuff, "-c /tmp/hs2_wlan0.conf ");			
			cmd_opt[cmd_cnt++] = "-c";
			cmd_opt[cmd_cnt++] = "/etc/hs2_wlan1.conf";
		}	
		if (isFileExist("/etc/hs2_wlan1_va0.conf") && (wlan1_va0_hs2_enable == 1)) {
            //strcat(tmpBuff, "-c /tmp/hs2_wlan0_va0.conf ");         
            cmd_opt[cmd_cnt++] = "-c";
            cmd_opt[cmd_cnt++] = "/etc/hs2_wlan1_va0.conf";
        }   
		if (isFileExist("/etc/hs2_wlan1_va1.conf") && (wlan1_va1_hs2_enable == 1)) {
            //strcat(tmpBuff, "-c /tmp/hs2_wlan0_va1.conf ");         
            cmd_opt[cmd_cnt++] = "-c";
            cmd_opt[cmd_cnt++] = "/etc/hs2_wlan1_va1.conf";
        }   
		if (isFileExist("/etc/hs2_wlan1_va2.conf") && (wlan1_va2_hs2_enable == 1)) {
            //strcat(tmpBuff, "-c /tmp/hs2_wlan0_va2.conf ");         
            cmd_opt[cmd_cnt++] = "-c";
            cmd_opt[cmd_cnt++] = "/etc/hs2_wlan1_va2.conf";
        }   
		if (isFileExist("/etc/hs2_wlan1_va3.conf") && (wlan1_va3_hs2_enable == 1)) {
            //strcat(tmpBuff, "-c /tmp/hs2_wlan0_va3.conf ");         
            cmd_opt[cmd_cnt++] = "-c";
            cmd_opt[cmd_cnt++] = "/etc/hs2_wlan1_va3.conf";
        }   
		//printf("hs2 cmd==> %s\n", tmpBuff);
		//system(tmpBuff);
		cmd_opt[cmd_cnt++] = 0;		
        DoCmd(cmd_opt, NULL_FILE);                                                    	
		wait_fifo=3;
		do{
			if(isFileExist("/tmp/hs2_pidname")){//check pid file is exist or not
				break;
            } 
			else {
				wait_fifo--;
				sleep(1);
            }
        }while(wait_fifo != 0);
	}
	else 
		printf("/bin/hs2 do not exist\n");
//	RunSystemCmd(NULL_FILE, "killall", "telnetd", NULL_STR);
	system("telnetd");
#endif

//========================================================
//for WPS
    if (isFileExist("/bin/wscd")) {
        char ap_interface[20];
        char client_interface[20];     

#ifdef CONFIG_RTL_P2P_SUPPORT  
        char p2p_interface[20];
        memset(p2p_interface, 0x00, sizeof(p2p_interface));
#endif        
        memset(ap_interface, 0, sizeof(ap_interface));
        memset(client_interface, 0, sizeof(client_interface));  // 0x00 ->0       
        memset(tmpBuff, 0x00, sizeof(tmpBuff));
        token=NULL;
        savestr1=NULL;	       
#if 0//def CONFIG_RTL_TRI_BAND_SUPPORT
		if(three_wps_icons)
			if(!strstr(valid_wlan_interface, "wlan0-va0")) // avoid IAPP disabled
				strcat(valid_wlan_interface, " wlan0-va0");		
#endif
		sprintf(arg_buff, "%s", valid_wlan_interface);
#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)


        //SDEBUG(" isRptEnabled1=[%d]\n",isRptEnabled1);
        //SDEBUG(" wlan0_wsc_disabled_vxd=[%d]\n",wlan0_wsc_disabled_vxd);
        //SDEBUG(" isRptEnabled2=[%d]\n",isRptEnabled2);
        //SDEBUG(" wlan1_wsc_disabled_vxd=[%d]\n",wlan1_wsc_disabled_vxd);

        memset(wlan_vxd, 0x00, sizeof(wlan_vxd));
        if(isRptEnabled1 == 1 && wlan_wsc_disabled_root == 0			
#if defined(CONFIG_RTL_ULINKER)
            && wlan_mode_root != CLIENT_MODE
#endif			
#if defined(CONFIG_ONLY_SUPPORT_CLIENT_REPEATER_WPS)
            && wlan_mode_root == CLIENT_MODE
#endif			
        )
        {
            sprintf(wlan_vxd, "%s", "wlan0-vxd");
        }

#if defined(FOR_DUAL_BAND)
        if(isRptEnabled2 == 1 && wlan_wsc1_disabled == 0
#if defined(CONFIG_RTL_ULINKER)
            && wlan_mode_root != CLIENT_MODE
#endif			
#if defined(CONFIG_ONLY_SUPPORT_CLIENT_REPEATER_WPS)
            && wlan_mode_root == CLIENT_MODE
#endif			
        )
        {			
            strcat(wlan_vxd, " wlan1-vxd");
        }
#endif

        sprintf(tmpBuff," %s",wlan_vxd);
        strcat(arg_buff, tmpBuff);
#endif	//#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)			

        //printf("\r\n arg_buff=[%s],__[%s-%u]\r\n",arg_buff,__FILE__,__LINE__);

        token = strtok_r(arg_buff," ", &savestr1);
			
        //token = strtok_r(valid_wlan_interface," ", &savestr1);
        do{
            if (token == NULL){
                break;
            }else{
                _enable_1x=0;
                WSC=1;				
                if(!strcmp(token, "wlan0") //root if
                    || !strcmp(token, "wlan1") 
#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)
                    || strstr(token, "-vxd")
#endif	
                )
                {
                    SetWlan_idx(token);
                    apmib_get( MIB_WLAN_MODE, (void *)&wlan_mode_root);                        
                    apmib_get( MIB_WLAN_ENABLE_1X, (void *)&wlan_1x_enabled_root);						
                    apmib_get( MIB_WLAN_ENCRYPT, (void *)&wlan_encrypt_root);	
                    apmib_get(MIB_WLAN_HIDDEN_SSID, (void *)&wlan_hidden_ssid_enabled);

#ifdef CONFIG_RTL_WAPI_SUPPORT
                    apmib_get(MIB_WLAN_WAPI_AUTH, (void *)&wlan_wapi_auth_root);
#endif
                    apmib_get( MIB_WLAN_MAC_AUTH_ENABLED, (void *)&wlan_mac_auth_enabled_root);
                    apmib_get( MIB_WLAN_NETWORK_TYPE, (void *)&wlan_network_type_root);
                    apmib_get( MIB_WLAN_WPA_AUTH, (void *)&wlan_wpa_auth_root);


                    if(wlan_encrypt_root < 2){ //ENCRYPT_DISABLED=0, ENCRYPT_WEP=1, ENCRYPT_WPA=2, ENCRYPT_WPA2=4, ENCRYPT_WPA2_MIXED=6 ,ENCRYPT_WAPI=7

                    if(wlan_1x_enabled_root != 0 || wlan_mac_auth_enabled_root !=0)
                        _enable_1x=1;
                    }else{
                        if(wlan_encrypt_root != 7)	//not wapi
                            _enable_1x=1;
                    }

                    if(!strcmp(token, "wlan0") && ((wlan_wsc_disabled_root != 0) || (wlan_disabled_root != 0) || (wlan_mode_root == WDS_MODE) || (wlan_mode_root == MESH_MODE))){
                        WSC=0;
                    }

#if defined(FOR_DUAL_BAND)		
                    else if(!strcmp(token, "wlan1") && ((wlan_wsc1_disabled != 0) || (wlan1_disabled_root != 0) || (wlan_mode_root == WDS_MODE) || (wlan_mode_root == MESH_MODE))){
                        WSC=0;
                    }
#endif

#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT)					
                    else if(!strcmp(token, "wlan0-vxd") && (wlan0_wsc_disabled_vxd != 0 || wlan_disabled_root != 0))
                    {
                        WSC=0;
                    }
#if defined(FOR_DUAL_BAND)					
                    else if(!strcmp(token, "wlan1-vxd") && (wlan1_wsc_disabled_vxd != 0 || wlan1_disabled_root != 0))
                    {
                        WSC=0;
                    }
#endif					
#endif
                    else{

#if defined(UNIVERSAL_REPEATER) && defined(CONFIG_REPEATER_WPS_SUPPORT) && defined(CONFIG_WPS_EITHER_AP_OR_VXD)
                        if(strlen(token) == 5 && !strcmp(token, "wlan0") && isRptEnabled1 == 1)
                        {
                            WSC=0;
                        }
                        else
#if defined(FOR_DUAL_BAND)												
                        if(strlen(token) == 5 && !strcmp(token, "wlan1") && isRptEnabled2 == 1)
                        {
                            WSC=0;
                        }
                        else
#endif							
#endif						
                        if(wlan_mode_root == CLIENT_MODE){
                            if(wlan_network_type_root != 0)
                                WSC=0;
                        }
                        else if(wlan_mode_root == AP_MODE || wlan_mode_root == AP_WDS_MODE || wlan_mode_root == AP_MESH_MODE){
                            if(wlan_hidden_ssid_enabled) {
                                WSC = 0;
                            }                                
                            else if(wlan_encrypt_root  == 0 && _enable_1x !=0 )
                                WSC=0;		
                            else if(wlan_encrypt_root == 1) /* wps do not support wep*/
                                WSC=0;
                            else if(wlan_encrypt_root >= 2 && wlan_encrypt_root != 7 && wlan_wpa_auth_root ==1 )
                                WSC=0;
#ifdef CONFIG_RTL_WAPI_SUPPORT
                            else if(wlan_encrypt_root == 7 && wlan_wapi_auth_root == 1)
                                WSC=0;
#endif
                        }

                    }						

                    if(WSC==1){ //start wscd
                        if(wlan_mode_root == CLIENT_MODE){							
                            if(client_interface[0] == 0) {
                                sprintf(client_interface, "%s", token);
                            }
                            else {
                                strcat(client_interface, " ");                                
                                strcat(client_interface, token);
                            }							
                        }						
#ifdef CONFIG_RTL_P2P_SUPPORT                        
                        else if(wlan_mode_root == P2P_SUPPORT_MODE) {  
                            apmib_get( MIB_WLAN_P2P_TYPE, (void *)&p2p_mode); 
                            sprintf(p2p_interface, "%s", token);
                        }
#endif                        
                        else { /* AP mode*/ 							
                            apmib_get( MIB_WLAN_WSC_UPNP_ENABLED, (void *)&WSC_UPNP_Enabled);
                            if(ap_interface[0] == 0) {
                                sprintf(ap_interface, "%s", token);
                            }
                            else {
                                strcat(ap_interface, " ");                                
                                strcat(ap_interface, token);
                            }
                        }
                    }						
                }		
            }   
            token = strtok_r(NULL, " ", &savestr1);

        }while(token !=NULL);
       
        /* start wsc deamon in ap mode*/
        if(ap_interface[0]) {
			if(three_wps_icons)
			{			
#ifdef CONFIG_RTL_TRI_BAND_SUPPORT
				int vapDisabled = 1, wlan2Disabled=1;
				char vapIfname[15], wlan2Ifname[15];	
				int vap_1x_enable=0, vap_encrypt=0, vap_hidden_ssid=0;			 	
				sprintf(vapIfname, "%s", WSCD_VAP_INTERFACE);   // just for test ......				 
				printf("### vap Ifname(%s)(%d)\n", vapIfname, __LINE__);				
				SetWlan_idx(vapIfname);  
				apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&vapDisabled);

				// wep, auth, wapi , hidden ssid all not suppot wps
				apmib_get(MIB_WLAN_HIDDEN_SSID, (void *)&vap_hidden_ssid);
				apmib_get(MIB_WLAN_ENCRYPT, (void *)&vap_encrypt);
				apmib_get(MIB_WLAN_ENABLE_1X, (void *)&vap_1x_enable);

                sprintf(wlan2Ifname, "%s", WSCD_WLAN2_INTERFACE);
				SetWlan_idx(wlan2Ifname);
				apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&wlan2Disabled);
				
				if(!vapDisabled && !vap_hidden_ssid && (vap_encrypt!=ENCRYPT_WEP && 
					vap_encrypt!=ENCRYPT_WAPI) && !vap_1x_enable && wlan2Disabled)
				{
					strcat(ap_interface, " "); 
					strcat(ap_interface, vapIfname); 
				}else
					printf("##### %s wps failed !(%d:%d:%d:%d:%d:%d)\n", vapIfname, vapDisabled,
							three_wps_icons,vap_hidden_ssid,vap_encrypt,vap_1x_enable,wlan2Disabled);
							
#endif				
				token = strtok_r(ap_interface," ", &savestr1);				
				do {
					WSC_THREE_DEBUG("==================== start %s=================>>>\n", token);
					WSC_THREE_DEBUG("token(%s), ------->start_wsc_deamon\n", token);
					deamon_created = start_wsc_deamon(token, 0, WSC_UPNP_Enabled, bridge_iface);
					WSC_THREE_DEBUG("<------end start_wsc_deamon, deamon_created(%d)\n", deamon_created);
					WSC_THREE_DEBUG("<<<===================end %s=================\n", token);
					token = strtok_r(NULL," ", &savestr1);					
					WSC_THREE_DEBUG("\n");
				}while(token != NULL);
			}
			else
			{	
				// just for test
            	deamon_created= start_wsc_deamon(ap_interface, 0, WSC_UPNP_Enabled, bridge_iface);
        	}
        }


        /* start wsc deamon in client mode*/
		printf("#### client_interface[0]:%d !\n", client_interface[0]);
        if(client_interface[0] && client_interface[0]!='0') {
			printf("start client wps....\n");
#ifdef CONFIG_COMBINE_TWO_WPS_DEAMON_FOR_CLIENT            
            /*start only one deamon for both clients*/			
            deamon_created = start_wsc_deamon(client_interface, 1, 0, NULL);
#else
            /*start wsc deamon for every client interface*/
            token = strtok_r(client_interface," ", &savestr1);
            do {
				//if(token==0 || !strcmp(token, "0"))
				//	break;
                deamon_created = start_wsc_deamon(token, 1, 0, NULL);                
                token = strtok_r(NULL," ", &savestr1);
            }while(token != NULL);
#endif
        }
        
#ifdef CONFIG_RTL_P2P_SUPPORT                        
        /* start wsc deamon for p2p*/
        if(p2p_interface[0]) {
            if(p2p_mode == P2P_DEVICE || p2p_mode == P2P_CLIENT)
                deamon_created= start_wsc_deamon(p2p_interface, 1, 0, NULL);            
            else
                deamon_created= start_wsc_deamon(p2p_interface, 0, 0, NULL);
        }
#endif        
    }

	if(deamon_created==1){
		if(wlan_vxd[0]){
				sprintf(tmpBuff, "iwcontrol %s %s",valid_wlan_interface, wlan_vxd);
		}else{
				WSC_THREE_DEBUG("iwcontrol %s \n", valid_wlan_interface);
				sprintf(tmpBuff, "iwcontrol %s",valid_wlan_interface);
		}
		system(tmpBuff);	
		WSC_THREE_DEBUG("start WSC daemon end !!!!!!! \n");
	}
/*for WAPI*/
	//first, to kill daemon related wapi-cert
	//in order to avoid multiple daemon existing
#ifdef CONFIG_RTL_WAPI_SUPPORT
#ifdef CONFIG_RTL_WAPI_LOCAL_AS_SUPPORT
	RunSystemCmd(NULL_FILE, "killall", "-9", "aseUdpServer", NULL_STR); 
#endif
	RunSystemCmd(NULL_FILE, "killall", "-9", "aeUdpClient", NULL_STR);

	///////////////////////////////
	//no these operations in script
	//should sync with WLAN_INTERFACE_LIST: "wlan0,wlan0-va0,wlan0-va1,wlan0-va2,wlan0-va3"
	//At first, check virtual wlan interface
	apAsAS=0;//Initial, note: as IP only need to be set once because all wlan interfaces use the same as IP setting
	memset(wapiconf,0x0,sizeof(WAPI_ASSERVER_CONF_T)*MAX_WAPI_CONF_NUM);
	apmib_get(MIB_WLAN_BAND2G5G_SELECT,(void *)&wlanBand2G5GSelect);	
	for(wlan_index=0; wlan_index<NUM_WLAN_INTERFACE; wlan_index++)
	{
		if((wlanBand2G5GSelect!=BANDMODEBOTH)&&(wlan_index>0))
			break;
			
		sprintf(wlan_name,"wlan%d",wlan_index);
		for(i=0;i<4;i++)
		{
			memset(wlan_vname,0,sizeof(wlan_vname));
			sprintf(wlan_vname, "%s-va%d",wlan_name,i);
			if(SetWlan_idx(wlan_vname)){
				apmib_get( MIB_WLAN_ENCRYPT, (void *)&wlan_encrypt_virtual);
				apmib_get(MIB_WLAN_WAPI_AUTH, (void *)&wlan_wapi_auth);
				memset(wlan_wapi_asipaddr,0x00,sizeof(wlan_wapi_asipaddr));
				apmib_get(MIB_WLAN_WAPI_ASIPADDR,  (void*)wlan_wapi_asipaddr);
				apmib_get(MIB_WLAN_WAPI_CERT_SEL,  (void*)&wlan_wapi_cert_sel);
				apmib_get(MIB_WLAN_WLAN_DISABLED,(void *)&wlan_disabled);
			}
	//		printf("%s(%d): wlan_vname(%s), wlan_encrypt_virtual(%d), wlan_wapi_auth(%d), wlan_wapi_asipaddr(%s)\n",
	//			__FUNCTION__,__LINE__,wlan_vname, wlan_encrypt_virtual,wlan_wapi_auth,inet_ntoa(*((struct in_addr *)wlan_wapi_asipaddr)));//Added for test
			if(wlan_encrypt_virtual == 7 && wlan_disabled==0){
				if(wlan_wapi_auth == 1){
#ifdef CONFIG_RTL_WAPI_LOCAL_AS_SUPPORT
					if(!apAsAS){
						apmib_get(MIB_IP_ADDR,  (void*)tmpBuff1);
						if(!memcmp(wlan_wapi_asipaddr, tmpBuff1, 4)){
							apAsAS=1;
						}
					}
#endif
					if(asIpExist(wapiconf, (unsigned char)wlan_wapi_cert_sel,wlan_wapi_asipaddr,&index)){
						if(wapiconf[index].valid){
							strcat(wapiconf[index].network_inf,",");
							strcat(wapiconf[index].network_inf,wlan_vname);
						}
					}else {
						memcpy(wapiconf[index].wapi_asip,wlan_wapi_asipaddr,4);
						strcpy(wapiconf[index].network_inf,wlan_vname);
						wapiconf[index].wapi_cert_sel=wlan_wapi_cert_sel;
						wapiconf[index].valid=1;
					}
				}
			}
		}
		////////////////////////////////////

		//At last, check root wlan interface
		if(SetWlan_idx(wlan_name)){
			apmib_get( MIB_WLAN_ENCRYPT, (void *)&wlan_encrypt_root);
			apmib_get(MIB_WLAN_WAPI_AUTH, (void *)&wlan_wapi_auth);
			memset(wlan_wapi_asipaddr,0x00,sizeof(wlan_wapi_asipaddr));
			apmib_get(MIB_WLAN_WAPI_ASIPADDR,  (void*)wlan_wapi_asipaddr);
			apmib_get(MIB_WLAN_WAPI_CERT_SEL,  (void*)&wlan_wapi_cert_sel);
			apmib_get(MIB_WLAN_WLAN_DISABLED,(void *)&wlan_disabled);
		}
	//	printf("%s(%d): wlan0, wlan_encrypt_root(%d), wlan_wapi_auth(%d), wlan_wapi_asipaddr(%s)\n",
	//		__FUNCTION__,__LINE__,wlan_encrypt_root,wlan_wapi_auth,inet_ntoa(*((struct in_addr *)wlan_wapi_asipaddr)));//Added for test
		if(wlan_encrypt_root == 7 && wlan_disabled==0){
			if(wlan_wapi_auth == 1){
#ifdef CONFIG_RTL_WAPI_LOCAL_AS_SUPPORT
				if(!apAsAS){
					apmib_get(MIB_IP_ADDR,  (void*)tmpBuff1);
					if(!memcmp(wlan_wapi_asipaddr, tmpBuff1, 4)){		
						apAsAS=1;
					}
				}
#endif
				if(asIpExist(wapiconf, (unsigned char)wlan_wapi_cert_sel,wlan_wapi_asipaddr,&index)){
					if(wapiconf[index].valid){
						strcat(wapiconf[index].network_inf,",");
						strcat(wapiconf[index].network_inf,wlan_name);
					}
				}else {
					memcpy(wapiconf[index].wapi_asip,wlan_wapi_asipaddr,4);
					strcpy(wapiconf[index].network_inf,wlan_name);
					wapiconf[index].wapi_cert_sel=wlan_wapi_cert_sel;
					wapiconf[index].valid=1;
				}
			}
		}
	}
	
#ifdef CONFIG_RTL_WAPI_LOCAL_AS_SUPPORT
	if(apAsAS){
		system("aseUdpServer &");
	}
#endif


	for(index=0;index<MAX_WAPI_CONF_NUM;index++)
	{
//		printf("%s:%d index=%d network_inf=%s\n", __FUNCTION__,__LINE__,index,wapiconf[index].network_inf);
		if(wapiconf[index].valid){
	//		printf("%s:%d index=%d network_inf=%s\n", __FUNCTION__,__LINE__,index,wapiconf[index].network_inf);

	//		printf("%s:%d wlan_disabled=%d wlan_encrypt=%d\n", __FUNCTION__,__LINE__,wlan_disabled,wlan_encrypt);

			sprintf(arg_buff,"aeUdpClient -d %s -i %s -s %d &", inet_ntoa(*((struct in_addr *)wapiconf[index].wapi_asip)), wapiconf[index].network_inf, wapiconf[index].wapi_cert_sel);
			system(arg_buff);
		}
	}	
#endif

#if defined(CONFIG_APP_SIMPLE_CONFIG)
	int sc_enabled, sc_store, sc_sync, vxd_enabled, root_disabled;
	memset(cmd_opt, 0x00, 16);
	cmd_cnt=0;

	SetWlan_idx("wlan0");
	apmib_get(MIB_WLAN_MODE, (void *)&wlan0_mode);
	apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disabled);
	apmib_get(MIB_WLAN_SC_ENABLED, (void *)&sc_enabled);
	apmib_get(MIB_WLAN_SC_SAVE_PROFILE, (void *)&sc_store);
	apmib_get(MIB_WLAN_SC_SYNC_PROFILE, (void *)&sc_sync);
	root_disabled = wlan_disabled;
	
	if((wlan0_mode == CLIENT_MODE)&&(wlan_disabled == 0) && (sc_enabled==1))
	{
		sprintf(arg_buff,"simple_config -i wlan0 -store %d &", sc_store);

		system(arg_buff);
	}
	else
	{
		
		SetWlan_idx("wlan0-vxd");
		apmib_get(MIB_WLAN_MODE, (void *)&wlan0_mode);
		apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disabled);
		apmib_get(MIB_WLAN_SC_ENABLED, (void *)&sc_enabled);
		apmib_get(MIB_WLAN_SC_SAVE_PROFILE, (void *)&sc_store);
		apmib_get(MIB_REPEATER_ENABLED1, (void *)&vxd_enabled);
		if((wlan0_mode == CLIENT_MODE)&&(wlan_disabled == 0)  && (sc_enabled==1) && (vxd_enabled==1) && (root_disabled==0))
		{
			sprintf(arg_buff,"simple_config -i wlan0-vxd -store %d &", sc_store);
			system(arg_buff);
		}
		
	}
	
#if defined(FOR_DUAL_BAND)	
	SetWlan_idx("wlan1");
	apmib_get(MIB_WLAN_MODE, (void *)&wlan1_mode);
	apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&wlan1_disabled_root);
	apmib_get(MIB_WLAN_SC_ENABLED, (void *)&sc_enabled);
	apmib_get(MIB_WLAN_SC_SAVE_PROFILE, (void *)&sc_store);
	apmib_get(MIB_WLAN_SC_SYNC_PROFILE, (void *)&sc_sync);
	
	if((wlan1_mode == CLIENT_MODE)&&(wlan1_disabled_root == 0) && (sc_enabled==1))
	{
		sprintf(arg_buff,"simple_config -i wlan1 -store %d &", sc_store);
		system(arg_buff);
	}
	else
	{
			
		SetWlan_idx("wlan1-vxd");
		apmib_get(MIB_WLAN_MODE, (void *)&wlan1_mode);
		apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disabled);
		apmib_get(MIB_WLAN_SC_ENABLED, (void *)&sc_enabled);
		apmib_get(MIB_WLAN_SC_SAVE_PROFILE, (void *)&sc_store);
		apmib_get(MIB_REPEATER_ENABLED2, (void *)&vxd_enabled);
		
		if((wlan1_mode == CLIENT_MODE)&&(wlan_disabled == 0) && (sc_enabled==1) && (wlan1_disabled_root==0) && (vxd_enabled==1))
		{
			sprintf(arg_buff,"simple_config -i wlan1-vxd -store %d &", sc_store);
			system(arg_buff);
		}
			
	}
#endif
#endif

#ifdef RTK_CROSSBAND_REPEATER
	start_crossbandDaemon();
#endif

return 0;	
	
		
}

 
 
 
 
 
 
 
 
 
 
 
