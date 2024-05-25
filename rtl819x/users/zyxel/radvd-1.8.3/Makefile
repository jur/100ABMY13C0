PACKAGE_NAME=radvd-1.8.3
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
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/000-avoid_multicast_loop.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/001-generate_multiple_radvd && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/002-avoid_receiving_ENOBUFS_errors.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/003-avoid_send_amout_of_ra.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/004-ra_lifetimes_set_zero_if_wan_ip_empty.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/005-always_show_prefix_in_RA.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/006-PD_change_then_send_3_RA.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/007-ra_6in4.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/008-ra_ULA.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/009-tunnelingIntf.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/010-set-ralt-to-0-when-prefix-plt-decrease-to-0.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/011-6rd-if-cable-unplug-will-send-routerlt-zero.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/012-set_ralt_to_zero_when_no_default_router.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/013-6rd-get-interface-name-in-tunneling-mode.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/014-check_wan_ip_empty-refacetoring.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/015-popen-cause-zombie.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/016-support-multiprefix-and-send-RA-speedy-if-one-prefix-plt-decrease-to-0.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/017-send-ra-with-old-prefix-lifetime-zero-if-get-dhcpv6-reply-with-that.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/018-keep-ra-prefix-lifetimes-be-origin-value-while-get-wan-ralt-0.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/019-Should_not_check_defaul_route_on_not_v6_env.patch.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/020-Enable_prefix_when_DecrementLifetimesFlag_off.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/021-fix-socket-port-inherit.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/022-when-fget-null-should-fclose-file.patch

endif
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && \
	./configure --target=mips-linux --host=mips-linux --build=i686-linux-gnu \
	--disable-nls

build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && \
	make -C ./ \
	CFLAGS="$(TARGET_CFLAGS) -std=gnu99"

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)/radvd $(DIR_ROMFS)/usr/bin/radvd
	cp -d ./files/etc/uci_defconfig/* $(DIR_ROMFS)/etc/uci_defconfig/
	cp -d ./files/etc/init.d/* $(DIR_ROMFS)/etc/init.d/
	echo "radvd do romfs done."
