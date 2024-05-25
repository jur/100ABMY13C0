PACKAGE_NAME=conntrack-tools
PKG_SOURCE_VERSION:=1.0.1
PACKAGE_PATH=$(ZYXEL_BUILD_DIR)
TAR_FILE=$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION).tar.bz2

all: prepare build


prepare:
ifneq ("$(ZYXEL_DL)/$(TAR_FILE)","$(wildcard $(ZYXEL_DL)/$(TAR_FILE))")
	# download source code
	wget -P $(ZYXEL_DL) $(ZYXEL_SITE)/$(TAR_FILE)
endif

ifneq ("$(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION)","$(wildcard $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION) )")
	# prepare build_dir source code
	cd $(PACKAGE_PATH) && \
	tar -jxvf $(ZYXEL_DL)/$(TAR_FILE) && \
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION) && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/001-lost-include-file.patch
endif
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION) && \
	LD="$(CROSS_COMPILE)ld" \
	CC="$(CROSS_COMPILE)gcc" \
	GCC="$(CROSS_COMPILE)gcc" \
	CFLAGS="-I$(DIR_USERS)/lib/include/" LIBS="-L$(DIR_USERS)/lib/lib/" \
	PKG_CONFIG_PATH="$(DIR_USERS)/lib/lib/pkgconfig" \
	PKG_CONFIG_LIBDIR="$(DIR_USERS)/lib/lib/" \
	./configure --target=mips-gnu-linux --host=mips-gnu-linux --build=i686-linux-gnu
	echo "$(PACKAGE_NAME) do prepare done."

build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION) && \
	make
	echo "$(PACKAGE_NAME) do build done."


# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION)
	echo "$(PACKAGE_NAME) do clean done."


romfs:
	mkdir -p $(DIR_ROMFS)/etc/conntrackd
	mkdir -p $(DIR_ROMFS)/usr/sbin
	mkdir -p $(DIR_ROMFS)/etc/init.d/
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION)/doc/stats/conntrackd.conf $(DIR_ROMFS)/etc/conntrackd/
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION)/src/conntrack $(DIR_ROMFS)/usr/sbin/
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION)/src/conntrackd $(DIR_ROMFS)/usr/sbin/
	cp ./files/conntrackd.init $(DIR_ROMFS)/etc/init.d/conntrackd
	echo "$(PACKAGE_NAME) do romfs done."

