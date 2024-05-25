PACKAGE_NAME=ppp-2.4.7
TAR_FILE=$(PACKAGE_NAME).tar.gz
PACKAGE_PATH=$(ZYXEL_BUILD_DIR)

TARGET_CFLAGS += -Dlog=ppp_log

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
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/010-use_target_for_configure.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/100-debian_ip-ip_option.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/101-debian_close_dev_ppp.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/103-debian_fix_link_pidfile.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/105-debian_demand.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/106-debian_stripMSdomain.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/107-debian_pppoatm_wildcard.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/110-debian_defaultroute.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/120-debian_ipv6_updown_option.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/121-debian_adaptive_lcp_echo.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/130-no_cdefs_h.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/131-missing_prototype_macro.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/132-fix_linux_includes.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/133-fix_sha1_include.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/140-pppoe_compile_fix.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/200-makefile.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/201-mppe_mppc_1.1.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/202-no_strip.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/203-opt_flags.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/204-radius_config.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/205-no_exponential_timeout.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/206-compensate_time_change.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/207-lcp_mtu_max.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/208-fix_status_code.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/300-filter-pcap-includes-lib.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/310-precompile_filter.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/320-custom_iface_names.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/321-multilink_support_custom_iface_names.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/330-retain_foreign_default_routes.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/340-populate_default_gateway.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/400-simplify_kernel_checks.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/401-no_record_file.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/403-no_wtmp.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/404-remove_obsolete_protocol_names.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/405-no_multilink_option.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/500-add-pptp-plugin.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/510-pptp_compile_fix.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/520-uniq.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/530-pppoe_send_padt.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/531-pppoe_no_disconnect_warning.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/540-save-pppol2tp_fd_str.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/550-fix-printer-args.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/701-pppd-Fix-bounds-check-in-EAP-code.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/600-ZYXEL_add_function_to_support_WSR30_project.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/601-ZYXEL_disable_PRECOMPILED_FILTER_flag_WSR30_project.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/602_ZYXEL_add_zlog_when_link_up_or_link_down.patch
endif

	cd $(PACKAGE_PATH)/$(PACKAGE_NAME)
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && \
	./configure --target=mips-linux --host=mips-linux --build=i686-linux-gnu

build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && \
	make -C ./pppd && \
	make -C ./pppd/plugins/rp-pppoe

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)/pppd/pppd $(DIR_ROMFS)/bin/
	mkdir -p $(DIR_ROMFS)/usr/lib/pppd
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)/pppd/plugins/rp-pppoe/pppoe-discovery $(DIR_ROMFS)/bin/
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)/pppd/plugins/rp-pppoe/rp-pppoe.so $(DIR_ROMFS)/usr/lib/pppd/
	echo "$(PACKAGE_NAME) do romfs done."
