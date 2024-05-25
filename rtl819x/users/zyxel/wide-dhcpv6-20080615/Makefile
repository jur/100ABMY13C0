PACKAGE_NAME=wide-dhcpv6-20080615
TAR_FILE=$(PACKAGE_NAME).tar.gz
PACKAGE_PATH=$(ZYXEL_BUILD_DIR)

all: prepare build

prepare:
ifneq ("$(ZYXEL_DL)/$(TAR_FILE)","$(wildcard $(ZYXEL_DL)/$(TAR_FILE))")
	# download source code
	wget -P $(ZYXEL_DL) $(ZYXEL_SITE)/$(TAR_FILE)
endif

ifneq ("$(PACKAGE_PATH)/$(PACKAGE_NAME)","$(wildcard $(PACKAGE_PATH)/$(PACKAGE_NAME))")
	@echo "PACKAGE_PATH=" $(PACKAGE_PATH) 
	@echo "PACKAGE_NAME=" $(PACKAGE_NAME)
	# prepare build_dir source code
	cd $(PACKAGE_PATH) && \
	tar zxvf $(ZYXEL_DL)/$(TAR_FILE) && \
	cd $(PACKAGE_NAME) && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/000-cftoken-noyywrap.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/001-linux_old_compat.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/002-fix-dprintf-clash.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/003-take_off_ctl_sock_and_gen_pid.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/004-lifetime_in_RA.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/005-sync_brlan_ip_with_radvdconf.conf && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/006-revise_6s_duid_format_to_fit_rfc3315.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/007-PD_change_then_send_3_RA.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/008-support_decline_message && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/009-support_retransmit_arch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/010-change_dhcp6s_conf_path && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/011-add_iaid && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/012-set_and_Renew_WAN-LAN_IP_lifetimes_by_IANA-IAPD.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/013-MRT.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/014-Reconf_Opt.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/015-Set_Current_LAN_IP_Timeout_When_Get_Diff_PD.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/016-Set_Same_Transaction_ID_When_Recv_Unexpected_Reply.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/017-fix_DAD_be_ignored.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/018-Reset_DHCP_When_Receipt_reply_with_status_code_notonlink.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/019-Retrans_renew_when_reply_no_prefix_in_prefix_option.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/020-bypass_dnssl_info_to_radvd_uci_config.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/021-Reply_Lan_Confirm_Without_IA_NA_option.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/022-Lan_Request_IA_NA_Address_Error_Should_Reply_Notonlink.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/023-SOL_MAX_RT.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/024-fix-017-patch-wan-global-ip-lifetime-do-not-update.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/025-process_auth_with_reconfigure.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/026-receive-nobinding-reply-should-not-restart-radvd.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/027-dhcp6c-reconfigureAccept.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/028-support-expire-old-pd-speedy-or-count-down-to-0-by-nature-while-update-new-pd.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/029-set-route-information-to-radvd-uci.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/030-send-ra-with-old-prefix-lifetime-zero-if-get-dhcpv6-reply-with-that.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/031-dhcp6s-send-reply-with-option-23-24-by-client-needy.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/032-Add-dhcp6s-signal-reload.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/033-add-pkg-info-for-kernel-3_10-.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/034-fix-socket-port-inherit.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/035-cancel_checking_script_file.patch


endif
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && \
	export ac_cv_func_setpgrp_void=yes && \
	./configure --target=mips-linux --host=mips-linux --build=i686-linux-gnu --with-localdbdir=/var \

build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && \
	make -C ./ \

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)/dhcp6c $(DIR_ROMFS)/usr/bin/dhcp6c
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)/dhcp6s $(DIR_ROMFS)/usr/bin/dhcp6s
	cp -d ./files/etc/uci_defconfig/* $(DIR_ROMFS)/etc/uci_defconfig/
	cp -d ./files/etc/init.d/* $(DIR_ROMFS)/etc/init.d/
	cp -d ./files/usr/sbin/* $(DIR_ROMFS)/usr/sbin/
	echo "wide-dhcpv6 do romfs done."
