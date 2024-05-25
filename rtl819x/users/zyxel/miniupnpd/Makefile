PACKAGE_NAME=miniupnpd
PACKAGE_VERSION=2.0
TAR_FILE=$(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz
PACKAGE_PATH=$(ZYXEL_BUILD_DIR)
TARGET_OPENWRT=1
IPTABLES_VERSION=iptables-1.4.4
#PKG_CONFIG_PATH=$(DIR_ROOT)/users/lib/lib/pkgconfig
PKG_CONFIG=$(DIR_ROOT)/users/lib/lib/pkgconfig
TEST=0

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
	cd $(PACKAGE_NAME)-$(PACKAGE_VERSION) && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/100-device_info.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/101-no-ssl-uuid.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/102-ipv6-ext-port.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/103-no-ipv6-autodetection.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/104-libiptc.patch && \
	echo "OpenWrt" > $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/os.openwrt
endif

build:
	cd $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)-$(PACKAGE_VERSION) && \
	PKG_CONFIG=$(DIR_ROOT)/users/lib/lib/pkgconfig \
	LDFLAGS="-L$(DIR_USERS)/$(IPTABLES_VERSION)/libiptc/" && \
	PKG_CONFIG=$(DIR_ROOT)/users/lib/lib/pkgconfig \
	CC="$(CROSS_COMPILE)gcc -L$(DIR_USERS)/lib/lib/ -DIPTABLES_143 -luuid" \
	CONFIG_OPTIONS="--leasefile" \
	IPTABLESPATH=$(DIR_USERS)/$(IPTABLES_VERSION) make -f Makefile.linux

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)-$(PACKAGE_VERSION)

romfs:
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/miniupnpd $(DIR_ROMFS)/usr/sbin
	cp -d ./files/miniupnpd.init $(DIR_ROMFS)/etc/init.d/miniupnpd
	cp -d ./files/act_miniupnpd $(DIR_ROMFS)/sbin/
	cp -d ./files/upnpd.config $(DIR_ROMFS)/etc/uci_defconfig/upnp

	echo "$(PACKAGE_NAME) do romfs done."
