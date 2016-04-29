/*
 *      Advanced Function Implementation 
 *
 *      Authors: Erishen  <lsun@actiontec.com>
 *
 */
 
#include "act_advanced.h"
#include "act_session.h"
#include "utility.h"
#include "act_l3_local.h"
#include <time.h>
#include "md5.h"

extern char error_buf[];

extern char countDownTitle[64];
extern int countDownTime;

#ifdef ACTIONTEC_MULTIACCESS
extern char *last_request_uri;
#endif

void getAdvValue(char *type, char *retvalue)
{
	kinfo("Enter");
}

void actAdvdefault(request *wp, char *path, char *query)
{
	char *mypage = NULL;
	kinfo("Enter");
	
    act_reset_wsc_pin(); // Copy from DEF_WLAN_WSC_PIN to HW_WSC_PIN
	apmib_updateDef();
	apmib_reinit();
	apmib_update_web(CURRENT_SETTING);	// update configuration to flash

	system( "killall data_center" );
	system( "rm -f /mnt/rt_conf/pre_cfg.xml.gz" );
	system( "sync" );

	strncpy(countDownTitle, "Reload setting successfully!", 64);
	countDownTime = RESTORE_COUNTDOWN_TIME;
	
	mypage = req_get_cstream_var(wp, ("mypage"), "");
	sprintf(lastUrl, "%s", mypage);
	send_redirect_perm(wp, COUNTDOWN_PAGE);
}

void actAdvSwapImage(request *wp, char *path, char *query)
{
	char *mypage = NULL;
	int ret = 0;
	kinfo("Enter");

	if((ret=aei_swap_img())< 0)
	{
		strcpy(error_buf, ("Error!aei_swap_img error."));
		goto getErr_adv;
	}


	strncpy(countDownTitle, "Swap setting successfully!", 64);
	countDownTime = SWAP_COUNTDOWN_TIME;
	
	mypage = req_get_cstream_var(wp, ("mypage"), "");
	sprintf(lastUrl, "%s", mypage);
	send_redirect_perm(wp, COUNTDOWN_PAGE);
	return;

getErr_adv:
	kerror("error_buf: %s", error_buf);
}


void actGetImageVersion(request *wp, char *path, char *query)
{

	int ret=0;
	char active_version[BUF_LEN20]={0};
	char inactive_version[BUF_LEN20]={0};
	AEI_FLASH_DATA_T flash_data;

	if((ret=aei_get_flash_data(&flash_data, BOOT_IMAGE))<0)
	{
		strcpy(error_buf, ("Error!Get BOOT_IMAGE error."));
		goto getErr_adv;
	}
	strncpy(active_version,flash_data.version,sizeof(active_version));	
	
	if((ret=aei_get_flash_data(&flash_data, BACK_IMAGE))<0)
	{
		strcpy(error_buf, ("Error!Get BACK_IMAGE error."));
		goto getErr_adv;
	}
	strncpy(inactive_version,flash_data.version,sizeof(inactive_version));	
	req_format_write(wp, ("%s|%s|"),active_version,inactive_version);
	return;
getErr_adv:
	kerror("error_buf: %s", error_buf);

	//req_format_write(wp, ("%s"),"Inactive_test");
}

void actAdvreboot(request *wp, char *path, char *query)
{
	kinfo("Enter");
	system("reboot");	
}

void actAdvmanage(request *wp, char *path, char *query)
{
	char *strVal = NULL;
	char *mypage = NULL;	
	struct in_addr ip;
	struct in_addr subnet;
	struct in_addr gateway;

	char autoconfig_str[10] = {0};
	struct in_addr intaddr;
	DHCP_T dhcp;
	L3Addr_t ips[BUF_LEN10] = {0};
	int cnt = 0;
	int i = 0;

	char default_gateway[BUF_LEN20]={0};
	char default_ip[BUF_LEN20]={0};
	char default_subnet[BUF_LEN20]={0};
	
	kinfo("Enter");
	
	strVal = req_get_cstream_var(wp, ("command"), "");

	if(!safe_strcmp(strVal, "reboot"))
	{
		strncpy(countDownTitle, "Reboot successfully!", 64);
		countDownTime = REBOOT_COUNTDOWN_TIME;
		mypage = req_get_cstream_var(wp, ("mypage"), "");
		sprintf(lastUrl, "%s", mypage);
		send_redirect_perm(wp, COUNTDOWN_PAGE);
	}	
	else if(!safe_strcmp(strVal, "apply"))
	{
		strVal = req_get_cstream_var(wp, ("ip"), "");
		if(strVal[0]) {
			kinfo("ip: %s", strVal);

			if(!inet_aton(strVal, &ip))
			{
				strcpy(error_buf, ("Error! IP Address is invalid."));
				goto setErr_adv;
			}

#if 0
			if (!apmib_set( MIB_IP_ADDR, (void *)&(ip.s_addr))) {
		  		strcpy(error_buf, ("Set MIB_IP_ADDR error!"));
				goto setErr_adv;
			}
#endif
            if(tr69SetUnfreshLeafDataString(DEVICE_DHCPV4_CLIENT, 1, "X_ACTIONTEC_COM_DefaultIPAddress", strVal))
            {
                strcpy(error_buf, "Set Device.DHCPv4.Client.1.X_ACTIONTEC_COM_DefaultIPAddress error!");
				goto setErr_adv;
            }
		}

		strVal = req_get_cstream_var(wp, ("subnet"), "");		
		if(strVal[0]) {
			kinfo("subnet: %s", strVal);

			if(!inet_aton(strVal, &subnet))
			{
				strcpy(error_buf, ("Error! Subnet Mask is invalid."));
				goto setErr_adv;
			}
#if 0
			if (!apmib_set( MIB_SUBNET_MASK, (void *)&(subnet.s_addr))) {
		  		strcpy(error_buf, ("Set MIB_SUBNET_MASK error!"));
				goto setErr_adv;
			}
#endif
            if(tr69SetUnfreshLeafDataString(DEVICE_DHCPV4_CLIENT, 1, "X_ACTIONTEC_COM_DefaultSubnetMask", strVal))
            {
                strcpy(error_buf, "Set Device.DHCPv4.Client.1.X_ACTIONTEC_COM_DefaultSubnetMask error!");
				goto setErr_adv;
            }
		}

		strVal = req_get_cstream_var(wp, ("gateway"), "");
		if(!strVal[0])
		{
			//fprintf(stdout,"\ngateway=%s\n",strVal);
	            if(tr69SetUnfreshLeafDataString(DEVICE_DHCPV4_CLIENT, 1, "X_ACTIONTEC_COM_DefaultIPRouters", strVal))
	            {
	                strcpy(error_buf, "Set Device.DHCPv4.Client.1.X_ACTIONTEC_COM_DefaultIPRouters error!");
					goto setErr_adv;
	            }
		}
		else{
			kinfo("gateway: %s", strVal);
			
			if(!inet_aton(strVal, &gateway))
			{
				strcpy(error_buf, ("Error! Default Gateway is invalid."));
				goto setErr_adv;
			}
            
#if 0			
			if (!apmib_set( MIB_DEFAULT_GATEWAY, (void *)&(gateway.s_addr))) {
		  		strcpy(error_buf, ("Set MIB_DEFAULT_GATEWAY error!"));
				goto setErr_adv;
			}
#endif
            if(tr69SetUnfreshLeafDataString(DEVICE_DHCPV4_CLIENT, 1, "X_ACTIONTEC_COM_DefaultIPRouters", strVal))
            {
                strcpy(error_buf, "Set Device.DHCPv4.Client.1.X_ACTIONTEC_COM_DefaultIPRouters error!");
				goto setErr_adv;
            }
		}

		strVal = req_get_cstream_var(wp, ("autoconfig"), "");		
		if(strVal[0]) {
			kinfo("autoconfig: %s", strVal);
			if(tr69SetLeafDataInt("Device.X_ACTIONTEC_COM_ZeroConf.Extender", 0, "Enable", atoi(strVal)))
            		{
             			strcpy(error_buf, "Set Device.X_ACTIONTEC_COM_ZeroConf.Extender Enable error!");
				goto setErr_adv;
           		}
		}
        
        if(tr69SaveNow())
        {
            strcpy(error_buf, "tr69SaveNow error!");
            goto setErr_adv;
        }
		thankyouPage(wp);
	}
	else if(!safe_strcmp(strVal, "getvalue"))
	{	
#if 0
        if (getInAddr("br0", IP_ADDR, (void *)&intaddr))
			snprintf(ip_str, 20, "%s", inet_ntoa(intaddr));			
		else
			strncpy(ip_str, "0.0.0.0", 20);

		if (getInAddr("br0", SUBNET_MASK, (void *)&intaddr))
			snprintf(subnet_str, 20, "%s", inet_ntoa(intaddr));			
		else
			strncpy(subnet_str, "0.0.0.0", 20);
		
	  	apmib_get(MIB_DHCP, (void *)&dhcp);

		if (dhcp == DHCP_SERVER)
		{
			// if DHCP server, default gateway is set to LAN IP
			if (getInAddr("br0", IP_ADDR, (void *)&intaddr ) )
				snprintf(gateway_str, 20, "%s", inet_ntoa(intaddr));	
			else
				strncpy(gateway_str, "0.0.0.0", 20);
		}
		else
		{
			if ( getDefaultRoute("br0", &intaddr) )
				snprintf(gateway_str, 20, "%s", inet_ntoa(intaddr));	
			else
				strncpy(gateway_str, "0.0.0.0", 20);
		}
#endif

 
        if(tr69GetUnfreshLeafData(default_ip, DEVICE_DHCPV4_CLIENT_1, "X_ACTIONTEC_COM_DefaultIPAddress"))
        {
            strcpy(error_buf, "Get Device.DHCPv4.Client.1.X_ACTIONTEC_COM_DefaultIPAddress error!");
            goto getErr_adv;
        } 

        if(tr69GetUnfreshLeafData(default_subnet, DEVICE_DHCPV4_CLIENT_1, "X_ACTIONTEC_COM_DefaultSubnetMask"))
        {
            strcpy(error_buf, "Get Device.DHCPv4.Client.1.X_ACTIONTEC_COM_DefaultSubnetMask error!");
            goto getErr_adv;
        } 

        if(tr69GetUnfreshLeafData(default_gateway, DEVICE_DHCPV4_CLIENT_1, "X_ACTIONTEC_COM_DefaultIPRouters"))
        {
            strcpy(error_buf, "Get Device.DHCPv4.Client.1.X_ACTIONTEC_COM_DefaultIPRouters error!");
            goto getErr_adv;
        } 

		
        if(tr69GetFreshLeafData(autoconfig_str, "Device.X_ACTIONTEC_COM_ZeroConf.Extender", "Enable"))
        {
            strcpy(error_buf, "Get Device.X_ACTIONTEC_COM_ZeroConf.Extender Enable error!");
            goto getErr_adv;
        } 
		cutGetTR69(autoconfig_str);	
		changeString(default_ip);
		changeString(default_subnet);
		changeString(default_gateway);
		changeString(autoconfig_str);


		req_format_write(wp, ("%s|%s|%s|%s||"),default_ip,default_subnet,default_gateway,autoconfig_str);			
	}
	else if(!safe_strcmp(strVal, "getipv6"))
	{	
		cnt = getL3LocalList(ips, 10, NULL);

		kinfo("ipaddr (totally %d)", cnt);
		for(i = 0; i < cnt; i++) {
			kinfo("%s, type(%d)", ips[i].ipAddrStr, ips[i].type);

			changeString(ips[i].ipAddrStr);
			
			req_format_write(wp, ("%d|%s||"), ips[i].type, ips[i].ipAddrStr);	
		}
	}
	
	return;

setErr_adv:
	kerror("error_buf: %s", error_buf);
	backPage(wp);
getErr_adv:
    kerror("error_buf: %s", error_buf);
}


void actAdvtr69(request *wp, char *path, char *query)
{
	char *strVal = NULL;
	char *mypage = NULL;	

	char tr69_url[BUF_LEN64]={0};
	char tr69_username[BUF_LEN64]={0};
	char tr69_password[BUF_LEN64]={0};
	char tr69_informenable[BUF_LEN64]={0};
	char tr69_informinterval[BUF_LEN64]={0};
	char tr69_informtime[BUF_LEN64]={0};
	char tr69_con_username[BUF_LEN64]={0};
	char tr69_con_password[BUF_LEN64]={0};
	
	kinfo("Enter");
	
	strVal = req_get_cstream_var(wp, ("command"), "");

       if(!safe_strcmp(strVal, "apply"))
	{
		strVal = req_get_cstream_var(wp, ("advanced_tr69_url"), "");
		//fprintf(stdout,"\n#@#@advanced_tr69_url=%s\n",strVal);
		if(strVal[0]) {
			kinfo("syslog_server: %s", strVal);

	            if(tr69SetLeafDataString(DEVICE_MANAGEMENT_SERVER, 0, "URL", strVal))
	            {
	                strcpy(error_buf, "Set Device.ManagementServer.URL error!");
			  goto setErr_adv;
	            }
		}

		strVal = req_get_cstream_var(wp, ("advanced_tr69_username"), "");
		//fprintf(stdout,"\n#@#@advanced_tr69_username=%s\n",strVal);
		if(strVal[0]) {
			kinfo("subnet: %s", strVal);

	            if(tr69SetLeafDataString(DEVICE_MANAGEMENT_SERVER, 0, "Username", strVal))
	            {
	                strcpy(error_buf, "Set Device.ManagementServer.Username error!");
			  goto setErr_adv;
	            }
		}

		strVal = req_get_cstream_var(wp, ("advanced_tr69_password"), "");
		//fprintf(stdout,"\n#@#@advanced_tr69_password=%s\n",strVal);
		if(strVal[0]) {
			kinfo("syslog_server: %s", strVal);

	            if(tr69SetLeafDataString(DEVICE_MANAGEMENT_SERVER, 0, "Password", strVal))
	            {
	                strcpy(error_buf, "Set Device.ManagementServer.Password error!");
			  goto setErr_adv;
	            }
		}

		strVal = req_get_cstream_var(wp, ("advanced_tr69_informenable"), "");
		//fprintf(stdout,"\n#@#@advanced_tr69_informenable=%s,d=%d\n",strVal,atoi(strVal));
		if(strVal[0]) {
			kinfo("subnet: %s", strVal);

	            if(tr69SetLeafDataInt(DEVICE_MANAGEMENT_SERVER, 0, "PeriodicInformEnable", atoi(strVal)))
	            {
	                strcpy(error_buf, "Set Device.ManagementServer.PeriodicInformEnable error!");
			  goto setErr_adv;
	            }
		}

		strVal = req_get_cstream_var(wp, ("advanced_tr69_informinterval"), "");
		//fprintf(stdout,"\n#@#@advanced_tr69_informinterval=%s,d=%d\n",strVal,atoi(strVal));
		if(strVal[0]) {
			kinfo("syslog_server: %s", strVal);

	            if(tr69SetLeafDataInt(DEVICE_MANAGEMENT_SERVER, 0, "PeriodicInformInterval", atoi(strVal)))
	            {
	                strcpy(error_buf, "Set Device.ManagementServer.PeriodicInformInterval error!");
			  goto setErr_adv;
	            }
		}

		strVal = req_get_cstream_var(wp, ("advanced_tr69_connection_username"), "");
		//fprintf(stdout,"\n#@#@advanced_tr69_connection_username=%s\n",strVal);
		if(strVal[0]) {
			kinfo("subnet: %s", strVal);

	            if(tr69SetLeafDataString(DEVICE_MANAGEMENT_SERVER, 0, "ConnectionRequestUsername", strVal))
	            {
	                strcpy(error_buf, "Set Device.ManagementServer.ConnectionRequestUsername error!");
			  goto setErr_adv;
	            }
		}

		strVal = req_get_cstream_var(wp, ("advanced_tr69_connection_password"), "");
		//fprintf(stdout,"\n#@#@advanced_tr69_connection_password=%s\n",strVal);
		if(strVal[0]) {
			kinfo("subnet: %s", strVal);

	            if(tr69SetLeafDataString(DEVICE_MANAGEMENT_SERVER, 0, "ConnectionRequestPassword", strVal))
	            {
	                strcpy(error_buf, "Set Device.ManagementServer.ConnectionRequestPassword error!");
			  goto setErr_adv;
	            }
		}
        					 
	           if(tr69SaveNow())
	           {
	              strcpy(error_buf, "tr69SaveNow error!");
	              goto setErr_adv;
	           }
		    thankyouPage(wp);
	}
	else if(!safe_strcmp(strVal, "getvalue"))
	{	 
	         if(tr69GetFreshLeafData(tr69_url,DEVICE_MANAGEMENT_SERVER,"URL"))
	         {
	              strcpy(error_buf, "Get Device.ManagementServer.URL error!");
	              goto getErr_adv;
	         }
			 
	         if(tr69GetFreshLeafData(tr69_username,DEVICE_MANAGEMENT_SERVER,"Username"))
	         {
	              strcpy(error_buf, "Get Device.ManagementServer.Username error!");
	              goto getErr_adv;
	         }

	         if(tr69GetFreshLeafData(tr69_password,DEVICE_MANAGEMENT_SERVER,"Password"))
	         {
	              strcpy(error_buf, "Get Device.ManagementServer.Password error!");
	              goto getErr_adv;
	         }

	         if(tr69GetFreshLeafData(tr69_informenable,DEVICE_MANAGEMENT_SERVER,"PeriodicInformEnable"))
	         {
	              strcpy(error_buf, "Get Device.ManagementServer.PeriodicInformEnable error!");
	              goto getErr_adv;
	         }

	         if(tr69GetFreshLeafData(tr69_informinterval,DEVICE_MANAGEMENT_SERVER,"PeriodicInformInterval"))
	         {
	              strcpy(error_buf, "Get Device.ManagementServer.PeriodicInformInterval error!");
	              goto getErr_adv;
	         }

	         if(tr69GetFreshLeafData(tr69_informtime,DEVICE_MANAGEMENT_SERVER,"PeriodicInformTime"))
	         {
	              strcpy(error_buf, "Get Device.ManagementServer.PeriodicInformTime error!");
	              goto getErr_adv;
	         }

	         if(tr69GetFreshLeafData(tr69_con_username,DEVICE_MANAGEMENT_SERVER,"ConnectionRequestUsername"))
	         {
	              strcpy(error_buf, "Get Device.ManagementServer.ConnectionRequestUsername error!");
	              goto getErr_adv;
	         }

	         if(tr69GetFreshLeafData(tr69_con_password,DEVICE_MANAGEMENT_SERVER,"ConnectionRequestPassword"))
	         {
	              strcpy(error_buf, "Get Device.ManagementServer.ConnectionRequestPassword error!");
	              goto getErr_adv;
	         }
			 
		changeString(tr69_url);
		changeString(tr69_username);
		changeString(tr69_password);
		changeString(tr69_informenable);		
		changeString(tr69_informinterval);
		changeString(tr69_informtime);
		changeString(tr69_con_username);
		changeString(tr69_con_password);
		//fprintf(stdout,"\n#@#@tr69_url=%s,tr69_username=%s\n",tr69_url,tr69_username);
		//fprintf(stdout,"\n#@#@tr69_password=%s,tr69_informenable=%s\n",tr69_password,tr69_informenable);
		//fprintf(stdout,"\n#@#@tr69_informinterval=%s,tr69_informtime=%s\n",tr69_informinterval,tr69_informtime);
		//fprintf(stdout,"\n#@#@tr69_con_username=%s,tr69_con_password=%s\n",tr69_con_username,tr69_con_password);

		 req_format_write(wp, ("%s|%s|%s|%s|%s|%s|%s|%s|"),
		 						tr69_url,tr69_username,
		 						tr69_password,tr69_informenable,
		 						tr69_informinterval,tr69_informtime,
		 						tr69_con_username,tr69_con_password
		 						);			
	}
	
	return;

setErr_adv:
	kerror("error_buf: %s", error_buf);
	backPage(wp);
getErr_adv:
       kerror("error_buf: %s", error_buf);
}

void actgetCurrentAddrInfo(request *wp, char *path, char *query)
{

        char cur_ip_str[BUF_LEN20] = {0};
        char cur_subnet_str[BUF_LEN20] = {0};
        char cur_gateway_str[BUF_LEN20] = {0};
	    char gateway_str[BUF_LEN20]={0};


        if(tr69GetUnfreshLeafData(cur_ip_str, DEVICE_IP_INTER1_IPADDR_1, "IPAddress"))
        {
            strcpy(error_buf, "Get Device.IP.Interface.1.IPv4Address.1.IPAddress error!");
            goto getErr_adv;   
        }


        if(tr69GetUnfreshLeafData(cur_subnet_str, DEVICE_IP_INTER1_IPADDR_1, "SubnetMask"))
        {
            strcpy(error_buf, "Get Device.IP.Interface.1.IPv4Address.1.SubnetMask error!");
            goto getErr_adv;   
        }


        if(tr69GetUnfreshLeafData(cur_gateway_str, DEVICE_DHCPV4_CLIENT_1, "IPRouters"))
        {
            strcpy(error_buf, "Get Device.DHCPv4.Client.1.IPRouters error!");
            goto getErr_adv; 
        }
        
        if(!safe_strcmp(cur_gateway_str, "NULL") || !safe_strcmp(cur_gateway_str, "0.0.0.0"))
        {            
            if(tr69GetUnfreshLeafData(gateway_str, DEVICE_DHCPV4_CLIENT_1, "X_ACTIONTEC_COM_DefaultIPRouters"))
            {
                strcpy(error_buf, "Get Device.DHCPv4.Client.1.X_ACTIONTEC_COM_DefaultIPRouters error!");
                goto getErr_adv;
            } 
            strcpy(cur_gateway_str,gateway_str);
        }

		changeString(cur_ip_str);
		changeString(cur_subnet_str);
		changeString(cur_gateway_str);

		req_format_write(wp, ("%s|%s|%s||"), cur_ip_str, cur_subnet_str, cur_gateway_str);
	
	return;
getErr_adv:
      kerror("error_buf: %s", error_buf);

}

void actUserlogin(request *wp, char *path, char *query)
{
	char tr69_username[64] = {0};
	char tr69_password[64] = {0};
	char tr69_auth_level[4] = {0};
	char tr69_user_enable[4] = {0};
	char *login_username;
	char *login_password;
	int auth_level = 0;
	char buf[4] = {0};
	char mdmoid_user_object[256] = {0};
	int user_entries = 0;
	int i = 0;
	
#ifdef ACTIONTEC_MD5
	struct MD5Context md5c;
	unsigned char signature[16] = {0};
	unsigned char csig[16] = {0};
	char *clabel = NULL, *hexfmt = "%02X";
	unsigned int bp;
	int j = 0;
#endif

#ifdef ACTIONTEC_MULTIACCESS
	struct http_session *hs = NULL;
	struct timeval tv;	
#endif	
	
	kinfo("Enter");	
	
	login_username = req_get_cstream_var(wp, ("login_username"), "");
	kinfo("login_username: %s", login_username);	

	login_password = req_get_cstream_var(wp, ("login_password"), "");
	kinfo("login_password: %s", login_password);

#ifdef ACTIONTEC_MD5
	MD5Init(&md5c);
	MD5Update(&md5c, (unsigned char *)login_password, strlen(login_password));
	MD5Final(signature, &md5c);
#endif

	if(tr69GetUnfreshLeafData(buf, "Device.Users", "UserNumberOfEntries"))
    {
        strcpy(error_buf, "Get Device.Users.UserNumberOfEntries error!");
	    goto getErr_login;
    }
    
	user_entries = atoi(buf);

	for(i = 1; i <= user_entries; i++)
	{
		sprintf(mdmoid_user_object, "%s.%d", "Device.Users.User", i);

		if(tr69GetUnfreshLeafData(tr69_username, mdmoid_user_object, "Username"))	
        {
            sprintf(error_buf, "Get %s.Username error!", mdmoid_user_object);
    	    goto getErr_login;
        }
        
		if(tr69GetUnfreshLeafData(tr69_password, mdmoid_user_object, "Password"))
        {
            sprintf(error_buf, "Get %s.Password error!", mdmoid_user_object);
    	    goto getErr_login;
        }

	 if(tr69GetUnfreshLeafData(tr69_user_enable, mdmoid_user_object, "Enable"))
        {
            sprintf(error_buf, "Get %s.Enable error!", mdmoid_user_object);
    	    goto getErr_login;
        }
        
        if(tr69GetUnfreshLeafData(tr69_auth_level, mdmoid_user_object, "X_ACTIONTEC_COM_AuthLevel"))
        {
            sprintf(error_buf, "Get %s.X_ACTIONTEC_COM_AuthLevel error!", mdmoid_user_object);
    	    goto getErr_login;
        }

#ifdef ACTIONTEC_MD5
		clabel = tr69_password;		
		for (j = 0; j < 16; j++) {
			if (isxdigit((int) clabel[0]) && isxdigit((int) clabel[1]) &&
                            sscanf((tr69_password + (j * 2)), hexfmt, &bp) == 1) {
			    csig[j] = (unsigned char) bp;
			} else {				
			    break;
			}
			clabel += 2;
		}
#endif

		if(!safe_strcmp(login_username, tr69_username)&&(atoi(tr69_user_enable)!=0))
		{
#ifdef ACTIONTEC_MD5
			for (j = 0; j < sizeof signature; j++) 
			{
				kdebug("signature[%d]: %d, csig[%d]: %d", j, signature[j], j, csig[j]);				
				if (signature[j] != csig[j]) {							
			   	 	goto setErr_login;
				}
			}
#else
			if(safe_strcmp(login_password, tr69_password))
				goto setErr_login;
#endif
		
			kinfo("login");
			auth_level = atoi(tr69_auth_level);

#ifdef ACTIONTEC_MULTIACCESS
			if(wp->session_id != 0)
			{
				hs = er_http_session_lookup(wp->session_id);

				if(hs != NULL)
				{
					gettimeofday(&tv, NULL);
					hs->start_time = (unsigned int) tv.tv_sec;
					hs->auth_level = auth_level;					
					kinfo("login changed start_time: %u", hs->start_time);
				}
				else
					hs = er_http_session_add(login_username, auth_level);
			}
			else	
				hs = er_http_session_add(login_username, auth_level);				

			if(hs->id != 0)
			{
				kdebug("hs->id: %d", hs->id);	

				if(last_request_uri != NULL)
				{
					kdebug("redirect cookie1 to %s", last_request_uri);					
					send_redirect_cookie(wp, last_request_uri, hs->id);
					CMSMEM_FREE_BUF_AND_NULL_PTR(last_request_uri);
				}
				else
				{
					kdebug("redirect cookie2 to %s", MAIN_PAGE);
					send_redirect_cookie(wp, MAIN_PAGE, hs->id);
				}
				return;
			}	
#else
			send_redirect_perm(wp, MAIN_PAGE);
			return;
#endif
		}
	}	

setErr_login:
    kerror("error_buf: %s", error_buf);
	send_redirect_perm(wp, HOME_PAGE_K1);
getErr_login:
    kerror("error_buf: %s", error_buf);
}

void actUserlogout(request *wp, char *path, char *query)
{
	kinfo("Enter");	
	req_format_write(wp, ("1"));		
}

void actLogincheck(request *wp, char *path, char *query)
{
	char buf[4] = {0};	
	int user_entries = 0;

	kinfo("Enter");
	
	if(tr69GetUnfreshLeafData(buf, "Device.Users", "UserNumberOfEntries"))
    {
        strcpy(error_buf, "Get Device.Users.UserNumberOfEntries error!");
        req_format_write(wp, ("0"));
    }
	user_entries = atoi(buf);

	if(user_entries > 0)	
		req_format_write(wp, ("1"));
	else
		req_format_write(wp, ("0"));
}

void actPassword(request *wp, char *path, char *query)
{
	char *strVal = NULL;
	char tr69_username[64] = {0};
	char tr69_password[64] = {0};
	char tr69_auth_level[4] = {0};
	char mdmoid_user_object[256] = {0};
	int user_entries = 0;
	int i = 0;
	int auth_level = 0;
	char buf[4] = {0};
	int current_entry = 0;
	
#ifdef ACTIONTEC_MD5
	struct MD5Context md5c;
	unsigned char signature[16] = {0};
	unsigned char csig[16] = {0};
	char *clabel = NULL, *hexfmt = "%02X";
	unsigned int bp;
	int j = 0;
	char pass_buf[4] = {0};
	char new_password[64] = {0};
#endif

	if(tr69GetUnfreshLeafData(buf, "Device.Users", "UserNumberOfEntries"))		
    {
        strcpy(error_buf, "Get Device.Users.UserNumberOfEntries error!");
        goto getErr_pass;
    }

	user_entries = atoi(buf);
	
#ifdef ACTIONTEC_MULTIACCESS
	struct http_session *hs = NULL;
	
	if(wp->session_id != 0)
	{
		hs = er_http_session_lookup(wp->session_id);

		if(hs != NULL)
		{
			for(i = 1; i <= user_entries; i++)
			{
				sprintf(mdmoid_user_object, "%s.%d", "Device.Users.User", i);

                if(tr69GetUnfreshLeafData(tr69_username, mdmoid_user_object, "Username"))	
                {
                    sprintf(error_buf, "Get %s.Username error!", mdmoid_user_object);
            	    goto getErr_pass;
                }
                
        		if(tr69GetUnfreshLeafData(tr69_password, mdmoid_user_object, "Password"))
                {
                    sprintf(error_buf, "Get %s.Password error!", mdmoid_user_object);
            	    goto getErr_pass;
                }
                
                if(tr69GetUnfreshLeafData(tr69_auth_level, mdmoid_user_object, "X_ACTIONTEC_COM_AuthLevel"))
                {
                    sprintf(error_buf, "Get %s.X_ACTIONTEC_COM_AuthLevel error!", mdmoid_user_object);
            	    goto getErr_pass;
                }
				
				if(!safe_strcmp(tr69_username, hs->username) 
					&& (atoi(tr69_auth_level) == hs->auth_level))
				{
					current_entry = i;
					break;
				}
			}
		}
	}
#endif
	
	strVal = req_get_cstream_var(wp, ("currentpass"), "");
	if(strVal[0]){
		kinfo("currentpass: %s", strVal);

#ifdef ACTIONTEC_MD5
		MD5Init(&md5c);
		MD5Update(&md5c, (unsigned char *)strVal, strlen(strVal));
		MD5Final(signature, &md5c);

		clabel = tr69_password;		
		for (j = 0; j < 16; j++) {
			if (isxdigit((int) clabel[0]) && isxdigit((int) clabel[1]) &&
                            sscanf((tr69_password + (j * 2)), hexfmt, &bp) == 1) {
			    csig[j] = (unsigned char) bp;
			} else {				
			    break;
			}
			clabel += 2;
		}

		for (j = 0; j < sizeof signature; j++) 
		{
			kdebug("signature[%d]: %d, csig[%d]: %d", j, signature[j], j, csig[j]); 			
			if (signature[j] != csig[j]) {							
				strcpy(error_buf, ("Error! Current Password is not right"));
				req_format_write(wp, ("1"));
				goto setErr_pass;
			}
		}
#else
		if(safe_strcmp(tr69_password, strVal))
		{
			strcpy(error_buf, ("Error! Current Password is not right"));
			req_format_write(wp, ("1"));
			goto setErr_pass;
		}
#endif
	}

	strVal = req_get_cstream_var(wp, ("newpass"), "");
	if(strVal[0]){
		kinfo("newpass: %s", strVal);

#ifdef ACTIONTEC_MD5
		MD5Init(&md5c);
		MD5Update(&md5c, (unsigned char *)strVal, strlen(strVal));
		MD5Final(signature, &md5c);

		for (j = 0; j < sizeof signature; j++) {
        	sprintf(pass_buf, hexfmt, signature[j]);
			kdebug("pass_buf: %s", pass_buf);
			strcat(new_password, pass_buf);
	    }

		strVal = new_password;
		kinfo("newpass_md5: %s", strVal);
#endif
		if(current_entry != 0)
		{			
			if(tr69SetUnfreshLeafDataString("Device.Users.User", current_entry, "Password", strVal))
            {   
                sprintf(error_buf, "Set Device.Users.User.%d.Password error!", current_entry);
				goto setErr_pass;
            }         
			if(tr69SaveNow())
            {
                strcpy(error_buf, "tr69SaveNow error!");
                goto setErr_pass;
            }         
		}

	}

	//thankyouPage(wp);
	req_format_write(wp, ("0"));
	return;

setErr_pass:
	kerror("error_buf: %s", error_buf);
	//backPage(wp);	
getErr_pass:
    kerror("error_buf: %s", error_buf);
}

