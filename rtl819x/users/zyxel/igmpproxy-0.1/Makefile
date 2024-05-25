PACKAGE_NAME=igmpproxy-0.1
TAR_FILE=$(PACKAGE_NAME).tar.gz
PACKAGE_PATH=$(ZYXEL_BUILD_DIR)

TARGET_CFLAGS += -Dlog=igmpproxy_log

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
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/001-Send-IGMP-packets-with-IP-Router-Alert-option-RFC-21.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/002-Change-default-interface-state-to-disabled-wrt-29458.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/003-Restrict-igmp-reports-for-downstream-interfaces-wrt-.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/004-Restrict-igmp-reports-forwarding-to-upstream-interfa.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/010-missing_include.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/011_config_igmp_status.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/020-Silence-downstream-interface-igmp-messages.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/100-use-monotic-clock-instead-of-time-of-day.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/200-allow_wildcard_addr.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/250-fix_multiple_downlink_interfaces.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/260-ZyXEL_IGMP_Leave.patch
endif

	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && \
	./configure --target=mips-linux --host=mips-linux --build=i686-linux-gnu \
	--disable-nls

build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && \
	make -C ./src \
	CFLAGS="$(TARGET_CFLAGS) -std=gnu99"

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)/src/igmpproxy $(DIR_ROMFS)/usr/bin/igmpproxy-0.1
	cp -d ./files/etc/init.d/* $(DIR_ROMFS)/etc/init.d/
	cp -d ./files/usr/sbin/* $(DIR_ROMFS)/usr/sbin/
	echo "igmpproxy-0.1 do romfs done."
