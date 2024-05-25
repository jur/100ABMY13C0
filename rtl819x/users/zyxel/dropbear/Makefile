PACKAGE_NAME=dropbear-2017.75
TAR_FILE=$(PACKAGE_NAME).tar.bz2
PACKAGE_PATH=$(ZYXEL_BUILD_DIR)

all: prepare build

prepare:
ifneq ("$(ZYXEL_DL)/$(TAR_FILE)","$(wildcard $(ZYXEL_DL)/$(TAR_FILE))")
	# download source code
	wget -P $(ZYXEL_DL) $(ZYXEL_SITE)/$(TAR_FILE)
endif
ifneq ("$(PACKAGE_PATH)/$(PACKAGE_NAME)","$(wildcard $(PACKAGE_PATH)/$(PACKAGE_NAME))")
	# prepare build_dir source code
	cd $(PACKAGE_PATH) && \
	tar xjvf $(ZYXEL_DL)/$(TAR_FILE)
endif

#--with-zlib=$(DIR_USERS)/lib && \

build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && \
	./configure --host=mips-linux \
	            --disable-loginfunc \
	            --disable-pam \
	            --disable-lastlog \
	            --disable-utmp \
	            --disable-wtmp \
	            --disable-wtmpx \
	            --disable-pututline \
	            --disable-pututxline \
	            --disable-zlib && \
	make PROGRAMS="dropbear dropbearkey scp"

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:
	install -d -m0755 $(DIR_ROMFS)/usr/sbin
	install -m0755 $(PACKAGE_PATH)/$(PACKAGE_NAME)/dropbear $(DIR_ROMFS)/usr/sbin/dropbear
	install -d -m0755 $(DIR_ROMFS)/usr/bin
	install -m0755 $(PACKAGE_PATH)/$(PACKAGE_NAME)/dropbearkey $(DIR_ROMFS)/usr/bin/dropbearkey
	install -m0755 $(PACKAGE_PATH)/$(PACKAGE_NAME)/scp $(DIR_ROMFS)/usr/bin/scp
	install -d -m0755 $(DIR_ROMFS)/etc/uci_defconfig
	install -m0644 files/dropbear.config $(DIR_ROMFS)/etc/uci_defconfig/dropbear
	install -d -m0755 $(DIR_ROMFS)/etc/init.d
	install -m0755 files/dropbear.init $(DIR_ROMFS)/etc/init.d/dropbear
	echo "dropbear do romfs done."
